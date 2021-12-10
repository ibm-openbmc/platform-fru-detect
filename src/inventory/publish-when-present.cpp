/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */
#include "dbus.hpp"
#include "inventory.hpp"

#include <cassert>

using namespace inventory;
using namespace dbus;

PublishWhenPresentInventoryDecorator::PublishWhenPresentInventoryDecorator(
    Inventory* inventory) :
    inventory(inventory)
{}

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
    const std::string& path, const interfaces::Interface iface)
{
    if (!objectCache.contains(path))
    {
        objectCache.try_emplace(path);
    }

    objectCache[path].push_back(iface);

    if (presentCache.contains(path) && presentCache[path])
    {
        inventory->add(path, iface);
    }
}

void PublishWhenPresentInventoryDecorator::remove(
    const std::string& path, const interfaces::Interface iface)
{
    if (presentCache.contains(path) && !presentCache[path])
    {
        inventory->remove(path, iface);
    }

    if (!objectCache.contains(path))
    {
        return;
    }

    /* Neuter it */
    ObjectType removed;
    iface.depopulateObject(removed);
    auto [k, v] = *(removed.begin());
    const auto addProps{v};
    const auto removeProps{v};
    objectCache[path].emplace_back(k, std::move(addProps),
                                   std::move(removeProps));
}

void PublishWhenPresentInventoryDecorator::markPresent(const std::string& path)
{
    bool alreadyPresent = presentCache.contains(path) && presentCache[path];

    presentCache.insert_or_assign(path, true);

    if (!alreadyPresent && objectCache.contains(path))
    {
        for (const auto& iface : objectCache[path])
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

        for (const auto& iface : objectCache[path])
        {
            inventory->remove(path, iface);
        }
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
