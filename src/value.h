#pragma once

#include "types.h"
#include <memory>

namespace vtex
{
    typedef std::unique_ptr<Type> uType;
    class Value
    {
        public:
        uType val;
            Value(uType val) : val(std::move(val)) {}
            Value(std::unique_ptr<vtex::Value> Val) : val(std::move(Val->val)) {}
            Value() {}
            Value(Value && Val) : val(std::forward<uType>(Val.val)) {}
            uType &value() {return val;}
            //const uType value() {return std::move(val);}
            std::string tostring()
            {
                if (!!val)
                    return val->tostring();
                return "___null";
            }

    };
}