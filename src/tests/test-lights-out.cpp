/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2022 */
#include "devices/nvme.hpp"
#include "inventory.hpp"
#include "mock-inventory.hpp"
#include "platform.hpp"

#include <vector>

#include "gtest/gtest.h"

using namespace inventory;

class MockNVMeDrive : public NVMeDrive, public FRU, public Device
{
  public:
    MockNVMeDrive(Inventory* inventory, int index) :
        NVMeDrive(), inventory(inventory), basic(0, eepromAddress),
        vini(std::vector<uint8_t>({'N', 'V', 'M', 'e'}),
             std::vector<uint8_t>({'A', 'B', 'C', 'D'})),
        index(index)
    {}

    static std::string getInventoryPathFor(int index)
    {
        return "/system/chassis/motherboard/disk_backplane0/nvme" +
               std::to_string(index);
    }

    std::string getInventoryPath() const override
    {
        return getInventoryPathFor(index);
    }

    void plug([[maybe_unused]] Notifier& notifier) override
    {
        addToInventory(inventory);
    }

    void unplug([[maybe_unused]] Notifier& notifer,
                [[maybe_unused]] int mode = UNPLUG_REMOVES_INVENTORY) override
    {
        removeFromInventory(inventory);
    }

  protected:
    Inventory* inventory;
    const inventory::interfaces::I2CDevice basic;
    const inventory::interfaces::VINI vini;
    int index;
};

class MockWilliwakasNVMeDrive : public MockNVMeDrive
{
  public:
    MockWilliwakasNVMeDrive(Inventory* inventory, int index) :
        MockNVMeDrive(inventory, index)
    {}
    MockWilliwakasNVMeDrive(const MockWilliwakasNVMeDrive& other) = delete;
    MockWilliwakasNVMeDrive(MockWilliwakasNVMeDrive&& other) = delete;
    virtual ~MockWilliwakasNVMeDrive() = default;

    MockWilliwakasNVMeDrive&
        operator=(const MockWilliwakasNVMeDrive& other) = delete;
    MockWilliwakasNVMeDrive&
        operator=(MockWilliwakasNVMeDrive&& other) = delete;

    void addToInventory(Inventory* inventory) override
    {
        std::string path = getInventoryPath();
        inventory->markPresent(path);
    }

    void removeFromInventory(Inventory* inventory) override
    {
        std::string path = getInventoryPath();
        inventory->markAbsent(path);
    }
};

class MockFlettNVMeDrive : public MockNVMeDrive
{
  public:
    MockFlettNVMeDrive(Inventory* inventory, int index) :
        MockNVMeDrive(inventory, index)
    {}
    MockFlettNVMeDrive(const MockFlettNVMeDrive& other) = delete;
    MockFlettNVMeDrive(MockFlettNVMeDrive&& other) = delete;
    virtual ~MockFlettNVMeDrive() = default;

    MockFlettNVMeDrive& operator=(const MockFlettNVMeDrive& other) = delete;
    MockFlettNVMeDrive& operator=(MockFlettNVMeDrive&& other) = delete;

    void addToInventory(Inventory* inventory) override
    {
        std::string path = getInventoryPath();
        inventory->add(path, basic);
        inventory->add(path, vini);
    }

    void removeFromInventory(Inventory* inventory) override
    {
        std::string path = getInventoryPath();
        inventory->remove(path, basic);
        inventory->remove(path, vini);
    }
};

