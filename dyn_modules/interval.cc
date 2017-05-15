#include "kernel/mod2.h"
#include "coeffs/numbers.h"
#include "Singular/ipid.h"
#include "Singular/blackbox.h"

struct interval {
    number lower;
    number upper;

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
};

// type ID
static int intervalID;

void* interval_Init(blackbox *b) {
    interval *i = new interval();
    return (void*)i;
}

// convert interval to string
char* interval_String(blackbox *b, void *d) {
    if (d == NULL) {
        // invalid object
        return omStrDup("[?]");
    } else {
        interval* i = (interval*) d;

        // use nWrite since nothing better (?) exists
        StringSetS("[");
        nWrite(i->lower);
        StringAppendS(", ");
        nWrite(i->upper);
        StringAppendS("]");

        return StringEndS();
    }
}

// TODO may not actually work
void* interval_Copy(blackbox *b, void *d) {
    interval *i = (interval*) d;
    interval *inew = new interval(nCopy(i->lower), nCopy(i->upper));

    return (void*) inew;
}

// helper function
void intervalDelete(interval *I) {
    nDelete(&(I->lower));
    nDelete(&(I->upper));
    delete I;
}

void interval_Destroy(blackbox *b, void *d) {
    intervalDelete((interval*) d);
}

// assigning values to intervals
BOOLEAN interval_Assign(leftv result, leftv args) {
    assume(result->Typ() == intervalID);
    interval *RES;

    // destroy data of result if it exists
    if (result != NULL && result->Data() != NULL && result->Typ() == intervalID) {
        // "I=I" (same pointers)
        if (result->Data() == args->Data()) {
            return FALSE;
        }
        intervalDelete((interval*) result->Data());
    }

    /*
     * Allow assignments of the form
     *  I = a,
     *  I = a, b,
     *  I = J
     * where a, b numbers or ints and J interval
     */

    number n1, n2;

    if (args->Typ() == INT_CMD) {
        n1 = nInit((int)(long) args->Data());
    } else if (args->Typ() == NUMBER_CMD) {
        n1 = nCopy((number) args->Data());
    } else if (args->Typ() == intervalID) {
        interval *I = (interval*) args->Data();
        n1 = nCopy(I->lower);
        n2 = nCopy(I->upper);
    } else {
        Werror("Input not supported: first argument not int or number");
        return TRUE;
    }

    // check if second argument exists
    if (args->Typ() == intervalID) {
        RES = new interval(n1, n2);
    } else if (args->next == NULL) {
        RES = new interval(n1);
    } else {
        if (args->next->Typ() == INT_CMD) {
            n2 = nInit((int)(long) args->next->Data());
        } else if (args->next->Typ() == NUMBER_CMD) {
            n2 = nCopy((number) args->next->Data());
        } else {
            Werror("Input not supported: second argument not int or number");
            return TRUE;
        }

        RES = new interval(n1, n2);
    }

    if (result->rtyp == IDHDL) {
        IDDATA((idhdl)result->data) = (char*) RES;
    } else {
        result->rtyp = intervalID;
        result->data = (void*) RES;
    }

    return FALSE;
}

// alias to interval_Assign
BOOLEAN bounds(leftv result, leftv args) {
    return interval_Assign(result, args);
}

// interval -> interval procedures

interval* intervalScalarMultiply(number a, interval *I) {
    number lo, up;
    if (nGreaterZero(a)) {
        lo = nMult(a, I->lower);
        up = nMult(a, I->upper);
    } else {
        lo = nMult(a, I->upper);
        up = nMult(a, I->lower);
    }

    return new interval(lo, up);
}

interval* intervalMultiply(interval *I, interval *J) {
    number lo, up;
    number nums[4];
    nums[0] = nMult(I->lower, J->lower);
    nums[1] = nMult(I->lower, J->upper);
    nums[2] = nMult(I->upper, J->lower),
    nums[3] = nMult(I->upper, J->upper);

    int i, imax = 0, imin = 0;
    for (i = 1; i < 4; i++) {
        if (nGreater(nums[i], nums[imax])) {
            imax = i;
        }
        if (nGreater(nums[imin], nums[i])) {
            imin = i;
        }
    }

    lo = nCopy(nums[imin]);
    up = nCopy(nums[imax]);

    // delete products
    for (i = 0; i < 4; i++) {
        nDelete(&nums[i]);
    }

    return new interval(lo, up);
}

