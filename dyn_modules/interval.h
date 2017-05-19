#ifndef INTERVAL_H
#define INTERVAL_H

#include "kernel/mod2.h"
#include "coeffs/numbers.h"
#include "Singular/ipid.h"
#include "Singular/blackbox.h"

struct interval {
    number lower;
    number upper;

    // constructors/destructor
    interval() {
        lower = nInit(0);
        upper = nInit(0);
    }

    interval(number a) {
        lower = a;
        upper = nCopy(a);
    }

    interval(number a, number b) {
        lower = a;
        upper = b;
    }

    interval(const interval &I) {
        lower = nCopy(I.lower);
        upper = nCopy(I.upper);
    }

    // destructor: triggers on delete
    ~interval() {
        nDelete(&lower);
        nDelete(&upper);
    }
};

struct box {
    interval** intervals;

    // constructors/destructor
    box() {
        int i, n = currRing->N;
        intervals = (interval**) malloc(n * sizeof(interval*));
        if (intervals != NULL) {
            for (i = 0; i < n; i++) {
                intervals[i] = new interval();
            }
        }
    }

    box(const box &B) {
        int i, n = currRing->N;
        intervals = (interval**) malloc(n * sizeof(interval*));
        if (intervals != NULL) {
            for (i = 0; i < n; i++) {
                intervals[i] = new interval(*B.intervals[i]);
            }
        }
    }

    ~box() {
        int i, n = currRing->N;
        for (i = 0; i < n; i++) {
            delete intervals[i];
        }
        free((void**) intervals);
    }
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