TEST(LightsOut, unpopulatedInventoryDepopulatedPCIeDepopulateI2C)
{
    constexpr int driveIndex = 0;
    Notifier notifier;
    MockInventory inventory{};
    PublishWhenPresentInventoryDecorator decorator(&inventory);
    Connector<MockWilliwakasNVMeDrive> pcieConnector(0, &decorator, driveIndex);
    Connector<MockFlettNVMeDrive> i2cConnector(0, &decorator, driveIndex);

    pcieConnector.depopulate(notifier,
                             MockWilliwakasNVMeDrive::UNPLUG_REMOVES_INVENTORY);
    i2cConnector.depopulate(notifier,
                            MockFlettNVMeDrive::UNPLUG_REMOVES_INVENTORY);

    std::string path = MockNVMeDrive::getInventoryPathFor(driveIndex);
    std::map<std::string, ObjectType> expected = {
        {
            path,
            {
                {
                    INVENTORY_DECORATOR_I2CDEVICE_IFACE,
                    {
                        {"Bus", static_cast<size_t>(INT_MAX)},
                        {"Address", static_cast<size_t>(0)},
                    },
                },
                {
                    INVENTORY_IPZVPD_VINI_IFACE,
                    {
                        {"RT", std::vector<uint8_t>({'V', 'I', 'N', 'I'})},
                        {"CC", std::vector<uint8_t>(0)},
                        {"SN", std::vector<uint8_t>(0)},
                    },
                },
            },
        },
    };

    EXPECT_EQ(expected, inventory.store);
}

TEST(LightsOut, unpopulatedInventoryPopulatePCIePopulateI2C)
{
    constexpr int driveIndex = 0;
    Notifier notifier;
    MockInventory inventory{};
    PublishWhenPresentInventoryDecorator decorator(&inventory);
    Connector<MockWilliwakasNVMeDrive> pcieConnector(0, &decorator, driveIndex);
    Connector<MockFlettNVMeDrive> i2cConnector(0, &decorator, driveIndex);

    pcieConnector.populate(notifier);
    i2cConnector.populate(notifier);

    std::string path = MockNVMeDrive::getInventoryPathFor(driveIndex);
    std::map<std::string, ObjectType> expected = {
        {
            path,
            {
                {
                    INVENTORY_DECORATOR_I2CDEVICE_IFACE,
                    {
                        {"Bus", static_cast<size_t>(0)},
                        {"Address", static_cast<size_t>(0x53)},
                    },
                },
                {
                    INVENTORY_IPZVPD_VINI_IFACE,
                    {
                        {"RT", std::vector<uint8_t>({'V', 'I', 'N', 'I'})},
                        {"CC", std::vector<uint8_t>({'N', 'V', 'M', 'e'})},
                        {"SN", std::vector<uint8_t>({'A', 'B', 'C', 'D'})},
                    },
                },
            },
        },
    };

    EXPECT_EQ(expected, inventory.store);
}

TEST(LightsOut, populatedInventoryDepopulatePCIeDepopulateI2C)
{
    constexpr int driveIndex = 0;
    Notifier notifier;
    MockInventory inventory{};
    PublishWhenPresentInventoryDecorator decorator(&inventory);
    Connector<MockWilliwakasNVMeDrive> pcieConnector(0, &decorator, driveIndex);
    Connector<MockFlettNVMeDrive> i2cConnector(0, &decorator, driveIndex);
    std::string path = MockNVMeDrive::getInventoryPathFor(driveIndex);

    pcieConnector.populate(notifier);
    i2cConnector.populate(notifier);

    std::map<std::string, ObjectType> expectedPopulated = {
        {
            path,
            {
                {
                    INVENTORY_DECORATOR_I2CDEVICE_IFACE,
                    {
                        {"Bus", static_cast<size_t>(0)},
                        {"Address", static_cast<size_t>(0x53)},
                    },
                },
                {
                    INVENTORY_IPZVPD_VINI_IFACE,
                    {
                        {"RT", std::vector<uint8_t>({'V', 'I', 'N', 'I'})},
                        {"CC", std::vector<uint8_t>({'N', 'V', 'M', 'e'})},
                        {"SN", std::vector<uint8_t>({'A', 'B', 'C', 'D'})},
                    },
                },
            },
        },
    };

    EXPECT_EQ(expectedPopulated, inventory.store);

    pcieConnector.depopulate(notifier,
                             MockWilliwakasNVMeDrive::UNPLUG_REMOVES_INVENTORY);
    i2cConnector.depopulate(notifier,
                            MockFlettNVMeDrive::UNPLUG_REMOVES_INVENTORY);

    std::map<std::string, ObjectType> expectedUnpopulated = {
        {
            path,
            {
                {
                    INVENTORY_DECORATOR_I2CDEVICE_IFACE,
                    {
                        {"Bus", static_cast<size_t>(INT_MAX)},
                        {"Address", static_cast<size_t>(0)},
                    },
                },
                {
                    INVENTORY_IPZVPD_VINI_IFACE,
                    {
                        {"RT", std::vector<uint8_t>({'V', 'I', 'N', 'I'})},
                        {"CC", std::vector<uint8_t>(0)},
                        {"SN", std::vector<uint8_t>(0)},
                    },
                },
            },
        },
    };

    EXPECT_EQ(expectedUnpopulated, inventory.store);
}

