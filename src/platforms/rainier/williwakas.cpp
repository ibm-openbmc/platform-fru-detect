/* SPDX-License-Identifier: Apache-2.0 */
#include "log.hpp"
#include "devices/nvme.hpp"
#include "platforms/rainier.hpp"
#include "sysfs/gpio.hpp"

#include <gpiod.hpp>

#include <iostream>

static constexpr const char *name = "foo";

const Nisqually& Williwakas::getSystemBackplane() const
{
    return nisqually;
}

const Flett& Williwakas::getFlett() const
{
    return flett;
}

Williwakas::Williwakas(Nisqually nisqually, Flett flett) : nisqually(nisqually), flett(flett)
{
    SysfsI2CBus bus(Williwakas::drive_backplane_bus.at(getIndex()));

    SysfsI2CDevice dev = bus.probeDevice("pca9552", Williwakas::drivePresenceDeviceAddress);

    std::string chipName = SysfsGPIOChip(dev).getName().string();

    gpiod::chip chip(chipName, gpiod::chip::OPEN_BY_NAME);

    std::vector<unsigned int> offsets(Williwakas::drive_presence_map.begin(),
				      Williwakas::drive_presence_map.end());

    lines = chip.get_lines(offsets);

    lines.request({ name,
		    gpiod::line::DIRECTION_INPUT,
		    gpiod::line::ACTIVE_LOW });

    log_debug("Constructed drive backplane for index %d\n", getIndex());
}

bool Williwakas::isDrivePresent(int index)
{
    return lines.get(drive_presence_map[index]).get_value();
}

std::vector<NVMeDrive> Williwakas::getDrives(void) const
{
    std::vector<NVMeDrive> drives{};

    std::vector<int> presence = lines.get_values();

    for (size_t index = 0; index < presence.size(); index++) {
	/* FIXME: work around libgpiod bug */
	if (presence.at(index))
	{
	    log_info("Found drive %zu\n", index);
	    drives.emplace_back(flett.getDrive(*this, index));
	}
	else
	{
	    log_debug("Drive %zu not present\n", index);
	}
    }

    log_debug("Found %zu drives for backplane %d\n", drives.size(), getIndex());

    return drives;
}

std::string Williwakas::getInventoryPath() const
{
    return nisqually.getInventoryPath() + "/" + "disk_backplane" + std::to_string(flett.getIndex());
}

int Williwakas::getIndex() const
{
    return flett.getIndex();
}
