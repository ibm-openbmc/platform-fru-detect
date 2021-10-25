#include "inventory.hpp"

#include "gtest/gtest.h"

static constexpr auto TEST_PATH = "/path/test";
static constexpr auto TEST_PATH_1 = "/path/test/1";
static constexpr auto TEST_PATH_2 = "/path/test/2";
static constexpr auto TEST_INTERFACE = "interface.Test";
static constexpr auto TEST_INTERFACE_1 = "interface.Test.1";
static constexpr auto TEST_INTERFACE_2 = "interface.Test.2";

struct MockInventory : public Inventory
{
    virtual void updateObject(const std::string& path,
                              const inventory::ObjectType& updates) override
    {
        inventory::accumulate(store, path, updates);
    }

    virtual void markPresent(const std::string& path) override
    {
        present.insert_or_assign(path, true);
    }

    virtual void markAbsent(const std::string& path) override
    {
        present.insert_or_assign(path, false);
    }

    std::map<std::string, inventory::ObjectType> store;
    std::map<std::string, bool> present;
};

TEST(MockInventory, updateAnObjectOnce)
{
    MockInventory inventory;

    inventory::ObjectType testUpdate = {
        {
            TEST_INTERFACE,
            {
                {"TestProperty", 1},
            },
        },
    };

    inventory.updateObject(TEST_PATH, testUpdate);

    std::map<std::string, inventory::ObjectType> expected = {
        {TEST_PATH, testUpdate}};

    EXPECT_EQ(expected, inventory.store);
}

TEST(MockInventory, updateTwiceDistinctObjects)
{
    MockInventory inventory;

    inventory::ObjectType firstUpdate = {
        {
            TEST_INTERFACE,
            {
                {"FirstProperty", true},
            },
        },
    };

    inventory.updateObject(TEST_PATH_1, firstUpdate);

    inventory::ObjectType secondUpdate = {
        {
            TEST_INTERFACE,
            {
                {"SecondProperty", false},
            },
        },
    };

    inventory.updateObject(TEST_PATH_2, secondUpdate);

    EXPECT_TRUE(inventory.store.contains(TEST_PATH_1));
    EXPECT_FALSE(inventory.store[TEST_PATH_1][TEST_INTERFACE].contains(
        "SecondProperty"));
    EXPECT_EQ(
        true,
        std::get<bool>(
            inventory.store[TEST_PATH_1][TEST_INTERFACE]["FirstProperty"]));
    EXPECT_TRUE(inventory.store.contains(TEST_PATH_2));
    EXPECT_FALSE(
        inventory.store[TEST_PATH_2][TEST_INTERFACE].contains("FirstProperty"));
    EXPECT_EQ(
        false,
        std::get<bool>(
            inventory.store[TEST_PATH_2][TEST_INTERFACE]["SecondProperty"]));
}

TEST(MockInventory, updateAnObjectTwiceDistinctInterfaces)
{
    MockInventory inventory;

    inventory::ObjectType firstUpdate = {
        {
            TEST_INTERFACE_1,
            {
                {"FirstProperty", true},
            },
        },
    };

    inventory.updateObject(TEST_PATH, firstUpdate);

    inventory::ObjectType secondUpdate = {
        {
            TEST_INTERFACE_2,
            {
                {"SecondProperty", false},
            },
        },
    };

    inventory.updateObject(TEST_PATH, secondUpdate);

    EXPECT_TRUE(inventory.store[TEST_PATH].contains(TEST_INTERFACE_1));
    EXPECT_FALSE(inventory.store[TEST_PATH][TEST_INTERFACE_1].contains(
        "SecondProperty"));
    EXPECT_EQ(
        true,
        std::get<bool>(
            inventory.store[TEST_PATH][TEST_INTERFACE_1]["FirstProperty"]));
    EXPECT_TRUE(inventory.store[TEST_PATH].contains(TEST_INTERFACE_2));
    EXPECT_FALSE(
        inventory.store[TEST_PATH][TEST_INTERFACE_2].contains("FirstProperty"));
    EXPECT_EQ(
        false,
        std::get<bool>(
            inventory.store[TEST_PATH][TEST_INTERFACE_2]["SecondProperty"]));
}