TEST(LightsOut, discoverRemovalDepopulatePCIeDepopulateI2C)
{
    constexpr int driveIndex = 0;
    Notifier notifier;
    MockInventory inventory{};
    MockWilliwakasNVMeDrive pciePrimer(&inventory, driveIndex);
    MockFlettNVMeDrive i2cPrimer(&inventory, driveIndex);
    PublishWhenPresentInventoryDecorator decorator(&inventory);
    Connector<MockWilliwakasNVMeDrive> pcieConnector(0, &decorator, driveIndex);
    Connector<MockFlettNVMeDrive> i2cConnector(0, &decorator, driveIndex);
    std::string path = MockNVMeDrive::getInventoryPathFor(driveIndex);

    // Hide the addition of the devices to the inventory from
    // PublishWhenPresentInventoryDecorator.
    pciePrimer.addToInventory(&inventory);
    i2cPrimer.addToInventory(&inventory);

    std::map<std::string, ObjectType> expectedPopulated = {
        {
            path,
            {
                {
                    INVENTORY_DECORATOR_I2CDEVICE_IFACE,
                    {
                        {"Bus", static_cast<size_t>(0)},
                        {"Address", static_cast<size_t>(0x53)},
                    },
                },
                {
                    INVENTORY_IPZVPD_VINI_IFACE,
                    {
                        {"RT", std::vector<uint8_t>({'V', 'I', 'N', 'I'})},
                        {"CC", std::vector<uint8_t>({'N', 'V', 'M', 'e'})},
                        {"SN", std::vector<uint8_t>({'A', 'B', 'C', 'D'})},
                    },
                },
            },
        },
    };

    EXPECT_EQ(expectedPopulated, inventory.store);

    // Now remove the devices from the inventory through
    // PublishWhenPresentInventoryDecorator
    pcieConnector.depopulate(notifier,
                             MockWilliwakasNVMeDrive::UNPLUG_REMOVES_INVENTORY);
    i2cConnector.depopulate(notifier,
                            MockFlettNVMeDrive::UNPLUG_REMOVES_INVENTORY);

    std::map<std::string, ObjectType> expected = {
        {
            path,
            {
                {
                    INVENTORY_DECORATOR_I2CDEVICE_IFACE,
                    {
                        {"Bus", static_cast<size_t>(INT_MAX)},
                        {"Address", static_cast<size_t>(0)},
                    },
                },
                {
                    INVENTORY_IPZVPD_VINI_IFACE,
                    {
                        {"RT", std::vector<uint8_t>({'V', 'I', 'N', 'I'})},
                        {"CC", std::vector<uint8_t>(0)},
                        {"SN", std::vector<uint8_t>(0)},
                    },
                },
            },
        },
    };

    EXPECT_EQ(expected, inventory.store);
}

