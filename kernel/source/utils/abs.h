/* abs.h
   Purpose: The absolute value of X. */
#pragma once

static inline int abs(int x) 
{
    return (x < 0) ? x * -1 : x;    
}
