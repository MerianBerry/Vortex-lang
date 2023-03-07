#pragma once

#include "vstring.h"
#include <cstdlib>
#include <string>
#include <memory>
#include <math.h>


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
        virtual Type equals(Type _t) {return {};}
        virtual Type nequals(Type RHS) {return {};}
        virtual Type operator !() {return {};}
        virtual Type add(Type RHS) {return {};}
        virtual Type sub(Type RHS) {return {};}
        virtual Type mul(Type RHS) {return {};}
        virtual Type div(Type RHS) {return {};}
        virtual Type fmod(Type RHS) {return {};}
        virtual Type greater(Type RHS) {return {};}
        virtual Type less(Type RHS) {return {};}
        virtual Type greatereq(Type RHS) {return {};}
        virtual Type lesseq(Type RHS) {return {};}
        virtual Type band(Type RHS) {return {};}
        virtual Type bor(Type RHS) {return {};}

        virtual Type operator &&(Type RHS) {return {};}
        virtual Type operator ||(Type RHS) {return {};}
        virtual Type operator ==(Type RHS) {return {};}
        virtual Type operator !=(Type RHS) {return {};}
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
            Type equals(Type _t) override
            {
                if (_t.type() != boolean)
                    return Boolean(false);

                return Boolean(Val == *(bool*)_t.get());
            }
            Type nequals(Type _t) override
            {
                if (_t.type() != boolean)
                    return Boolean(false);

                return Boolean(Val != *(bool*)_t.get());
            }
            Type operator !() override
            {
                return Boolean(!Val);
            }
            Type operator ==(Type RHS) override
            {
                return this->equals(RHS);
            }
            Type operator !=(Type RHS) override
            {
                return this->nequals(RHS);
            }
            Type add(Type RHS) override
            {
                if (RHS.type() != boolean)
                    return {};

                return Boolean(Val | *(bool*)RHS.get());
            }
            Type band(Type RHS) override
            {
                if (RHS.type() != boolean)
                    return {};

                return Boolean(Val && *(bool*)RHS.get());
            }
            Type bor(Type RHS) override
            {
                if (RHS.type() != boolean)
                    return {};
                
                return Boolean(Val || *(bool*)RHS.get());
            }

            Type operator &&(Type RHS) override
            {
                if (RHS.type() != boolean)
                    return {};
                
                return Boolean(Val && *(bool*)RHS.get());
            }
            Type operator ||(Type RHS) override
            {
                if (RHS.type() != boolean)
                    return {};
                
                return Boolean(Val || *(bool*)RHS.get());
            }
    };

    class LFloat : public Type
    {
        long double Val = 0.0;
        bool isnan = false;
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
            Type equals(Type _t) override
            {
                if (_t.type() != number)
                    return Boolean(false);
                auto num = *(long double*)_t.get();
                return Boolean(num > Val-0.0001 && num < Val+0.0001);
            }
            Type nequals(Type _t) override
            {
                if (_t.type() != number)
                    return Boolean(false);
                auto num = *(long double*)_t.get();
                return Boolean(!(num > Val-0.0001 && num < Val+0.0001));
            }
            Type operator ==(Type RHS) override
            {
                return this->equals(RHS);
            }
            Type operator !=(Type RHS) override
            {
                return this->nequals(RHS);
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
            Type mul(Type RHS) override
            {
                if (RHS.type() == number)
                    return LFloat(Val * *(long double*)RHS.get());
                else
                    return {};
            }
            Type div(Type RHS) override
            {
                if (RHS.type() == number)
                    return LFloat(Val / *(long double*)RHS.get());
                else
                    return {};
            }
            Type fmod(Type RHS) override
            {
                if (RHS.type() != number)
                    return {};
                
                return LFloat(fmodl(Val, *(long double*)RHS.get()));
            }
            Type greater(Type RHS) override
            {
                if (RHS.type() != number)
                    return {};
                
                return Boolean(Val > *(long double*)RHS.get());
            }
            Type less(Type RHS) override
            {
                if (RHS.type() != number)
                    return {};
                
                return Boolean(Val < *(long double*)RHS.get());
            }
            Type greatereq(Type RHS) override
            {
                if (RHS.type() != number)
                    return {};
                
                return Boolean(Val > *(long double*)RHS.get()) || *this == RHS;
            }
            Type lesseq(Type RHS) override
            {
                if (RHS.type() != number)
                    return {};
                
                return Boolean(Val < *(long double*)RHS.get()) || *this == RHS;
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
                //return stringf("%s", str.c_str());
            }
            int type() override {return string;}
            Type equals(Type _t) override
            {
                if (_t.type() != this->type())
                    return Boolean(false);
                return Boolean(str == *(std::string*)_t.get());
            }
            Type nequals(Type _t) override
            {
                if (_t.type() != this->type())
                    return Boolean(false);
                return Boolean(!(str == *(std::string*)_t.get()));
            }
            Type operator ==(Type RHS) override
            {
                return this->equals(RHS);
            }
            Type operator !=(Type RHS) override
            {
                return this->nequals(RHS);
            }
            Type add(Type RHS) override
            {
                return String(str + RHS.tostring());
            }
    };
}