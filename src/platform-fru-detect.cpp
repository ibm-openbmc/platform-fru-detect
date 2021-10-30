/* SPDX-License-Identifier: Apache-2.0 */
#include "inventory.hpp"
#include "platform.hpp"
#include "platforms/rainier.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>

#include <array>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <variant>
#include <vector>

PHOSPHOR_LOG2_USING;

int main(void)
{
    if (!PlatformManager::isSupportedPlatform())
    {
        warning("Unsupported platform: '{PLATFORM_MODEL}'", "PLATFORM_MODEL",
                PlatformManager::getPlatformModel());
        return 0;
    }

    info("Detecting FRUs for '{PLATFORM_MODEL}'", "PLATFORM_MODEL",
         PlatformManager::getPlatformModel());

    sdbusplus::bus::bus dbus = sdbusplus::bus::new_default();
    InventoryManager inventory(dbus);
    PublishWhenPresentInventoryDecorator decoratedInventory(&inventory);
    Ingraham ingraham(&decoratedInventory);
    Notifier notifier;
    ingraham.plug(notifier);
    notifier.run();
    /* Clean up the application state but leave the inventory in-tact. */
    ingraham.unplug(notifier, ingraham.UNPLUG_RETAINS_INVENTORY);

    return 0;
}
