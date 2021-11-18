/* SPDX-License-Identifier: Apache-2.0 */
#include "inventory.hpp"

#include "platform.hpp"

#include <phosphor-logging/lg2.hpp>
#include <xyz/openbmc_project/State/Decorator/PowerState/server.hpp>

#include <regex>

PHOSPHOR_LOG2_USING;

static constexpr auto INVENTORY_BUS_NAME =
    "xyz.openbmc_project.Inventory.Manager";

// https://github.com/openbmc/phosphor-dbus-interfaces/blob/08baf48ad5f15774d393fbbf4e9479a0ef3e82d0/yaml/xyz/openbmc_project/Inventory/Manager.interface.yaml
static constexpr auto INVENTORY_MANAGER_IFACE =
    "xyz.openbmc_project.Inventory.Manager";
static constexpr auto INVENTORY_MANAGER_OBJECT =
    "/xyz/openbmc_project/inventory";

// https://dbus.freedesktop.org/doc/dbus-specification.html#standard-interfaces-properties
static constexpr auto DBUS_PROPERTY_IFACE = "org.freedesktop.DBus.Properties";

static constexpr auto INVENTORY_DECORATOR_ASSET_IFACE =
    "xyz.openbmc_project.Inventory.Decorator.Asset";

static constexpr auto POWER_STATE_IFACE =
    "xyz.openbmc_project.State.Decorator.PowerState";

using namespace inventory;
using PowerState =
    sdbusplus::xyz::openbmc_project::State::Decorator::server::PowerState;

void InventoryManager::updateObject(const std::string& path,
                                    const ObjectType& updates)
{
    auto call =
        dbus.new_method_call(INVENTORY_BUS_NAME, INVENTORY_MANAGER_OBJECT,
                             INVENTORY_MANAGER_IFACE, "Notify");

    std::map<sdbusplus::message::object_path, ObjectType> inventoryUpdate = {
        {path, updates},
    };

    call.append(inventoryUpdate);
    dbus.call(call);
}

void InventoryManager::markPresent(const std::string& path)
{
    std::string absolute = std::string("/xyz/openbmc_project/inventory") + path;

    auto call = dbus.new_method_call(INVENTORY_BUS_NAME, absolute.c_str(),
                                     DBUS_PROPERTY_IFACE, "Set");

    call.append(INVENTORY_ITEM_IFACE, "Present", std::variant<bool>(true));
    dbus.call(call);
}

void InventoryManager::markAbsent(const std::string& path)
{
    std::string absolute = std::string("/xyz/openbmc_project/inventory") + path;

    auto call = dbus.new_method_call(INVENTORY_BUS_NAME, absolute.c_str(),
                                     DBUS_PROPERTY_IFACE, "Set");

    call.append(INVENTORY_ITEM_IFACE, "Present", std::variant<bool>(false));
    dbus.call(call);
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
        /* This is expected for PCIe slots populated with non-cable cards  */
        debug("Failed to determine device presence at path {INVENTORY_PATH}",
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
        /* This is expected for PCIe slots populated with non-cable cards  */
        debug("Failed to determine device presence at path {INVENTORY_PATH}",
              "INVENTORY_PATH", absolute);
        debug(
            "Inventory lookup for device at path {INVENTORY_PATH} resulted in an exception: {EXCEPTION}",
            "INVENTORY_PATH", absolute, "EXCEPTION", ex);
    }

    return false;
}

/* inventory::accumulate */

void inventory::accumulate(std::map<std::string, ObjectType>& store,
                           const std::string& path, const ObjectType& updates)
{
    if (store.contains(path))
    {
        auto& object = store[path];

        for (const auto& [ikey, ival] : updates)
        {
            if (object.contains(ikey))
            {
                auto& interface = object[ikey];

                for (const auto& [pkey, pval] : ival)
                {
                    interface[pkey] = pval;
                }
            }
            else
            {
                object[ikey] = ival;
            }
        }
    }
    else
    {
        store[path] = updates;
    }
}

void InventoryManager::watchSlotPowerState(PlatformManager* pm)
{
    namespace match_rules = sdbusplus::bus::match::rules;

    slotPowerStateChangedMatch = std::make_unique<sdbusplus::bus::match::match>(
        dbus,
        match_rules::propertiesChangedNamespace(
            "/xyz/openbmc_project/inventory", POWER_STATE_IFACE),
        std::bind(&InventoryManager::slotPowerStateChanged, this,
                  std::placeholders::_1, pm));
}

void InventoryManager::slotPowerStateChanged(sdbusplus::message::message& msg,
                                             PlatformManager* pm)
{
    std::string path = msg.get_path();
    std::map<std::string, std::variant<std::string>> properties;
    std::string interface;
    msg.read(interface, properties);

    if (interface != POWER_STATE_IFACE)
    {
        return;
    }

    if (!properties.contains("PowerState"))
    {
        return;
    }

    auto value = std::get<std::string>(properties.at("PowerState"));
    auto state = PowerState::convertStateFromString(value);
    auto slot = getSlotFromPath(path);

    // Don't care about NVME drive power states
    if (!pm->ignoreSlotPowerState(path))
    {
        pm->slotPowerStateChanged(slot, state == PowerState::State::On);
    }
}

int InventoryManager::getSlotFromPath(const std::string& path)
{
    std::regex exp{R"((\d+)$)"};
    std::smatch match;

    if (std::regex_search(path, match, exp))
    {
        auto slot = match[1].str().c_str();
        return (std::atoi(slot));
    }

    throw std::runtime_error{"Bad slot path: " + path};
}

/* PublishWhenPresentInventoryDecorator */

PublishWhenPresentInventoryDecorator::PublishWhenPresentInventoryDecorator(
    Inventory* inventory) :
    inventory(inventory)
{}

void PublishWhenPresentInventoryDecorator::updateObject(
    const std::string& path, const ObjectType& updates)
{
    inventory::accumulate(objectCache, path, updates);

    if (presentCache.contains(path) && presentCache[path])
    {
        inventory->updateObject(path, objectCache[path]);
        inventory->markPresent(path);
    }
}

void PublishWhenPresentInventoryDecorator::markPresent(const std::string& path)
{
    bool alreadyPresent = presentCache.contains(path) && presentCache[path];

    presentCache.insert_or_assign(path, true);

    if (!alreadyPresent && objectCache.contains(path))
    {
        inventory->updateObject(path, objectCache[path]);
        inventory->markPresent(path);
    }
}

void PublishWhenPresentInventoryDecorator::markAbsent(const std::string& path)
{
    bool wasPresent = !presentCache.contains(path) || presentCache[path];

    presentCache.insert_or_assign(path, false);

    if (wasPresent)
    {
        inventory->markAbsent(path);
    }
}

bool PublishWhenPresentInventoryDecorator::isPresent(const std::string& path)
{
    return inventory->isPresent(path);
}

bool PublishWhenPresentInventoryDecorator::isModel(const std::string& path,
                                                   const std::string& model)
{
    return inventory->isModel(path, model);
}
