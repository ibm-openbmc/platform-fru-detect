/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright IBM Corp. 2022 */
#include "notify.hpp"
#include "platform.hpp"

#include <utility>

#include "gtest/gtest.h"

struct MockDeviceState
{
    int plugged;
    int unplugged;
};

class MockDevice : public Device
{
  public:
    explicit MockDevice(MockDeviceState* state) : state(state) {}
    MockDevice(const MockDevice& other) = delete;
    MockDevice(MockDevice&& other) = delete;
    virtual ~MockDevice() = default;

    MockDevice& operator=(const MockDevice& other) = delete;
    MockDevice& operator=(MockDevice&& other) = delete;

    void plug([[maybe_unused]] Notifier& notifier) override
    {
        state->plugged++;
    }

    void unplug([[maybe_unused]] Notifier& notifier,
                [[maybe_unused]] int mode = UNPLUG_REMOVES_INVENTORY) override
    {
        state->unplugged++;
    }

  private:
    MockDeviceState* state;
};

TEST(ConnectorActions, populateFromInit)
{
    Notifier notifier;
    MockDeviceState state{0, 0};
    Connector<MockDevice> connector(0, &state);

    connector.populate(notifier);

    EXPECT_EQ(1, state.plugged);
    EXPECT_EQ(0, state.unplugged);
}

TEST(ConnectorActions, depopulateFromInit)
{
    Notifier notifier;
    MockDeviceState state{0, 0};
    Connector<MockDevice> connector(0, &state);

    connector.depopulate(notifier);

    EXPECT_EQ(0, state.plugged);
    EXPECT_EQ(1, state.unplugged);
}

TEST(ConnectorActions, populateFromPopulated)
{
    Notifier notifier;
    MockDeviceState state{0, 0};
    Connector<MockDevice> connector(0, &state);

    connector.populate(notifier);

    EXPECT_EQ(1, state.plugged);
    EXPECT_EQ(0, state.unplugged);

    connector.populate(notifier);

    EXPECT_EQ(1, state.plugged);
    EXPECT_EQ(0, state.unplugged);
}

TEST(ConnectorActions, depopulateFromDepopulated)
{
    Notifier notifier;
    MockDeviceState state{0, 0};
    Connector<MockDevice> connector(0, &state);

    connector.depopulate(notifier);

    EXPECT_EQ(0, state.plugged);
    EXPECT_EQ(1, state.unplugged);

    connector.depopulate(notifier);

    EXPECT_EQ(0, state.plugged);
    EXPECT_EQ(1, state.unplugged);
}

TEST(ConnectorActions, depopulateFromPopulated)
{
    Notifier notifier;
    MockDeviceState state{0, 0};
    Connector<MockDevice> connector(0, &state);

    connector.populate(notifier);

    EXPECT_EQ(1, state.plugged);
    EXPECT_EQ(0, state.unplugged);

    connector.depopulate(notifier);

    EXPECT_EQ(1, state.plugged);
    EXPECT_EQ(1, state.unplugged);
}

TEST(ConnectorActions, populateFromDepopulated)
{
    Notifier notifier;
    MockDeviceState state{0, 0};
    Connector<MockDevice> connector(0, &state);

    connector.depopulate(notifier);

    EXPECT_EQ(0, state.plugged);
    EXPECT_EQ(1, state.unplugged);

    connector.populate(notifier);

    EXPECT_EQ(1, state.plugged);
    EXPECT_EQ(1, state.unplugged);
}
