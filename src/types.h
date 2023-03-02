#pragma once

#include "vstring.h"
#include <cstdlib>
#include <string>
#include <memory>


namespace vtex
{
    enum TypeTokens
    {
        null = -70,
        nil,
        nan,
        number,
        string,
        boolean
    };

    class Type
    {
        public:
        virtual ~Type() = default;
        virtual void* get() { return nullptr; }
        virtual std::string tostring() {return "__null";};
        virtual int type() {return null;}
        virtual int equals(Type _t) {return false;};
        virtual Type add(Type RHS) {return {};}
        virtual Type sub(Type RHS) {return {};}
    };

    class LFloat : public Type
    {
        long double Val = 0.0;
        //bool isnan = false;
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
        int equals(Type _t) override
        {
            if (_t.type() != this->type())
                return null;
            auto num = *(long double*)_t.get();
            return num > Val-0.0001 && num < Val+0.0001;
        }
        Type add(Type RHS) override
        {
            if (RHS.type() == number)
            {
                return LFloat(Val + *(long double*)RHS.get());
            } else if (RHS.type() == boolean)
            {
                return LFloat(Val + (long double)*(bool*)RHS.get());
            }

            return {};
        }
        Type sub(Type RHS) override
        {
            if (RHS.type() == number)
            {
                return LFloat(Val - *(long double*)RHS.get());
            } else if (RHS.type() == boolean)
            {
                return LFloat(Val - (long double)*(bool*)RHS.get());
            }

            return {};
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
            return stringf("\"%s\"", str.c_str());
        }
        int type() override {return string;}
        int equals(Type _t) override
        {
            if (_t.type() != this->type())
                return null;
            return (*(std::string*)_t.get()) == str;
        }
        Type add(Type RHS) override
        {
            return String(str + RHS.tostring());
        }
    };

    class Boolean : public Type
    {
        bool Val = false;
        public:
            Boolean(bool Val) : Val(Val) {}
            void* get() override
            {
                return &Val;
            }
            std::string tostring() override
            {
                if (Val)
                    return "true";

                return "false";
            }
            int type() override {return boolean;}
            int equals(Type _t) override
            {
                if (_t.type() != this->type())
                return null;
                return (*(bool*)_t.get()) == Val;
            }
            Type add(Type RHS) override
            {
                if (RHS.type() != boolean)
                    return {};

                return Boolean(Val | *(bool*)RHS.get());
            }
    };

    class Function : public Type
    {
        public:
        
    };
}