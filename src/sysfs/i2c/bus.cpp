/* SPDX-License-Identifier: Apache-2.0 */
#include "sysfs/i2c.hpp"

/* TODO: Try to remove this */
#include "platforms/rainier.hpp"

#include <phosphor-logging/lg2.hpp>

#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>

PHOSPHOR_LOG2_USING_WITH_FLAGS;

static constexpr std::array<const char *, 8> mux_channel_map = {
    "channel-0",
    "channel-1",
    "channel-2",
    "channel-3",
    "channel-4",
    "channel-5",
    "channel-6",
    "channel-7",
};

SysfsI2CBus::SysfsI2CBus(SysfsI2CMux mux, int channel) :
    SysfsEntry(mux.getPath() / mux_channel_map.at(channel))
{
}

int SysfsI2CBus::getMuxChannel()
{
    std::string name;
    std::ifstream namestream(path / "name");

    assert(namestream.is_open());
    std::getline(namestream, name);
    namestream.close();

    return SysfsI2CMux::extractChannel(name);
}

bool SysfsI2CBus::isMuxBus()
{
    return std::filesystem::exists(path / "mux_device");
}

std::string SysfsI2CBus::getID()
{
    std::filesystem::path target;

    if (std::filesystem::is_symlink(path))
    {
	target = std::filesystem::read_symlink(path);
    }
    else
    {
	target = path;
    }

    return target.filename().string();
}

int SysfsI2CBus::getAddress()
{
    std::string name = getID();
    std::string::size_type pos = name.find('-');

    return std::stoi(name.substr(pos + 1), nullptr, 10);
}

std::filesystem::path SysfsI2CBus::getDevicePath(int address)
{
    std::ostringstream ss;
    ss << getAddress() << "-" << std::setfill('0') << std::setw(4) << std::hex << address;

    return path / ss.str();
}

bool SysfsI2CBus::isDevicePresent(int address)
{
    return std::filesystem::exists(getDevicePath(address));
}

SysfsI2CDevice SysfsI2CBus::newDevice(std::string type, int address)
{
    std::fstream new_device(path / "new_device", new_device.out);
    if (!new_device.is_open())
    {
	warning("Failed to open '{SYSFS_I2C_NEW_DEVICE_PATH}'", "SYSFS_I2C_NEW_DEVICE_PATH",
	     path.string());
	throw -1;
    }

    new_device << type << " 0x" << std::hex << address << "\n";
    new_device.flush();

    if (new_device.bad() || new_device.fail())
    {
	warning("Failed to add new device {I2C_DEVICE_TYPE} at {I2C_DEVICE_ADDRESS} via '{SYSFS_I2C_NEW_DEVICE_PATH}'",
	     "I2C_DEVICE_TYPE", type,
	     "I2C_DEVICE_ADDRESS", hex | field8, address,
	     "SYSFS_I2C_NEW_DEVICE_PATH", path);
	throw -1;
    }

    return SysfsI2CDevice(getDevicePath(address));
}

void SysfsI2CBus::deleteDevice(int address)
{
    std::fstream delete_device(path / "delete_device", delete_device.out);
    if (!delete_device.is_open())
    {
	warning("Failed to open '{SYSFS_I2C_DELETE_DEVICE_PATH}'",
	     "SYSFS_I2C_DELETE_DEVICE_PATH", path);
	throw -1;
    }

    delete_device << "0x" << std::hex << address << "\n";
    delete_device.flush();

    if (delete_device.bad() || delete_device.fail())
    {
	warning("Failed to delete device at {I2C_DEVICE_ADDRESS} via '{SYFS_I2C_DELETE_DEVICE_PATH}'",
		 "I2C_DEVICE_ADDRESS", address, "SYSFS_I2C_DELETE_DEVICE_PATH", path);
	throw -1;
    }
}

SysfsI2CDevice SysfsI2CBus::probeDevice(std::string type, int address)
{
    if (isDevicePresent(address))
    {
	std::filesystem::path path = getDevicePath(address);
	debug("Device already exists at '{SYSFS_I2C_DEVICE_PATH}'", "SYSFS_I2C_DEVICE_PATH", path);
	return SysfsI2CDevice(path);
    }

    return newDevice(type, address);
}

void SysfsI2CBus::removeDevice(int address)
{
    if (isDevicePresent(address))
    {
	deleteDevice(address);
    }
    else
    {
	warning("No device exists at '{SYSFS_I2C_DEVICE_PATH}'", "SYSFS_I2C_DEVICE_PATH",
	     getDevicePath(address));
    }
}
