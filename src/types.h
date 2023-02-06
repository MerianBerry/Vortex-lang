#pragma once

#include <cstdlib>
#include <string>
#include <memory>

namespace vtex
{
    enum TypeTokens
    {
        null,
        number,
        string
    };

    class Type
    {
        public:
        virtual ~Type() = default;
        virtual void* get() { return nullptr; }
        virtual std::string tostring() = 0;
        virtual int type() {return null;}
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
        int type() override {return number;}
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
            return stringf("\"%s\"", str.c_str());
        }
        int type() override {return string;}
    };
}