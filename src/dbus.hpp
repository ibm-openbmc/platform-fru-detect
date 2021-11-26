/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */
#pragma once

#include "notify.hpp"

#include <sdbusplus/bus.hpp>

class DBusNotifySink : public NotifySink
{
  public:
    explicit DBusNotifySink(sdbusplus::bus::bus& dbus);
    DBusNotifySink() = delete;
    ~DBusNotifySink() = default;

    /* NotifySink */
    virtual int getFD() override;
    virtual void notify(Notifier& notifier) override;

  private:
    sdbusplus::bus::bus& dbus;
};
