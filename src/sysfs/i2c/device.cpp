/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */
#include "sysfs/i2c.hpp"

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

std::string SysfsI2CDevice::generateI2CDeviceID(const SysfsI2CBus& bus,
                                                int address)
{
    std::ostringstream oss;

    oss << bus.getAddress() << "-" << std::setfill('0') << std::setw(4)
        << std::hex << address;

    return oss.str();
}

SysfsI2CDevice::SysfsI2CDevice(const SysfsI2CBus& bus, int address) :
    SysfsEntry(fs::path(bus.getPath() / generateI2CDeviceID(bus, address)))
{}

std::string SysfsI2CDevice::getID()
{
    return path.filename().string();
}

int SysfsI2CDevice::getAddress()
{
    std::string name = getID();
    std::string::size_type pos = name.find('-');

    return std::stoi(name.substr(pos + 1), nullptr, 16);
}

SysfsI2CBus SysfsI2CDevice::getBus()
{
    return {path.parent_path()};
}