TEST(MockInventory, updateAnObjectInterfaceTwiceDistinctProperties)
{
    MockInventory inventory;

    inventory::ObjectType firstUpdate = {
        {
            TEST_INTERFACE,
            {
                {"FirstProperty", true},
            },
        },
    };

    inventory.updateObject(TEST_PATH, firstUpdate);

    inventory::ObjectType secondUpdate = {
        {
            TEST_INTERFACE,
            {
                {"SecondProperty", false},
            },
        },
    };

    inventory.updateObject(TEST_PATH, secondUpdate);

    EXPECT_TRUE(
        inventory.store[TEST_PATH][TEST_INTERFACE].contains("FirstProperty"));
    EXPECT_EQ(true,
              std::get<bool>(
                  inventory.store[TEST_PATH][TEST_INTERFACE]["FirstProperty"]));
    EXPECT_TRUE(
        inventory.store[TEST_PATH][TEST_INTERFACE].contains("SecondProperty"));
    EXPECT_EQ(
        false,
        std::get<bool>(
            inventory.store[TEST_PATH][TEST_INTERFACE]["SecondProperty"]));
}

TEST(MockInventory, updateAnObjectTwiceSameProperty)
{
    MockInventory inventory;

    inventory::ObjectType firstUpdate = {
        {
            TEST_INTERFACE,
            {
                {"FirstProperty", true},
            },
        },
    };

    inventory.updateObject(TEST_PATH, firstUpdate);

    inventory::ObjectType secondUpdate = {
        {
            TEST_INTERFACE,
            {
                {"FirstProperty", false},
            },
        },
    };

    inventory.updateObject(TEST_PATH, secondUpdate);

    EXPECT_EQ(false,
              std::get<bool>(
                  inventory.store[TEST_PATH][TEST_INTERFACE]["FirstProperty"]));
}

TEST(PublishWhenPresent, notifyInterfacesButNotPresent)
{
    MockInventory inventory;
    PublishWhenPresentInventoryDecorator decorator(&inventory);

    inventory::ObjectType update = {
        {
            TEST_INTERFACE,
            {
                {"FirstProperty", true},
            },
        },
    };

    decorator.updateObject(TEST_PATH, update);

    EXPECT_TRUE(inventory.store.empty());
    EXPECT_TRUE(inventory.present.empty());
}

TEST(PublishWhenPresent, setPresentWithNoInterfaces)
{
    MockInventory inventory;
    PublishWhenPresentInventoryDecorator decorator(&inventory);

    decorator.markPresent(TEST_PATH);

    EXPECT_TRUE(inventory.store.empty());
    EXPECT_TRUE(inventory.present.empty());
}

TEST(PublishWhenPresent, notifyInterfacesAndSetPresent)
{
    MockInventory inventory;
    PublishWhenPresentInventoryDecorator decorator(&inventory);

    inventory::ObjectType update = {
        {
            TEST_INTERFACE,
            {
                {"FirstProperty", true},
            },
        },
    };

    decorator.updateObject(TEST_PATH, update);
    decorator.markPresent(TEST_PATH);

    EXPECT_FALSE(inventory.store.empty());
    EXPECT_TRUE(inventory.store.contains(TEST_PATH));
    EXPECT_TRUE(inventory.store[TEST_PATH].contains(TEST_INTERFACE));
    EXPECT_TRUE(
        inventory.store[TEST_PATH][TEST_INTERFACE].contains("FirstProperty"));
    EXPECT_TRUE(std::get<bool>(
        inventory.store[TEST_PATH][TEST_INTERFACE]["FirstProperty"]));

    EXPECT_FALSE(inventory.present.empty());
    EXPECT_TRUE(inventory.present.contains(TEST_PATH));
    EXPECT_TRUE(inventory.present[TEST_PATH]);
}
