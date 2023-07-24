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
    void enrollEnvironment(ExecutionEnvironment* env);
    void run(PlatformManager& pm, Notifier& notifier, Inventory* inventory);

  private:
    std::list<ExecutionEnvironment*> environments;
};

class ExecutionEnvironment
{
  public:
    virtual bool probe() = 0;
    virtual void run(PlatformManager& pm, Notifier& notifier,
                     Inventory* inventory) = 0;
};

class SimicsExecutionEnvironment : public ExecutionEnvironment
{
  public:
    static bool isSimicsExecutionEnvironment();

    /* ExecutionEnvironment */
    bool probe() override;
    void run(PlatformManager& pm, Notifier& notifier,
             Inventory* inventory) override;
};

class HardwareExecutionEnvironment : public ExecutionEnvironment
{
  public:
    /* ExecutionEnvironment */
    bool probe() override;
    void run(PlatformManager& pm, Notifier& notifier,
             Inventory* inventory) override;
};