TEST(LightsOut, unpopulatedInventoryDepopulatedI2CDepopulatePCIe)
{
    constexpr int driveIndex = 0;
    Notifier notifier;
    MockInventory inventory{};
    PublishWhenPresentInventoryDecorator decorator(&inventory);
    Connector<MockWilliwakasNVMeDrive> pcieConnector(0, &decorator, driveIndex);
    Connector<MockFlettNVMeDrive> i2cConnector(0, &decorator, driveIndex);

    i2cConnector.depopulate(notifier,
                            MockFlettNVMeDrive::UNPLUG_REMOVES_INVENTORY);
    pcieConnector.depopulate(notifier,
                             MockWilliwakasNVMeDrive::UNPLUG_REMOVES_INVENTORY);

    std::string path = MockNVMeDrive::getInventoryPathFor(driveIndex);
    std::map<std::string, ObjectType> expected = {
        {
            path,
            {
                {
                    INVENTORY_DECORATOR_I2CDEVICE_IFACE,
                    {
                        {"Bus", static_cast<size_t>(INT_MAX)},
                        {"Address", static_cast<size_t>(0)},
                    },
                },
                {
                    INVENTORY_IPZVPD_VINI_IFACE,
                    {
                        {"RT", std::vector<uint8_t>({'V', 'I', 'N', 'I'})},
                        {"CC", std::vector<uint8_t>(0)},
                        {"SN", std::vector<uint8_t>(0)},
                    },
                },
            },
        },
    };

    EXPECT_EQ(expected, inventory.store);
}

TEST(LightsOut, unpopulatedInventoryPopulateI2CPopulatePCIe)
{
    constexpr int driveIndex = 0;
    Notifier notifier;
    MockInventory inventory{};
    PublishWhenPresentInventoryDecorator decorator(&inventory);
    Connector<MockWilliwakasNVMeDrive> pcieConnector(0, &decorator, driveIndex);
    Connector<MockFlettNVMeDrive> i2cConnector(0, &decorator, driveIndex);

    i2cConnector.populate(notifier);
    pcieConnector.populate(notifier);

    std::string path = MockNVMeDrive::getInventoryPathFor(driveIndex);
    std::map<std::string, ObjectType> expected = {
        {
            path,
            {
                {
                    INVENTORY_DECORATOR_I2CDEVICE_IFACE,
                    {
                        {"Bus", static_cast<size_t>(0)},
                        {"Address", static_cast<size_t>(0x53)},
                    },
                },
                {
                    INVENTORY_IPZVPD_VINI_IFACE,
                    {
                        {"RT", std::vector<uint8_t>({'V', 'I', 'N', 'I'})},
                        {"CC", std::vector<uint8_t>({'N', 'V', 'M', 'e'})},
                        {"SN", std::vector<uint8_t>({'A', 'B', 'C', 'D'})},
                    },
                },
            },
        },
    };

    EXPECT_EQ(expected, inventory.store);
}

TEST(LightsOut, populatedInventoryDepopulateI2CDepopulatePCIe)
{
    constexpr int driveIndex = 0;
    Notifier notifier;
    MockInventory inventory{};
    PublishWhenPresentInventoryDecorator decorator(&inventory);
    Connector<MockWilliwakasNVMeDrive> pcieConnector(0, &decorator, driveIndex);
    Connector<MockFlettNVMeDrive> i2cConnector(0, &decorator, driveIndex);
    std::string path = MockNVMeDrive::getInventoryPathFor(driveIndex);

    i2cConnector.populate(notifier);
    pcieConnector.populate(notifier);

    std::map<std::string, ObjectType> expectedPopulated = {
        {
            path,
            {
                {
                    INVENTORY_DECORATOR_I2CDEVICE_IFACE,
                    {
                        {"Bus", static_cast<size_t>(0)},
                        {"Address", static_cast<size_t>(0x53)},
                    },
                },
                {
                    INVENTORY_IPZVPD_VINI_IFACE,
                    {
                        {"RT", std::vector<uint8_t>({'V', 'I', 'N', 'I'})},
                        {"CC", std::vector<uint8_t>({'N', 'V', 'M', 'e'})},
                        {"SN", std::vector<uint8_t>({'A', 'B', 'C', 'D'})},
                    },
                },
            },
        },
    };

    EXPECT_EQ(expectedPopulated, inventory.store);

    i2cConnector.depopulate(notifier,
                            MockFlettNVMeDrive::UNPLUG_REMOVES_INVENTORY);
    pcieConnector.depopulate(notifier,
                             MockWilliwakasNVMeDrive::UNPLUG_REMOVES_INVENTORY);

    std::map<std::string, ObjectType> expectedUnpopulated = {
        {
            path,
            {
                {
                    INVENTORY_DECORATOR_I2CDEVICE_IFACE,
                    {
                        {"Bus", static_cast<size_t>(INT_MAX)},
                        {"Address", static_cast<size_t>(0)},
                    },
                },
                {
                    INVENTORY_IPZVPD_VINI_IFACE,
                    {
                        {"RT", std::vector<uint8_t>({'V', 'I', 'N', 'I'})},
                        {"CC", std::vector<uint8_t>(0)},
                        {"SN", std::vector<uint8_t>(0)},
                    },
                },
            },
        },
    };

    EXPECT_EQ(expectedUnpopulated, inventory.store);
}

