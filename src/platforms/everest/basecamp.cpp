/* SPDX-License-Identifier: Apache-2.0 */

#include "platforms/everest.hpp"
#include "sysfs/gpio.hpp"

#include <phosphor-logging/lg2.hpp>

#include <stdexcept>

PHOSPHOR_LOG2_USING;

BasecampNVMeDrive::BasecampNVMeDrive(Inventory* inventory,
                                     const Basecamp* basecamp, int index) :
    BasicNVMeDrive(basecamp->getDriveBus(index), inventory, index,
                   getInventoryPathFor(basecamp, index)),
    basecamp(basecamp)
{}

std::string BasecampNVMeDrive::getInventoryPathFor(const Basecamp* basecamp,
                                                   int index)
{
    return basecamp->getInventoryPath() + "/" + "nvme" + std::to_string(index);
}

void BasecampNVMeDrive::plug([[maybe_unused]] Notifier& notifier)
{
    debug("Drive {NVME_ID} plugged on Basecamp", "NVME_ID", index);
    addToInventory(inventory);
}

void BasecampNVMeDrive::unplug([[maybe_unused]] Notifier& notifier,
                               [[maybe_unused]] int mode)
{
    if (mode == UNPLUG_REMOVES_INVENTORY)
    {
        removeFromInventory(inventory);
    }
    debug("Drive {NVME_ID} unplugged on Basecamp", "NVME_ID", index);
}

void BasecampNVMeDrive::addToInventory(Inventory* inventory)
{
    BasicNVMeDrive::addToInventory(inventory);
    inventory->markPresent(getInventoryPath());
}

void BasecampNVMeDrive::removeFromInventory(Inventory* inventory)
{
    BasicNVMeDrive::addToInventory(inventory);
    inventory->markAbsent(getInventoryPath());
}

Basecamp::Basecamp(Inventory* inventory, const Bellavista* bellavista) :
    inventory(inventory), bellavista(bellavista),
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
{
    SysfsI2CBus root(driveMetadataBus);
    SysfsI2CMux mux(root, driveMetadataMuxAddress);
    SysfsI2CBus bus(mux, driveMetadataMuxChannel);

    SysfsI2CDevice dev = bus.probeDevice("pca9552", drivePresenceDeviceAddress);

    std::string chipName = SysfsGPIOChip(dev).getName().string();
    gpiod::chip chip(chipName, gpiod::chip::OPEN_BY_NAME);

    std::vector<unsigned int> offsets(drivePresenceMap.begin(),
                                      drivePresenceMap.end());

    assert(drivePresenceMap.size() == lines.size());
    for (std::size_t i = 0; i < lines.size(); i++)
    {
        auto line = chip.get_line(drivePresenceMap[i]);
        line.request({program_invocation_short_name,
                      gpiod::line::DIRECTION_INPUT, gpiod::line::ACTIVE_LOW});
        lines[i] = line;
    }
}

SysfsI2CBus Basecamp::getDriveBus(int index) const
{
    SysfsI2CBus root(driveManagementBus);
    SysfsI2CMux driveMux(root, driveMuxMap.at(index));

    return {driveMux, driveChannelMap.at(index)};
}

class BasecampNVMeDrivePresence
{
  public:
    BasecampNVMeDrivePresence() = delete;
    BasecampNVMeDrivePresence(gpiod::line* line, SysfsI2CBus bus) :
        line(line), bus(std::move(bus)), haveEndpoint(false)
    {}

    bool operator()()
    {
        // Once we've observed both GPIO presence and the basic I2C endpoint,
        // only unplug() the drive if the GPIO indicates the drive is unplugged.
        // The I2C endpoint will come and go as the host power state changes.
        if (!line->get_value())
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
    gpiod::line* line;
    SysfsI2CBus bus;
    bool haveEndpoint;
};

void Basecamp::plug(Notifier& notifier)
{
    for (auto& poller : polledDriveConnectors)
    {
        auto index = poller.index();
        auto presence =
            BasecampNVMeDrivePresence(&lines.at(index), getDriveBus(index));
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
