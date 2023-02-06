#pragma once
#include "types.h"
#include <memory>

namespace vtex
{
    LFloat lfadd(LFloat LHS, LFloat RHS)
    {
        return LFloat(*(long double*)LHS.get() + *(long double*)RHS.get());
    }
}
