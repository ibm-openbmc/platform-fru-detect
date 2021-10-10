/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

#include <map>
#include <string>
#include <variant>
#include <vector>

/* Forward-declarations for minor dependencies */
namespace sdbusplus
{
namespace bus
{
class bus;
}
} // namespace sdbusplus

class NVMeDrive;

namespace inventory
{
// dict[path,dict[string,dict[string,variant[boolean,size,int64,string,array[byte]]]]]
using PropertyType =
    std::variant<bool, std::size_t, int64_t, std::string, std::vector<uint8_t>>;
using InterfaceType = std::map<std::string, PropertyType>;
using ObjectType = std::map<std::string, InterfaceType>;
} // namespace inventory

class Inventory
{
  public:
    Inventory(sdbusplus::bus::bus& dbus) : dbus(dbus)
    {}
    ~Inventory() = default;

    void updateObject(const std::string& path,
                      const inventory::ObjectType& updates);

    void decorateWithI2CDevice(const NVMeDrive& drive);
    void decorateWithVINI(const NVMeDrive& drive);
    void markPresent(const NVMeDrive& drive);

  private:
    sdbusplus::bus::bus& dbus;
};
