/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2022 */

#include "inventory/migrations.hpp"

#include <phosphor-logging/lg2.hpp>

#include <filesystem>

PHOSPHOR_LOG2_USING;

using namespace inventory;

Migration::Result MigrateNVMeI2CEndpointFromSlotToDrive::migrate(
    Inventory* inventory, const std::string& path,
    const ObjectType& object) const
{
    std::filesystem::path fp(path);

    if (!object.contains(INVENTORY_ITEM_PCIESLOT_IFACE))
    {
        return Result::INVALID;
    }

    // A bit hacky as D-Bus paths aren't filesystem paths, but whatever
    if (!fp.filename().string().starts_with("nvme"))
    {
        return Result::INVALID;
    }

    if (!object.contains(INVENTORY_DECORATOR_I2CDEVICE_IFACE))
    {
        return Result::INVALID;
    }

    inventory->remove(path, interfaces::I2CDevice());

    return Result::SUCCESS;
}
