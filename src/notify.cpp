/* SPDX-License-Identifier: Apache-2.0 */

#include "notify.hpp"

#include <unistd.h>

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/source/signal.hpp>
#include <stdplus/signal.hpp>

#include <cerrno>
#include <cstring>
#include <system_error>

extern "C"
{
#include <signal.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
};

PHOSPHOR_LOG2_USING;

Notifier::Notifier() : sdEvent(sdeventplus::Event::get_default())
{}

Notifier::~Notifier()
{}

void Notifier::add(NotifySink* sink)
{
    sink->arm();

    auto eventSource = std::make_unique<sdeventplus::source::IO>(
        sdEvent, sink->getFD(), EPOLLIN,
        [sink, this](sdeventplus::source::IO& /*io*/, int /*fd*/,
                     uint32_t revents) {
            if (!(revents & EPOLLIN))
            {
                return;
            }
            sink->notify(*this);
        });

    if (sources.contains(sink->getFD()))
    {
        error("{FD} already in sources map!", "FD", sink->getFD());
        throw std::runtime_error("Bad FD");
    }

    sources.emplace(sink->getFD(), std::move(eventSource));

    debug("Added event descriptor {EVENT_FD} to Notifier", "EVENT_FD",
          sink->getFD());
}

void Notifier::remove(NotifySink* sink)
{
    int fd = sink->getFD();
    if (fd == -1)
    {
        debug("Skipping disarmed sink");
        return;
    }

    auto source = sources.find(fd);
    if (source != sources.end())
    {
        sources.erase(source);

        debug("Removed event descriptor {EVENT_FD} from event source",
              "EVENT_FD", sink->getFD());

        sink->disarm();
    }
}

void Notifier::run()
{
    // Also hook up D-Bus to the event loop
    auto bus = sdbusplus::bus::new_default();
    bus.attach_event(sdEvent.get(), SD_EVENT_PRIORITY_NORMAL);

    stdplus::signal::block(SIGINT);
    sdeventplus::source::Signal sigint(sdEvent, SIGINT,
                                       [](sdeventplus::source::Signal& source,
                                          const struct signalfd_siginfo*) {
                                           debug("SIGINT received");
                                           source.get_event().exit(0);
                                       });

    stdplus::signal::block(SIGQUIT);
    sdeventplus::source::Signal sigquit(sdEvent, SIGQUIT,
                                        [](sdeventplus::source::Signal& source,
                                           const struct signalfd_siginfo*) {
                                            debug("SIGQUIT received");
                                            source.get_event().exit(0);
                                        });
    sdEvent.loop();
    info("Exiting notify event loop");
}
