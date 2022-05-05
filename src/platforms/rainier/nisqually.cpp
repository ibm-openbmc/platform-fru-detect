/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */
#include "devices/nvme.hpp"
#include "inventory.hpp"
#include "platforms/rainier.hpp"
#include "sysfs/gpio.hpp"
#include "sysfs/i2c.hpp"

#include <gpiod.hpp>
#include <phosphor-logging/lg2.hpp>

#include <cassert>
#include <cerrno>
#include <filesystem>
#include <ranges>
#include <stdexcept>
#include <string>

PHOSPHOR_LOG2_USING;

static const std::map<int, int> flettMuxChannelMap = {
    {8, 3},
    {9, 2},
    {10, 0},
    {11, 1},
};

/* See Rainier_System_Workbook_v1.7.pdf, 4.4.4 NVMe JBOF to Backplane Cabling
 *
 * The workbook dictates that only two Williwakas/Flett pairs are present, but
 * the schematics haven't elided the support for the presence of three pairs. We
 * follow the schematics as an engineering reference point.
 */
static const std::map<int, int> flettSlotIndexMap = {
    {8, 0},
    {9, 2},
    {10, 0},
    {11, 1},
};

static const std::map<int, int> flettConnectorSlotMap = {
    {0, 8},
    {1, 9},
    {2, 10},
    {3, 11},
};

/* Nisqually */

int Nisqually::getFlettIndex(int slot)
{
    return flettSlotIndexMap.at(slot);
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
            std::filesystem::path(Nisqually::williwakasPresenceDevicePath))
            .getName()
            .string(),
        gpiod::chip::OPEN_BY_NAME),
    williwakasConnectors{{
        Connector<Williwakas>(this->inventory, this, 0),
        Connector<Williwakas>(this->inventory, this, 1),
        Connector<Williwakas>(this->inventory, this, 2),
    }}
{
    for (int i : std::views::iota(0UL, Nisqually::williwakasPresenceMap.size()))
    {
        int offset = Nisqually::williwakasPresenceMap.at(i);
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
        connector.depopulate(notifier, mode);
    }

    for (auto& connector : flettConnectors)
    {
        connector.depopulate(notifier, mode);
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
            williwakasConnectors[index].populate(notifier);
            info("Initialised Williwakas {WILLIWAKAS_ID}", "WILLIWAKAS_ID",
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

bool Nisqually::isFlettPresentAt(int slot)
{
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
    for (auto& [connector, slot] : flettConnectorSlotMap)
    {
        if (!isFlettPresentAt(slot))
        {
            debug("No Flett in slot {PCIE_SLOT}", "PCIE_SLOT", slot);
            continue;
        }

        try
        {
            flettConnectors[connector].populate(notifier);
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

/* Nisqually0z */

Nisqually0z::Nisqually0z(Inventory* inventory) : Nisqually(inventory)
{}

SysfsI2CBus Nisqually0z::getFlettSlotI2CBus(int slot) const
{
    return Ingraham::getPCIeSlotI2CBus(slot);
}

/* Nisqually1z */

Nisqually1z::Nisqually1z(Inventory* inventory) : Nisqually(inventory)
{
    /* Slot 9 is on the same mux as slot 8 */
    Ingraham::getPCIeSlotI2CBus(8).probeDevice("pca9546",
                                               Nisqually1z::slotMuxAddress);
    /* Slot 11 is on the same mux as slot 10 */
    Ingraham::getPCIeSlotI2CBus(10).probeDevice("pca9546",
                                                Nisqually1z::slotMuxAddress);
}

SysfsI2CBus Nisqually1z::getFlettSlotI2CBus(int slot) const
{
    SysfsI2CBus rootBus = Ingraham::getPCIeSlotI2CBus(slot);
    SysfsI2CMux mux(rootBus, Nisqually1z::slotMuxAddress);

    debug("Looking up mux channel for Flett in slot {PCIE_SLOT}", "PCIE_SLOT",
          slot);

    int channel = flettMuxChannelMap.at(slot);

    return {mux, channel};
}
