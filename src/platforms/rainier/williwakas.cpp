/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */
#include "devices/nvme.hpp"
#include "inventory.hpp"
#include "platforms/rainier.hpp"
#include "sysfs/gpio.hpp"

#include <gpiod.hpp>
#include <phosphor-logging/lg2.hpp>

#include <cassert>
#include <iostream>
#include <stdexcept>

PHOSPHOR_LOG2_USING;

/* WilliwakasNVMeDrive */

WilliwakasNVMeDrive::WilliwakasNVMeDrive(Inventory* inventory,
                                         const Williwakas* williwakas,
                                         int index) :
    NVMeDrive(inventory, index),
    williwakas(williwakas)
{}

void WilliwakasNVMeDrive::plug([[maybe_unused]] Notifier& notifier)
{
    debug("Drive {NVME_ID} plugged on Williwakas {WILLIWAKAS_ID}", "NVME_ID",
          index, "WILLIWAKAS_ID", williwakas->getIndex());
    addToInventory(inventory);
}

void WilliwakasNVMeDrive::unplug([[maybe_unused]] Notifier& notifier,
                                 [[maybe_unused]] int mode)
{
    if (mode == UNPLUG_REMOVES_INVENTORY)
    {
        removeFromInventory(inventory);
    }
    debug("Drive {NVME_ID} unplugged on Williwakas {WILLIWAKAS_ID}", "NVME_ID",
          index, "WILLIWAKAS_ID", williwakas->getIndex());
}

std::string WilliwakasNVMeDrive::getInventoryPath() const
{
    return williwakas->getInventoryPath() + "/" + "nvme" +
           std::to_string(index);
}

void WilliwakasNVMeDrive::addToInventory(Inventory* inventory)
{
    std::string path = getInventoryPath();

    inventory->markPresent(path);
}

void WilliwakasNVMeDrive::removeFromInventory(Inventory* inventory)
{
    std::string path = getInventoryPath();

    inventory->markAbsent(path);
}

/* Williwakas */

std::string Williwakas::getInventoryPathFor(const Nisqually* nisqually,
                                            int index)
{
    return nisqually->getInventoryPath() + "/" + "disk_backplane" +
           std::to_string(index);
}

Williwakas::Williwakas(Inventory* inventory, const Nisqually* nisqually,
                       int index) :
    inventory(inventory),
    nisqually(nisqually),
    index(index), driveConnectors{{
                      Connector<WilliwakasNVMeDrive>(inventory, this, 0),
                      Connector<WilliwakasNVMeDrive>(inventory, this, 1),
                      Connector<WilliwakasNVMeDrive>(inventory, this, 2),
                      Connector<WilliwakasNVMeDrive>(inventory, this, 3),
                      Connector<WilliwakasNVMeDrive>(inventory, this, 4),
                      Connector<WilliwakasNVMeDrive>(inventory, this, 5),
                      Connector<WilliwakasNVMeDrive>(inventory, this, 6),
                      Connector<WilliwakasNVMeDrive>(inventory, this, 7),
                  }}
{
    SysfsI2CBus bus(Williwakas::driveBackplaneBus.at(index));

    SysfsI2CDevice dev =
        bus.probeDevice("pca9552", Williwakas::drivePresenceDeviceAddress);

    std::string chipName = SysfsGPIOChip(dev).getName().string();

    gpiod::chip chip(chipName, gpiod::chip::OPEN_BY_NAME);

    std::vector<unsigned int> offsets(Williwakas::drivePresenceMap.begin(),
                                      Williwakas::drivePresenceMap.end());

    assert(Williwakas::drive_presence_map.size() == lines.size());
    for (std::size_t i = 0; i < lines.size(); i++)
    {
        auto line = chip.get_line(Williwakas::drivePresenceMap[i]);
        line.request({program_invocation_short_name,
                      gpiod::line::DIRECTION_INPUT, gpiod::line::ACTIVE_LOW});
        lines[i] = line;
    }

    debug("Constructed drive backplane for index {WILLIWAKAS_ID}",
          "WILLIWAKAS_ID", index);
}

int Williwakas::getIndex() const
{
    return index;
}

std::string Williwakas::getInventoryPath() const
{
    return getInventoryPathFor(nisqually, index);
}

void Williwakas::addToInventory([[maybe_unused]] Inventory* inventory)
{
    throw std::logic_error("Unimplemented");
}

void Williwakas::removeFromInventory([[maybe_unused]] Inventory* inventory)
{
    throw std::logic_error("Unimplemented");
}

void Williwakas::plug(Notifier& notifier)
{
    assert(driveConnectors.size() == lines.size());
    for (std::size_t i = 0; i < driveConnectors.size(); i++)
    {
        gpiod::line* line = &lines[i];
        presenceAdaptors[i] = PolledDevicePresence<WilliwakasNVMeDrive>(
            &driveConnectors.at(i), [line]() { return line->get_value(); });
        notifier.add(&presenceAdaptors.at(i));
    }
}

void Williwakas::unplug(Notifier& notifier, int mode)
{
    for (auto& presenceAdaptor : presenceAdaptors)
    {
        notifier.remove(&presenceAdaptor);
    }

    for (auto& connector : driveConnectors)
    {
        connector.depopulate(notifier, mode);
    }
}
