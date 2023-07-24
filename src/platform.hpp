/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */
#pragma once

#include "notify.hpp"
#include "sysfs/i2c.hpp"

#include <gpiod.hpp>
#include <phosphor-logging/lg2.hpp>

#include <cassert>
#include <cerrno>
#include <cstring>
#include <functional>
#include <map>
#include <optional>
#include <stdexcept>
#include <type_traits>

extern "C"
{
#include <sys/timerfd.h>
}

class Inventory;

class Device
{
  public:
    enum
    {
        UNPLUG_RETAINS_INVENTORY = 1,
        UNPLUG_REMOVES_INVENTORY,
    };

    virtual void plug(Notifier& notifier) = 0;
    virtual void unplug(Notifier& notifier,
                        int mode = UNPLUG_REMOVES_INVENTORY) = 0;
};

template <typename T>
concept DerivesDevice = std::is_base_of<Device, T>::value;

template <DerivesDevice T>
class Connector
{
  public:
    template <typename... DeviceArgs>
    explicit Connector(int idx, DeviceArgs&&... args) :
        state(CONNECTOR_UNINITIALISED), idx(idx), device(),
        ctor([this, args...]() mutable {
            device.emplace(std::forward<DeviceArgs>(args)...);
        })
    {}
    Connector(const Connector<T>& other) = delete;
    Connector(Connector<T>&& other) = delete;
    ~Connector() = default;

    Connector& operator=(const Connector<T>& other) = delete;
    Connector& operator=(Connector<T>&& other) = delete;

    void populate(Notifier& notifier)
    {
        switch (state)
        {
            case CONNECTOR_UNINITIALISED:
            case CONNECTOR_DEPOPULATED:
                lg2::debug("Populating connector");
                ctor();
                device->plug(notifier);
                state = CONNECTOR_POPULATED;
                return;
            case CONNECTOR_POPULATED:
                return;
        }
    }

    void depopulate(Notifier& notifier, int mode = T::UNPLUG_REMOVES_INVENTORY)
    {
        switch (state)
        {
            case CONNECTOR_DEPOPULATED:
                return;
            case CONNECTOR_UNINITIALISED:
                // Construct the device so we can explicitly unplug() it. This
                // is used to e.g. remove the device from the inventory when it
                // has been removed from the system while AC power is unplugged.
                ctor();
                [[fallthrough]];
            case CONNECTOR_POPULATED:
                lg2::debug("Depopulating connector");
                device->unplug(notifier, mode);
                device.reset();
                state = CONNECTOR_DEPOPULATED;
                return;
        }
    }

    int index() const
    {
        return idx;
    }

  private:
    enum ConnectorState
    {
        CONNECTOR_UNINITIALISED,
        CONNECTOR_DEPOPULATED,
        CONNECTOR_POPULATED
    };

    ConnectorState state;
    const int idx;
    std::optional<T> device;
    std::function<void(void)> ctor;
};

class FRU
{
  public:
    virtual std::string getInventoryPath() const = 0;
    virtual void addToInventory(Inventory* inventory) = 0;
    virtual void removeFromInventory(Inventory* inventory) = 0;
};

/*
 * TODO: Add multiple instances to a single timerfd to make it more efficient
 *
 * Haven't done this yet as I already have enough to think about to get it all
 * to work.
 */
template <DerivesDevice T>
class PolledDevicePresence : public NotifySink
{
  public:
    PolledDevicePresence() = delete;
    PolledDevicePresence(Connector<T>* connector,
                         const std::function<bool()>& poll) :
        connector(connector),
        poll(poll), timerfd(-1)
    {}
    PolledDevicePresence(const PolledDevicePresence<T>& other) = default;
    PolledDevicePresence(PolledDevicePresence<T>&& other) noexcept = default;
    virtual ~PolledDevicePresence() = default;

    PolledDevicePresence<T>&
        operator=(const PolledDevicePresence<T>& other) = default;
    PolledDevicePresence<T>&
        operator=(PolledDevicePresence<T>&& other) noexcept = default;

