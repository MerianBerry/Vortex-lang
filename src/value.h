#pragma once

#include "types.h"

namespace vtex
{
    class Value
    {
        public:
        std::unique_ptr<vtex::Type> val;
            Value(std::unique_ptr<vtex::Type> val) : val(std::move(val)) {}
    };
}