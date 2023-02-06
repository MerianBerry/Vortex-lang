#pragma once

#include "tokens.h"

#include <string>
#include <vector>
#include <tuple>

namespace vtex
{
    const inline std::vector<std::tuple<const char*, int>> keys =
    {
        {"new", tok_new}
    };
}