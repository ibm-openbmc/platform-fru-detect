/* SPDX-License-Identifier: Apache-2.0 */
#include "platforms/rainier.hpp"

#include <algorithm>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

Ingraham::Ingraham(Inventory& inventory) : inventory(inventory)
{}

SysfsI2CBus Ingraham::getPCIeSlotI2CBus(int slot)
{
    return SysfsI2CBus(fs::path(Ingraham::pcie_slot_bus_map.at(slot)));
}

Nisqually Ingraham::getBackplane() const
{
    Nisqually backplane{};

    backplane.probe();

    return backplane;
}
