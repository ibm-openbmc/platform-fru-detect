/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */

#include "inventory.hpp"
#include "inventory/migrations.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>

#include <cstring>
#include <exception>

PHOSPHOR_LOG2_USING;

// NOLINTNEXTLINE(readability-identifier-naming)
static constexpr auto INVENTORY_BUS_NAME =
    "xyz.openbmc_project.Inventory.Manager";

// https://github.com/openbmc/phosphor-dbus-interfaces/blob/08baf48ad5f15774d393fbbf4e9479a0ef3e82d0/yaml/xyz/openbmc_project/Inventory/Manager.interface.yaml
// NOLINTNEXTLINE(readability-identifier-naming)
static constexpr auto INVENTORY_MANAGER_IFACE =
    "xyz.openbmc_project.Inventory.Manager";

// NOLINTNEXTLINE(readability-identifier-naming)
static constexpr auto INVENTORY_MANAGER_OBJECT =
    "/xyz/openbmc_project/inventory";

// https://dbus.freedesktop.org/doc/dbus-specification.html#standard-interfaces-properties
// NOLINTNEXTLINE(readability-identifier-naming)
static constexpr auto DBUS_PROPERTY_IFACE = "org.freedesktop.DBus.Properties";

// https://dbus.freedesktop.org/doc/dbus-specification.html#standard-interfaces-objectmanager
// NOLINTNEXTLINE(readability-identifier-naming)
static constexpr auto DBUS_OBJECTMANAGER_IFACE =
    "org.freedesktop.DBus.ObjectManager";

// NOLINTNEXTLINE(readability-identifier-naming)
static constexpr auto INVENTORY_DECORATOR_ASSET_IFACE =
    "xyz.openbmc_project.Inventory.Decorator.Asset";

using namespace inventory;
using namespace dbus;

void InventoryManager::migrate(std::span<Migration*>&& migrations)
{
    auto call =
        dbus.new_method_call(INVENTORY_BUS_NAME, INVENTORY_MANAGER_OBJECT,
                             DBUS_OBJECTMANAGER_IFACE, "GetManagedObjects");
    try
    {
        std::map<sdbusplus::message::object_path, ObjectType> objects;
        auto reply = dbus.call(call);
        reply.read(objects);

        for (const auto& [objectPath, object] : objects)
        {
            const auto itemPath = extractItemPath({objectPath});

            for (auto migration : migrations)
            {
                Migration::Result r = migration->migrate(this, itemPath,
                                                         object);
                switch (r)
                {
                    case Migration::Result::INVALID:
                        debug(
                            "Migration {MIGRATION_NAME} not applicable on inventory object {OBJECT_PATH}",
                            "MIGRATION_NAME", migration->name(), "OBJECT_PATH",
                            itemPath);
                        break;
                    case Migration::Result::SUCCESS:
                        info(
                            "Applied migration {MIGRATION_NAME} on inventory object {OBJECT_PATH}",
                            "MIGRATION_NAME", migration->name(), "OBJECT_PATH",
                            itemPath);
                        break;
                    case Migration::Result::FAILED:
                        warning(
                            "Failed to apply migration {MIGRATION_NAME} on inventory object {OBJECT_PATH}",
                            "MIGRATION_NAME", migration->name(), "OBJECT_PATH",
                            itemPath);
                        break;
                }
            }
        }
    }
    catch (const sdbusplus::exception::exception& ex)
    {
        warning(
            "Failed to fetch inventory objects for migration: {EXCEPTION_DESCRIPTION}",
            "EXCEPTION_DESCRIPTION", ex);
        throw;
    }
}

std::string InventoryManager::extractItemPath(const std::string& objectPath)
{
    if (!objectPath.starts_with(INVENTORY_MANAGER_OBJECT))
    {
        return {objectPath};
    }

    return objectPath.substr(strlen(INVENTORY_MANAGER_OBJECT));
}

std::weak_ptr<PropertiesChangedListener>
    InventoryManager::addPropertiesChangedListener(
        const std::string& path, const std::string& interface,
        std::function<void(PropertiesChanged&&)> callback)
{
    auto pcl = sharedPropertiesChangedListener(dbus, path, interface, callback);
    return *(listeners.insert(pcl).first);
}

void InventoryManager::removePropertiesChangedListener(
    std::weak_ptr<PropertiesChangedListener> listener)
{
    std::shared_ptr<PropertiesChangedListener> shared = listener.lock();
    if (!listeners.contains(shared))
    {
        debug("Cannot remove unrecognised PropertiesChangedListener");
        return;
    }

    listeners.erase(shared);
}

