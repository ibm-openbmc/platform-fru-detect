/* SPDX-License-Identifier: Apache-2.0 */

#include "devices/nvme.hpp"

#include "i2c.hpp"

#include <phosphor-logging/lg2.hpp>

#include <sstream>

PHOSPHOR_LOG2_USING;

bool BasicNVMeDrive::isBasicEndpointPresent(const SysfsI2CBus& bus)
{
    return i2c::isDeviceResponsive(bus, BasicNVMeDrive::endpointAddress);
}

BasicNVMeDrive::BasicNVMeDrive(const SysfsI2CBus& bus, Inventory* inventory,
                               int index) :
    NVMeDrive(inventory, index)
{
    std::vector<uint8_t> data;

    debug("Reading drive metadata on {I2C_DEV_PATH} for drive index {NVME_ID}",
          "I2C_DEV_PATH", bus.getBusDevice().string(), "NVME_ID", index);

    i2c::oneshotSMBusBlockRead(bus, BasicNVMeDrive::endpointAddress,
                               BasicNVMeDrive::vendorMetadataOffset, data);

    assert(data.size() >= 2);

    std::stringstream ds;
    ds << std::noskipws << " ";
    for (auto v : data)
    {
        ds << std::hex << (unsigned int)v << " ";
    }
    debug("Drive manufacturing metadata: [{DRIVE_METADATA}]", "DRIVE_METADATA",
          ds.str());

    manufacturer.insert(manufacturer.begin(), data.begin(), data.begin() + 2);
    std::stringstream ms;
    ms << std::noskipws << " ";
    for (auto v : manufacturer)
    {
        ms << std::hex << (unsigned int)v << " ";
    }

    serial.insert(serial.begin(), data.begin() + 2, data.end());
    std::string prettySerial(serial.begin(), serial.end());

    info(
        "Instantiated drive for device on bus {I2C_BUS} with manufacturer [{DRIVE_MANUFACTURER_ID}] and serial [{DRIVE_SERIAL}]",
        "I2C_BUS", bus.getAddress(), "DRIVE_MANUFACTURER_ID", ms.str(),
        "DRIVE_SERIAL", prettySerial);
}

const std::vector<uint8_t>& BasicNVMeDrive::getManufacturer() const
{
    return manufacturer;
}

const std::vector<uint8_t>& BasicNVMeDrive::getSerial() const
{
    return serial;
}
