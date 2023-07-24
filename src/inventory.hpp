/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */
#pragma once

#include "dbus.hpp"

#include <exception>
#include <functional>
#include <list>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <variant>
#include <vector>

/* Forward-declarations for minor dependencies */
namespace sdbusplus
{
namespace bus
{
struct bus;
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
// NOLINTNEXTLINE(readability-identifier-naming)
static constexpr auto INVENTORY_ITEM_IFACE =
    "xyz.openbmc_project.Inventory.Item";

// NOLINTNEXTLINE(readability-identifier-naming)
static constexpr auto INVENTORY_ITEM_PCIESLOT_IFACE =
    "xyz.openbmc_project.Inventory.Item.PCIeSlot";

// https://github.com/openbmc/phosphor-dbus-interfaces/blob/08baf48ad5f15774d393fbbf4e9479a0ef3e82d0/yaml/xyz/openbmc_project/Inventory/Decorator/I2CDevice.interface.yaml
// NOLINTNEXTLINE(readability-identifier-naming)
static constexpr auto INVENTORY_DECORATOR_I2CDEVICE_IFACE =
    "xyz.openbmc_project.Inventory.Decorator.I2CDevice";

// NOLINTNEXTLINE(readability-identifier-naming)
static constexpr auto INVENTORY_IPZVPD_VINI_IFACE = "com.ibm.ipzvpd.VINI";

class Migration;

namespace interfaces
{
class Interface
{
  public:
    Interface(const std::string& interface,
              const InterfaceType&& removeProperties) :
        name(interface),
        addProperties(std::nullopt), removeProperties(removeProperties)
    {}
    Interface(const std::string& interface, const InterfaceType&& addProperties,
              const InterfaceType&& removeProperties) :
        name(interface),
        addProperties(addProperties), removeProperties(removeProperties)
    {}
    Interface(const Interface& other) = default;
    Interface(Interface&& other) = default;
    virtual ~Interface() = default;
    Interface& operator=(const Interface& other) = default;
    Interface& operator=(Interface&& other) = default;
    bool operator==(const Interface& other) const = default;

    void populateObject(ObjectType& object) const
    {
        updateObject(object, addProperties.value());
    }

    void depopulateObject(ObjectType& object) const
    {
        updateObject(object, removeProperties);
    }

    const std::string& getInterfaceName() const
    {
        return name;
    }

  private:
    void updateObject(ObjectType& object, const InterfaceType& updates) const
    {
        if (!object.contains(name))
        {
            object.try_emplace(name);
        }

        InterfaceType& container = object[name];
        for (const auto& [property, value] : updates)
        {
            container[property] = value;
        }
    }

    std::string name;
    std::optional<InterfaceType> addProperties;
    InterfaceType removeProperties;
};

class I2CDevice : public Interface
{
  public:
    I2CDevice() :
        Interface(INVENTORY_DECORATOR_I2CDEVICE_IFACE,
                  {{"Bus", static_cast<size_t>(INT_MAX)},
                   {"Address", static_cast<size_t>(0)}})
    {}
    explicit I2CDevice(int bus, int address) :
        Interface(INVENTORY_DECORATOR_I2CDEVICE_IFACE,
                  {{"Bus", static_cast<size_t>(bus)},
                   {"Address", static_cast<size_t>(address)}},
                  {{"Bus", static_cast<size_t>(INT_MAX)},
                   {"Address", static_cast<size_t>(0)}})
    {}
};

class VINI : public Interface
{
  public:
    VINI() :
        Interface(INVENTORY_IPZVPD_VINI_IFACE,
                  {{"RT", std::vector<uint8_t>({'V', 'I', 'N', 'I'})},
                   {"CC", std::vector<uint8_t>(0)},
                   {"SN", std::vector<uint8_t>(0)}})
    {}
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
};
} // namespace interfaces
} // namespace inventory

template <typename T>
concept DerivesMigration = std::is_base_of<inventory::Migration, T>::value;

class NoSuchInventoryItem : public std::exception
{
  public:
    NoSuchInventoryItem(const std::string& path) :
        description("No such inventory item: " + path)
    {}
    NoSuchInventoryItem(const NoSuchInventoryItem& other) = default;
    NoSuchInventoryItem(NoSuchInventoryItem&& other) = default;
    ~NoSuchInventoryItem() override = default;

