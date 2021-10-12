/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

#include <functional>
#include <optional>
#include <stdexcept>
#include <string>

class Inventory;

template <class T>
class Connector
{
  public:
    template <typename... Args>
    Connector(Args&&... args) :
        device(), ctor([this, args...]() mutable {
            device.emplace(std::forward<Args>(args)...);
        })
    {}
    ~Connector() = default;

    void populate()
    {
        ctor();
    }

    void depopulate()
    {
        device.reset();
    }

    bool isPopulated()
    {
        return device.has_value();
    }

    T& getDevice()
    {
        return device.value();
    }

  private:
    std::optional<T> device;
    std::function<void(void)> ctor;
};

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
