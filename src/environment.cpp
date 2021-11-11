/* SPDX-License-Identifier: Apache-2.0 */

#include "environment.hpp"

#include <phosphor-logging/lg2.hpp>

#include <mntent.h>
#include <stdio.h>
#include <string.h>

PHOSPHOR_LOG2_USING;

void EnvironmentManager::enrollEnvironment(ExecutionEnvironment* env)
{
    environments.push_back(env);
}

void EnvironmentManager::run(PlatformManager& pm, Inventory* inventory)
{
    for (auto& env : environments)
    {
        if (env->probe())
        {
            return env->run(pm, inventory);
        }
    }
}

bool SimicsExecutionEnvironment::isSimicsExecutionEnvironment()
{
    /*
     * Test whether we're running under simics by establishing whether we have a simicsfs fuse
     * filesystem mounted
     */

    static const char* mountinfo_path = "/proc/mounts";
    FILE* mntinfo = ::setmntent(mountinfo_path, "r");
    if (!mntinfo)
    {
        error("Failed to open {MOUNTINFO_PATH}: {ERRNO_DESCRIPTION}", "MOUNTINFO_PATH",
            mountinfo_path, "ERRNO_DESCRIPTION", ::strerror(errno), "ERRNO", errno);
        throw std::system_category().default_error_condition(errno);
    }

    bool simicsfs = false;

    struct mntent *mntent;
    while ((mntent = ::getmntent(mntinfo)))
    {
        if (!::strcmp("fuse.simicsfs-client", mntent->mnt_type))
        {
            simicsfs = true;
            break;
        }
    }

    ::endmntent(mntinfo);

    return simicsfs;
}

bool SimicsExecutionEnvironment::probe()
{
    return SimicsExecutionEnvironment::isSimicsExecutionEnvironment();
}

void SimicsExecutionEnvironment::run(PlatformManager& pm, Inventory* inventory)
{
    /* Work around any issues in simics modelling to unblock CI */
    warning("Executing in a simics environment, catching all exceptions");
    try
    {
        pm.detectPlatformFrus(inventory);
    }
    catch (const std::exception& ex)
    {
        error("Exception caught in a simics environment: {EXCEPTION}", "EXCEPTION", ex);
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

void HardwareExecutionEnvironment::run(PlatformManager& pm, Inventory* inventory)
{
    debug("Executing in a hardware environment, propagating all exceptions");
    pm.detectPlatformFrus(inventory);
}
