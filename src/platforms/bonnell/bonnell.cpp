/* SPDX-License-Identifier: Apache-2.0 */

#include "platforms/bonnell.hpp"

void Bonnell::enrollWith(PlatformManager& pm)
{
    pm.enrollPlatform("Bonnell", this);
    pm.enrollPlatform("Balcones", this);
}

void Bonnell::detectFrus(Notifier& notifier, Inventory* inventory)
{
    Pennybacker pennybacker(inventory);

    /* Cold-plug devices */
    pennybacker.plug(notifier);

    /* Hot-plug devices */
    notifier.run();

    /* Clean up the application state but leave the inventory in-tact */
    pennybacker.unplug(notifier, Pennybacker::UNPLUG_RETAINS_INVENTORY);
}
