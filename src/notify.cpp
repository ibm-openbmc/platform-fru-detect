/* SPDX-License-Identifier: Apache-2.0 */

#include "notify.hpp"

#include "sysfs/i2c.hpp"

#include <unistd.h>

#include <phosphor-logging/lg2.hpp>

#include <cerrno>
#include <cstring>
#include <system_error>

extern "C"
{
#include <sys/epoll.h>
};

PHOSPHOR_LOG2_USING;

Notifier::Notifier()
{
    epollfd = ::epoll_create1(0);
    if (epollfd < 0)
    {
        error("epoll create operation failed: {ERRNO_DESCRIPTION}",
              "ERRNO_DESCRIPTION", ::strerror(errno), "ERRNO", errno);
        throw std::system_category().default_error_condition(errno);
    }
}

Notifier::~Notifier()
{
    ::close(epollfd);
}

void Notifier::add(NotifySink* sink)
{
    sink->arm();

    // Initialises ptr of the epoll_data union as it's the first element
    struct epoll_event event
    {
        EPOLLIN | EPOLLPRI, sink
    };
    int rc = ::epoll_ctl(epollfd, EPOLL_CTL_ADD, sink->getFD(), &event);
    if (rc < 0)
    {
        error(
            "epoll add operation failed on epoll descriptor {EPOLL_FD} for event descriptor {EVENT_FD}: {ERRNO_DESCRIPTION}",
            "EPOLL_FD", epollfd, "EVENT_FD", sink->getFD(), "ERRNO_DESCRIPTION",
            ::strerror(errno), "ERRNO", errno);
        throw std::system_category().default_error_condition(errno);
    }

    debug("Added event descriptor {EVENT_FD} to epoll ({EPOLL_FD})", "EVENT_FD",
          sink->getFD(), "EPOLL_FD", epollfd);
}

void Notifier::remove(NotifySink* sink)
{
    int rc = ::epoll_ctl(epollfd, EPOLL_CTL_DEL, sink->getFD(), NULL);
    if (rc < 0)
    {
        error(
            "epoll delete operation failed on epoll descriptor {EPOLL_FD} for event descriptor {EVENT_FD}: {ERRNO_DESCRIPTION}",
            "EPOLL_FD", epollfd, "EVENT_FD", sink->getFD(), "ERRNO_DESCRIPTION",
            ::strerror(errno), "ERRNO", errno);
        throw std::system_category().default_error_condition(errno);
    }

    sink->disarm();

    debug("Removed event descriptor {EVENT_FD} from epoll ({EPOLL_FD})",
          "EVENT_FD", sink->getFD(), "EPOLL_FD", epollfd);
}

void Notifier::run()
{
    struct epoll_event event;
    int rc;

    while ((rc = ::epoll_wait(epollfd, &event, 1, -1)) > 0)
    {
        NotifySink* sink = static_cast<NotifySink*>(event.data.ptr);

        try
        {
            sink->notify(*this);
        }
        catch (const SysfsI2CDeviceDriverBindException& ex)
        {
            error("Failed to bind device from Notifier: {EXCEPTION}",
                  "EXCEPTION", ex);
        }
    }

    if (rc < 0)
    {
        error(
            "epoll wait operation failed on epoll descriptor {EPOLL_FD}: {ERRNO_DESCRIPTION}",
            "EPOLL_FD", epollfd, "ERRNO_DESCRIPTION", ::strerror(errno),
            "ERRNO", errno);
    }
}
