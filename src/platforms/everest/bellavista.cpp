/* SPDX-License-Identifier: Apache-2.0 */

#include "platforms/everest.hpp"
#include "sysfs/gpio.hpp"

#include <cerrno>
#include <stdexcept>

Bellavista::Bellavista(Inventory* inventory) :
    inventory(inventory), polledBasecampConnector(0, this->inventory, this)
{}

void Bellavista::plug(Notifier& notifier)
{
    std::filesystem::path path(Bellavista::basecampPresenceDevicePath);
    SysfsGPIOChip sysfsChip(path);
    gpiod::chip chip(sysfsChip.getName().string(), gpiod::chip::OPEN_BY_NAME);

    auto line = chip.get_line(Bellavista::basecampPresenceOffset);

    line.request({program_invocation_short_name, gpiod::line::DIRECTION_INPUT,
                  gpiod::line::ACTIVE_LOW});

    polledBasecampConnector.start(
        notifier, [line = std::move(line)]() { return line.get_value(); });
}

void Bellavista::unplug(Notifier& notifier, int mode)
{
    polledBasecampConnector.stop(notifier, mode);
}

std::string Bellavista::getInventoryPath() const
{
    return "/system/chassis/motherboard";
}

void Bellavista::addToInventory([[maybe_unused]] Inventory* inventory)
{
    throw std::logic_error("Not implemented");
}

void Bellavista::removeFromInventory([[maybe_unused]] Inventory* inventory)
{
    throw std::logic_error("Not implemented");
}
