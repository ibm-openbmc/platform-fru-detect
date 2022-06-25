/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */
#include "dbus.hpp"
#include "inventory.hpp"

#include <cassert>
#include <utility>

using namespace inventory;
using namespace dbus;

PublishWhenPresentInventoryDecorator::PublishWhenPresentInventoryDecorator(
    Inventory* inventory) :
    inventory(inventory)
{}

void PublishWhenPresentInventoryDecorator::migrate(
    std::span<Migration*>&& migrations)
{
    inventory->migrate(std::forward<std::span<Migration*>>(migrations));
}

std::weak_ptr<PropertiesChangedListener>
    PublishWhenPresentInventoryDecorator::addPropertiesChangedListener(
        const std::string& path, const std::string& interface,
        std::function<void(PropertiesChanged&&)> callback)
{
    return inventory->addPropertiesChangedListener(path, interface, callback);
}

void PublishWhenPresentInventoryDecorator::removePropertiesChangedListener(
    std::weak_ptr<PropertiesChangedListener> listener)
{
    inventory->removePropertiesChangedListener(listener);
}

void PublishWhenPresentInventoryDecorator::add(
    const std::string& path, interfaces::Interface iface)
{
    objectCache[path].insert_or_assign(iface.getInterfaceName(), iface);

    if (presentCache.contains(path) && presentCache[path])
    {
        inventory->add(path, iface);
    }
}

void PublishWhenPresentInventoryDecorator::remove(
    const std::string& path, interfaces::Interface iface)
{
    if (presentCache.contains(path) && presentCache[path])
    {
        // Remove it when it's marked absent
        objectCache[path].insert_or_assign(iface.getInterfaceName(), iface);
    }
    else
    {
        inventory->remove(path, iface);
    }
}

void PublishWhenPresentInventoryDecorator::markPresent(const std::string& path)
{
    bool alreadyPresent = presentCache.contains(path) && presentCache[path];

    presentCache.insert_or_assign(path, true);

    if (!alreadyPresent && objectCache.contains(path))
    {
        for (const auto& [_, iface] : objectCache[path])
        {
            inventory->add(path, iface);
        }

        inventory->markPresent(path);
    }
}

void PublishWhenPresentInventoryDecorator::markAbsent(const std::string& path)
{
    bool wasPresent = !presentCache.contains(path) || presentCache[path];

    presentCache.insert_or_assign(path, false);

    if (wasPresent && objectCache.contains(path))
    {
        inventory->markAbsent(path);

        for (const auto& [_, iface] : objectCache[path])
        {
            inventory->remove(path, iface);
        }

        objectCache.erase(path);
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