    /* NotifySink */
    void arm() override
    {
        assert(timerfd == -1 && "Bad state: timer already armed");

        timerfd = ::timerfd_create(CLOCK_MONOTONIC, 0);
        if (timerfd == -1)
        {
            lg2::error("Failed to create timerfd: {ERRNO_DESCRIPTION}",
                       "ERRNO_DESCRIPTION", ::strerror(errno), "ERRNO", errno);
            std::system_category().default_error_condition(errno);
        }

        struct itimerspec interval
        {
            {1, 0},
            {
                1, 0
            }
        };

        int rc = ::timerfd_settime(timerfd, 0, &interval, nullptr);
        if (rc == -1)
        {
            close(timerfd);

            lg2::error(
                "Failed to set interval for timerfd: {ERRNO_DESCRIPTION}",
                "ERRNO_DESCRIPTION", ::strerror(errno), "ERRNO", errno);
            std::system_category().default_error_condition(errno);
        }
    }

    int getFD() override
    {
        return timerfd;
    }

    void notify(Notifier& notifier) override
    {
        drain();

        if (poll())
        {
            try
            {
                connector->populate(notifier);
            }
            catch (const SysfsI2CDeviceDriverBindException& ex)
            {
                lg2::error(
                    "Failed to bind driver for device reporting present, disabling notifier: {EXCEPTION}",
                    "EXCEPTION", ex);
                notifier.remove(this);
            }
        }
        else
        {
            connector->depopulate(notifier);
        }
    }

    void disarm() override
    {
        assert(timer > -1 && "Bad state: Timer already disarmed");
        ::close(timerfd);
        timerfd = -1;
    }

  private:
    uint64_t drain()
    {
        uint64_t res = 0;

        ssize_t rc = ::read(this->timerfd, &res, sizeof(res));
        if (rc != sizeof(res))
        {
            if (rc == -1)
            {
                lg2::error(
                    "Failed to read from timerfd {TIMER_FD}: {ERRNO_DESCRIPTION}",
                    "TIMER_FD", this->timerfd, "ERRNO_DESCRIPTION",
                    ::strerror(errno), "ERRNO", errno);
                std::system_category().default_error_condition(errno);
            }
            else
            {
                lg2::warning(
                    "Short read from timerfd {TIMER_FD}: {READ_LENGTH}",
                    "TIMER_FD", this->timerfd, "READ_LENGTH", rc);
            }
        }

        return res;
    }

    Connector<T>* connector;
    std::function<bool()> poll;
    int timerfd;
};

template <DerivesDevice T>
class PolledConnector
{
  public:
    PolledConnector() = delete;
    template <typename... DeviceArgs>
    explicit PolledConnector(int index, DeviceArgs&&... args) :
        connector(index, args...)
    {}
    PolledConnector(const PolledConnector& other) = delete;
    PolledConnector(PolledConnector&& other) = delete;
    ~PolledConnector() = default;

    PolledConnector& operator=(const PolledConnector& other) = delete;
    PolledConnector& operator=(PolledConnector&& other) = delete;

    void start(Notifier& notifier, std::function<bool()>&& probe)
    {
        poller.emplace(&connector, probe);
        notifier.add(&poller.value());
    }

    void stop(Notifier& notifier, int mode)
    {
        if (poller)
        {
            notifier.remove(&poller.value());
            poller.reset();
        }
        connector.depopulate(notifier, mode);
    }

    int index() const
    {
        return connector.index();
    }

  private:
    Connector<T> connector;
    std::optional<PolledDevicePresence<T>> poller;
};

class Platform;

class PlatformManager
{
  public:
    PlatformManager();
    PlatformManager(const PlatformManager& other) = delete;
    PlatformManager(PlatformManager&& other) = delete;
    ~PlatformManager() = default;

    PlatformManager& operator=(const PlatformManager& other) = delete;
    PlatformManager& operator=(PlatformManager&& other) = delete;

    const std::string& getPlatformModel() noexcept;
    void enrollPlatform(const std::string& model, Platform* platform);
    bool isSupportedPlatform() noexcept;
    void detectPlatformFrus(Notifier& notifier, Inventory*);

  private:
    std::map<std::string, Platform*> platforms;
    std::string model;
};

class Platform
{
  public:
    virtual void enrollWith(PlatformManager& pm) = 0;
    virtual void detectFrus(Notifier& notifier, Inventory* inventory) = 0;
};
