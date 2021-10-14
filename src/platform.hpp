/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

#include <stdexcept>
#include <string>

class Inventory;

class FRU
{
  public:
    FRU() = default;
    virtual ~FRU() = default;

    virtual std::string getInventoryPath() const = 0;
    virtual void addToInventory([[maybe_unused]] Inventory& inventory) = 0;
    virtual void removeFromInventory(Inventory& inventory) = 0;
};

class Device
{
  public:
    enum
    {
        UNPLUG_RETAINS_INVENTORY = 1,
        UNPLUG_REMOVES_INVENTORY,
    };

    virtual ~Device() = default;

    virtual void plug() = 0;
    virtual void unplug(int mode = UNPLUG_REMOVES_INVENTORY) = 0;
};

class Platform
{
  public:
    static std::string getModel();
    static bool isSupported();
};
