/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */
#pragma once

#include "sysfs/i2c.hpp"

#include <phosphor-logging/lg2.hpp>

#include <filesystem>
#include <iostream>

PHOSPHOR_LOG2_USING;

class SysfsGPIOChip : public SysfsEntry
{
  public:
    explicit SysfsGPIOChip(const std::filesystem::path& path) :
        SysfsGPIOChip(SysfsEntry(path))
    {}
    explicit SysfsGPIOChip(const SysfsEntry& entry) :
        SysfsEntry(SysfsGPIOChip::getGPIOChipPath(entry))
    {}

    static bool hasGPIOChip(const SysfsEntry& entry)
    {
        /* FIXME: This is the deprecated attribute */
        return std::filesystem::exists(entry.getPath() / "gpio");
    }

    std::filesystem::path getName()
    {
        return path.filename();
    }

  private:
    static std::filesystem::path getGPIOChipPath(const SysfsEntry& entry)
    {
        namespace fs = std::filesystem;

        debug("Inspecting '{SYSFS_PATH}' for associated gpiochip", "SYSFS_PATH",
              entry.getPath().string());

        for (const auto& dirent : fs::directory_iterator{entry.getPath()})
        {
            if (dirent.path().filename().string().starts_with("gpiochip"))
            {
                debug("Found '{GPIOCHIP_NAME}'", "GPIOCHIP_NAME",
                      dirent.path().filename().string(), "SYSFS_PATH",
                      entry.getPath().string());
                return dirent.path();
            }
        }

        error("sysfs path '{SYSFS_PATH}' has no associated gpiochip",
              "SYSFS_PATH", entry.getPath().string());

        /* FIXME: Throw something better? */
        throw -1;
    }
};
