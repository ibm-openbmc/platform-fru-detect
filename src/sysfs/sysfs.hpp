/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

#include <phosphor-logging/lg2.hpp>

#include <filesystem>
#include <iostream>

class SysfsEntry
{
  public:
    SysfsEntry(std::filesystem::path path) : path(path)
    {

        if (!std::filesystem::exists(path))
        {
            lg2::error("sysfs path '{SYSFS_PATH}' does not exist", "SYSFS_PATH",
                       path.string());

            throw -1;
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
