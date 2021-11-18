/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

#include <functional>
#include <map>
#include <stdexcept>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/io.hpp>

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
    sdeventplus::Event sdEvent;
    std::map<int, std::unique_ptr<sdeventplus::source::IO>> sources;

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
