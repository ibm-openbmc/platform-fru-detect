/* SPDX-License-Identifier: Apache-2.0 */

#include "platforms/everest.hpp"

Tola::Tola(Inventory* inventory) : inventory(inventory), bellavista(inventory)
{}

void Tola::plug(Notifier& notifier)
{
    bellavista.plug(notifier);
}

void Tola::unplug(Notifier& notifier, int mode)
{
    bellavista.unplug(notifier, mode);
}
