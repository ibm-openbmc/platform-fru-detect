/* SPDX-License-Identifier: Apache-2.0 */

#include "platforms/rainier.hpp"

static const std::set<std::string> models{
    "Rainier 1S4U",
    "Rainier 4U",
    "Rainier 2U",
};

std::set<std::string> Rainier::getSupportedModels()
{
    return models;
}
