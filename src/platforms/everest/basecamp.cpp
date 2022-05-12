/* SPDX-License-Identifier: Apache-2.0 */

#include "platforms/everest.hpp"
#include "sysfs/gpio.hpp"

#include <phosphor-logging/lg2.hpp>

#include <stdexcept>

PHOSPHOR_LOG2_USING;

BasecampNVMeDrive::BasecampNVMeDrive(Inventory* inventory,
                                     const Basecamp* basecamp, int index) :
    BasicNVMeDrive(basecamp->getDriveBus(index), inventory, index),
    basecamp(basecamp)
{}

std::string BasecampNVMeDrive::getInventoryPath() const
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
    driveConnectors{{Connector<BasecampNVMeDrive>(inventory, this, 0),
                     Connector<BasecampNVMeDrive>(inventory, this, 1),
                     Connector<BasecampNVMeDrive>(inventory, this, 2),
                     Connector<BasecampNVMeDrive>(inventory, this, 3),
                     Connector<BasecampNVMeDrive>(inventory, this, 4),
                     Connector<BasecampNVMeDrive>(inventory, this, 5),
                     Connector<BasecampNVMeDrive>(inventory, this, 6),
                     Connector<BasecampNVMeDrive>(inventory, this, 7),
                     Connector<BasecampNVMeDrive>(inventory, this, 8),
                     Connector<BasecampNVMeDrive>(inventory, this, 9)}}
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

    return SysfsI2CBus(driveMux, driveChannelMap.at(index));
}

void Basecamp::plug(Notifier& notifier)
{
    for (std::size_t i = 0; i < presenceAdaptors.size(); i++)
    {
        gpiod::line* line = &lines[i];
        SysfsI2CBus bus = getDriveBus(i);
        presenceAdaptors[i] = PolledDevicePresence<BasecampNVMeDrive>(
            &driveConnectors.at(i), [line, bus]() {
                return line->get_value() && BasicNVMeDrive::isDriveReady(bus);
            });
        notifier.add(&presenceAdaptors.at(i));
    }
}

void Basecamp::unplug([[maybe_unused]] Notifier& notifier,
                      [[maybe_unused]] int mode)
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