void InventoryManager::updateObject(const std::string& path,
                                    const ObjectType& updates)
{
    auto call = dbus.new_method_call(INVENTORY_BUS_NAME,
                                     INVENTORY_MANAGER_OBJECT,
                                     INVENTORY_MANAGER_IFACE, "Notify");

    std::map<sdbusplus::message::object_path, ObjectType> inventoryUpdate = {
        {path, updates},
    };

    call.append(inventoryUpdate);

    try
    {
        dbus.call(call);
    }
    catch (const sdbusplus::exception::SdBusError& ex)
    {
        /* Probably a bit bogus, but sdbusplus doesn't help us differentiate? */
        std::throw_with_nested(NoSuchInventoryItem(path));
    }
}

void InventoryManager::add(const std::string& path,
                           const interfaces::Interface iface)
{
    ObjectType updates;

    iface.populateObject(updates);

    updateObject(path, updates);
}

void InventoryManager::remove(const std::string& path,
                              const interfaces::Interface iface)
{
    ObjectType updates;

    iface.depopulateObject(updates);

    updateObject(path, updates);
}

void InventoryManager::markPresent(const std::string& path)
{
    std::string absolute = std::string("/xyz/openbmc_project/inventory") + path;

    auto call = dbus.new_method_call(INVENTORY_BUS_NAME, absolute.c_str(),
                                     DBUS_PROPERTY_IFACE, "Set");

    call.append(INVENTORY_ITEM_IFACE, "Present", std::variant<bool>(true));

    try
    {
        dbus.call(call);
    }
    catch (const sdbusplus::exception::SdBusError& ex)
    {
        /* Probably a bit bogus, but sdbusplus doesn't help us differentiate? */
        std::throw_with_nested(NoSuchInventoryItem(path));
    }
}

void InventoryManager::markAbsent(const std::string& path)
{
    std::string absolute = std::string("/xyz/openbmc_project/inventory") + path;

    auto call = dbus.new_method_call(INVENTORY_BUS_NAME, absolute.c_str(),
                                     DBUS_PROPERTY_IFACE, "Set");

    call.append(INVENTORY_ITEM_IFACE, "Present", std::variant<bool>(false));

    try
    {
        dbus.call(call);
    }
    catch (const sdbusplus::exception::SdBusError& ex)
    {
        /* Probably a bit bogus, but sdbusplus doesn't help us differentiate? */
        std::throw_with_nested(NoSuchInventoryItem(path));
    }
}

bool InventoryManager::isPresent(const std::string& path)
{
    std::string absolute = std::string("/xyz/openbmc_project/inventory") + path;

    auto call = dbus.new_method_call(INVENTORY_BUS_NAME, absolute.c_str(),
                                     DBUS_PROPERTY_IFACE, "Get");

    call.append(INVENTORY_ITEM_IFACE, "Present");

    try
    {
        std::variant<bool> present;
        auto reply = dbus.call(call);
        reply.read(present);

        return std::get<bool>(present);
    }
    catch (const sdbusplus::exception::exception& ex)
    {
        /* TODO: Define an exception for inventory operations? */
        info("Failed to determine device presence at path {INVENTORY_PATH}",
             "INVENTORY_PATH", absolute);
        debug(
            "Inventory lookup for device at path {INVENTORY_PATH} resulted in an exception: {EXCEPTION}",
            "INVENTORY_PATH", absolute, "EXCEPTION", ex);
    }

    return false;
}

bool InventoryManager::isModel(const std::string& path,
                               const std::string& model)
{
    std::string absolute = std::string("/xyz/openbmc_project/inventory") + path;

    auto call = dbus.new_method_call(INVENTORY_BUS_NAME, absolute.c_str(),
                                     DBUS_PROPERTY_IFACE, "Get");

    call.append(INVENTORY_DECORATOR_ASSET_IFACE, "Model");

    try
    {
        std::variant<std::string> found;
        auto reply = dbus.call(call);
        reply.read(found);

        return std::get<std::string>(found) == model;
    }
    catch (const sdbusplus::exception::exception& ex)
    {
        /* TODO: Define an exception for inventory operations? */
        info("Failed to determine device presence at path {INVENTORY_PATH}",
             "INVENTORY_PATH", absolute);
        debug(
            "Inventory lookup for device at path {INVENTORY_PATH} resulted in an exception: {EXCEPTION}",
            "INVENTORY_PATH", absolute, "EXCEPTION", ex);
    }

    return false;
}
