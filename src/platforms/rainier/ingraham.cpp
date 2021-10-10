/* SPDX-License-Identifier: Apache-2.0 */
#include "devices/nvme.hpp"
#include "inventory.hpp"
#include "platforms/rainier.hpp"

#include <phosphor-logging/lg2.hpp>

#include <algorithm>
#include <filesystem>
#include <string>

PHOSPHOR_LOG2_USING;

namespace fs = std::filesystem;

Ingraham::Ingraham(Inventory& inventory) : inventory(inventory)
{}

SysfsI2CBus Ingraham::getPCIeSlotI2CBus(int slot)
{
    return SysfsI2CBus(fs::path(Ingraham::pcie_slot_bus_map.at(slot)));
}

Nisqually Ingraham::getBackplane() const
{
    Nisqually backplane(inventory);

    backplane.probe();

    return backplane;
}

void Ingraham::plug()
{
    Nisqually systemBackplane = getBackplane();
    for (auto& driveBackplane : systemBackplane.getDriveBackplanes())
    {
        for (auto& drive : driveBackplane.getDrives())
        {
            int rc;

            if ((rc = drive.probe()))
            {
                error("Failed to probe drive: {ERROR_CODE}\n", "ERROR_CODE",
                      rc);
                continue;
            }

            inventory.markPresent(drive);
            inventory.decorateWithI2CDevice(drive);
            inventory.decorateWithVINI(drive);
        }
    }
}
