/* SPDX-License-Identifier: Apache-2.0 */
#include "devices/nvme.hpp"
#include "inventory.hpp"
#include "platforms/rainier.hpp"
#include "sysfs/gpio.hpp"
#include "sysfs/i2c.hpp"

#include <errno.h>

#include <gpiod.hpp>
#include <phosphor-logging/lg2.hpp>

#include <cassert>
#include <filesystem>
#include <ranges>
#include <stdexcept>
#include <string>

PHOSPHOR_LOG2_USING;

static const std::map<int, int> flett_mux_channel_map = {
    {8, 3},
    {9, 2},
    {10, 0},
    {11, 1},
};

static const std::map<int, int> pcie_slot_mux_channel_map = {
    {0, 0}, {1, 1}, {2, 2}, {3, 0},  {4, 1},
    {7, 1}, {8, 3}, {9, 2}, {10, 0}, {11, 1}};

static const std::map<int, int> flett_slot_presence_map = {
    {8, 8},
    {9, 9},
    {10, 10},
    {11, 11},
};

/* See Rainier_System_Workbook_v1.7.pdf, 4.4.4 NVMe JBOF to Backplane Cabling
 *
 * The workbook dictates that only two Williwakas/Flett pairs are present, but
 * the schematics haven't elided the support for the presence of three pairs. We
 * follow the schematics as an engineering reference point.
 */
static const std::map<int, int> flett_slot_index_map = {
    {8, 0},
    {9, 2},
    {10, 0},
    {11, 1},
};

static const std::map<int, int> flett_connector_slot_map = {
    {0, 8},
    {1, 9},
    {2, 10},
    {3, 11},
};

static const std::map<int, int> pcie_card_tmp435_address_map = {
    {0, 0x4C}, {1, 0x4D}, {2, 0x4E}, {3, 0x4C},  {4, 0x4D},
    {7, 0x4E}, {8, 0x4D}, {9, 0x4C}, {10, 0x4C}, {11, 0x4D}};

static const std::array<int, 10> cable_card_slots = {0, 1, 2, 3,  4,
                                                     7, 8, 9, 10, 11};

/* Nisqually */

int Nisqually::getFlettIndex(int slot)
{
    return flett_slot_index_map.at(slot);
}

Nisqually::Nisqually(Inventory* inventory) :
    inventory(inventory), flettConnectors{{
                              Connector<Flett>(this->inventory, this, 8),
                              Connector<Flett>(this->inventory, this, 9),
                              Connector<Flett>(this->inventory, this, 10),
                              Connector<Flett>(this->inventory, this, 11),
                          }},
    williwakasPresenceChip(
        SysfsGPIOChip(
            std::filesystem::path(Nisqually::williwakas_presence_device_path))
            .getName()
            .string(),
        gpiod::chip::OPEN_BY_NAME),
    williwakasConnectors{{
        Connector<Williwakas>(this->inventory, this, 0),
        Connector<Williwakas>(this->inventory, this, 1),
        Connector<Williwakas>(this->inventory, this, 2),
    }}
{
    for (int i :
         std::views::iota(0UL, Nisqually::williwakas_presence_map.size()))
    {
        int offset = Nisqually::williwakas_presence_map.at(i);
        gpiod::line line = williwakasPresenceChip.get_line(offset);
        line.request({program_invocation_short_name,
                      gpiod::line::DIRECTION_INPUT, gpiod::line::ACTIVE_LOW});
        williwakasPresenceLines[i] = line;
    }
}

void Nisqually::plug(Notifier& notifier)
{
    detectFlettCards(notifier);
    detectWilliwakasCards(notifier);
}

void Nisqually::unplug(Notifier& notifier, int mode)
{
    for (auto& connector : williwakasConnectors)
    {
        if (connector.isPopulated())
        {
            connector.getDevice().unplug(notifier, mode);
            connector.depopulate();
        }
    }

    for (auto& connector : flettConnectors)
    {
        if (connector.isPopulated())
        {
            connector.getDevice().unplug(notifier, mode);
            connector.depopulate();
        }
    }
}

void Nisqually::detectWilliwakasCards(Notifier& notifier)
{
    debug("Locating Williwakas cards");

    for (std::size_t index = 0; index < williwakasConnectors.size(); index++)
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
            williwakasConnectors[index].populate();
            williwakasConnectors[index].getDevice().plug(notifier);
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
}

std::string Nisqually::getInventoryPath() const
{
    return "/system/chassis/motherboard";
}

