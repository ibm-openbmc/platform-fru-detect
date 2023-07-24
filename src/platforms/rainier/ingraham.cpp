/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */
#include "platforms/rainier.hpp"
#include "sysfs/i2c.hpp"

#include <filesystem>

namespace fs = std::filesystem;

Ingraham::Ingraham(Nisqually* nisqually) : nisqually(nisqually) {}

SysfsI2CBus Ingraham::getPCIeSlotI2CBus(int slot)
{
    return {fs::path(Ingraham::pcieSlotBusMap.at(slot))};
}

void Ingraham::plug(Notifier& notifier)
{
    nisqually->plug(notifier);
}

void Ingraham::unplug(Notifier& notifier, int mode)
{
    nisqually->unplug(notifier, mode);
}
