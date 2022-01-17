/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */
#pragma once

#include "inventory.hpp"
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

    virtual void
        removeFromInventory([[maybe_unused]] Inventory* inventory) override
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
    static bool isDriveReady(const SysfsI2CBus& bus);

    BasicNVMeDrive(const SysfsI2CBus& bus, Inventory* inventory, int index);
    BasicNVMeDrive(const SysfsI2CBus& bus, Inventory* inventory, int index,
                   const std::vector<uint8_t>&& metadata);
    virtual ~BasicNVMeDrive() = default;

    /* FRU */
    virtual void addToInventory(Inventory* inventory) override;
    virtual void removeFromInventory(Inventory* inventory) override;

  protected:
    const std::vector<uint8_t>& getManufacturer() const;
    const std::vector<uint8_t>& getSerial() const;

  private:
    static std::vector<uint8_t> fetchMetadata(SysfsI2CBus bus);
    static std::vector<uint8_t>
        extractManufacturer(const std::vector<uint8_t>& metadata);
    static std::vector<uint8_t>
        extractSerial(const std::vector<uint8_t>& metadata);

    static constexpr int endpointAddress = 0x6a;
    static constexpr int vendorMetadataOffset = 0x08;

    const inventory::interfaces::I2CDevice basic;
    const inventory::interfaces::VINI vini;

    std::vector<uint8_t> manufacturer;
    std::vector<uint8_t> serial;
};
