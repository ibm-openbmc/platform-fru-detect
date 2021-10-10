/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

#include "sysfs/eeprom.hpp"
#include "sysfs/i2c.hpp"

#include <phosphor-logging/lg2.hpp>

#include <array>

class Inventory;
class Williwakas;

class NVMeDrive
{
  public:
    NVMeDrive(const NVMeDrive& drive) :
        inventory(drive.inventory), backplane(drive.backplane),
        index(drive.index)
    {}
    NVMeDrive(Inventory& inventory, const Williwakas& backplane, int index) :
        inventory(inventory), backplane(backplane), index(index)
    {}

    static bool isPresent(SysfsI2CBus bus);
    std::filesystem::path getEEPROMDevicePath() const;
    SysfsI2CDevice getEEPROMDevice() const;

    int probe();

    std::string getInventoryPath() const;
    std::array<uint8_t, 2> getSerial() const;

  private:
    /* FRU Information Device, NVMe Storage Device (non-Carrier) */
    static constexpr int eepromAddress = 0x53;

    Inventory& inventory;
    const Williwakas& backplane;
    int index;
};
