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
struct match;
}
struct bus;
} // namespace bus
namespace message
{
class message;
}
} // namespace sdbusplus

class DBusNotifySink : public NotifySink
{
  public:
    explicit DBusNotifySink(sdbusplus::bus::bus& dbus);
    DBusNotifySink() = delete;
    ~DBusNotifySink() override = default;

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
    ~PropertiesChanged() = default;

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
