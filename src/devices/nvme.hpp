/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

#include "log.hpp"
#include "platforms/rainier.hpp"

#include "sysfs/eeprom.hpp"
#include "sysfs/i2c.hpp"

class NVMeDrive {
    public:
	NVMeDrive(const NVMeDrive& drive) : backplane(drive.backplane), index(drive.index) { }
	NVMeDrive(Williwakas backplane, int index) : backplane(backplane), index(index) { }

	static bool isPresent(SysfsI2CBus bus)
	{
	    return bus.isDevicePresent(NVMeDrive::eepromAddress);
	}

	std::filesystem::path getEEPROMDevicePath() const
	{
	    return backplane.getFlett().getDriveBus(index).getDevicePath(NVMeDrive::eepromAddress);
	}

	SysfsI2CDevice getEEPROMDevice() const
	{
	    return SysfsI2CDevice(getEEPROMDevicePath());
	}

	int probe()
	{
	    SysfsI2CBus bus = backplane.getFlett().getDriveBus(index);
	    SysfsI2CDevice eeprom = bus.probeDevice("24c02", NVMeDrive::eepromAddress);
	    log_info("EEPROM device exists at %s\n", eeprom.getPath().string().c_str());

	    return 0;
	}

	std::string getInventoryPath() const
	{
	    return backplane.getInventoryPath() + "/" + "nvme" + std::to_string(index);
	}

	const Williwakas& getBackplane() const
	{
	    return backplane;
	}

	int getIndex() const
	{
	    return index;
	}

    private:
	/* FRU Information Device, NVMe Storage Device (non-Carrier) */
	static constexpr int eepromAddress = 0x53;

	Williwakas backplane;
	int index;
};