void Nisqually::addToInventory([[maybe_unused]] Inventory* inventory)
{
    throw std::logic_error("Unimplemented");
}

void Nisqually::removeFromInventory([[maybe_unused]] Inventory* inventory)
{
    throw std::logic_error("Unimplemented");
}

bool Nisqually::isWilliwakasPresent(int index)
{
    bool present = williwakasPresenceLines.at(index).get_value();

    debug("Williwakas presence at index {WILLIWAKAS_ID}: {WILLIWAKAS_PRESENT}",
          "WILLIWAKAS_ID", index, "WILLIWAKAS_PRESENT", present);

    return present;
}

void Nisqually::detectFlettCards(Notifier& notifier)
{
    debug("Locating Flett cards");

    /* FIXME: do something more ergonomic */
    for (auto& [connector, slot] : flett_connector_slot_map)
    {
        if (!isFlettPresentAt(slot))
        {
            debug("No Flett in slot {PCIE_SLOT}", "PCIE_SLOT", slot);
            continue;
        }

        try
        {
            flettConnectors[connector].populate();
            flettConnectors[connector].getDevice().plug(notifier);
            debug("Initialised Flett {FLETT_ID} in slot {PCIE_SLOT}",
                  "FLETT_ID", getFlettIndex(slot), "PCIE_SLOT", slot);
        }
        catch (const SysfsI2CDeviceDriverBindException& ex)
        {
            debug(
                "Required drivers failed to bind for devices on Flett {FLETT_ID} in slot {PCIE_SLOT}: {EXCEPTION}",
                "FLETT_ID", getFlettIndex(slot), "PCIE_SLOT", slot, "EXCEPTION",
                ex);
            warning("Failed to detect Flett in slot {PCIE_SLOT}", "PCIE_SLOT",
                    slot);
            continue;
        }
    }
}

bool Nisqually::isCableCardPresentAt(int slot) const
{
    if (std::find(cable_card_slots.begin(), cable_card_slots.end(), slot) ==
        cable_card_slots.end())
    {
        return false;
    }

    // This also handles cable cards
    std::string path = Flett::getInventoryPathFor(this, slot);

    if (!inventory->isPresent(path))
    {
        debug("Inventory reports slot {PCIE_SLOT} is not populated",
              "PCIE_SLOT", slot);
        return false;
    }

    // 6B99 = 4U Bear Lake cable card
    // 6B92 = 2U Bear River cable card
    if (inventory->isModel(path, "6B99") || inventory->isModel(path, "6B92"))
    {
        debug(
            "Inventory reports slot {PCIE_SLOT} is populated with a cable card",
            "PCIE_SLOT", slot);
        return true;
    }

    debug("Inventory reports the card in slot {PCIE_SLOT} is not a cable card",
          "PCIE_SLOT", slot);

    return false;
}

/* Nisqually0z */

Nisqually0z::Nisqually0z(Inventory* inventory) : Nisqually(inventory)
{}

SysfsI2CBus Nisqually0z::getFlettSlotI2CBus(int slot) const
{
    return Ingraham::getPCIeSlotI2CBus(slot);
}

bool Nisqually0z::isFlettPresentAt(int slot)
{
    if (!flett_slot_presence_map.contains(slot))
    {
        return false;
    }

    std::string path = Flett::getInventoryPathFor(this, slot);

    bool populated = inventory->isPresent(path);
    if (!populated)
    {
        debug("Inventory reports slot {PCIE_SLOT} is not populated",
              "PCIE_SLOT", slot);
        return false;
    }

    bool validModel = inventory->isModel(path, "6B87");
    if (!validModel)
    {
        debug(
            "Inventory reports the card in slot {PCIE_SLOT} is not a Flett card",
            "PCIE_SLOT", slot);
        return false;
    }

    debug("Inventory reports slot {PCIE_SLOT} is populated with Flett card",
          "PCIE_SLOT", slot);

    return true;
}

SysfsI2CBus Nisqually0z::getCableCardI2CBus(int slot) const
{
    return Ingraham::getPCIeSlotI2CBus(slot);
}

/* Nisqually1z */

