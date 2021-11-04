/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

#include "sysfs/i2c.hpp"

#include <vector>

namespace i2c
{
bool isDeviceResponsive(const SysfsI2CBus& bus, int address);
void oneshotSMBusBlockRead(const SysfsI2CBus& bus, int address, uint8_t command,
                           std::vector<uint8_t>& data);
} // namespace i2c
