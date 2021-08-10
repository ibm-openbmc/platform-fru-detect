/* SPDX-License-Identifier: Apache-2.0 */
#include "log.hpp"
#include "dbus/inventory.hpp"
#include "devices/nvme.hpp"
#include "platforms/rainier.hpp"

#include <array>
#include <chrono>
#include <iostream>
#include <variant>
#include <vector>

#include <cstdlib>

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
                log_error("Failed to probe drive: %d\n", rc);
                continue;
            }

            inventory.markPresent(drive);
            inventory.decorateWithI2CDevice(drive);
            inventory.decorateWithVINI(drive);
        }
    }
}
