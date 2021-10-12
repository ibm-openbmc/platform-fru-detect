/* SPDX-License-Identifier: Apache-2.0 */
#include "devices/nvme.hpp"
#include "inventory.hpp"
#include "platforms/rainier.hpp"
#include "sysfs/gpio.hpp"
#include "sysfs/i2c.hpp"

#include <gpiod.hpp>
#include <phosphor-logging/lg2.hpp>

#include <cassert>
#include <filesystem>
#include <stdexcept>
#include <string>

PHOSPHOR_LOG2_USING;

static constexpr const char* app_name = "NVMe Drive Management";

static const std::map<int, int> flett_mux_channel_map = {
    {8, 3},
    {9, 2},
    {10, 0},
    {11, 1},
};

static const std::map<int, int> flett_slot_presence_map = {
    {8, 8},
    {9, 9},
    {10, 10},
    {11, 11},
};

static const std::map<int, int> flett_index_map = {
    {8, 0},
    {9, 1},
    {10, 0},
    {11, 2},
};

int Nisqually::getFlettIndex(int slot)
{
    return flett_index_map.at(slot);
}

SysfsI2CBus Nisqually::getFlettSlotI2CBus(int slot)
{
    SysfsI2CBus rootBus = Ingraham::getPCIeSlotI2CBus(slot);
    SysfsI2CMux mux(rootBus, Nisqually::slotMuxAddress);

    debug("Looking up mux channel for Flett in slot {PCIE_SLOT}", "PCIE_SLOT",
          slot);

    int channel = flett_mux_channel_map.at(slot);

    return SysfsI2CBus(mux, channel);
}

Nisqually::Nisqually(Inventory& inventory) :
    flett_presence(SysfsGPIOChip(std::filesystem::path(
                                     Nisqually::flett_presence_device_path))
                       .getName()
                       .string(),
                   gpiod::chip::OPEN_BY_NAME),
    williwakas_presence(
        SysfsGPIOChip(
            std::filesystem::path(Nisqually::williwakas_presence_device_path))
            .getName()
            .string(),
        gpiod::chip::OPEN_BY_NAME),
    inventory(inventory)
{
    /* Slot 9 is on the same mux as slot 8 */
    Ingraham::getPCIeSlotI2CBus(8).probeDevice("pca9546",
                                               Nisqually::slotMuxAddress);
    /* Slot 11 is on the same mux as slot 10 */
    Ingraham::getPCIeSlotI2CBus(10).probeDevice("pca9546",
                                                Nisqually::slotMuxAddress);
}

void Nisqually::plug()
{
    detectExpanderCards(ioExpanders);
    for (auto& ioExpander : ioExpanders)
    {
        ioExpander.plug();
    }

    detectDriveBackplanes(driveBackplanes);
    for (auto& driveBackplane : driveBackplanes)
    {
        driveBackplane.plug();
    }
}

void Nisqually::detectDriveBackplanes(std::vector<Williwakas>& driveBackplanes)
{
    static constexpr std::array<int, 3> backplane_indexes = {0, 1, 2};

    assert(driveBackplanes.empty() && "Already detected drive backplanes");

    for (auto& index : backplane_indexes)
    {
        debug("Testing for Williwakas presence at index {WILLIWAKAS_ID}",
              "WILLIWAKAS_ID", index);

        if (!isWilliwakasPresent(index))
        {
            info("Williwakas {WILLIWAKAS_ID} is not present", "WILLIWAKAS_ID",
                 index);
            continue;
        }

        try
        {
            Williwakas williwakas(inventory, *this, index);
            driveBackplanes.push_back(williwakas);
            debug("Initialised Williwakas {WILLIWAKAS_ID}", "WILLIWAKAS_ID",
                  index);
        }
        catch (const SysfsI2CDeviceDriverBindException& ex)
        {
            std::string what(ex.what());
            error(
                "Required drivers failed to bind for devices on Williwakas {WILLIWAKAS_ID}: {EXCEPTION_DESCRIPTION}",
                "WILLIWAKAS_ID", index, "EXCEPTION_DESCRIPTION", what);
            warning("Skipping FRU detection on Williwakas {WILLIWAKAS_ID}",
                    "WILLIWAKAS_ID", index);
            continue;
        }
    }

    debug("Found {WILLIWAKAS_COUNT} Williwakas drive backplanes",
          "WILLIWAKAS_COUNT", driveBackplanes.size());
}

std::string Nisqually::getInventoryPath() const
{
    return "/system/chassis/motherboard";
}

void Nisqually::addToInventory([[maybe_unused]] Inventory& inventory)
{
    std::logic_error("Unimplemented");
}

