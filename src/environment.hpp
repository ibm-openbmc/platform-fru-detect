/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

#include "platform.hpp"

#include <list>

class ExecutionEnvironment;

class EnvironmentManager
{
  public:
    EnvironmentManager() = default;
    ~EnvironmentManager() = default;

    void enrollEnvironment(ExecutionEnvironment* env);
    void run(PlatformManager& pm, Inventory* inventory);

  private:
    std::list<ExecutionEnvironment*> environments;
};

class ExecutionEnvironment
{
  public:
    ExecutionEnvironment() = default;
    virtual ~ExecutionEnvironment() = default;

    virtual bool probe() = 0;
    virtual void run(PlatformManager& pm, Inventory* inventory) = 0;
};

class SimicsExecutionEnvironment : public ExecutionEnvironment
{
  public:
    static bool isSimicsExecutionEnvironment();

    SimicsExecutionEnvironment() = default;
    ~SimicsExecutionEnvironment() = default;

    /* ExecutionEnvironment */
    virtual bool probe() override;
    virtual void run(PlatformManager& pm, Inventory* inventory) override;
};

class HardwareExecutionEnvironment : public ExecutionEnvironment
{
  public:
    HardwareExecutionEnvironment() = default;
    ~HardwareExecutionEnvironment() = default;

    /* ExecutionEnvironment */
    virtual bool probe() override;
    virtual void run(PlatformManager& pm, Inventory* inventory) override;
};
