/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */

#include "environment.hpp"

#include <sys/mman.h>
#include <unistd.h>

#include <phosphor-logging/lg2.hpp>

#include <cerrno>
#include <cstdint>
#include <cstring>

PHOSPHOR_LOG2_USING;

void EnvironmentManager::enrollEnvironment(ExecutionEnvironment* env)
{
    environments.push_back(env);
}

void EnvironmentManager::run(PlatformManager& pm, Notifier& notifier,
                             Inventory* inventory)
{
    for (auto& env : environments)
    {
        if (env->probe())
        {
            return env->run(pm, notifier, inventory);
        }
    }
}

#if defined(__arm__)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define reg(r) "r" #r

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define trigger(r, p)                                                          \
    asm volatile("mov " reg(r) ", %0;"                                         \
                               "orr " reg(r) ", " reg(r) ", " reg(r)           \
                 :                                                             \
                 : "r"(p)                                                      \
                 : "memory")
#else
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define trigger(r, p)                                                          \
    (void)(r);                                                                 \
    (void)(p);
#endif

#define MAGIC_MMAP_FLAGS                                                       \
    (MAP_PRIVATE | MAP_ANONYMOUS | MAP_LOCKED | MAP_POPULATE)

static const uint64_t magic = 0xd09f7d8134f13f46ull;
static const struct
{
    uint64_t magic;
    uint16_t pages;
    uint16_t checksum;
    uint32_t data;
} header = {
    magic,
    0,
    0x8d2b,
    0,
};

bool SimicsExecutionEnvironment::isSimicsExecutionEnvironment()
{
    long page = ::sysconf(_SC_PAGESIZE);
    void* region = ::mmap(nullptr, page, PROT_READ | PROT_WRITE,
                          MAGIC_MMAP_FLAGS, -1, 0);
    if (region == nullptr)
    {
        error("Failed to map buffer: {ERRNO_DESCRIPTION}", "ERRNO_DESCRIPTION",
              ::strerror(errno), "ERRNO", errno);
        throw std::system_category().default_error_condition(errno);
    }

    ::memcpy(region, &header, sizeof(header));
    trigger(12, region);
    bool isSimics = static_cast<bool>(::memcmp(region, &magic, sizeof(magic)));

    int rc = ::munmap(region, page);
    if (rc == -1)
    {
        warning("Failed to unmap buffer: {ERRNO_DESCRIPTION}",
                "ERRNO_DESCRIPTION", ::strerror(errno), "ERRNO", errno);
    }

    return isSimics;
}

bool SimicsExecutionEnvironment::probe()
{
    return SimicsExecutionEnvironment::isSimicsExecutionEnvironment();
}

void SimicsExecutionEnvironment::run(PlatformManager& pm, Notifier& notifier,
                                     Inventory* inventory)
{
    /* Work around any issues in simics modelling to unblock CI */
    warning("Executing in a simics environment, catching all exceptions");
    try
    {
        pm.detectPlatformFrus(notifier, inventory);
    }
    catch (const std::exception& ex)
    {
        error("Exception caught in a simics environment: {EXCEPTION}",
              "EXCEPTION", ex);
    }
    catch (...)
    {
        error("Untyped exception caught in simics environment");
    }

    info("Exiting gracefully");
}

bool HardwareExecutionEnvironment::probe()
{
    return !SimicsExecutionEnvironment::isSimicsExecutionEnvironment();
}

void HardwareExecutionEnvironment::run(PlatformManager& pm, Notifier& notifier,
                                       Inventory* inventory)
{
    debug("Executing in a hardware environment, propagating all exceptions");
    pm.detectPlatformFrus(notifier, inventory);
}
