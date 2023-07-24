/* SPDX-License-Identifier: Apache-2.0 */

#include "platforms/everest.hpp"
#include "sysfs/gpio.hpp"

#include <phosphor-logging/lg2.hpp>

#include <stdexcept>

PHOSPHOR_LOG2_USING;

BasecampNVMeDrive::BasecampNVMeDrive(Inventory* inventory,
                                     const Basecamp* basecamp, int index) :
    inventory(inventory),
    basecamp(basecamp), index(index)
{}

void BasecampNVMeDrive::plug([[maybe_unused]] Notifier& notifier)
{
    drive.emplace(basecamp->getDriveBus(index), getInventoryPath());
    addToInventory(inventory);
    debug("Drive {NVME_ID} plugged on Basecamp", "NVME_ID", index);
}

void BasecampNVMeDrive::unplug([[maybe_unused]] Notifier& notifier,
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
    debug("Drive {NVME_ID} unplugged on Basecamp", "NVME_ID", index);
}

std::string BasecampNVMeDrive::getInventoryPath() const
{
    return basecamp->getInventoryPath() + "/" + "nvme" + std::to_string(index) +
           "/" + "drive" + std::to_string(index);
}

void BasecampNVMeDrive::addToInventory(Inventory* inventory)
{
    drive->addToInventory(inventory);
    inventory->markPresent(getInventoryPath());
}

void BasecampNVMeDrive::removeFromInventory(Inventory* inventory)
{
    inventory->markAbsent(getInventoryPath());
    drive->removeFromInventory(inventory);
}

Basecamp::Basecamp(Inventory* inventory, const Bellavista* bellavista) :
    bellavista(bellavista),
    polledDriveConnectors{{
        PolledConnector<BasecampNVMeDrive>(0, inventory, this, 0),
        PolledConnector<BasecampNVMeDrive>(1, inventory, this, 1),
        PolledConnector<BasecampNVMeDrive>(2, inventory, this, 2),
        PolledConnector<BasecampNVMeDrive>(3, inventory, this, 3),
        PolledConnector<BasecampNVMeDrive>(4, inventory, this, 4),
        PolledConnector<BasecampNVMeDrive>(5, inventory, this, 5),
        PolledConnector<BasecampNVMeDrive>(6, inventory, this, 6),
        PolledConnector<BasecampNVMeDrive>(7, inventory, this, 7),
        PolledConnector<BasecampNVMeDrive>(8, inventory, this, 8),
        PolledConnector<BasecampNVMeDrive>(9, inventory, this, 9),
    }}
{}

SysfsI2CBus Basecamp::getDriveBus(int index)
{
    SysfsI2CBus root(driveManagementBus);
    SysfsI2CMux driveMux(root, driveMuxMap.at(index));

    return {driveMux, driveChannelMap.at(index)};
}

class BasecampNVMeDrivePresence
{
  public:
    BasecampNVMeDrivePresence() = delete;
    BasecampNVMeDrivePresence(gpiod::line&& line, SysfsI2CBus bus) :
        line(line), bus(std::move(bus)), haveEndpoint(false)
    {}

    bool operator()()
    {
        // Once we've observed both GPIO presence and the basic I2C endpoint,
        // only unplug() the drive if the GPIO indicates the drive is unplugged.
        // The I2C endpoint will come and go as the host power state changes.
        if (line.get_value() == 0)
        {
            haveEndpoint = false;
            return false;
        }

        if (!haveEndpoint)
        {
            haveEndpoint = BasicNVMeDrive::isBasicEndpointPresent(bus);
        }

        return haveEndpoint;
    }

  private:
    gpiod::line line;
    SysfsI2CBus bus;
    bool haveEndpoint;
};

void Basecamp::plug(Notifier& notifier)
{
    SysfsI2CBus root(driveMetadataBus);
    SysfsI2CMux mux(root, driveMetadataMuxAddress);
    SysfsI2CBus bus(mux, driveMetadataMuxChannel);

    SysfsI2CDevice dev = bus.probeDevice("pca9552", drivePresenceDeviceAddress);

    std::string chipName = SysfsGPIOChip(dev).getName().string();
    gpiod::chip chip(chipName, gpiod::chip::OPEN_BY_NAME);

    for (auto& poller : polledDriveConnectors)
    {
        int index = poller.index();
        auto line = chip.get_line(drivePresenceMap[index]);
        line.request({program_invocation_short_name,
                      gpiod::line::DIRECTION_INPUT, gpiod::line::ACTIVE_LOW});
        auto presence = BasecampNVMeDrivePresence(std::move(line),
                                                  getDriveBus(index));
        poller.start(notifier, std::move(presence));
    }
}

void Basecamp::unplug([[maybe_unused]] Notifier& notifier,
                      [[maybe_unused]] int mode)
{
    for (auto& poller : polledDriveConnectors)
    {
        poller.stop(notifier, mode);
    }

    try
    {
        SysfsI2CBus root(driveMetadataBus);
        SysfsI2CMux mux(root, driveMetadataMuxAddress);
        SysfsI2CBus bus(mux, driveMetadataMuxChannel);

        bus.removeDevice(drivePresenceDeviceAddress);
    }
    catch (const std::error_condition& err)
    {
        if (err.value() != ENOENT)
        {
            throw err;
        }
    }
}

std::string Basecamp::getInventoryPath() const
{
    return bellavista->getInventoryPath() + "/" + "dasd_backplane";
}

void Basecamp::addToInventory([[maybe_unused]] Inventory* inventory)
{
    throw std::logic_error("Not implemented");
}

void Basecamp::removeFromInventory([[maybe_unused]] Inventory* inventory)
{
    throw std::logic_error("Not implemented");
}
