/* SPDX-License-Identifier: Apache-2.0 */
#include "devices/nvme.hpp"
#include "inventory.hpp"
#include "platforms/rainier.hpp"
#include "sysfs/gpio.hpp"
#include "sysfs/i2c.hpp"

#include <phosphor-logging/lg2.hpp>

#include <cassert>
#include <map>
#include <string>

PHOSPHOR_LOG2_USING;

/* FlettNVMeDrive */

FlettNVMeDrive::FlettNVMeDrive(Inventory& inventory, const Nisqually& nisqually,
                               const Flett& flett, int index) :
    NVMeDrive(inventory, index),
    nisqually(nisqually), flett(flett)
{
    try
    {
        SysfsI2CBus bus = flett.getDriveBus(index);
        SysfsI2CDevice eeprom =
            bus.probeDevice("24c02", NVMeDrive::eepromAddress);
        lg2::info("EEPROM device exists at '{EEPROM_PATH}'", "EEPROM_PATH",
                  eeprom.getPath().string());
    }
    catch (const SysfsI2CDeviceDriverBindException& ex)
    {
        NVMeDrive::~NVMeDrive();
        throw ex;
    }
}

void FlettNVMeDrive::plug()
{
    /* TODO: Probe NVMe MI endpoints on I2C? */
    debug("Drive {NVME_ID} plugged on Flett {FLETT_ID}", "NVME_ID", index,
          "FLETT_ID", flett.getIndex());
    addToInventory(inventory);
}

std::string FlettNVMeDrive::getInventoryPath() const
{
    std::string williwakasPath =
        Williwakas::getInventoryPathFor(nisqually, flett.getIndex());

    return williwakasPath + "/" + "nvme" + std::to_string(index);
}

void FlettNVMeDrive::addToInventory(Inventory& inventory)
{
    std::string path = getInventoryPath();

    decorateWithI2CDevice(path, inventory);
    decorateWithVINI(path, inventory);
}

void FlettNVMeDrive::removeFromInventory([[maybe_unused]] Inventory& inventory)
{
    debug(
        "I'm not sure how to remove drive {NVME_ID} on Flett {FLETT_ID} from the inventory!",
        "NVME_ID", index, "FLETT_ID", flett.getIndex());
}

bool FlettNVMeDrive::isPresent(SysfsI2CBus bus)
{
    return bus.isDevicePresent(NVMeDrive::eepromAddress);
}

std::filesystem::path FlettNVMeDrive::getEEPROMDevicePath() const
{
    return flett.getDriveBus(index).getDevicePath(NVMeDrive::eepromAddress);
}

SysfsI2CDevice FlettNVMeDrive::getEEPROMDevice() const
{
    return SysfsI2CDevice(getEEPROMDevicePath());
}

std::array<uint8_t, 2> FlettNVMeDrive::getSerial() const
{
    return std::array<uint8_t, 2>(
        {static_cast<uint8_t>(flett.getIndex()), static_cast<uint8_t>(index)});
}

void FlettNVMeDrive::decorateWithI2CDevice(const std::string& path,
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

void FlettNVMeDrive::decorateWithVINI(const std::string& path,
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

/* Flett */

static constexpr const char* name = "foo";

static const std::map<std::string, std::map<int, int>> flett_mux_slot_map = {
    {"i2c-6", {{0x74, 9}, {0x75, 8}}},
    {"i2c-11", {{0x74, 10}, {0x75, 11}}},
};

static const std::map<int, int> flett_slot_mux_map = {
    {8, 0x75},
    {9, 0x74},
    {10, 0x74},
    {11, 0x75},
};

static const std::map<int, int> flett_slot_eeprom_map = {
    {8, 0x51},
    {9, 0x50},
    {10, 0x50},
    {11, 0x51},
};

Flett::Flett(Inventory& inventory, const Nisqually& nisqually, int slot) :
    inventory(inventory), nisqually(nisqually), slot(slot)
{
    SysfsI2CBus bus = Nisqually::getFlettSlotI2CBus(slot);

#if 0 /* FIXME: Well, fix qemu */
    bus.probeDevice("24c02", flett_slot_eeprom_map.at(slot));
#endif
    bus.probeDevice("pca9548", flett_slot_mux_map.at(slot));

    debug("Instantiated Flett in slot {PCIE_SLOT}", "PCIE_SLOT", slot);
}

int Flett::getIndex() const
{
    return Nisqually::getFlettIndex(slot);
}

SysfsI2CBus Flett::getDriveBus(int index) const
{
    SysfsI2CMux flettMux(Nisqually::getFlettSlotI2CBus(slot),
                         flett_slot_mux_map.at(slot));

    return SysfsI2CBus(flettMux, index);
}

void Flett::plug()
{
    detectDrives(drives);
}

void Flett::detectDrives(std::vector<FlettNVMeDrive>& drives)
{
    for (int i = 0; i < 8; i++)
    {
        try
        {
            FlettNVMeDrive drive(inventory, nisqually, *this, i);
            drives.push_back(drive);
            drives.back().plug();
            info("Detected drive at index {NVME_ID} on Flett {FLETT_ID}",
                 "NVME_ID", i, "FLETT_ID", getIndex());
        }
        catch (const SysfsI2CDeviceDriverBindException& ex)
        {
            debug("Failed to probe drive {DRIVE_ID} on Flett {FLETT_ID}",
                  "DRIVE_ID", i, "FLETT_ID", getIndex());
        }
    }
}
