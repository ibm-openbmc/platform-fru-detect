/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */
#pragma once

#include "sysfs/sysfs.hpp"

#include <exception>
#include <filesystem>
#include <string>

class SysfsI2CMux;
class SysfsI2CDevice;

class SysfsI2CDeviceDriverBindException : public std::exception
{
  public:
    explicit SysfsI2CDeviceDriverBindException(const SysfsEntry& entry)
    {
        description.append("No driver bound for ");
        description.append(entry.getPath().string());
    }
    SysfsI2CDeviceDriverBindException(
        const SysfsI2CDeviceDriverBindException& other) = default;
    SysfsI2CDeviceDriverBindException(
        SysfsI2CDeviceDriverBindException&& other) = default;
    ~SysfsI2CDeviceDriverBindException() override = default;

    SysfsI2CDeviceDriverBindException&
        operator=(const SysfsI2CDeviceDriverBindException& other) = default;
    SysfsI2CDeviceDriverBindException&
        operator=(SysfsI2CDeviceDriverBindException&& other) = default;

    const char* what() const noexcept override
    {
        return description.c_str();
    }

  private:
    std::string description;
};

class SysfsI2CBus : public SysfsEntry
{
  public:
    explicit SysfsI2CBus(const std::filesystem::path& path, bool check = true) :
        SysfsEntry(path, check)
    {}
    SysfsI2CBus(const SysfsI2CMux& mux, int channel);

    bool isMuxBus();
    int getMuxChannel();
    std::string getID() const;
    int getAddress() const;
    std::filesystem::path getBusDevice() const;
    std::filesystem::path getDevicePath(int address);
    bool isDevicePresent(int address);

    /* Fail if pre-conditions are not met */
    SysfsI2CDevice newDevice(std::string type, int address);
    void deleteDevice(int address);

    /* Succeed if post-conditions are already met */
    SysfsI2CDevice requireDevice(std::string type, int address);
    void releaseDevice(int address);

    /* Require driver be bound */
    SysfsI2CDevice probeDevice(std::string type, int address);
    void removeDevice(int address);
};

class SysfsI2CDevice : public SysfsEntry
{
  public:
    explicit SysfsI2CDevice(const std::filesystem::path& path) :
        SysfsEntry(path)
    {}
    SysfsI2CDevice(const SysfsI2CBus& bus, int address);

    SysfsI2CBus getBus();
    std::string getID();
    int getAddress();

  private:
    static std::string generateI2CDeviceID(const SysfsI2CBus& bus, int address);
};

class SysfsI2CMux : public SysfsI2CDevice
{
  public:
    explicit SysfsI2CMux(const SysfsI2CDevice& device) : SysfsI2CDevice(device)
    {}
    explicit SysfsI2CMux(const std::filesystem::path& path) :
        SysfsI2CDevice(path)
    {}
    SysfsI2CMux(const SysfsI2CBus& bus, int address) :
        SysfsI2CDevice(bus, address)
    {}

    static int extractChannel(std::string& name);
};
