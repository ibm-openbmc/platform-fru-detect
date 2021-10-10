/* SPDX-License-Identifier: Apache-2.0 */
#include "inventory.hpp"

#include "devices/nvme.hpp"
#include "sysfs/i2c.hpp"

#include <sdbusplus/message.hpp>

// https://dbus.freedesktop.org/doc/dbus-specification.html#standard-interfaces-properties
static constexpr auto DBUS_PROPERTY_IFACE = "org.freedesktop.DBus.Properties";

static constexpr auto INVENTORY_BUS_NAME =
    "xyz.openbmc_project.Inventory.Manager";

// https://github.com/openbmc/phosphor-dbus-interfaces/blob/08baf48ad5f15774d393fbbf4e9479a0ef3e82d0/yaml/xyz/openbmc_project/Inventory/Item.interface.yaml
static constexpr auto INVENTORY_ITEM_IFACE =
    "xyz.openbmc_project.Inventory.Item";

// https://github.com/openbmc/phosphor-dbus-interfaces/blob/08baf48ad5f15774d393fbbf4e9479a0ef3e82d0/yaml/xyz/openbmc_project/Inventory/Decorator/I2CDevice.interface.yaml
static constexpr auto INVENTORY_DECORATOR_I2CDEVICE_IFACE =
    "xyz.openbmc_project.Inventory.Decorator.I2CDevice";

// https://github.com/openbmc/phosphor-dbus-interfaces/blob/08baf48ad5f15774d393fbbf4e9479a0ef3e82d0/yaml/xyz/openbmc_project/Inventory/Manager.interface.yaml
static constexpr auto INVENTORY_MANAGER_IFACE =
    "xyz.openbmc_project.Inventory.Manager";
static constexpr auto INVENTORY_MANAGER_OBJECT =
    "/xyz/openbmc_project/inventory";

static constexpr auto INVENTORY_IPZVPD_VINI_IFACE = "com.ibm.ipzvpd.VINI";

// dict[path,dict[string,dict[string,variant[boolean,size,int64,string,array[byte]]]]]
using PropertyType =
    std::variant<bool, size_t, int64_t, std::string, std::vector<uint8_t>>;
using InterfaceType = std::map<std::string, PropertyType>;
using ObjectType = std::map<std::string, InterfaceType>;

void Inventory::decorateWithI2CDevice(const NVMeDrive& drive)
{
    SysfsI2CDevice eepromDevice = drive.getEEPROMDevice();

    size_t bus = static_cast<size_t>(eepromDevice.getBus().getAddress());
    size_t address = static_cast<size_t>(eepromDevice.getAddress());

    auto call =
        dbus.new_method_call(INVENTORY_BUS_NAME, INVENTORY_MANAGER_OBJECT,
                             INVENTORY_MANAGER_IFACE, "Notify");

    std::map<sdbusplus::message::object_path, ObjectType> inventoryUpdate = {
        {
            drive.getInventoryPath(),
            {
                {
                    INVENTORY_DECORATOR_I2CDEVICE_IFACE,
                    {
                        {"Bus", bus},
                        {"Address", address},
                    },
                },
            },
        },
    };

    call.append(inventoryUpdate);
    dbus.call(call);
}

void Inventory::decorateWithVINI(const NVMeDrive& drive)
{
    auto call =
        dbus.new_method_call(INVENTORY_BUS_NAME, INVENTORY_MANAGER_OBJECT,
                             INVENTORY_MANAGER_IFACE, "Notify");

    std::map<sdbusplus::message::object_path, ObjectType> inventoryUpdate = {
        {
            drive.getInventoryPath(),
            {
                {
                    INVENTORY_IPZVPD_VINI_IFACE,
                    {
                        {"RT", std::vector<uint8_t>({'V', 'I', 'N', 'I'})},
                        {"CC", std::vector<uint8_t>({'N', 'V', 'M', 'e'})},
                        {
                            "SN",
                            std::vector<uint8_t>(
                                {static_cast<uint8_t>(
                                     drive.getBackplane().getIndex()),
                                 static_cast<uint8_t>(drive.getIndex())}),
                        },
                    },
                },
            },
        },
    };

    call.append(inventoryUpdate);
    dbus.call(call);
}

void Inventory::markPresent(const NVMeDrive& drive)
{
    std::string path = std::string("/xyz/openbmc_project/inventory") +
                       drive.getInventoryPath();

    auto call = dbus.new_method_call(INVENTORY_BUS_NAME, path.c_str(),
                                     DBUS_PROPERTY_IFACE, "Set");

    call.append(INVENTORY_ITEM_IFACE, "Present", std::variant<bool>(true));
    dbus.call(call);
}