    NoSuchInventoryItem& operator=(const NoSuchInventoryItem& other) = delete;
    NoSuchInventoryItem& operator=(NoSuchInventoryItem&& other) = delete;

    const char* what() const noexcept override
    {
        return description.c_str();
    }

  private:
    const std::string description;
};

class Inventory
{
  public:
    template <DerivesMigration... Ms>
    static void migrate(Inventory* inventory, Ms&&... impls)
    {
        auto migrations = std::to_array<inventory::Migration*>({&impls...});
        inventory->migrate(migrations);
    }

    virtual void migrate(std::span<inventory::Migration*>&& migrations) = 0;

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
    InventoryManager() = delete;
    InventoryManager(const InventoryManager& other) = delete;
    InventoryManager(InventoryManager&& other) = delete;
    InventoryManager(sdbusplus::bus::bus& dbus) : dbus(dbus) {}
    virtual ~InventoryManager() = default;

    InventoryManager& operator=(const InventoryManager& other) = delete;
    InventoryManager& operator=(InventoryManager&& other) = delete;

    void migrate(std::span<inventory::Migration*>&& migrations) override;

    /* Inventory */
    std::weak_ptr<dbus::PropertiesChangedListener> addPropertiesChangedListener(
        const std::string& path, const std::string& interface,
        std::function<void(dbus::PropertiesChanged&& props)> callback) override;
    void removePropertiesChangedListener(
        std::weak_ptr<dbus::PropertiesChangedListener> listener) override;
    void add(const std::string& path,
             const inventory::interfaces::Interface iface) override;
    void remove(const std::string& path,
                const inventory::interfaces::Interface iface) override;
    void markPresent(const std::string& path) override;
    void markAbsent(const std::string& path) override;
    bool isPresent(const std::string& path) override;
    bool isModel(const std::string& path, const std::string& model) override;

  private:
    virtual void updateObject(const std::string& path,
                              const inventory::ObjectType& updates);

    static std::string extractItemPath(const std::string& objectPath);

    sdbusplus::bus::bus& dbus;
    std::set<std::shared_ptr<dbus::PropertiesChangedListener>> listeners;
};

/* Unifies the split we have with WilliwakasNVMeDrive and FlettNVMeDrive */
class PublishWhenPresentInventoryDecorator : public Inventory
{
  public:
    PublishWhenPresentInventoryDecorator() = delete;
    PublishWhenPresentInventoryDecorator(Inventory* inventory);
    PublishWhenPresentInventoryDecorator(
        const PublishWhenPresentInventoryDecorator& other) = delete;
    PublishWhenPresentInventoryDecorator(
        PublishWhenPresentInventoryDecorator&& other) = delete;
    virtual ~PublishWhenPresentInventoryDecorator() = default;

    PublishWhenPresentInventoryDecorator&
        operator=(const PublishWhenPresentInventoryDecorator& other) = delete;
    PublishWhenPresentInventoryDecorator&
        operator=(PublishWhenPresentInventoryDecorator&& other) = delete;

    void migrate(std::span<inventory::Migration*>&& migrations) override;

    /* Inventory */
    std::weak_ptr<dbus::PropertiesChangedListener> addPropertiesChangedListener(
        const std::string& path, const std::string& interface,
        std::function<void(dbus::PropertiesChanged&& props)> callback) override;
    void removePropertiesChangedListener(
        std::weak_ptr<dbus::PropertiesChangedListener> listener) override;
    void add(const std::string& path,
             const inventory::interfaces::Interface iface) override;
    void remove(const std::string& path,
                const inventory::interfaces::Interface iface) override;
    void markPresent(const std::string& path) override;
    void markAbsent(const std::string& path) override;
    bool isPresent(const std::string& path) override;
    bool isModel(const std::string& path, const std::string& model) override;

  private:
    Inventory* inventory;
    std::map<std::string,
             std::map<std::string, inventory::interfaces::Interface>>
        objectCache;
    std::map<std::string, bool> presentCache;
};