// ckeck if zero is contained in an interval
bool intervalContainsZero(interval *I) {
    number n = nMult(I->lower, I->upper);
    bool result = !nGreaterZero(n);
    // delete helper number
    nDelete(&n);

    return result;
}

/*
 * BINARY OPERATIONS:
 * Cases handled:
 *      I + J,
 *
 *      I - J,
 *
 *      a * J,
 *      I * a,
 *      I * J,
 *
 *      a / J,
 *      I / a,
 *      I / J
 *
 *      I ^ n,
 *
 *      I == J
 *
 *  where I, J, interval, a, b int or number, n int
 */

BOOLEAN interval_Op2(int op, leftv result, leftv i1, leftv i2) {
    interval *RES;

    // destroy data of result if it exists
    if (result->Data() != NULL && result->Typ() == intervalID) {
        intervalDelete((interval*) result->Data());
    }

    switch(op) {
        case '+':
        {
            number lo, up;
            if (i1->Typ() != intervalID || i2->Typ() != intervalID) {
                Werror("syntax: <interval> + <interval>");
                return TRUE;
            }
            interval *I1, *I2;
            I1 = (interval*) i1->Data();
            I2 = (interval*) i2->Data();

            lo = nAdd(I1->lower, I2->lower);
            up = nAdd(I1->upper, I2->upper);

            RES = new interval(lo, up);
            break;
        }
        case '-':
        {
            number lo, up;
            if (i1->Typ() != intervalID || i2->Typ() != intervalID) {
                Werror("syntax: <interval> - <interval>");
                return TRUE;
            }
            interval *I1, *I2;
            I1 = (interval*) i1->Data();
            I2 = (interval*) i2->Data();

            lo = nSub(I1->lower, I2->upper);
            up = nSub(I1->upper, I2->lower);

            RES = new interval(lo, up);
            break;
        }
        case '*':
        {
            if (i1->Typ() == intervalID && i2->Typ() == intervalID) {
                interval *I1, *I2;
                I1 = (interval*) i1->Data();
                I2 = (interval*) i2->Data();

                RES = intervalMultiply(I1, I2);
            } else {
                // one arg is scalar, one is interval
                // give new names to reduce to one case
                leftv iscalar, iinterv;
                if (i1->Typ() != intervalID) {
                    iscalar = i1;
                    iinterv = i2;
                } else {
                    iscalar = i2;
                    iinterv = i1;
                }
                number n;

                switch (iscalar->Typ()) {
                    case INT_CMD:
                        { n = nInit((int)(long) iscalar->Data()); break; }
                    case NUMBER_CMD:
                        { n = nCopy((number) iscalar->Data()); break; }
                    default:
                        { Werror("first argument not int/number/interval"); return TRUE;}
                }

                interval *I = (interval*) iinterv->Data();

                RES = intervalScalarMultiply(n, I);
                // n no longer needed, delete it
                nDelete(&n);
            }

            break;
        }
        case '/':
        {
            if(i2->Typ() == intervalID) {
                interval *I2;
                I2 = (interval*) i2->Data();

                // make sure I2 is invertible
                if(intervalContainsZero(I2)) {
                    Werror("second interval contains zero");
                    return TRUE;
                }

                number invlo, invup;
                invlo = nInvers(I2->lower);
                invup = nInvers(I2->upper);

                // inverse interval
                interval *I2inv = new interval(invup, invlo);

                if (i1->Typ() == intervalID) {
                    interval *I1 = (interval*) i1->Data();
                    RES = intervalMultiply(I1, I2inv);
                    break;
                } else {
                    // i1 is not an interval
                    number n;
                    switch (i1->Typ()) {
                        case INT_CMD:
                        {
                            n = nInit((int)(long) i1->Data());
                            break;
                        }
                        case NUMBER_CMD:
                        {
                            n = nCopy((number) i1->Data());
                            break;
                        }
                        default:
                        {
                            Werror("first argument not int/number/interval");
                            return TRUE;
                        }
                    }
                    RES = intervalScalarMultiply(n, I2inv);
                    nDelete(&n);
                }

                intervalDelete(I2inv);
                break;
            } else {
                // i2 is not an interval
                interval *I1 = (interval*) i1->Data();
                number n;
                switch(i2->Typ()) {
                    case INT_CMD:
                        { n = nInit((int)(long) i2->Data()); break; }
                    case NUMBER_CMD:
                        { n = nCopy((number) i2->Data()); break; }
                    default:
                    {
                        Werror("second argument not int/number/interval");
                        return TRUE;
                    }
                }
                number nInv = nInvers(n);
                nDelete(&n);
                RES = intervalScalarMultiply(nInv, I1);
                nDelete(&nInv);

                break;
            }

            break;
        }
        case '^':
        {
            if (i1->Typ() != intervalID || i2->Typ() != INT_CMD) {
                Werror("syntax: <interval> ^ <int>");
                return TRUE;
            }
            int p = (int)(long) i2->Data();
            if (p < 0) {
                Werror("<interval> ^ n not implemented for n < 0");
                return TRUE;
            }
            interval *I = (interval*) i1->Data();

            // initialise to be sure
            number lo = nInit(0), up = nInit(0);

            // TODO rewrite conditions to be more intuitive
            if (p % 2 == 1 || p == 0 || !intervalContainsZero(I) ||
                    nIsZero(I->lower) || nIsZero(I->upper)) {
                nPower(I->lower, p, &lo);
                nPower(I->upper, p, &up);
            } else {
                lo = nInit(0);
                if (nGreater(I->lower, I->upper)) {
                    nPower(I->lower, p, &up);
                } else {
                    nPower(I->upper, p, &up);
                }
            }
            RES = new interval(lo, up);
            break;
        }
        case EQUAL_EQUAL:
        {
            if (i1->Typ() != intervalID || i2->Typ() != intervalID) {
                Werror("syntax: <interval> == <interval>");
                return TRUE;
            }
            interval *I1, *I2;
            I1 = (interval*) i1->Data();
            I2 = (interval*) i2->Data();

            bool isEq;
            isEq = nEqual(I1->lower, I2->lower) && nEqual(I1->upper, I2->upper);
            result->rtyp = INT_CMD;
            result->data = (void*) isEq;
            return FALSE;
        }
        case '[':
        {
            if (i1->Typ() != intervalID || i2->Typ() != INT_CMD) {
                Werror("syntax: <interval>[<int>]");
                return TRUE;
            }
            interval *I = (interval*) i1->Data();
            int n = (int)(long) i2->Data();
            number out;
            if (n == 1) {
                out = nCopy(I->lower);
                result->rtyp = NUMBER_CMD;
                result->data = (void*) out;
                return FALSE;
            }
            if (n == 2) {
                out = nCopy(I->upper);
                result->rtyp = NUMBER_CMD;
                result->data = (void*) out;
                return FALSE;
            }

            Werror("Allowed indices are 1 and 2");
            return TRUE;
        }
        default:
        {
            // use default error
            return blackboxDefaultOp2(op, result, i1, i2);
        }
    }

    nNormalize(RES->lower);
    nNormalize(RES->upper);


    result->rtyp = intervalID;
    result->data = (void*) RES;
    return FALSE;
}

extern "C" int mod_init(SModulFunctions* psModulFunctions) {
    blackbox *b = (blackbox*)omAlloc0(sizeof(blackbox));

    b->blackbox_Init    = interval_Init;
    b->blackbox_String  = interval_String;
    b->blackbox_Copy    = interval_Copy;
    b->blackbox_destroy = interval_Destroy;
    b->blackbox_Assign  = interval_Assign;
    b->blackbox_Op2     = interval_Op2;

    intervalID = setBlackboxStuff(b, "interval");
    // debug
    Print("Created type interval with id %d\n", intervalID);

    psModulFunctions->iiAddCproc("interval.lib", "bounds", FALSE, bounds);

    return MAX_TOK;
}

// vim: spell spelllang=en
