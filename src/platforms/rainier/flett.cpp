/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */
#include "devices/nvme.hpp"
#include "inventory.hpp"
#include "platforms/rainier.hpp"
#include "sysfs/gpio.hpp"
#include "sysfs/i2c.hpp"

#include <phosphor-logging/lg2.hpp>

#include <cassert>
#include <cerrno>
#include <map>
#include <string>

PHOSPHOR_LOG2_USING;

/* FlettNVMeDrive */

FlettNVMeDrive::FlettNVMeDrive(Inventory* inventory, const Nisqually* nisqually,
                               const Flett* flett, int index) :
    BasicNVMeDrive(flett->getDriveBus(index), inventory, index),
    nisqually(nisqually), flett(flett)
{}

void FlettNVMeDrive::plug([[maybe_unused]] Notifier& notifier)
{
    /* TODO: Probe NVMe MI endpoints on I2C? */
    debug("Drive {NVME_ID} plugged on Flett {FLETT_ID}", "NVME_ID", index,
          "FLETT_ID", flett->getIndex());
    addToInventory(inventory);
}

void FlettNVMeDrive::unplug([[maybe_unused]] Notifier& notifier,
                            [[maybe_unused]] int mode)
{
    if (mode == UNPLUG_REMOVES_INVENTORY)
    {
        removeFromInventory(inventory);
    }
    debug("Drive {NVME_ID} unplugged on Williwakas {WILLIWAKAS_ID}", "NVME_ID",
          index, "WILLIWAKAS_ID", flett->getIndex());
}

std::string FlettNVMeDrive::getInventoryPath() const
{
    std::string williwakasPath =
        Williwakas::getInventoryPathFor(nisqually, flett->getIndex());

    return williwakasPath + "/" + "nvme" + std::to_string(index);
}

bool FlettNVMeDrive::isPresent(SysfsI2CBus bus)
{
    return bus.isDevicePresent(NVMeDrive::eepromAddress);
}

/* Flett */

static const std::map<std::string, std::map<int, int>> flett_mux_slot_map = {
    {"i2c-6", {{0x74, 9}, {0x75, 8}}},
    {"i2c-11", {{0x74, 10}, {0x75, 11}}},
};

static const std::map<int, int> flett_slot_mux_map = {
    {8, 0x75},
    {9, 0x74},
    {10, 0x74},
    {11, 0x75},
};

static const std::map<int, int> flett_slot_eeprom_map = {
    {8, 0x51},
    {9, 0x50},
    {10, 0x50},
    {11, 0x51},
};

/*
 * Yes, this mux channel / drive index mapping really is the case.
 *
 * See flett2z_2021OCT07.pdf, page 27
 */
static const std::map<int, int> flett_channel_drive_map = {
    {0, 0}, {1, 2}, {2, 1}, {3, 3}, {4, 5}, {5, 6}, {6, 4}, {7, 7},
};

/* Reverse map of the above */
static const std::map<int, int> flett_drive_channel_map = {
    {0, 0}, {1, 2}, {2, 1}, {3, 3}, {4, 6}, {5, 4}, {6, 5}, {7, 7},
};

std::string Flett::getInventoryPathFor(const Nisqually* nisqually, int slot)
{
    return nisqually->getInventoryPath() + "/" + "pcieslot" +
           std::to_string(slot) + "/" + "pcie_card" + std::to_string(slot);
}

Flett::Flett(Inventory* inventory, const Nisqually* nisqually, int slot) :
    inventory(inventory), nisqually(nisqually),
    slot(slot), driveConnectors{{
                    Connector<FlettNVMeDrive>(inventory, this->nisqually, this,
                                              flett_channel_drive_map.at(0)),
                    Connector<FlettNVMeDrive>(inventory, this->nisqually, this,
                                              flett_channel_drive_map.at(1)),
                    Connector<FlettNVMeDrive>(inventory, this->nisqually, this,
                                              flett_channel_drive_map.at(2)),
                    Connector<FlettNVMeDrive>(inventory, this->nisqually, this,
                                              flett_channel_drive_map.at(3)),
                    Connector<FlettNVMeDrive>(inventory, this->nisqually, this,
                                              flett_channel_drive_map.at(4)),
                    Connector<FlettNVMeDrive>(inventory, this->nisqually, this,
                                              flett_channel_drive_map.at(5)),
                    Connector<FlettNVMeDrive>(inventory, this->nisqually, this,
                                              flett_channel_drive_map.at(6)),
                    Connector<FlettNVMeDrive>(inventory, this->nisqually, this,
                                              flett_channel_drive_map.at(7)),
                }}
{
    SysfsI2CBus bus = nisqually->getFlettSlotI2CBus(slot);

#if 0 /* FIXME: Well, fix qemu */
    bus.probeDevice("24c02", flett_slot_eeprom_map.at(slot));
#endif
    bus.probeDevice("pca9548", flett_slot_mux_map.at(slot));

    debug("Instantiated Flett in slot {PCIE_SLOT}", "PCIE_SLOT", slot);
}

int Flett::getIndex() const
{
    return Nisqually::getFlettIndex(slot);
}

SysfsI2CBus Flett::getDriveBus(int index) const
{
    SysfsI2CMux flettMux(nisqually->getFlettSlotI2CBus(slot),
                         flett_slot_mux_map.at(slot));

    return {flettMux, flett_drive_channel_map.at(index)};
}

void Flett::plug(Notifier& notifier)
{
    for (std::size_t i = 0; i < presenceAdaptors.size(); i++)
    {
        SysfsI2CBus bus = getDriveBus(flett_channel_drive_map.at(i));
        presenceAdaptors[i] = PolledDevicePresence<FlettNVMeDrive>(
            &driveConnectors.at(i),
            [bus]() { return BasicNVMeDrive::isBasicEndpointPresent(bus); });
        notifier.add(&presenceAdaptors.at(i));
    }
}

void Flett::unplug(Notifier& notifier, int mode)
{
    for (auto& adaptor : presenceAdaptors)
    {
        notifier.remove(&adaptor);
    }

    for (auto& connector : driveConnectors)
    {
        if (connector.isPopulated())
        {
            connector.getDevice().unplug(notifier, mode);
            connector.depopulate();
        }
    }
}
