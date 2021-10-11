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

namespace inventory
{
// dict[path,dict[string,dict[string,variant[boolean,size,int64,string,array[byte]]]]]
using PropertyType =
    std::variant<bool, std::size_t, int64_t, std::string, std::vector<uint8_t>>;
using InterfaceType = std::map<std::string, PropertyType>;
using ObjectType = std::map<std::string, InterfaceType>;

// https://github.com/openbmc/phosphor-dbus-interfaces/blob/08baf48ad5f15774d393fbbf4e9479a0ef3e82d0/yaml/xyz/openbmc_project/Inventory/Item.interface.yaml
static constexpr auto INVENTORY_ITEM_IFACE =
    "xyz.openbmc_project.Inventory.Item";

// https://github.com/openbmc/phosphor-dbus-interfaces/blob/08baf48ad5f15774d393fbbf4e9479a0ef3e82d0/yaml/xyz/openbmc_project/Inventory/Decorator/I2CDevice.interface.yaml
static constexpr auto INVENTORY_DECORATOR_I2CDEVICE_IFACE =
    "xyz.openbmc_project.Inventory.Decorator.I2CDevice";

static constexpr auto INVENTORY_IPZVPD_VINI_IFACE = "com.ibm.ipzvpd.VINI";
} // namespace inventory

class Inventory
{
  public:
    Inventory(sdbusplus::bus::bus& dbus) : dbus(dbus)
    {}
    ~Inventory() = default;

    void updateObject(const std::string& path,
                      const inventory::ObjectType& updates);
    void markPresent(const std::string& path);

  private:
    sdbusplus::bus::bus& dbus;
};
