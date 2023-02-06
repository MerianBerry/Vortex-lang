#pragma once

#include <cstdlib>
#include <string>
#include <memory>

namespace vtex
{

    class Type
    {
        public:
        virtual ~Type() = default;
        virtual void* get() { return nullptr; }
        virtual std::string tostring() = 0;
    };

    class LFloat : public Type
    {
        long double Val = 0.0;
        public:
        LFloat(long double Val) : Val(Val) {}
        void* get() override
        {
            return &Val;
        }
        std::string tostring() override
        {
            return std::to_string(Val);
        }
    };

    class String : public Type
    {
        std::string str = "";
        public:
        String(std::string str) : str(str) {}
        void* get() override
        {
            return &str;
        }
        std::string tostring() override
        {
            return str;
        }
    };
}