/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */

#include "dbus.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>

DBusNotifySink::DBusNotifySink(sdbusplus::bus::bus& dbus) : dbus(dbus)
{}

int DBusNotifySink::getFD()
{
    return dbus.get_fd();
}

void DBusNotifySink::notify([[maybe_unused]] Notifier& notifier)
{
    dbus.process_discard();
}

namespace dbus
{
template <typename... Args>
void PropertiesChanged::read(Args&&... args)
{
    message.read(args...);
}

class PropertiesChangedListener
{
  public:
    template <typename... Args>
    PropertiesChangedListener(Args&&... args) : match(args...)
    {}
    ~PropertiesChangedListener() = default;

  private:
    sdbusplus::bus::match::match match;
};

std::shared_ptr<PropertiesChangedListener> sharedPropertiesChangedListener(
    sdbusplus::bus::bus& dbus, const std::string& path,
    const std::string& interface,
    std::function<void(PropertiesChanged&&)> callback)
{
    auto wrapped = [callback](sdbusplus::message::message& msg) {
        callback(PropertiesChanged(msg));
    };
    auto pcm = sdbusplus::bus::match::rules::propertiesChangedNamespace(
        path, interface);
    return std::make_shared<PropertiesChangedListener>(dbus, pcm, wrapped);
}
} // namespace dbus
