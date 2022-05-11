/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */
#pragma once

#include "inventory.hpp"
#include "platform.hpp"
#include "sysfs/i2c.hpp"

#include <array>
#include <optional>
#include <stdexcept>
#include <vector>

class Inventory;

class NVMeDrive
{
  public:
    NVMeDrive() = default;
    virtual ~NVMeDrive() = default;

  protected:
    /* FRU Information Device, NVMe Storage Device (non-Carrier) */
    static constexpr int eepromAddress = 0x53;
};

class BasicNVMeDrive : public NVMeDrive, FRU
{
  public:
    static bool isBasicEndpointPresent(const SysfsI2CBus& bus);

    BasicNVMeDrive(std::string&& path);
    BasicNVMeDrive(const SysfsI2CBus& bus, std::string&& path);
    BasicNVMeDrive(const SysfsI2CBus& bus, std::string&& path,
                   const std::vector<uint8_t>&& metadata);
    ~BasicNVMeDrive() override = default;

    /* FRU */
    std::string getInventoryPath() const override;
    void addToInventory(Inventory* inventory) override;
    void removeFromInventory(Inventory* inventory) override;

  protected:
    const std::vector<uint8_t>& getManufacturer() const;
    const std::vector<uint8_t>& getSerial() const;

  private:
    static std::vector<uint8_t> fetchMetadata(const SysfsI2CBus& bus);
    static std::vector<uint8_t>
        extractManufacturer(const std::vector<uint8_t>& metadata);
    static std::vector<uint8_t>
        extractSerial(const std::vector<uint8_t>& metadata);

    static constexpr int endpointAddress = 0x6a;
    static constexpr int vendorMetadataOffset = 0x08;

    const std::string inventoryPath;
    const inventory::interfaces::I2CDevice basic;
    const inventory::interfaces::VINI vini;

    std::optional<std::vector<uint8_t>> manufacturer;
    std::optional<std::vector<uint8_t>> serial;
};
