#pragma once
#include "types.h"
#include "value.h"
#include <memory>
#include <vector>
#include <string>

namespace vtex
{
    vtex::Type set(uType val, vtex::Type t)
    {
        if (!val)
            return t;
        val.release();
        switch(t.type())
        {
            case number:
            {
                val = std::make_unique<vtex::LFloat>(*(long double*)t.get());
                break;
            }
            case string:
            {
                val = std::make_unique<vtex::String>(*(std::string*)t.get());
                break;
            }
            case boolean:
            {
                val = std::make_unique<vtex::Boolean>(*(bool*)t.get());
                break;
            }
            default:
            {
                val = std::make_unique<vtex::Type>();
                break;
            }
        }
        //val = std::make_unique<vtex::Type>();

        return t;
    }
    vtex::Type equals(uType val, vtex::Type t)
    {
        if (!val)
            return t;
        auto ret = val->equals(t);
        return ret;
    }
    vtex::Type nequals(uType val, vtex::Type t)
    {
        if (!val)
            return t;
        auto ret = val->nequals(t);
        return ret;
    }
    vtex::Type add(uType val, vtex::Type t)
    {
        if (!val)
            return t;
        auto ret = val->add(t);
        return ret;
    }
    vtex::Type addeq(uType val, vtex::Type t)
    {
        if (!val)
            return t;
        auto ret = val->add(t);
        val.release();
        val = std::make_unique<Type>(ret);
        return *(val.get());
    }
    vtex::Type subeq(uType val, vtex::Type t)
    {
        if (!val)
            return t;
        auto ret = val->sub(t);
        val.release();
        val = std::make_unique<Type>(ret);
        return *(val.get());
    }
    vtex::Type muleq(uType val, vtex::Type t)
    {
        if (!val)
            return t;
        auto ret = val->mul(t);
        val.release();
        val = std::make_unique<Type>(ret);
        return *(val.get());
    }
    vtex::Type diveq(uType val, vtex::Type t)
    {
        if (!val)
            return t;
        auto ret = val->div(t);
        val.release();
        val = std::make_unique<Type>(ret);
        return *(val.get());
    }
    vtex::Type sub(uType val, vtex::Type t)
    {
        if (!val)
            return t;
        auto ret = val->sub(t);
        return ret;
    }
    vtex::Type mul(uType val, vtex::Type t)
    {
        if (!val)
            return t;
        auto ret = val->mul(t);
        return ret;
    }
    vtex::Type div(uType val, vtex::Type t)
    {
        if (!val)
            return t;
        auto ret = val->div(t);
        return ret;
    }
    vtex::Type fmod(uType val, vtex::Type t)
    {
        if (!val)
            return t;
        auto ret = val->fmod(t);
        return ret;
    }
    vtex::Type greater(uType val, vtex::Type t)
    {
        if (!val)
            return t;
        auto ret = val->greater(t);
        return ret;
    }
    vtex::Type less(uType val, vtex::Type t)
    {
        if (!val)
            return t;
        auto ret = val->less(t);
        return ret;
    }
    vtex::Type greatereq(uType val, vtex::Type t)
    {
        if (!val)
            return t;
        auto ret = val->greatereq(t);
        return ret;
    }
    vtex::Type lesseq(uType val, vtex::Type t)
    {
        if (!val)
            return t;
        auto ret = val->lesseq(t);
        return ret;
    }
    vtex::Type _and(uType val, vtex::Type t)
    {
        if (!val)
            return t;
        auto ret = val->band(t);
        return ret;
    }
    vtex::Type _or(uType val, vtex::Type t)
    {
        if (!val)
            return t;
        auto ret = val->bor(t);
        return ret;
    }

    typedef Type(*opfunc)(uType, Type);
    std::vector<std::tuple<std::string, int, opfunc>> stdops =
    {
        {"+", 10, vtex::add},
        {"-", 10, vtex::sub},
        {"*", 30, vtex::mul},
        {"/", 30, vtex::div},
        {"%", 30, vtex::fmod},
        {">", 8, vtex::greater},
        {"<", 8, vtex::less},
        {"&&", 5, vtex::_and},
        {"||", 5, vtex::_or},
        {"=", 5, vtex::set},
        {"==", 5, vtex::equals},
        {"!=", 5, vtex::nequals},
        {">=", 5, vtex::greatereq},
        {"<=", 5, vtex::lesseq},
        {"+=", 5, vtex::addeq},
        {"-=", 5, vtex::subeq},
        {"*=", 5, vtex::muleq},
        {"/=", 5, vtex::diveq}
    };
}
