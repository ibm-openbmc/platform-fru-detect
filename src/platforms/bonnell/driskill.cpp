/* SPDX-License-Identifier: Apache-2.0 */

#include "platforms/bonnell.hpp"
#include "sysfs/gpio.hpp"

DriskillNVMeDrive::DriskillNVMeDrive(Inventory* inventory,
                                     const Driskill* driskill, int index) :
    inventory(inventory), driskill(driskill), index(index)
{}

void DriskillNVMeDrive::plug([[maybe_unused]] Notifier& notifier)
{
    drive.emplace(Driskill::getDriveBus(index), getInventoryPath());
    addToInventory(inventory);
    debug("Drive {NVME_ID} plugged on Driskill", "NVME_ID", index);
}

void DriskillNVMeDrive::unplug([[maybe_unused]] Notifier& notifier,
                               [[maybe_unused]] int mode)
{
    if (!drive)
    {
        drive.emplace(getInventoryPath());
    }
    if (mode == UNPLUG_REMOVES_INVENTORY)
    {
        removeFromInventory(inventory);
    }
    debug("Drive {NVME_ID} unplugged on Driskill", "NVME_ID", index);
}

std::string DriskillNVMeDrive::getInventoryPath() const
{
    return driskill->getInventoryPath() + "/" + "nvme" + std::to_string(index) +
           "/" + "dp0_drive" + std::to_string(index);
}

void DriskillNVMeDrive::addToInventory(Inventory* inventory)
{
    drive->addToInventory(inventory);
    inventory->markPresent(getInventoryPath());
}

void DriskillNVMeDrive::removeFromInventory(Inventory* inventory)
{
    inventory->markAbsent(getInventoryPath());
    drive->removeFromInventory(inventory);
}

Driskill::Driskill(Inventory* inventory, const Pennybacker* pennybacker) :
    pennybacker(pennybacker),
    polledDriveConnectors{{
        PolledConnector<DriskillNVMeDrive>(0, inventory, this, 0),
        PolledConnector<DriskillNVMeDrive>(1, inventory, this, 1),
        PolledConnector<DriskillNVMeDrive>(2, inventory, this, 2),
        PolledConnector<DriskillNVMeDrive>(3, inventory, this, 3),
    }}
{}

SysfsI2CBus Driskill::getDriveBus(int driveIndex)
{
    return Pennybacker::getDriveBus(driveIndex);
}

void Driskill::plug(Notifier& notifier)
{
    SysfsI2CBus bus = Pennybacker::getDrivePresenceBus();
    SysfsI2CDevice dev = bus.probeDevice("pca9849", drivePresenceDeviceAddress);

    std::string chipName = SysfsGPIOChip(dev).getName().string();
    gpiod::chip chip(chipName, gpiod::chip::OPEN_BY_NAME);

    for (auto& poller : polledDriveConnectors)
    {
        int index = poller.index();
        auto line = chip.get_line(drivePresenceMap[index]);
        line.request({program_invocation_short_name,
                      gpiod::line::DIRECTION_INPUT, gpiod::line::ACTIVE_LOW});
        auto presence = NVMeDrivePresence(std::move(line),
                                          Pennybacker::getDriveBus(index));
        poller.start(notifier, std::move(presence));
    }
}

void Driskill::unplug(Notifier& notifier, int mode)
{
    for (auto& poller : polledDriveConnectors)
    {
        poller.stop(notifier, mode);
    }
}

/* clang-format off */
/*
root@bonn003:~# busctl tree xyz.openbmc_project.Inventory.Manager | grep disk_backplane
         |-/xyz/openbmc_project/inventory/system/chassis/motherboard/disk_backplane0
         | |-/xyz/openbmc_project/inventory/system/chassis/motherboard/disk_backplane0/dp0_connector0
         | |-/xyz/openbmc_project/inventory/system/chassis/motherboard/disk_backplane0/dp0_connector1
         | |-/xyz/openbmc_project/inventory/system/chassis/motherboard/disk_backplane0/dp0_connector2
         | |-/xyz/openbmc_project/inventory/system/chassis/motherboard/disk_backplane0/dp0_connector3
         | |-/xyz/openbmc_project/inventory/system/chassis/motherboard/disk_backplane0/nvme0
         | | `-/xyz/openbmc_project/inventory/system/chassis/motherboard/disk_backplane0/nvme0/dp0_drive0
         | |-/xyz/openbmc_project/inventory/system/chassis/motherboard/disk_backplane0/nvme1
         | | `-/xyz/openbmc_project/inventory/system/chassis/motherboard/disk_backplane0/nvme1/dp0_drive1
         | |-/xyz/openbmc_project/inventory/system/chassis/motherboard/disk_backplane0/nvme2
         | | `-/xyz/openbmc_project/inventory/system/chassis/motherboard/disk_backplane0/nvme2/dp0_drive2
         | `-/xyz/openbmc_project/inventory/system/chassis/motherboard/disk_backplane0/nvme3
         |   `-/xyz/openbmc_project/inventory/system/chassis/motherboard/disk_backplane0/nvme3/dp0_drive3
 */
/* clang-format on */

std::string Driskill::getInventoryPath() const
{
    return pennybacker->getInventoryPath() + "/" + "disk_backplane0";
}

void Driskill::addToInventory([[maybe_unused]] Inventory* inventory)
{
    throw std::logic_error("Not implemented");
}

void Driskill::removeFromInventory([[maybe_unused]] Inventory* inventory)
{
    throw std::logic_error("Not implemented");
}
