/* SPDX-License-Identifier: Apache-2.0 */
#include "devices/nvme.hpp"
#include "inventory.hpp"
#include "platforms/rainier.hpp"
#include "sysfs/gpio.hpp"

#include <gpiod.hpp>
#include <phosphor-logging/lg2.hpp>

#include <cassert>
#include <iostream>

PHOSPHOR_LOG2_USING;

/* WilliwakasNVMeDrive */

WilliwakasNVMeDrive::WilliwakasNVMeDrive(Inventory& inventory,
                                         const Williwakas& williwakas,
                                         int index) :
    NVMeDrive(inventory, index),
    williwakas(williwakas)
{}

std::string WilliwakasNVMeDrive::getInventoryPath() const
{
    return williwakas.getInventoryPath() + "/" + "nvme" + std::to_string(index);
}

void WilliwakasNVMeDrive::addToInventory(Inventory& inventory)
{
    std::string path = getInventoryPath();

    markPresent(path, inventory);
}

void WilliwakasNVMeDrive::markPresent(const std::string& path,
                                      Inventory& inventory) const
{
    inventory.markPresent(path);
}

/* Williwakas */

static constexpr const char* name = "foo";

std::string Williwakas::getInventoryPathFor(const Nisqually& nisqually,
                                            int index)
{
    return nisqually.getInventoryPath() + "/" + "disk_backplane" +
           std::to_string(index);
}

Williwakas::Williwakas(Inventory& inventory, Nisqually& nisqually, int index) :
    inventory(inventory), nisqually(nisqually), index(index)
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

std::string Williwakas::getInventoryPath() const
{
    return getInventoryPathFor(nisqually, index);
}

void Williwakas::addToInventory([[maybe_unused]] Inventory& inventory)
{
    std::logic_error("Unimplemented");
}

void Williwakas::plug()
{
    detectDrives(drives);
    for (auto& drive : drives)
    {
        drive.addToInventory(inventory);
    }
}

void Williwakas::detectDrives(std::vector<WilliwakasNVMeDrive>& drives)
{
    assert(drives.empty() && "Already detected drives");

    std::vector<int> presence = lines.get_values();

    for (size_t id = 0; id < presence.size(); id++)
    {
        /* FIXME: work around libgpiod bug */
        if (presence.at(index))
        {
            info("Found drive {NVME_ID} on backplane {WILLIWAKAS_ID}",
                 "NVME_ID", id, "WILLIWAKAS_ID", index);
            drives.emplace_back(WilliwakasNVMeDrive(inventory, *this, id));
        }
        else
        {
            debug("Drive {NVME_ID} not present on backplane {WILLIWAKAS_ID}",
                  "NVME_ID", id, "WILLIWAKAS_ID", index);
        }
    }

    debug("Found {NVME_COUNT} drives for backplane {WILLIWAKAS_ID}",
          "NVME_COUNT", drives.size(), "WILLIWAKAS_ID", index);
}
