/* SPDX-License-Identifier: Apache-2.0 */

#include "devices/nvme.hpp"

#include "platforms/rainier.hpp" /* TODO: Get rid of this */

bool NVMeDrive::isPresent(SysfsI2CBus bus)
{
    return bus.isDevicePresent(NVMeDrive::eepromAddress);
}

std::filesystem::path NVMeDrive::getEEPROMDevicePath() const
{
    return backplane.getFlett().getDriveBus(index).getDevicePath(
        NVMeDrive::eepromAddress);
}

SysfsI2CDevice NVMeDrive::getEEPROMDevice() const
{
    return SysfsI2CDevice(getEEPROMDevicePath());
}

int NVMeDrive::probe()
{
    SysfsI2CBus bus = backplane.getFlett().getDriveBus(index);
    SysfsI2CDevice eeprom = bus.probeDevice("24c02", NVMeDrive::eepromAddress);
    lg2::info("EEPROM device exists at '{EEPROM_PATH}'", "EEPROM_PATH",
              eeprom.getPath().string());

    return 0;
}

std::string NVMeDrive::getInventoryPath() const
{
    return backplane.getInventoryPath() + "/" + "nvme" + std::to_string(index);
}

std::array<uint8_t, 2> NVMeDrive::getSerial() const
{
    return std::array<uint8_t, 2>({static_cast<uint8_t>(backplane.getIndex()),
                                   static_cast<uint8_t>(index)});
}
