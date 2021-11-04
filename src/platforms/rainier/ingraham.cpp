/* SPDX-License-Identifier: Apache-2.0 */
#include "platforms/rainier.hpp"
#include "sysfs/i2c.hpp"

#include <filesystem>

namespace fs = std::filesystem;

Ingraham::Ingraham(Inventory* inventory, Nisqually* nisqually) :
    inventory(inventory), nisqually(nisqually)
{}

SysfsI2CBus Ingraham::getPCIeSlotI2CBus(int slot)
{
    return SysfsI2CBus(fs::path(Ingraham::pcie_slot_bus_map.at(slot)));
}

void Ingraham::plug(Notifier& notifier)
{
    nisqually->plug(notifier);
}

void Ingraham::unplug(Notifier& notifier, int mode)
{
    nisqually->unplug(notifier, mode);
}
