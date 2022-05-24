/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2022 */
#include "inventory.hpp"
#include "inventory/migrations.hpp"
#include "mock-inventory.hpp"

#include "gtest/gtest.h"

using namespace inventory;

TEST(MigrateNVMeIPZVPDFromSlotToDrive, empty)
{
    MockInventory inventory;

    Inventory::migrate(&inventory, MigrateNVMeIPZVPDFromSlotToDrive());

    std::map<std::string, inventory::ObjectType> empty;

    EXPECT_EQ(empty, inventory.store);
}

TEST(MigrateNVMeIPZVPDFromSlotToDrive, noPCIeSlot)
{
    const char* path = "/system/chassis/motherboard/slot0";
    const ObjectType connector = {{
        "xyz.openbmc_project.Inventory.Item.Connector",
        {{}},
    }};

    MockInventory inventory;
    MockInventory::accumulate(inventory.store, path, connector);

    Inventory::migrate(&inventory, MigrateNVMeIPZVPDFromSlotToDrive());

    std::map<std::string, inventory::ObjectType> untouchedConnector = {
        {path, connector},
    };

    EXPECT_EQ(untouchedConnector, inventory.store);
}

TEST(MigrateNVMeIPZVPDFromSlotToDrive, noNVMePCIeSlot)
{
    const char* path = "/system/chassis/motherboard/slot0";
    const ObjectType slot = {{inventory::INVENTORY_ITEM_PCIESLOT_IFACE, {{}}}};

    MockInventory inventory;
    MockInventory::accumulate(inventory.store, path, slot);

    Inventory::migrate(&inventory, MigrateNVMeIPZVPDFromSlotToDrive());

    std::map<std::string, inventory::ObjectType> untouchedSlot = {{path, slot}};

    EXPECT_EQ(untouchedSlot, inventory.store);
}

TEST(MigrateNVMeIPZVPDFromSlotToDrive, noDecoration)
{
    const char* path = "/system/chassis/motherboard/nvme0";
    const ObjectType slot = {{inventory::INVENTORY_ITEM_PCIESLOT_IFACE, {{}}}};

    MockInventory inventory;
    MockInventory::accumulate(inventory.store, path, slot);

    Inventory::migrate(&inventory, MigrateNVMeIPZVPDFromSlotToDrive());

    std::map<std::string, inventory::ObjectType> untouchedSlot = {{path, slot}};

    EXPECT_EQ(untouchedSlot, inventory.store);
}

TEST(MigrateNVMeIPZVPDFromSlotToDrive, unmatchedVPDDecoration)
{
    const char* path = "/system/chassis/motherboard/nvme0";
    ObjectType slot = {{inventory::INVENTORY_ITEM_PCIESLOT_IFACE, {{}}}};

    interfaces::VINI vini({'F', 'O', 'O'}, {'a', 'b', 'c'});
    vini.populateObject(slot);

    MockInventory inventory;
    MockInventory::accumulate(inventory.store, path, slot);

    Inventory::migrate(&inventory, MigrateNVMeIPZVPDFromSlotToDrive());

    std::map<std::string, inventory::ObjectType> untouchedSlot = {{path, slot}};

    EXPECT_EQ(untouchedSlot, inventory.store);
}

TEST(MigrateNVMeIPZVPDFromSlotToDrive, matchedVPDDecoration)
{
    const char* path = "/system/chassis/motherboard/nvme0";
    ObjectType dirtySlot = {{inventory::INVENTORY_ITEM_PCIESLOT_IFACE, {{}}}};

    interfaces::VINI vini({'N', 'V', 'M', 'e'}, {'a', 'b', 'c'});
    vini.populateObject(dirtySlot);

    MockInventory inventory;
    MockInventory::accumulate(inventory.store, path, dirtySlot);

    std::map<std::string, inventory::ObjectType> dirty = {{path, dirtySlot}};

    EXPECT_EQ(dirty, inventory.store);

    Inventory::migrate(&inventory, MigrateNVMeIPZVPDFromSlotToDrive());

    ObjectType cleanSlot = {{inventory::INVENTORY_ITEM_PCIESLOT_IFACE, {{}}}};
    vini.depopulateObject(cleanSlot);

    std::map<std::string, inventory::ObjectType> clean = {{
        path,
        cleanSlot,
    }};

    EXPECT_EQ(clean, inventory.store);
}

TEST(MigrateNVMeI2CEndpointFromSlotToDrive, empty)
{
    MockInventory inventory;

    Inventory::migrate(&inventory, MigrateNVMeI2CEndpointFromSlotToDrive());

    std::map<std::string, inventory::ObjectType> empty;

    EXPECT_EQ(empty, inventory.store);
}

TEST(MigrateNVMeI2CEndpointFromSlotToDrive, noPCIeSlot)
{
    const char* path = "/system/chassis/motherboard/slot0";
    const ObjectType connector = {
        {"xyz.openbmc_project.Inventory.Item.Connector", {{}}}};

    MockInventory inventory;
    MockInventory::accumulate(inventory.store, path, connector);

    Inventory::migrate(&inventory, MigrateNVMeI2CEndpointFromSlotToDrive());

    std::map<std::string, inventory::ObjectType> untouchedConnector = {
        {path, connector}};

    EXPECT_EQ(untouchedConnector, inventory.store);
}

TEST(MigrateNVMeI2CEndpointFromSlotToDrive, noNVMePCIeSlot)
{
    const char* path = "/system/chassis/motherboard/slot0";
    const ObjectType slot = {{inventory::INVENTORY_ITEM_PCIESLOT_IFACE, {{}}}};

    MockInventory inventory;
    MockInventory::accumulate(inventory.store, path, slot);

    Inventory::migrate(&inventory, MigrateNVMeI2CEndpointFromSlotToDrive());

    std::map<std::string, inventory::ObjectType> untouchedSlot = {{path, slot}};

    EXPECT_EQ(untouchedSlot, inventory.store);
}

TEST(MigrateNVMeI2CEndpointFromSlotToDrive, noDecoration)
{
    const char* path = "/system/chassis/motherboard/nvme0";
    const ObjectType slot = {{inventory::INVENTORY_ITEM_PCIESLOT_IFACE, {{}}}};

    MockInventory inventory;
    MockInventory::accumulate(inventory.store, path, slot);

    Inventory::migrate(&inventory, MigrateNVMeI2CEndpointFromSlotToDrive());

    std::map<std::string, inventory::ObjectType> untouchedSlot = {{path, slot}};

    EXPECT_EQ(untouchedSlot, inventory.store);
}

TEST(MigrateNVMeI2CEndpointFromSlotToDrive, i2cDecoraion)
{
    const char* path = "/system/chassis/motherboard/nvme0";
    ObjectType dirtySlot = {{inventory::INVENTORY_ITEM_PCIESLOT_IFACE, {{}}}};

    interfaces::I2CDevice i2cDevice(12, 0x4a);
    i2cDevice.populateObject(dirtySlot);

    MockInventory inventory;
    MockInventory::accumulate(inventory.store, path, dirtySlot);

    std::map<std::string, inventory::ObjectType> dirty = {{path, dirtySlot}};

    EXPECT_EQ(dirty, inventory.store);

    Inventory::migrate(&inventory, MigrateNVMeI2CEndpointFromSlotToDrive());

    ObjectType cleanSlot = {{inventory::INVENTORY_ITEM_PCIESLOT_IFACE, {{}}}};
    i2cDevice.depopulateObject(cleanSlot);

    std::map<std::string, inventory::ObjectType> clean = {{path, cleanSlot}};

    EXPECT_EQ(clean, inventory.store);
}
