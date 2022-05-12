/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */

#include "devices/nvme.hpp"

#include "i2c.hpp"

#include <phosphor-logging/lg2.hpp>

#include <sstream>

PHOSPHOR_LOG2_USING;

/* NVMe Basic Management Command */
enum
{
    NVME_BASIC_SFLGS_NOT_READY = (1 << 5)
};

bool BasicNVMeDrive::isBasicEndpointPresent(const SysfsI2CBus& bus)
{
    return i2c::isDeviceResponsive(bus, BasicNVMeDrive::endpointAddress);
}

bool BasicNVMeDrive::isDriveReady(const SysfsI2CBus& bus)
{
    try
    {
        std::vector<uint8_t> status;

        i2c::oneshotSMBusBlockRead(bus, BasicNVMeDrive::endpointAddress, 0,
                                   status);

        if (status.empty())
        {
            return false;
        }

        return !(status[0] & NVME_BASIC_SFLGS_NOT_READY);
    }
    catch (const std::error_condition& ex)
    {
        if (ex.value() != ENODEV)
        {
            throw ex;
        }
    }

    return false;
}

std::vector<uint8_t> BasicNVMeDrive::fetchMetadata(const SysfsI2CBus& bus)
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
    if (metadata.size() >= 2)
    {
        manufacturer.insert(manufacturer.begin(), metadata.begin(),
                            metadata.begin() + 2);
    }
    else
    {
        warning("Invalid metadata length: {METADATA_LENGTH}", "METADATA_LENGTH",
                metadata.size());
    }
    return manufacturer;
}

std::vector<uint8_t>
    BasicNVMeDrive::extractSerial(const std::vector<uint8_t>& metadata)
{
    std::vector<uint8_t> serial;
    if (metadata.size() >= 2)
    {
        serial.insert(serial.begin(), metadata.begin() + 2, metadata.end());
    }
    else
    {
        warning(
            "No drive serial data present. Invalid metadata of length: {METADATA_LENGTH}",
            "METADATA_LENGTH", metadata.size());
    }
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
