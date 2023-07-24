/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2022 */
#include "mock-inventory.hpp"

#include "inventory/migrations.hpp"

using namespace inventory;

void MockInventory::accumulate(std::map<std::string, ObjectType>& store,
                               const std::string& path,
                               const ObjectType& updates)
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

void MockInventory::migrate(std::span<inventory::Migration*>&& migrations)
{
    auto old = store;

    for (const auto& [path, object] : old)
    {
        for (auto* migration : migrations)
        {
            migration->migrate(this, path, object);
        }
    }
}

std::weak_ptr<dbus::PropertiesChangedListener>
    MockInventory::addPropertiesChangedListener(
        [[maybe_unused]] const std::string& path,
        [[maybe_unused]] const std::string& interface,
        [[maybe_unused]] std::function<void(dbus::PropertiesChanged&&)>
            callback)
{
    throw std::logic_error("Unimplemented");
}

void MockInventory::removePropertiesChangedListener(
    [[maybe_unused]] std::weak_ptr<dbus::PropertiesChangedListener> listener)
{
    throw std::logic_error("Unimplemented");
}

void MockInventory::add(const std::string& path, interfaces::Interface iface)
{
    ObjectType update;

    iface.populateObject(update);

    accumulate(store, path, update);
}

void MockInventory::remove(const std::string& path, interfaces::Interface iface)
{
    ObjectType update;

    iface.depopulateObject(update);

    accumulate(store, path, update);
}

void MockInventory::markPresent(const std::string& path)
{
    present.insert_or_assign(path, true);
}

void MockInventory::markAbsent(const std::string& path)
{
    present.insert_or_assign(path, false);
}

bool MockInventory::isPresent(const std::string& path)
{
    return present.at(path);
}

bool MockInventory::isModel([[maybe_unused]] const std::string& path,
                            [[maybe_unused]] const std::string& model)
{
    throw std::logic_error("Unimplemented");
}
