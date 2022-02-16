/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */
#include "inventory.hpp"

#include "gtest/gtest.h"

static constexpr auto TEST_PATH = "/path/test";
static constexpr auto TEST_PATH_1 = "/path/test/1";
static constexpr auto TEST_PATH_2 = "/path/test/2";

using namespace inventory;

static void accumulate(std::map<std::string, ObjectType>& store,
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

struct MockInventory : public Inventory
{
    std::weak_ptr<dbus::PropertiesChangedListener>
        addPropertiesChangedListener(
            [[maybe_unused]] const std::string& path,
            [[maybe_unused]] const std::string& interface,
            [[maybe_unused]] std::function<void(dbus::PropertiesChanged&&)>
                callback) override
    {
        throw std::logic_error("Unimplemented");
    }

    void removePropertiesChangedListener(
        [[maybe_unused]] std::weak_ptr<dbus::PropertiesChangedListener>
            listener) override
    {
        throw std::logic_error("Unimplemented");
    }

    void add(const std::string& path, interfaces::Interface iface) override
    {
        ObjectType update;

        iface.populateObject(update);

        accumulate(store, path, update);
    }

    void remove(const std::string& path, interfaces::Interface iface) override
    {
        ObjectType update;

        iface.depopulateObject(update);

        accumulate(store, path, update);
    }

    void markPresent(const std::string& path) override
    {
        present.insert_or_assign(path, true);
    }

    void markAbsent(const std::string& path) override
    {
        present.insert_or_assign(path, false);
    }

    bool isPresent(const std::string& path) override
    {
        return present.at(path);
    }

    bool isModel([[maybe_unused]] const std::string& path,
                         [[maybe_unused]] const std::string& model) override
    {
        throw std::logic_error("Unimplemented");
    }

    std::map<std::string, ObjectType> store;
    std::map<std::string, bool> present;
};

TEST(MockInventory, addI2CDevice)
{
    MockInventory inventory;
    interfaces::I2CDevice i2cdevice{1, 2};

    inventory.add(TEST_PATH, i2cdevice);

    std::map<std::string, ObjectType> expected = {{
        TEST_PATH,
        {{
            INVENTORY_DECORATOR_I2CDEVICE_IFACE,
            {
                {"Bus", static_cast<size_t>(1)},
                {"Address", static_cast<size_t>(2)},
            },
        }},
    }};

    EXPECT_EQ(expected, inventory.store);
}

TEST(MockInventory, updateTwiceDistinctObjects)
{
    MockInventory inventory;

    interfaces::I2CDevice i2cdevice{1, 2};
    inventory.add(TEST_PATH_1, i2cdevice);

    interfaces::VINI vini(std::vector<uint8_t>({3}), std::vector<uint8_t>({4}));
    inventory.add(TEST_PATH_2, vini);

    std::map<std::string, ObjectType> expected = {
        {
            TEST_PATH_1,
            {
                {INVENTORY_DECORATOR_I2CDEVICE_IFACE,
                 {
                     {"Bus", static_cast<size_t>(1)},
                     {"Address", static_cast<size_t>(2)},
                 }},
            },
        },
        {
            TEST_PATH_2,
            {
                {
                    INVENTORY_IPZVPD_VINI_IFACE,
                    {
                        {"RT", std::vector<uint8_t>({'V', 'I', 'N', 'I'})},
                        {"CC", std::vector<uint8_t>({3})},
                        {"SN", std::vector<uint8_t>({4})},
                    },
                },
            },
        },
    };

    EXPECT_EQ(expected, inventory.store);
}

TEST(MockInventory, updateAnObjectTwiceDistinctInterfaces)
{
    MockInventory inventory;

    interfaces::I2CDevice i2cdevice{1, 2};
    inventory.add(TEST_PATH_1, i2cdevice);

    interfaces::VINI vini(std::vector<uint8_t>({3}), std::vector<uint8_t>({4}));
    inventory.add(TEST_PATH_1, vini);

    std::map<std::string, ObjectType> expected = {
        {
            TEST_PATH_1,
            {
                {INVENTORY_DECORATOR_I2CDEVICE_IFACE,
                 {
                     {"Bus", static_cast<size_t>(1)},
                     {"Address", static_cast<size_t>(2)},
                 }},
                {
                    INVENTORY_IPZVPD_VINI_IFACE,
                    {
                        {"RT", std::vector<uint8_t>({'V', 'I', 'N', 'I'})},
                        {"CC", std::vector<uint8_t>({3})},
                        {"SN", std::vector<uint8_t>({4})},
                    },
                },
            },
        },
    };

    EXPECT_EQ(expected, inventory.store);
}

TEST(MockInventory, updateAnObjectTwiceSameProperties)
{
    MockInventory inventory;
    interfaces::I2CDevice i2cdevice1{1, 2};
    inventory.add(TEST_PATH, i2cdevice1);

    interfaces::I2CDevice i2cdevice2{3, 4};
    inventory.add(TEST_PATH, i2cdevice2);

    std::map<std::string, ObjectType> expected = {{
        TEST_PATH,
        {{
            INVENTORY_DECORATOR_I2CDEVICE_IFACE,
            {
                {"Bus", static_cast<size_t>(3)},
                {"Address", static_cast<size_t>(4)},
            },
        }},
    }};

    EXPECT_EQ(expected, inventory.store);
}

TEST(PublishWhenPresent, addInterfacesButNotPresent)
{
    MockInventory inventory;
    PublishWhenPresentInventoryDecorator decorator(&inventory);

    interfaces::I2CDevice i2cdevice{1, 2};

    decorator.add(TEST_PATH, i2cdevice);

    EXPECT_EQ(true, inventory.store.empty());
    EXPECT_EQ(true, inventory.present.empty());
}

TEST(PublishWhenPresent, setPresentWithNoInterfaces)
{
    MockInventory inventory;
    PublishWhenPresentInventoryDecorator decorator(&inventory);

    decorator.markPresent(TEST_PATH);

    EXPECT_EQ(true, inventory.store.empty());
    EXPECT_EQ(true, inventory.present.empty());
}

TEST(PublishWhenPresent, addInterfacesAndSetPresent)
{
    MockInventory inventory;
    PublishWhenPresentInventoryDecorator decorator(&inventory);

    interfaces::I2CDevice i2cdevice{1, 2};

    EXPECT_TRUE(inventory.store.empty());
    EXPECT_TRUE(inventory.present.empty());

    decorator.add(TEST_PATH, i2cdevice);
    decorator.markPresent(TEST_PATH);

    std::map<std::string, ObjectType> expected = {
        {
            TEST_PATH,
            {
                {INVENTORY_DECORATOR_I2CDEVICE_IFACE,
                 {
                     {"Bus", static_cast<size_t>(1)},
                     {"Address", static_cast<size_t>(2)},
                 }},
            },
        },
    };

    EXPECT_EQ(expected, inventory.store);
    EXPECT_EQ(true, inventory.present.at(TEST_PATH));
}

TEST(PublishWhenPresent, removeInterfaceWhenPresent)
{
    MockInventory inventory;
    PublishWhenPresentInventoryDecorator decorator(&inventory);

    interfaces::I2CDevice i2cdevice{1, 2};

    decorator.add(TEST_PATH, i2cdevice);
    decorator.markPresent(TEST_PATH);

    std::map<std::string, ObjectType> expected = {
        {
            TEST_PATH,
            {
                {INVENTORY_DECORATOR_I2CDEVICE_IFACE,
                 {
                     {"Bus", static_cast<size_t>(1)},
                     {"Address", static_cast<size_t>(2)},
                 }},
            },
        },
    };

    EXPECT_EQ(expected, inventory.store);
    EXPECT_EQ(true, inventory.present.at(TEST_PATH));

    decorator.remove(TEST_PATH, i2cdevice);

    EXPECT_EQ(expected, inventory.store);
    EXPECT_EQ(true, inventory.present.at(TEST_PATH));
}

TEST(PublishWhenPresent, markAbsentWithInterfaces)
{
    MockInventory inventory;
    PublishWhenPresentInventoryDecorator decorator(&inventory);

    interfaces::I2CDevice i2cdevice{1, 2};

    decorator.add(TEST_PATH, i2cdevice);
    decorator.markPresent(TEST_PATH);

    std::map<std::string, ObjectType> present = {
        {
            TEST_PATH,
            {
                {INVENTORY_DECORATOR_I2CDEVICE_IFACE,
                 {
                     {"Bus", static_cast<size_t>(1)},
                     {"Address", static_cast<size_t>(2)},
                 }},
            },
        },
    };

    EXPECT_EQ(present, inventory.store);
    EXPECT_EQ(true, inventory.present.at(TEST_PATH));

    decorator.markAbsent(TEST_PATH);

    std::map<std::string, ObjectType> absent = {
        {
            TEST_PATH,
            {
                {INVENTORY_DECORATOR_I2CDEVICE_IFACE,
                 {
                     {"Bus", static_cast<size_t>(INT_MAX)},
                     {"Address", static_cast<size_t>(0)},
                 }},
            },
        },
    };

    EXPECT_EQ(absent, inventory.store);
    EXPECT_EQ(false, inventory.present.at(TEST_PATH));
}

TEST(PublishWhenPresent, removeInterfacesAndSetAbsent)
{
    MockInventory inventory;
    PublishWhenPresentInventoryDecorator decorator(&inventory);

    interfaces::I2CDevice i2cdevice{1, 2};

    decorator.add(TEST_PATH, i2cdevice);
    decorator.markPresent(TEST_PATH);

    std::map<std::string, ObjectType> present = {
        {
            TEST_PATH,
            {
                {INVENTORY_DECORATOR_I2CDEVICE_IFACE,
                 {
                     {"Bus", static_cast<size_t>(1)},
                     {"Address", static_cast<size_t>(2)},
                 }},
            },
        },
    };

    EXPECT_EQ(present, inventory.store);
    EXPECT_TRUE(inventory.present.at(TEST_PATH));

    decorator.remove(TEST_PATH, i2cdevice);
    decorator.markAbsent(TEST_PATH);

    std::map<std::string, ObjectType> absent = {
        {
            TEST_PATH,
            {
                {INVENTORY_DECORATOR_I2CDEVICE_IFACE,
                 {
                     {"Bus", static_cast<size_t>(INT_MAX)},
                     {"Address", static_cast<size_t>(0)},
                 }},
            },
        },
    };

    EXPECT_EQ(absent, inventory.store);
    EXPECT_EQ(false, inventory.present.at(TEST_PATH));
}
