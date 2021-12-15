/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */

#include "devices/nvme.hpp"

#include "i2c.hpp"

#include <phosphor-logging/lg2.hpp>

#include <sstream>

PHOSPHOR_LOG2_USING;

bool BasicNVMeDrive::isBasicEndpointPresent(const SysfsI2CBus& bus)
{
    return i2c::isDeviceResponsive(bus, BasicNVMeDrive::endpointAddress);
}

std::vector<uint8_t> BasicNVMeDrive::fetchMetadata(SysfsI2CBus bus)
{
    std::vector<uint8_t> data;

    debug("Reading metadata for drive on bus {I2C_DEV_PATH}", "I2C_DEV_PATH",
          bus.getBusDevice().string());

    i2c::oneshotSMBusBlockRead(bus, BasicNVMeDrive::endpointAddress,
                               BasicNVMeDrive::vendorMetadataOffset, data);

    assert(data.size() >= 2);

    return data;
}

std::vector<uint8_t>
    BasicNVMeDrive::extractManufacturer(const std::vector<uint8_t>& metadata)
{
    std::vector<uint8_t> manufacturer;
    manufacturer.insert(metadata.begin(), metadata.begin(),
                        metadata.begin() + 2);
    return manufacturer;
}

std::vector<uint8_t>
    BasicNVMeDrive::extractSerial(const std::vector<uint8_t>& metadata)
{
    std::vector<uint8_t> serial;
    serial.insert(metadata.begin(), metadata.begin() + 2, metadata.end());
    return serial;
}

BasicNVMeDrive::BasicNVMeDrive(const SysfsI2CBus& bus, Inventory* inventory,
                               int index) :
    BasicNVMeDrive(bus, inventory, index, BasicNVMeDrive::fetchMetadata(bus))
{}

BasicNVMeDrive::BasicNVMeDrive(const SysfsI2CBus& bus, Inventory* inventory,
                               int index,
                               const std::vector<uint8_t>&& metadata) :
    NVMeDrive(inventory, index),
    basic(bus.getAddress(), eepromAddress),
    vini(std::vector<uint8_t>({'N', 'V', 'M', 'e'}),
         BasicNVMeDrive::extractSerial(metadata)),
    manufacturer(BasicNVMeDrive::extractManufacturer(metadata)),
    serial(BasicNVMeDrive::extractSerial(metadata))
{
    std::stringstream ms;
    ms << std::noskipws << " ";
    for (auto v : manufacturer)
    {
        ms << std::hex << (unsigned int)v << " ";
    }

    std::string prettySerial(serial.begin(), serial.end());

    info(
        "Instantiated drive for device on bus {I2C_BUS} with manufacturer [{DRIVE_MANUFACTURER_ID}] and serial [{DRIVE_SERIAL}]",
        "I2C_BUS", bus.getAddress(), "DRIVE_MANUFACTURER_ID", ms.str(),
        "DRIVE_SERIAL", prettySerial);
}

void BasicNVMeDrive::addToInventory(Inventory* inventory)
{
    std::string path = getInventoryPath();

    inventory->add(path, basic);
    inventory->add(path, vini);
}

void BasicNVMeDrive::removeFromInventory(Inventory* inventory)
{
    std::string path = getInventoryPath();

    inventory->remove(path, vini);
    inventory->remove(path, basic);
}

const std::vector<uint8_t>& BasicNVMeDrive::getManufacturer() const
{
    return manufacturer;
}

const std::vector<uint8_t>& BasicNVMeDrive::getSerial() const
{
    return serial;
}