Nisqually1z::Nisqually1z(Inventory* inventory) :
    Nisqually(inventory),
    flettPresenceChip(
        SysfsGPIOChip(
            std::filesystem::path(Nisqually1z::flett_presence_device_path))
            .getName()
            .string(),
        gpiod::chip::OPEN_BY_NAME)
{
    /* Slot 9 is on the same mux as slot 8 */
    Ingraham::getPCIeSlotI2CBus(8).probeDevice("pca9546",
                                               Nisqually1z::slotMuxAddress);
    /* Slot 11 is on the same mux as slot 10 */
    Ingraham::getPCIeSlotI2CBus(10).probeDevice("pca9546",
                                                Nisqually1z::slotMuxAddress);

    /* Iterate in terms of Flett slot numbers for mapping to presence lines */
    for (auto& slot : flett_connector_slot_map | std::views::values)
    {
        int offset = flett_slot_presence_map.at(slot);
        gpiod::line line = flettPresenceChip.get_line(offset);
        line.request({program_invocation_short_name,
                      gpiod::line::DIRECTION_INPUT, gpiod::line::ACTIVE_LOW});
        flettPresenceLines[slot] = line;
    }
}

SysfsI2CBus Nisqually1z::getFlettSlotI2CBus(int slot) const
{
    SysfsI2CBus rootBus = Ingraham::getPCIeSlotI2CBus(slot);
    SysfsI2CMux mux(rootBus, Nisqually1z::slotMuxAddress);

    debug("Looking up mux channel for Flett in slot {PCIE_SLOT}", "PCIE_SLOT",
          slot);

    int channel = flett_mux_channel_map.at(slot);

    return SysfsI2CBus(mux, channel);
}

/*
 * Note that Bear River and Bear Lake cards also assert the presence GPIO.
 * However, if they are present in slots that can also house Flett cards there
 * will be no associated Williwakas card and as such there will be no drives
 * detected.
 */
bool Nisqually1z::isFlettPresentAt(int slot)
{
    bool present = false;
    if (flettPresenceLines.contains(slot))
    {
        present = flettPresenceLines.at(slot).get_value();

        debug("Flett {FLETT_ID} presence for slot {PCIE_SLOT}: {FLETT_PRESENT}",
              "FLETT_ID", getFlettIndex(slot), "PCIE_SLOT", slot,
              "FLETT_PRESENT", present);
    }
    else
    {
        debug("Slot {SLOT} is not a Flett slot", "SLOT", slot);
    }

    return present;
}

SysfsI2CBus Nisqually1z::getCableCardI2CBus(int slot) const
{
    SysfsI2CBus rootBus = Ingraham::getPCIeSlotI2CBus(slot);
    SysfsI2CMux mux(rootBus, Nisqually1z::slotMuxAddress);

    int channel = pcie_slot_mux_channel_map.at(slot);

    return SysfsI2CBus(mux, channel);
}

void Nisqually::slotPowerStateChanged(int slot, bool powerOn)
{
    debug("Nisqually slot power state changed slot {SLOT} state {STATE}",
          "SLOT", slot, "STATE", powerOn);

    handlePcieCardTMP435s(slot, powerOn);
}

void Nisqually::handlePcieCardTMP435s(int slot, bool powerOn)
{
    try
    {
        std::optional<SysfsI2CBus> bus;
        if (isFlettPresentAt(slot))
        {
            bus = getFlettSlotI2CBus(slot);
        }
        else if (isCableCardPresentAt(slot))
        {
            bus = getCableCardI2CBus(slot);
        }
        else
        {
            return;
        }

        if (powerOn)
        {
            debug("Calling tmp435 new_device for slot {SLOT} address {ADDRESS}",
                  "SLOT", slot, "ADDRESS",
                  pcie_card_tmp435_address_map.at(slot));
            bus->requireDevice("tmp435", pcie_card_tmp435_address_map.at(slot));
        }
        else
        {
            debug(
                "Calling tmp435 releaseDevice for slot {SLOT} address {ADDRESS}",
                "SLOT", slot, "ADDRESS", pcie_card_tmp435_address_map.at(slot));
            bus->releaseDevice(pcie_card_tmp435_address_map.at(slot));
        }
    }
    catch (const std::exception& ex)
    {
        info(
            "Failed requiring or releasing TMP435 in PCIE slot {SLOT}, powerOn = {STATE}: {EX}",
            "SLOT", slot, "STATE", powerOn, "EX", ex);
    }
    catch (...)
    {
        // May get here if device is already in requested state, so don't crash
        info(
            "Failed requiring or releasing TMP435 in PCIE slot {SLOT}, powerOn = {STATE}",
            "SLOT", slot, "STATE", powerOn);
    }
}
