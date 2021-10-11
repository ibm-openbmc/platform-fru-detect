/* SPDX-License-Identifier: Apache-2.0 */

#include "devices/nvme.hpp"

#include "inventory.hpp"
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

void NVMeDrive::addToInventory(Inventory& inventory)
{
    std::string path = getInventoryPath();

    markPresent(path, inventory);
    decorateWithI2CDevice(path, inventory);
    decorateWithVINI(path, inventory);
}

void NVMeDrive::decorateWithI2CDevice(const std::string& path,
                                      Inventory& inventory) const
{
    SysfsI2CDevice eepromDevice = getEEPROMDevice();

    size_t bus = static_cast<size_t>(eepromDevice.getBus().getAddress());
    size_t address = static_cast<size_t>(eepromDevice.getAddress());

    inventory::ObjectType updates = {
        {
            inventory::INVENTORY_DECORATOR_I2CDEVICE_IFACE,
            {
                {"Bus", bus},
                {"Address", address},
            },
        },
    };

    inventory.updateObject(path, updates);
}

void NVMeDrive::decorateWithVINI(const std::string& path,
                                 Inventory& inventory) const
{
    auto sn = getSerial();

    inventory::ObjectType updates = {
        {
            inventory::INVENTORY_IPZVPD_VINI_IFACE,
            {
                {"RT", std::vector<uint8_t>({'V', 'I', 'N', 'I'})},
                {"CC", std::vector<uint8_t>({'N', 'V', 'M', 'e'})},
                {"SN", std::vector<uint8_t>(sn.begin(), sn.end())},
            },
        },
    };

    inventory.updateObject(path, updates);
}

void NVMeDrive::markPresent(const std::string& path, Inventory& inventory) const
{
    inventory.markPresent(path);
}
