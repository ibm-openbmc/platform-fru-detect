/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

#include "platform.hpp"
#include "sysfs/i2c.hpp"

#include <array>
#include <stdexcept>
#include <vector>

class Inventory;

class NVMeDrive : public Device, FRU
{
  public:
    NVMeDrive(Inventory* inventory, int index) :
        inventory(inventory), index(index)
    {}
    virtual ~NVMeDrive() = default;

    /* FRU */
    virtual std::string getInventoryPath() const override
    {
        throw std::logic_error("Not implemented");
    }

    virtual void addToInventory([[maybe_unused]] Inventory* inventory) override
    {
        throw std::logic_error("Not implemented");
    }

  protected:
    /* FRU Information Device, NVMe Storage Device (non-Carrier) */
    static constexpr int eepromAddress = 0x53;

    Inventory* inventory;
    int index;
};

class BasicNVMeDrive : public NVMeDrive
{
  public:
    static bool isBasicEndpointPresent(const SysfsI2CBus& bus);

    BasicNVMeDrive(const SysfsI2CBus& bus, Inventory* inventory, int index);
    virtual ~BasicNVMeDrive() = default;

  protected:
    const std::vector<uint8_t>& getManufacturer() const;
    const std::vector<uint8_t>& getSerial() const;

  private:
    static constexpr int endpointAddress = 0x6a;
    static constexpr int vendorMetadataOffset = 0x08;

    std::vector<uint8_t> manufacturer;
    std::vector<uint8_t> serial;
};

template <DerivesDevice T>
class PolledBasicNVMeDrivePresence : public PolledDevicePresence<T>
{
  public:
    PolledBasicNVMeDrivePresence() : bus("/")
    {}
    PolledBasicNVMeDrivePresence(SysfsI2CBus bus, Connector<T>* connector) :
        PolledDevicePresence<T>(connector), bus(bus)
    {}
    ~PolledBasicNVMeDrivePresence() = default;

    PolledBasicNVMeDrivePresence<T>&
        operator=(const PolledBasicNVMeDrivePresence<T>& other) = default;

    /* PolledDevicePresence */
    virtual bool poll() override
    {
        return BasicNVMeDrive::isBasicEndpointPresent(bus);
    }

  private:
    SysfsI2CBus bus;
};
