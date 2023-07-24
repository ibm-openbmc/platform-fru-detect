/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */
#pragma once

#include "sysfs/i2c.hpp"
#include "sysfs/sysfs.hpp"

#include <filesystem>

class SysfsEEPROM : public SysfsEntry
{
  public:
    static bool isEEPROM(const std::filesystem::path& path);

    SysfsEEPROM(const std::filesystem::path& path) : SysfsEntry(path) {}
    SysfsEEPROM(const SysfsI2CDevice& device) :
        SysfsEntry(device.getPath() / "eeprom")
    {}
    SysfsEEPROM(const SysfsEEPROM& eeprom) = default;
    SysfsEEPROM(SysfsEEPROM&& eeprom) = delete;
    ~SysfsEEPROM() override = default;

    SysfsEEPROM& operator=(const SysfsEEPROM& other) = default;
    SysfsEEPROM& operator=(SysfsEEPROM&& other) = delete;

    SysfsI2CDevice getDevice();
};
