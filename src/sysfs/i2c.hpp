/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

#include "sysfs/sysfs.hpp"

#include <filesystem>
#include <string>

class SysfsI2CMux;
class SysfsI2CDevice;

class SysfsI2CBus : public SysfsEntry
{
  public:
    SysfsI2CBus(const SysfsI2CBus& bus) : SysfsEntry(bus)
    {}
    SysfsI2CBus(std::filesystem::path path) : SysfsEntry(path)
    {}
    SysfsI2CBus(SysfsI2CMux mux, int channel);

    bool isMuxBus();
    int getMuxChannel();
    std::string getID();
    int getAddress();
    std::filesystem::path getDevicePath(int address);
    bool isDevicePresent(int address);

    /* Fail if pre-conditions are not met */
    SysfsI2CDevice newDevice(std::string type, int address);
    void deleteDevice(int address);

    /* Succeed if post-conditions are already met */
    SysfsI2CDevice probeDevice(std::string type, int address);
    void removeDevice(int address);
};

class SysfsI2CDevice : public SysfsEntry
{
  public:
    SysfsI2CDevice(const SysfsI2CDevice& device) : SysfsEntry(device)
    {}
    SysfsI2CDevice(std::filesystem::path path) : SysfsEntry(path)
    {}
    SysfsI2CDevice(SysfsI2CBus bus, int address);
    virtual ~SysfsI2CDevice() = default;

    SysfsI2CBus getBus();
    std::string getID();
    int getAddress();

  private:
    static std::string generateI2CDeviceID(SysfsI2CBus bus, int address);
};

class SysfsI2CMux : public SysfsI2CDevice
{
  public:
    SysfsI2CMux(SysfsI2CDevice device) : SysfsI2CDevice(device)
    {}
    SysfsI2CMux(std::filesystem::path path) : SysfsI2CDevice(path)
    {}
    SysfsI2CMux(SysfsI2CBus bus, int address) : SysfsI2CDevice(bus, address)
    {}

    virtual ~SysfsI2CMux() = default;

    static int extractChannel(std::string& name);
};
