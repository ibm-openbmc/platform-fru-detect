/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

/* Forward-declarations for minor dependencies */
namespace sdbusplus
{
namespace bus
{
class bus;
}
} // namespace sdbusplus

class NVMeDrive;

class Inventory
{
  public:
    Inventory(sdbusplus::bus::bus& dbus) : dbus(dbus)
    {}
    ~Inventory() = default;

    void decorateWithI2CDevice(const NVMeDrive& drive);
    void decorateWithVINI(const NVMeDrive& drive);
    void markPresent(const NVMeDrive& drive);

  private:
    sdbusplus::bus::bus& dbus;
};
