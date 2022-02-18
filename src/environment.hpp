/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2021 */
#pragma once

#include "notify.hpp"
#include "platform.hpp"

#include <list>

class ExecutionEnvironment;

class EnvironmentManager
{
  public:
    EnvironmentManager() = default;
    ~EnvironmentManager() = default;

    void enrollEnvironment(ExecutionEnvironment* env);
    void run(PlatformManager& pm, Notifier& notifier, Inventory* inventory);

  private:
    std::list<ExecutionEnvironment*> environments;
};

class ExecutionEnvironment
{
  public:
    ExecutionEnvironment() = default;
    virtual ~ExecutionEnvironment() = default;

    virtual bool probe() = 0;
    virtual void run(PlatformManager& pm, Notifier& notifier,
                     Inventory* inventory) = 0;
};

class SimicsExecutionEnvironment : public ExecutionEnvironment
{
  public:
    static bool isSimicsExecutionEnvironment();

    SimicsExecutionEnvironment() = default;
    ~SimicsExecutionEnvironment() override = default;

    /* ExecutionEnvironment */
    bool probe() override;
    void run(PlatformManager& pm, Notifier& notifier, Inventory* inventory) override;
};

class HardwareExecutionEnvironment : public ExecutionEnvironment
{
  public:
    HardwareExecutionEnvironment() = default;
    ~HardwareExecutionEnvironment() override = default;

    /* ExecutionEnvironment */
    bool probe() override;
    void run(PlatformManager& pm, Notifier& notifier, Inventory* inventory) override;
};
