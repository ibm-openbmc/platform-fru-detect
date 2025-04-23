/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */

#include "notify.hpp"

#include <unistd.h>

#include <phosphor-logging/lg2.hpp>

#include <cerrno>
#include <csignal>
#include <cstring>
#include <system_error>

extern "C"
{
#include <sys/epoll.h>
#include <sys/signalfd.h>
};

PHOSPHOR_LOG2_USING;

Notifier::Notifier()
{
    // populating epollfd and exitfd using a member initializer exposes a
    // loss-of-information hazard as bot epoll_create1() and signalfd() set
    // errno on failure. This may lead the logging to report the errno value set
    // by a failure of signalfd() as the errno for a failure of epoll_create1().
    // Instead, silence the preference warning.
    // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
    epollfd = ::epoll_create1(0);
    if (epollfd < 0)
    {
        error("epoll create operation failed: {ERRNO_DESCRIPTION}",
              "ERRNO_DESCRIPTION", ::strerror(errno), "ERRNO", errno);
        throw std::system_category().default_error_condition(errno);
    }

    sigset_t mask;
    ::sigemptyset(&mask);
    ::sigaddset(&mask, SIGINT);
    ::sigaddset(&mask, SIGQUIT);
    int rc = ::sigprocmask(SIG_BLOCK, &mask, nullptr);
    if (rc == -1)
    {
        error(
            "Failed to mask exit signals with sigprocmask: {ERRNO_DESCRIPTION}",
            "ERRNO_DESCRIPTION", ::strerror(errno), "ERRNO", errno);
        throw std::system_category().default_error_condition(errno);
    }

    // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
    exitfd = signalfd(-1, &mask, 0);
    if (exitfd == -1)
    {
        ::close(epollfd);
        error("Failed to create signalfd: {ERRNO_DESCRIPTION}",
              "ERRNO_DESCRIPTION", ::strerror(errno), "ERRNO", errno);
        throw std::system_category().default_error_condition(errno);
    }

    /*
     * CAUTION: We're using nullptr as a sentinel for the signalfd. This saves
     * the source noise of creating a NotifySink class for it.
     */
    struct epoll_event event{EPOLLIN | EPOLLPRI, {nullptr}};
    rc = ::epoll_ctl(epollfd, EPOLL_CTL_ADD, exitfd, &event);
    if (rc == -1)
    {
        ::close(exitfd);
        ::close(epollfd);
        error(
            "epoll add operation failed on epoll descriptor {EPOLL_FD} for exitfd descriptor {EXIT_FD}: {ERRNO_DESCRIPTION}",
            "EPOLL_FD", epollfd, "EXIT_FD", exitfd, "ERRNO_DESCRIPTION",
            ::strerror(errno), "ERRNO", errno);
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
    struct epoll_event event{EPOLLIN | EPOLLPRI, {sink}};
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
    int fd = sink->getFD();
    if (fd == -1)
    {
        debug("Skipping disarmed sink");
        return;
    }

    int rc = ::epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, nullptr);
    if (rc < 0)
    {
        error(
            "epoll delete operation failed on epoll descriptor {EPOLL_FD} for event descriptor {EVENT_FD}: {ERRNO_DESCRIPTION}",
            "EPOLL_FD", epollfd, "EVENT_FD", fd, "ERRNO_DESCRIPTION",
            ::strerror(errno), "ERRNO", errno);
        throw std::system_category().default_error_condition(errno);
    }

    debug("Removed event descriptor {EVENT_FD} from epoll ({EPOLL_FD})",
          "EVENT_FD", sink->getFD(), "EPOLL_FD", epollfd);

    sink->disarm();
}

void Notifier::run()
{
    struct epoll_event event{};
    int rc = 0;

    for (;;)
    {
        rc = ::epoll_wait(epollfd, &event, 1, -1);
        if (rc == -1 && errno == EINTR)
        {
            continue;
        }

        if (rc == -1)
        {
            error(
                "epoll wait operation failed on epoll descriptor {EPOLL_FD}: {ERRNO_DESCRIPTION}",
                "EPOLL_FD", epollfd, "ERRNO_DESCRIPTION", ::strerror(errno),
                "ERRNO", errno);
            break;
        }

        NotifySink* sink = static_cast<NotifySink*>(event.data.ptr);

        /* Is it the exitfd sentinel? */
        if (sink == nullptr)
        {
            struct signalfd_siginfo fdsi{};

            ssize_t ingress = read(exitfd, &fdsi, sizeof(fdsi));
            if (ingress != sizeof(fdsi))
            {
                error(
                    "Short read from signalfd, expected {SIGNALFD_SIGINFO_SIZE}, got {READ_SIZE}",
                    "SIGNALFD_SIGINFO_SIZE", sizeof(fdsi), "READ_SIZE",
                    ingress);
                throw std::system_category().default_error_condition(EBADMSG);
            }

            if (fdsi.ssi_signo != SIGINT && fdsi.ssi_signo != SIGQUIT)
            {
                error("signalfd provided unexpected signal: {SIGNAL}", "SIGNAL",
                      fdsi.ssi_signo);
                throw std::system_category().default_error_condition(ENOTSUP);
            }

            /* Time to clean up */
            break;
        }

        /* If it's not the exitfd sentinel it's a regular NotifySink */
        try
        {
            sink->notify(*this);
        }
        catch (const std::exception& ex)
        {
            error(
                "Unhandled exception in notifier callback, disabling sink: {EXCEPTION}",
                "EXCEPTION", ex);
            remove(sink);
        }
        catch (const std::error_condition& err)
        {
            error(
                "Unhandled error condition in notifier callback, disabling sink: {ERROR}",
                "ERROR", err.value());
            remove(sink);
        }
    }

    info("Exiting notify event loop");
}
