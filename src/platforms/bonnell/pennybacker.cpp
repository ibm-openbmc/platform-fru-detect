/* SPDX-License-Identifier: Apache-2.0 */

#include "platforms/bonnell.hpp"
#include "sysfs/gpio.hpp"

#include <stdexcept>

Pennybacker::Pennybacker(Inventory* inventory) :
    inventory(inventory), polledDriskillConnector(0, this->inventory, this)
{}

SysfsI2CBus Pennybacker::getDrivePresenceBus()
{
    return SysfsI2CBus{drivePresenceBus};
}

SysfsI2CBus Pennybacker::getDriveBus(int driveIndex)
{
    SysfsI2CMux driveMux(driveManagementMux);

    return {driveMux, driveChannelMap.at(driveIndex)};
}

void Pennybacker::plug(Notifier& notifier)
{
    std::filesystem::path path(Pennybacker::driskillPresenceDevicePath);
    SysfsGPIOChip sysfsChip(path);
    gpiod::chip chip(sysfsChip.getName().string(), gpiod::chip::OPEN_BY_NAME);

    auto line = chip.get_line(Pennybacker::driskillPresenceOffset);

    line.request({program_invocation_short_name, gpiod::line::DIRECTION_INPUT,
                  gpiod::line::ACTIVE_LOW});

    polledDriskillConnector.start(
        notifier, [line = std::move(line)]() { return line.get_value(); });
}

void Pennybacker::unplug(Notifier& notifier, int mode)
{
    polledDriskillConnector.stop(notifier, mode);
}

std::string Pennybacker::getInventoryPath() const
{
    return "/system/chassis/motherboard";
}

void Pennybacker::addToInventory([[maybe_unused]] Inventory* inventory)
{
    throw std::logic_error("Not implemented");
}

void Pennybacker::removeFromInventory([[maybe_unused]] Inventory* inventory)
{
    throw std::logic_error("Not implemented");
}
