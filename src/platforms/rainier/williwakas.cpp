/* SPDX-License-Identifier: Apache-2.0 */
#include "devices/nvme.hpp"
#include "inventory.hpp"
#include "platforms/rainier.hpp"
#include "sysfs/gpio.hpp"

#include <gpiod.hpp>
#include <phosphor-logging/lg2.hpp>

#include <cassert>
#include <iostream>

PHOSPHOR_LOG2_USING;

static constexpr const char* name = "foo";

const Flett& Williwakas::getFlett() const
{
    return flett;
}

Williwakas::Williwakas(Inventory& inventory, Nisqually& nisqually,
                       Flett flett) :
    inventory(inventory),
    nisqually(nisqually), flett(flett)
{
    SysfsI2CBus bus(Williwakas::drive_backplane_bus.at(getIndex()));

    try
    {
        SysfsI2CDevice dev =
            bus.probeDevice("pca9552", Williwakas::drivePresenceDeviceAddress);

        std::string chipName = SysfsGPIOChip(dev).getName().string();

        gpiod::chip chip(chipName, gpiod::chip::OPEN_BY_NAME);

        std::vector<unsigned int> offsets(
            Williwakas::drive_presence_map.begin(),
            Williwakas::drive_presence_map.end());

        lines = chip.get_lines(offsets);

        lines.request(
            {name, gpiod::line::DIRECTION_INPUT, gpiod::line::ACTIVE_LOW});

        debug("Constructed drive backplane for index {WILLIWAKAS_ID}",
              "WILLIWAKAS_ID", getIndex());
    }
    catch (const SysfsI2CDeviceDriverBindException& ex)
    {
        flett.~Flett();
        throw ex;
    }
}

std::string Williwakas::getInventoryPath() const
{
    return nisqually.getInventoryPath() + "/" + "disk_backplane" +
           std::to_string(flett.getIndex());
}

int Williwakas::getIndex() const
{
    return flett.getIndex();
}

void Williwakas::plug()
{
    detectDrives(drives);
    for (auto& drive : drives)
    {
        int rc;

        if ((rc = drive.probe()))
        {
            error("Failed to probe drive: {ERROR_CODE}\n", "ERROR_CODE", rc);
            continue;
        }

        inventory.markPresent(drive);
        inventory.decorateWithI2CDevice(drive);
        inventory.decorateWithVINI(drive);
    }
}

bool Williwakas::isDrivePresent(int index)
{
    return lines.get(drive_presence_map[index]).get_value();
}

void Williwakas::detectDrives(std::vector<NVMeDrive>& drives)
{
    assert(drives.empty() && "Already detected drives");

    std::vector<int> presence = lines.get_values();

    for (size_t index = 0; index < presence.size(); index++)
    {
        /* FIXME: work around libgpiod bug */
        if (presence.at(index))
        {
            info("Found drive {NVME_ID} on backplane {WILLIWAKAS_ID}",
                 "NVME_ID", index, "WILLIWAKAS_ID", getIndex());
            drives.emplace_back(flett.getDrive(*this, index));
        }
        else
        {
            debug("Drive {NVME_ID} not present on backplane {WILLIWAKAS_ID}",
                  "NVME_ID", index, "WILLIWAKAS_ID", getIndex());
        }
    }

    debug("Found {NVME_COUNT} drives for backplane {WILLIWAKAS_ID}",
          "NVME_COUNT", drives.size(), "WILLIWAKAS_ID", getIndex());
}
