#pragma once

#include "types.h"
#include <memory>

namespace vtex
{
    typedef std::unique_ptr<Type> uType;
    class Value
    {
        public:
        std::unique_ptr<vtex::Type> val;
            Value(std::unique_ptr<vtex::Type> val) : val(std::move(val)) {}

    };
}