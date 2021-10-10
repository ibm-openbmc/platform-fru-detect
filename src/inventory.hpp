/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

#include <sdbusplus/bus.hpp>

class NVMeDrive;

class Inventory
{
  public:
    Inventory() : dbus(sdbusplus::bus::new_default())
    {}

    void decorateWithI2CDevice(const NVMeDrive& drive);
    void decorateWithVINI(const NVMeDrive& drive);
    void markPresent(const NVMeDrive& drive);

  private:
    sdbusplus::bus::bus dbus;
};