/* SPDX-License-Identifier: Apache-2.0 */
#include "environment.hpp"
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
    PlatformManager pm;

    sdbusplus::bus::bus dbus = sdbusplus::bus::new_default();
    InventoryManager inventory(dbus);
    PublishWhenPresentInventoryDecorator decoratedInventory(&inventory);
    std::unique_ptr<Rainier0z> rainier0z;
    std::unique_ptr<Rainier1z> rainier1z;

    if (Rainier0z::isPresent(pm.getPlatformModel()))
    {
        rainier0z = std::make_unique<Rainier0z>(&decoratedInventory);
        rainier0z->enrollWith(pm);
    }
    else if (Rainier1z::isPresent(pm.getPlatformModel()))
    {
        rainier1z = std::make_unique<Rainier1z>(&decoratedInventory);
        rainier1z->enrollWith(pm);
    }

    if (!pm.isSupportedPlatform())
    {
        warning("Unsupported platform: '{PLATFORM_MODEL}'", "PLATFORM_MODEL",
                pm.getPlatformModel());
        return 0;
    }

    info("Detecting FRUs for '{PLATFORM_MODEL}'", "PLATFORM_MODEL",
         pm.getPlatformModel());

    EnvironmentManager em;

    HardwareExecutionEnvironment hardware;
    em.enrollEnvironment(&hardware);

    SimicsExecutionEnvironment simics;
    em.enrollEnvironment(&simics);

    em.run(pm);

    return 0;
}
