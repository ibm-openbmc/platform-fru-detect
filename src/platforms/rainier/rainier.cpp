/* SPDX-License-Identifier: Apache-2.0 */

#include "platforms/rainier.hpp"

void Rainier::enrollWith(PlatformManager& pm)
{
    pm.enrollPlatform("Rainier 1S4U", this);
    pm.enrollPlatform("Rainier 4U", this);
    pm.enrollPlatform("Rainier 2U", this);
}

void Rainier::detectFrus(Notifier& notifier, Inventory* inventory)
{
    Ingraham ingraham(inventory);

    /* Cold-plug devices */
    ingraham.plug(notifier);

    /* Hot-plug devices */
    notifier.run();

    /* Clean up the application state but leave the inventory in-tact. */
    ingraham.unplug(notifier, ingraham.UNPLUG_RETAINS_INVENTORY);
}
