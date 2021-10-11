/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

#include "platform.hpp"
#include "sysfs/eeprom.hpp"
#include "sysfs/i2c.hpp"

#include <phosphor-logging/lg2.hpp>

#include <array>

class Inventory;
class Williwakas;

class NVMeDrive : public FRU
{
  public:
    static bool isPresent(SysfsI2CBus bus);

    NVMeDrive(const NVMeDrive& drive) :
        inventory(drive.inventory), backplane(drive.backplane),
        index(drive.index)
    {}
    NVMeDrive(Inventory& inventory, const Williwakas& backplane, int index) :
        inventory(inventory), backplane(backplane), index(index)
    {}

    int probe();

    /* FRU */
    virtual void addToInventory(Inventory& inventory) override;

  private:
    /* FRU Information Device, NVMe Storage Device (non-Carrier) */
    static constexpr int eepromAddress = 0x53;

    std::filesystem::path getEEPROMDevicePath() const;
    SysfsI2CDevice getEEPROMDevice() const;
    std::string getInventoryPath() const;
    std::array<uint8_t, 2> getSerial() const;

    void decorateWithI2CDevice(const std::string& path,
                               Inventory& inventory) const;
    void decorateWithVINI(const std::string& path, Inventory& inventory) const;
    void markPresent(const std::string& path, Inventory& inventory) const;

    Inventory& inventory;
    const Williwakas& backplane;
    int index;
};
