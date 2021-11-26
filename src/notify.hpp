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
    ~Notifier();

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
    virtual ~NotifySink() = default;

    virtual void arm()
    {}

    virtual int getFD() = 0;
    virtual void notify(Notifier& notifier) = 0;

    virtual void disarm()
    {}
};
