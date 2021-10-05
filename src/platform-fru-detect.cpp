/* SPDX-License-Identifier: Apache-2.0 */
#include "dbus/inventory.hpp"
#include "devices/nvme.hpp"
#include "platforms/rainier.hpp"

#include <phosphor-logging/lg2.hpp>

#include <array>
#include <chrono>
#include <iostream>
#include <variant>
#include <vector>

#include <cstdlib>

PHOSPHOR_LOG2_USING;

int main(void)
{
    Inventory inventory{};
    Ingraham ingraham{};
    Nisqually systemBackplane = ingraham.getBackplane();
    for (auto& driveBackplane : systemBackplane.getDriveBackplanes())
    {
        for (auto& drive : driveBackplane.getDrives())
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
}
