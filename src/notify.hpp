/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */
#pragma once

#include <functional>
#include <map>
#include <stdexcept>

class NotifySink;

class Notifier
{
  public:
    Notifier();
    Notifier(const Notifier& other) = delete;
    Notifier(Notifier&& other) = delete;
    ~Notifier();

    Notifier& operator=(const Notifier& other) = delete;
    Notifier& operator=(Notifier&& other) = delete;

    void add(NotifySink* sink);
    void remove(NotifySink* sink);
    void run();

  private:
    int epollfd;
    int exitfd;
};

class NotifySink
{
  public:
    virtual void arm() {}

    virtual int getFD() = 0;
    virtual void notify(Notifier& notifier) = 0;

    virtual void disarm() {}
};
