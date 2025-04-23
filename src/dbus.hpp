/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */
#pragma once

#include "notify.hpp"

#include <memory>

/* Forward-declarations for minor dependencies */
namespace sdbusplus
{
namespace bus
{
namespace match
{
// NOLINTNEXTLINE(readability-identifier-naming)
struct match;
} // namespace match
// NOLINTNEXTLINE(readability-identifier-naming)
struct bus;
} // namespace bus
namespace message
{
// NOLINTNEXTLINE(readability-identifier-naming)
class message;
} // namespace message
} // namespace sdbusplus

class DBusNotifySink : public NotifySink
{
  public:
    explicit DBusNotifySink(sdbusplus::bus::bus& dbus);
    DBusNotifySink() = delete;
    DBusNotifySink(const DBusNotifySink& other) = delete;
    DBusNotifySink(DBusNotifySink&& other) = delete;
    virtual ~DBusNotifySink() = default;

    DBusNotifySink& operator=(const DBusNotifySink& other) = delete;
    DBusNotifySink& operator=(DBusNotifySink&& other) = delete;

    /* NotifySink */
    int getFD() override;
    void notify(Notifier& notifier) override;

  private:
    sdbusplus::bus::bus& dbus;
};

namespace dbus
{
class PropertiesChanged
{
  public:
    explicit PropertiesChanged(sdbusplus::message::message& message) :
        message(message)
    {}
    PropertiesChanged() = delete;
    PropertiesChanged(const PropertiesChanged& other) = delete;
    PropertiesChanged(PropertiesChanged&& other) = delete;
    ~PropertiesChanged() = default;

    PropertiesChanged& operator=(const PropertiesChanged& other) = delete;
    PropertiesChanged& operator=(PropertiesChanged&& other) = delete;

    template <typename... Args>
    void read(Args&&... args);

  private:
    sdbusplus::message::message& message;
};

class PropertiesChangedListener;

std::shared_ptr<PropertiesChangedListener> sharedPropertiesChangedListener(
    sdbusplus::bus::bus& bus, const std::string& path,
    const std::string& interface,
    const std::function<void(PropertiesChanged&&)>& callback);
} // namespace dbus
