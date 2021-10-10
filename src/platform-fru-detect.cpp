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
    if (!Platform::isSupported())
    {
        warning("Unsupported platform: '{PLATFORM_MODEL}'", "PLATFORM_MODEL",
                Platform::getModel());
        return 0;
    }

    info("Detecting FRUs for '{PLATFORM_MODEL}'", "PLATFORM_MODEL",
         Platform::getModel());

    sdbusplus::bus::bus dbus = sdbusplus::bus::new_default();
    Inventory inventory(dbus);
    Ingraham ingraham(inventory);
    ingraham.plug();

    return 0;
}
