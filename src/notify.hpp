/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

#include <functional>
#include <map>
#include <stdexcept>

class NotifySink;

class Notifier
{
  public:
    Notifier();
    ~Notifier();

    void add(NotifySink* sink);
    void remove(NotifySink* sink);
    void run();

  private:
    int epollfd;
};

class NotifySink
{
  public:
    virtual ~NotifySink() = default;

    virtual void arm() = 0;
    virtual int getFD() = 0;
    virtual void notify(Notifier& notifier) = 0;
    virtual void disarm() = 0;
};
