/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */
#pragma once

#include "dbus.hpp"

#include <functional>
#include <list>
#include <map>
#include <set>
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

namespace interfaces
{
class Interface
{
  public:
    Interface(const std::string& interface, const InterfaceType&& addProperties,
              const InterfaceType&& removeProperties) :
        interface(interface),
        addProperties(addProperties), removeProperties(removeProperties)
    {}
    virtual ~Interface() = default;
    bool operator==(const Interface& other) const = default;

    void populateObject(ObjectType& object) const
    {
        updateObject(object, addProperties);
    }

    void depopulateObject(ObjectType& object) const
    {
        updateObject(object, removeProperties);
    }

  private:
    void updateObject(ObjectType& object, const InterfaceType& updates) const
    {
        if (!object.contains(interface))
        {
            object.try_emplace(interface);
        }

        InterfaceType& container = object[interface];
        for (const auto& [property, value] : updates)
        {
            container[property] = value;
        }
    }

    const std::string interface;
    const InterfaceType addProperties;
    const InterfaceType removeProperties;
};

class I2CDevice : public Interface
{
  public:
    explicit I2CDevice(int bus, int address) :
        Interface(INVENTORY_DECORATOR_I2CDEVICE_IFACE,
                  {{"Bus", static_cast<size_t>(bus)},
                   {"Address", static_cast<size_t>(address)}},
                  {{"Bus", static_cast<size_t>(INT_MAX)},
                   {"Address", static_cast<size_t>(0)}})
    {}
    ~I2CDevice() override = default;
};

class VINI : public Interface
{
  public:
    explicit VINI(const std::vector<uint8_t>&& model,
                  const std::vector<uint8_t>&& serial) :
        Interface(INVENTORY_IPZVPD_VINI_IFACE,
                  {{"RT", std::vector<uint8_t>({'V', 'I', 'N', 'I'})},
                   {"CC", model},
                   {"SN", serial}},
                  {{"RT", std::vector<uint8_t>({'V', 'I', 'N', 'I'})},
                   {"CC", std::vector<uint8_t>(0)},
                   {"SN", std::vector<uint8_t>(0)}})
    {}
    ~VINI() override = default;
};
} // namespace interfaces
} // namespace inventory

class Inventory
{
  public:
    Inventory() = default;
    virtual ~Inventory() = default;

    virtual std::weak_ptr<dbus::PropertiesChangedListener>
        addPropertiesChangedListener(
            const std::string& path, const std::string& interface,
            std::function<void(dbus::PropertiesChanged&& props)> callback) = 0;
    virtual void removePropertiesChangedListener(
        std::weak_ptr<dbus::PropertiesChangedListener> listener) = 0;

    virtual void add(const std::string& path,
                     const inventory::interfaces::Interface iface) = 0;
    virtual void remove(const std::string& path,
                        const inventory::interfaces::Interface iface) = 0;
    virtual void markPresent(const std::string& path) = 0;
    virtual void markAbsent(const std::string& path) = 0;
    virtual bool isPresent(const std::string& path) = 0;
    virtual bool isModel(const std::string& path, const std::string& model) = 0;
};

class InventoryManager : public Inventory
{
  public:
    InventoryManager(sdbusplus::bus::bus& dbus) : dbus(dbus)
    {}
    ~InventoryManager() override = default;

    /* Inventory */
    std::weak_ptr<dbus::PropertiesChangedListener>
        addPropertiesChangedListener(
            const std::string& path, const std::string& interface,
            std::function<void(dbus::PropertiesChanged&& props)> callback)
            override;
    void removePropertiesChangedListener(
        std::weak_ptr<dbus::PropertiesChangedListener> listener) override;
    void add(const std::string& path, const inventory::interfaces::Interface iface) override;
    void remove(const std::string& path, const inventory::interfaces::Interface iface) override;
    void markPresent(const std::string& path) override;
    void markAbsent(const std::string& path) override;
    bool isPresent(const std::string& path) override;
    bool isModel(const std::string& path, const std::string& model) override;

  private:
    virtual void updateObject(const std::string& path,
                              const inventory::ObjectType& updates);

    sdbusplus::bus::bus& dbus;
    std::set<std::shared_ptr<dbus::PropertiesChangedListener>> listeners;
};

/* Unifies the split we have with WilliwakasNVMeDrive and FlettNVMeDrive */
class PublishWhenPresentInventoryDecorator : public Inventory
{
  public:
    PublishWhenPresentInventoryDecorator(Inventory* inventory);
    ~PublishWhenPresentInventoryDecorator() override = default;

    /* Inventory */
    std::weak_ptr<dbus::PropertiesChangedListener>
        addPropertiesChangedListener(
            const std::string& path, const std::string& interface,
            std::function<void(dbus::PropertiesChanged&& props)> callback)
            override;
    void removePropertiesChangedListener(
        std::weak_ptr<dbus::PropertiesChangedListener> listener) override;
    void add(const std::string& path, const inventory::interfaces::Interface iface) override;
    void remove(const std::string& path, const inventory::interfaces::Interface iface) override;
    void markPresent(const std::string& path) override;
    void markAbsent(const std::string& path) override;
    bool isPresent(const std::string& path) override;
    bool isModel(const std::string& path, const std::string& model) override;

  private:
    Inventory* inventory;
    std::map<std::string, std::list<inventory::interfaces::Interface>>
        objectCache;
    std::map<std::string, bool> presentCache;
};
