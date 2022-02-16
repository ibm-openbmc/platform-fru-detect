/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */
#include "sysfs/eeprom.hpp"

#include "devices/nvme.hpp"

bool SysfsEEPROM::isEEPROM(std::filesystem::path path)
{
    return path.filename().string() == "eeprom";
}

SysfsI2CDevice SysfsEEPROM::getDevice()
{
    return {path.parent_path()};
}
