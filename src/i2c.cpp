/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */
#include "i2c.hpp"

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
    unsigned long funcs;
    int rc;

    int fd = ::open(path.c_str(), O_RDWR);
    if (fd == -1)
    {
        warning(
            "Failed to open I2C bus device {I2C_DEV_PATH}: {ERRNO_DESCRIPTION}",
            "I2C_DEV_PATH", path, "ERRNO_DESCRIPTION", strerror(errno), "ERRNO",
            errno);
        throw std::system_category().default_error_condition(errno);
    }

    rc = ::ioctl(fd, I2C_FUNCS, &funcs);
    if (rc == -1)
    {
        warning(
            "Failed to fetch I2C capabilities for {I2C_DEV_PATH}: {ERRNO_DESCRIPTION}",
            "I2C_DEV_PATH", path, "ERRNO_DESCRIPTION", strerror(errno), "ERRNO",
            errno);
        goto cleanup_fd;
    }

    if (!(funcs & I2C_FUNC_SMBUS_QUICK))
    {
        warning(
            "Bus device {I2C_DEV_PATH} doesn't support SMBus quick command capability",
            "I2C_DEV_PATH", path);
        errno = ENOTSUP;
        goto cleanup_fd;
    }

    rc = ::ioctl(fd, I2C_SLAVE, address);
    if (rc == -1)
    {
        warning(
            "Failed to configure bus device {I2C_DEV_PATH} with device address {DEVICE_ADDRESS}: {ERRNO_DESCRIPTION}",
            "I2C_DEV_PATH", path, "DEVICE_ADDRESS", lg2::hex, address,
            "ERRNO_DESCRIPTION", strerror(errno), "ERRNO", errno);
        goto cleanup_fd;
    }

    /*
     * This is the default address probe mode used by i2cdetect from i2c-tools
     *
     * https://git.kernel.org/pub/scm/utils/i2c-tools/i2c-tools.git/tree/tools/i2cdetect.c?h=v4.3#n102
     *
     * This is known to corrupt the Atmel AT24RF08 EEPROM
     */
    rc = ::i2c_smbus_write_quick(fd, I2C_SMBUS_WRITE);

    ::close(fd);

    return rc >= 0;

cleanup_fd:
    ::close(fd);

    throw std::system_category().default_error_condition(errno);
}

void oneshotSMBusBlockRead(const SysfsI2CBus& bus, int address, uint8_t command,
                           std::vector<uint8_t>& data)
{
    fs::path path = bus.getBusDevice();
    unsigned long funcs;
    int rc;

    int fd = ::open(path.c_str(), O_RDWR);
    if (fd == -1)
    {
        error(
            "Failed to open I2C bus device {I2C_DEV_PATH}: {ERRNO_DESCRIPTION}",
            "I2C_DEV_PATH", path, "ERRNO_DESCRIPTION", strerror(errno), "ERRNO",
            errno);
        throw std::system_category().default_error_condition(errno);
    }

    rc = ::ioctl(fd, I2C_FUNCS, &funcs);
    if (rc == -1)
    {
        error(
            "Failed to fetch I2C capabilities for {I2C_DEV_PATH}: {ERRNO_DESCRIPTION}",
            "I2C_DEV_PATH", path, "ERRNO_DESCRIPTION", strerror(errno), "ERRNO",
            errno);
        goto cleanup_fd;
    }

    if (!(funcs & I2C_FUNC_SMBUS_READ_BLOCK_DATA))
    {
        error(
            "Bus device {I2C_DEV_PATH} doesn't support SMBus block read capability",
            "I2C_DEV_PATH", path);
        errno = ENOTSUP;
        goto cleanup_fd;
    }

    rc = ::ioctl(fd, I2C_SLAVE, address);
    if (rc == -1)
    {
        error(
            "Failed to configure bus device {I2C_DEV_PATH} with device address {DEVICE_ADDRESS}: {ERRNO_DESCRIPTION}",
            "I2C_DEV_PATH", path, "DEVICE_ADDRESS", lg2::hex, address,
            "ERRNO_DESCRIPTION", strerror(errno), "ERRNO", errno);
        goto cleanup_fd;
    }

    data.resize(255);
    rc = ::i2c_smbus_read_block_data(fd, command, data.data());
    if (rc < 0)
    {
        error(
            "Failed to read block data from device {DEVICE_ADDRESS} on {I2C_DEV_PATH}: {ERRNO_DESCRIPTION}",
            "DEVICE_ADDRESS", lg2::hex, address, "I2C_DEV_PATH", path,
            "ERRNO_DESCRIPTION", strerror(-rc), "ERRNO", -rc);
        goto cleanup_fd;
    }
    debug("Read {BLOCK_READ_LENGTH} bytes of block data", "BLOCK_READ_LENGTH",
          rc);
    data.resize(rc);

    ::close(fd);

    return;

cleanup_fd:
    ::close(fd);

    throw std::system_category().default_error_condition(errno);
}
} // namespace i2c
