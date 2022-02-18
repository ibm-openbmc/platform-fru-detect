/* SPDX-License-Identifier: Apache-2.0 */

#include "platforms/everest.hpp"
#include "sysfs/gpio.hpp"

#include <cerrno>
#include <stdexcept>

Bellavista::Bellavista(Inventory* inventory) :
    inventory(inventory),
    basecampPresenceChip(
        SysfsGPIOChip(
            std::filesystem::path(Bellavista::basecampPresenceDevicePath))
            .getName()
            .string(),
        gpiod::chip::OPEN_BY_NAME),
    basecampConnector(this->inventory, this)
{
    basecampPresenceLine =
        basecampPresenceChip.get_line(Bellavista::basecampPresenceOffset);
    basecampPresenceLine.request({program_invocation_short_name,
                                  gpiod::line::DIRECTION_INPUT,
                                  gpiod::line::ACTIVE_LOW});
}

void Bellavista::plug(Notifier& notifier)
{
    presenceAdaptor =
        PolledDevicePresence<Basecamp>(&basecampConnector, [this]() {
            return this->basecampPresenceLine.get_value();
        });
    notifier.add(&presenceAdaptor);
}

void Bellavista::unplug(Notifier& notifier, int mode)
{
    notifier.remove(&presenceAdaptor);
    if (basecampConnector.isPopulated())
    {
        basecampConnector.getDevice().unplug(notifier, mode);
        basecampConnector.depopulate();
    }
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
