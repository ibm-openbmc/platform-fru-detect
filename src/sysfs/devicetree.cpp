/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */

#include "sysfs/devicetree.hpp"

#include <phosphor-logging/lg2.hpp>

#include <fstream>

PHOSPHOR_LOG2_USING;

std::string SysfsDevicetree::getModel()
{
    std::filesystem::path path("/sys/firmware/devicetree/base/model");
    std::ifstream property(path, property.in);

    if (!property.is_open())
    {
        error("Failed to open '{SYSFS_DEVICETREE_PLATFORM_MODEL_PATH}'",
              "SYSFS_DEVICETREE_PLATFORM_MODEL_PATH", path.string());
        throw -1;
    }

    std::string model;
    std::getline(property, model, '\0');
    if (property.bad() || property.fail())
    {
        error(
            "Failed while reading data from '{SYSFS_DEVICETREE_PLATFORM_MODEL_PATH}'",
            "SYSFS_DEVICETREE_PLATFORM_MODEL_PATH", path.string());
        throw -1;
    }

    return model;
}