/*
 * Note that Bear River and Bear Lake cards also assert the presence GPIO.
 * However, if they are present in slots that can also house Flett cards there
 * will be no associated Williwakas card and as such there will be no drives
 * detected.
 */
bool Nisqually::isFlettPresentAt(int slot) const
{
    debug("Looking up Flett presence GPIO index for slot {PCIE_SLOT}",
          "PCIE_SLOT", slot);

    int flett_presence_index = flett_slot_presence_map.at(slot);

    debug("Testing for Flett presence via index {FLETT_PRESENCE_INDEX}",
          "FLETT_PRESENCE_INDEX", flett_presence_index);

    gpiod::line line = flett_presence.get_line(flett_presence_index);
    line.request(
        {app_name, gpiod::line::DIRECTION_INPUT, gpiod::line::ACTIVE_LOW});

    bool present = line.get_value();

    debug(
        "Flett presence at index {FLETT_PRESENCE_INDEX} for slot {PCIE_SLOT}: {FLETT_PRESENT}",
        "FLETT_PRESENCE_INDEX", flett_presence_index, "PCIE_SLOT", slot,
        "FLETT_PRESENT", present);

    line.release();

    return present;
}

int Nisqually::getFlettSlot(int index) const
{
    debug("Inspecting whether Flett {FLETT_ID} is present", "FLETT_ID", index);
    if (index == 0)
    {
        debug("Probing PCIe slot 8 presence");
        if (Nisqually::isFlettPresentAt(8))
        {
            return 8;
        }

        debug("Probing PCIe slot 10 presence");
        if (Nisqually::isFlettPresentAt(10))
        {
            return 10;
        }

        return -1;
    }
    else if (index == 1)
    {
        debug("Probing PCIe slot 9 presence");
        return Nisqually::isFlettPresentAt(9) ? 9 : -1;
    }
    else if (index == 2)
    {
        debug("Probing PCIe slot 11 presence");
        return Nisqually::isFlettPresentAt(11) ? 11 : -1;
    }

    return -1;
}

bool Nisqually::isFlettSlot(int slot) const
{
    bool contains = flett_index_map.contains(slot);

    debug("Is {PCIE_SLOT} a Flett slot? {IS_FLETT_PCIE_SLOT}", "PCIE_SLOT",
          slot, "IS_FLETT_PCIE_SLOT", contains);

    return contains;
}

bool Nisqually::isWilliwakasPresent(int index) const
{
    debug(
        "Looking up Williwakas presence GPIO index for backplane {WILLIWAKAS_ID}",
        "WILLIWAKAS_ID", index);

    int williwakas_presence_index = williwakas_presence_map.at(index);

    debug(
        "Testing for Williwakas presence via index {WILLIWAKAS_PRESENCE_INDEX}",
        "WILLIWAKAS_PRESENCE_INDEX", williwakas_presence_index);

    gpiod::line line = williwakas_presence.get_line(williwakas_presence_index);

    line.request(
        {app_name, gpiod::line::DIRECTION_INPUT, gpiod::line::ACTIVE_LOW});

    bool present = line.get_value();

    debug("Williwakas presence at index {WILLIWAKAS_ID}: {WILLIWAKAS_PRESENT}",
          "WILLIWAKAS_ID", index, "WILLIWAKAS_PRESENT", present);

    line.release();

    return present;
}

void Nisqually::detectExpanderCards(std::vector<Flett>& expanders)
{
    static constexpr std::array<int, 3> expander_indexes = {0, 1, 2};

    assert(expanders.empty() && "Already detected Flett IO expanders");

    debug("Locating expander cards");

    for (auto& index : expander_indexes)
    {
        int slot = getFlettSlot(index);

        if (!isFlettSlot(slot))
        {
            debug("No Flett for index {FLETT_ID}", "FLETT_ID", index);
            continue;
        }

        try
        {
            Flett flett(inventory, *this, slot);
            expanders.push_back(flett);
            debug("Initialised Flett {FLETT_ID} in slot {PCIE_SLOT}",
                  "FLETT_ID", index, "PCIE_SLOT", slot);
        }
        catch (const SysfsI2CDeviceDriverBindException& ex)
        {
            std::string what(ex.what());
            error(
                "Required drivers failed to bind for devices on Flett {FLETT_ID}: {EXCEPTION_DESCRIPTION}",
                "FLETT_ID", index, "EXCEPTION_DESCRIPTION", what);
            warning("Skipping FRU detection on Flett {FLETT_ID}", "FLETT_ID",
                    index);
            continue;
        }
    }

    debug("Found {FLETT_COUNT} Flett IO expander cards", "FLETT_COUNT",
          expanders.size());
}
