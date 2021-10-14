/* SPDX-License-Identifier: Apache-2.0 */
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

WilliwakasNVMeDrive::WilliwakasNVMeDrive(Inventory& inventory,
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

void WilliwakasNVMeDrive::addToInventory(Inventory& inventory)
{
    std::string path = getInventoryPath();

    markPresent(path, inventory);
}

void WilliwakasNVMeDrive::removeFromInventory(Inventory& inventory)
{
    std::string path = getInventoryPath();

    inventory.markAbsent(path);
}

void WilliwakasNVMeDrive::markPresent(const std::string& path,
                                      Inventory& inventory) const
{
    inventory.markPresent(path);
}

/* Williwakas */

static constexpr const char* name = "foo";

std::string Williwakas::getInventoryPathFor(const Nisqually* nisqually,
                                            int index)
{
    return nisqually->getInventoryPath() + "/" + "disk_backplane" +
           std::to_string(index);
}

Williwakas::Williwakas(Inventory& inventory, const Nisqually* nisqually,
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
    SysfsI2CBus bus(Williwakas::drive_backplane_bus.at(index));

    SysfsI2CDevice dev =
        bus.probeDevice("pca9552", Williwakas::drivePresenceDeviceAddress);

    std::string chipName = SysfsGPIOChip(dev).getName().string();

    gpiod::chip chip(chipName, gpiod::chip::OPEN_BY_NAME);

    std::vector<unsigned int> offsets(Williwakas::drive_presence_map.begin(),
                                      Williwakas::drive_presence_map.end());

    lines = chip.get_lines(offsets);

    lines.request(
        {name, gpiod::line::DIRECTION_INPUT, gpiod::line::ACTIVE_LOW});

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

void Williwakas::addToInventory([[maybe_unused]] Inventory& inventory)
{
    std::logic_error("Unimplemented");
}

void Williwakas::removeFromInventory([[maybe_unused]] Inventory& inventory)
{
    std::logic_error("Unimplemented");
}

void Williwakas::plug(Notifier& notifier)
{
    for (std::size_t i = 0; i < driveConnectors.size(); i++)
    {
        gpiod::line& line = lines.get(i);
        presenceAdaptors[i] = PolledGPIODevicePresence<WilliwakasNVMeDrive>(
            &line, &driveConnectors.at(i));
        notifier.add(&presenceAdaptors.at(i));
    }

    detectDrives(notifier);
}

void Williwakas::unplug(Notifier& notifier, int mode)
{
    for (auto& presenceAdaptor : presenceAdaptors)
    {
        notifier.remove(&presenceAdaptor);
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

void Williwakas::detectDrives(Notifier& notifier)
{
    debug("Locating NVMe drives from drive backplane {WILLIWAKAS_ID}",
          "WILLIWAKAS_ID", index);

    std::vector<int> presence = lines.get_values();

    for (std::size_t i = 0; i < presence.size(); i++)
    {
        /* FIXME: work around libgpiod bug */
        if (presence.at(index))
        {
            driveConnectors[i].populate();
            driveConnectors[i].getDevice().plug(notifier);
            info("Found drive {NVME_ID} on backplane {WILLIWAKAS_ID}",
                 "NVME_ID", i, "WILLIWAKAS_ID", index);
        }
        else
        {
            debug("Drive {NVME_ID} not present on backplane {WILLIWAKAS_ID}",
                  "NVME_ID", i, "WILLIWAKAS_ID", index);
        }
    }
}