TEST(LightsOut, discoverRemovalDepopulateI2CDepopulatePCIe)
{
    constexpr int driveIndex = 0;
    Notifier notifier;
    MockInventory inventory{};
    MockWilliwakasNVMeDrive pciePrimer(&inventory, driveIndex);
    MockFlettNVMeDrive i2cPrimer(&inventory, driveIndex);
    PublishWhenPresentInventoryDecorator decorator(&inventory);
    Connector<MockWilliwakasNVMeDrive> pcieConnector(0, &decorator, driveIndex);
    Connector<MockFlettNVMeDrive> i2cConnector(0, &decorator, driveIndex);
    std::string path = MockNVMeDrive::getInventoryPathFor(driveIndex);

    // Hide the addition of the devices to the inventory from
    // PublishWhenPresentInventoryDecorator.
    i2cPrimer.addToInventory(&inventory);
    pciePrimer.addToInventory(&inventory);

    std::map<std::string, ObjectType> expectedPopulated = {
        {
            path,
            {
                {
                    INVENTORY_DECORATOR_I2CDEVICE_IFACE,
                    {
                        {"Bus", static_cast<size_t>(0)},
                        {"Address", static_cast<size_t>(0x53)},
                    },
                },
                {
                    INVENTORY_IPZVPD_VINI_IFACE,
                    {
                        {"RT", std::vector<uint8_t>({'V', 'I', 'N', 'I'})},
                        {"CC", std::vector<uint8_t>({'N', 'V', 'M', 'e'})},
                        {"SN", std::vector<uint8_t>({'A', 'B', 'C', 'D'})},
                    },
                },
            },
        },
    };

    EXPECT_EQ(expectedPopulated, inventory.store);

    // Now remove the devices from the inventory through
    // PublishWhenPresentInventoryDecorator
    i2cConnector.depopulate(notifier,
                            MockFlettNVMeDrive::UNPLUG_REMOVES_INVENTORY);
    pcieConnector.depopulate(notifier,
                             MockWilliwakasNVMeDrive::UNPLUG_REMOVES_INVENTORY);

    std::map<std::string, ObjectType> expected = {
        {
            path,
            {
                {
                    INVENTORY_DECORATOR_I2CDEVICE_IFACE,
                    {
                        {"Bus", static_cast<size_t>(INT_MAX)},
                        {"Address", static_cast<size_t>(0)},
                    },
                },
                {
                    INVENTORY_IPZVPD_VINI_IFACE,
                    {
                        {"RT", std::vector<uint8_t>({'V', 'I', 'N', 'I'})},
                        {"CC", std::vector<uint8_t>(0)},
                        {"SN", std::vector<uint8_t>(0)},
                    },
                },
            },
        },
    };

    EXPECT_EQ(expected, inventory.store);
}
