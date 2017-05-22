#ifndef INTERVAL_H
#define INTERVAL_H

#include "Singular/ipid.h"

struct interval
{
    number lower;
    number upper;

    interval();
    interval(number);
    interval(number, number);
    interval(interval*);
    ~interval();
};

struct box
{
    interval** intervals;

    box();
    box(box*);
    ~box();
};

extern int intervalID;
extern int boxID;

// helpful functions
interval* intervalScalarMultiply(number, interval*);
interval* intervalMultiply(interval*, interval*);
interval* intervalAdd(interval*, interval*);
interval* intervalSubtract(interval*, interval*);
bool intervalEqual(interval*, interval*);
bool intervalContainsZero(interval*);

extern "C" int mod_init(SModulFunctions*);

#endif
