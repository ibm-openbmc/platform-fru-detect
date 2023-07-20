/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */
#pragma once

#include <phosphor-logging/lg2.hpp>

#include <filesystem>
#include <iostream>
#include <system_error>

class SysfsEntry
{
  public:
    SysfsEntry(const std::filesystem::path& path, bool check = true) :
        path(path)
    {
        if (check && !std::filesystem::exists(path))
        {
            lg2::error("sysfs path '{SYSFS_PATH}' does not exist", "SYSFS_PATH",
                       path.string());

            throw std::system_category().default_error_condition(ENOENT);
        }

        lg2::debug("Instantiated SysfsEntry for '{SYSFS_PATH}'", "SYSFS_PATH",
                   path.string());
    }

    virtual ~SysfsEntry() = default;

    std::filesystem::path getPath() const
    {
        return path;
    }

  protected:
    std::filesystem::path path;
};
