/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2022 */
#include "inventory.hpp"

struct MockInventory : public Inventory
{
    void migrate(std::span<inventory::Migration*>&& migrations) override;

    std::weak_ptr<dbus::PropertiesChangedListener> addPropertiesChangedListener(
        const std::string& path, const std::string& interface,
        std::function<void(dbus::PropertiesChanged&&)> callback) override;
    void removePropertiesChangedListener(
        std::weak_ptr<dbus::PropertiesChangedListener> listener) override;
    void add(const std::string& path,
             inventory::interfaces::Interface iface) override;
    void remove(const std::string& path,
                inventory::interfaces::Interface iface) override;
    void markPresent(const std::string& path) override;
    void markAbsent(const std::string& path) override;
    bool isPresent(const std::string& path) override;
    bool isModel(const std::string& path, const std::string& model) override;

    std::map<std::string, inventory::ObjectType> store;
    std::map<std::string, bool> present;
};
