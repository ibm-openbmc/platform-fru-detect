/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */
#pragma once

#include "sysfs/i2c.hpp"
#include "sysfs/sysfs.hpp"

#include <filesystem>

class SysfsEEPROM : public SysfsEntry
{
  public:
    static bool isEEPROM(std::filesystem::path path);

    SysfsEEPROM(const SysfsEEPROM& eeprom) : SysfsEntry(eeprom.path)
    {}
    SysfsEEPROM(std::filesystem::path path) : SysfsEntry(path)
    {}
    SysfsEEPROM(SysfsI2CDevice device) : SysfsEntry(device.getPath() / "eeprom")
    {}

    SysfsEEPROM(SysfsEEPROM& other) = default;

    SysfsI2CDevice getDevice();
};
