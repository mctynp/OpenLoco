#pragma once

#include "Industry.h"
#include <array>
#include <cstddef>

namespace OpenLoco::IndustryManager
{
    constexpr size_t max_industries = 128;

    void reset();
    std::array<industry, max_industries>& industries();
    industry* get(industry_id_t id);
    void update();
    void updateMonthly();
}
