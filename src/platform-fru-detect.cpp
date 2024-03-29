/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */
#include "dbus.hpp"
#include "environment.hpp"
#include "inventory.hpp"
#include "platform.hpp"
#include "platforms/bonnell.hpp"
#include "platforms/everest.hpp"
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

int main()
{
    PlatformManager pm;

    Bonnell bonnell;
    bonnell.enrollWith(pm);

    Everest everest;
    everest.enrollWith(pm);

    Rainier0z rainier0z;
    rainier0z.enrollWith(pm);

    Rainier1z rainier1z;
    rainier1z.enrollWith(pm);

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

    sdbusplus::bus::bus dbus = sdbusplus::bus::new_default();
    Notifier notifier;
    InventoryManager inventory(dbus);

    DBusNotifySink dbusSink(dbus);
    notifier.add(&dbusSink);

    em.run(pm, notifier, &inventory);

    notifier.remove(&dbusSink);

    return 0;
}
