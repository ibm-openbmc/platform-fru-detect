/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */
#include "i2c.hpp"

#include "descriptor.hpp"

#include <phosphor-logging/lg2.hpp>

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <system_error>

extern "C"
{
#include <fcntl.h>
#include <i2c/smbus.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
};

namespace fs = std::filesystem;

PHOSPHOR_LOG2_USING;

namespace i2c
{
bool isDeviceResponsive(const SysfsI2CBus& bus, int address)
{
    fs::path path = bus.getBusDevice();
    FileDescriptor fd(path);
    unsigned long funcs = 0;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    int rc = ::ioctl(fd.descriptor(), I2C_FUNCS, &funcs);
    if (rc == -1)
    {
        warning(
            "Failed to fetch I2C capabilities for {I2C_DEV_PATH}: {ERRNO_DESCRIPTION}",
            "I2C_DEV_PATH", path, "ERRNO_DESCRIPTION", strerror(errno), "ERRNO",
            errno);
        throw std::system_category().default_error_condition(errno);
    }

    if ((funcs & I2C_FUNC_SMBUS_QUICK) == 0U)
    {
        warning(
            "Bus device {I2C_DEV_PATH} doesn't support SMBus quick command capability",
            "I2C_DEV_PATH", path);
        throw std::system_category().default_error_condition(ENOTSUP);
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    rc = ::ioctl(fd.descriptor(), I2C_SLAVE, address);
    if (rc == -1)
    {
        warning(
            "Failed to configure bus device {I2C_DEV_PATH} with device address {DEVICE_ADDRESS}: {ERRNO_DESCRIPTION}",
            "I2C_DEV_PATH", path, "DEVICE_ADDRESS", lg2::hex, address,
            "ERRNO_DESCRIPTION", strerror(errno), "ERRNO", errno);
        throw std::system_category().default_error_condition(errno);
    }

    /*
     * This is the default address probe mode used by i2cdetect from i2c-tools
     *
     * https://git.kernel.org/pub/scm/utils/i2c-tools/i2c-tools.git/tree/tools/i2cdetect.c?h=v4.3#n102
     *
     * This is known to corrupt the Atmel AT24RF08 EEPROM
     */
    rc = ::i2c_smbus_write_quick(fd.descriptor(), I2C_SMBUS_WRITE);

    return rc >= 0;
}

void oneshotSMBusBlockRead(const SysfsI2CBus& bus, int address, uint8_t command,
                           std::vector<uint8_t>& data)
{
    fs::path path = bus.getBusDevice();
    FileDescriptor fd(path);
    unsigned long funcs = 0;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    int rc = ::ioctl(fd.descriptor(), I2C_FUNCS, &funcs);
    if (rc == -1)
    {
        error(
            "Failed to fetch I2C capabilities for {I2C_DEV_PATH}: {ERRNO_DESCRIPTION}",
            "I2C_DEV_PATH", path, "ERRNO_DESCRIPTION", strerror(errno), "ERRNO",
            errno);
        throw std::system_category().default_error_condition(errno);
    }

    if ((funcs & I2C_FUNC_SMBUS_READ_BLOCK_DATA) == 0U)
    {
        error(
            "Bus device {I2C_DEV_PATH} doesn't support SMBus block read capability",
            "I2C_DEV_PATH", path);
        throw std::system_category().default_error_condition(ENOTSUP);
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    rc = ::ioctl(fd.descriptor(), I2C_SLAVE, address);
    if (rc == -1)
    {
        error(
            "Failed to configure bus device {I2C_DEV_PATH} with device address {DEVICE_ADDRESS}: {ERRNO_DESCRIPTION}",
            "I2C_DEV_PATH", path, "DEVICE_ADDRESS", lg2::hex, address,
            "ERRNO_DESCRIPTION", strerror(errno), "ERRNO", errno);
        throw std::system_category().default_error_condition(errno);
    }

    data.resize(255);
    rc = ::i2c_smbus_read_block_data(fd.descriptor(), command, data.data());
    if (rc < 0)
    {
        error(
            "Failed to read block data from device {DEVICE_ADDRESS} on {I2C_DEV_PATH}: {ERRNO_DESCRIPTION}",
            "DEVICE_ADDRESS", lg2::hex, address, "I2C_DEV_PATH", path,
            "ERRNO_DESCRIPTION", strerror(-rc), "ERRNO", -rc);
        throw std::system_category().default_error_condition(errno);
    }
    debug("Read {BLOCK_READ_LENGTH} bytes of block data", "BLOCK_READ_LENGTH",
          rc);
    data.resize(rc);
}
} // namespace i2c
