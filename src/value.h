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
            Value(std::unique_ptr<vtex::Value> Val) : val(std::move(Val->val)) {}
            Value() {}
            explicit Value(Value && Val) : val(std::forward<uType>(Val.val)) {}
            std::string tostring()
            {
                if (!!val)
                    return val->tostring();
                return "___null";
            }

    };
}