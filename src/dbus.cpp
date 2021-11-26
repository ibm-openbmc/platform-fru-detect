/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */

#include "dbus.hpp"

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
