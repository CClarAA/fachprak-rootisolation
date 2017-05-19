#include "interval.h"

// type ID
int intervalID;
int boxID;

/*
 * INTERVAL FUNCTIONS
 */

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
    interval *inew = new interval(*i);

    return (void*) inew;
}

// destroy interval
void interval_Destroy(blackbox *b, void *d) {
    delete (interval*) d;
}

// assigning values to intervals
BOOLEAN interval_Assign(leftv result, leftv args) {
    assume(result->Typ() == intervalID);
    interval *RES;

    // destroy data of result if it exists
    if (result != NULL && result->Data() != NULL) {
        // "I=I" (same pointers)
        if (result->Data() == args->Data()) {
            return FALSE;
        }
        delete (interval*) result->Data();
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

// alias to interval_Assign, used in interval.lib
BOOLEAN bounds(leftv result, leftv args) {
    return interval_Assign(result, args);
}

BOOLEAN length(leftv result, leftv arg) {
    if (arg != NULL && arg->Typ() == intervalID) {
        if (result != NULL || result->Data() != NULL) {
            number r = (number) result->Data();
            nDelete(&r);
        }

        interval *I = (interval*) arg->Data();
        result->rtyp = NUMBER_CMD;
        result->data = (void*) nSub(I->upper, I->lower);
        return FALSE;
    }

    Werror("syntax: length(<interval>)");
    return TRUE;
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

    nNormalize(lo);
    nNormalize(up);

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

    nNormalize(lo);
    nNormalize(up);

    return new interval(lo, up);
}

interval* intervalAdd(interval *I, interval *J) {
    number lo, up;
    lo = nAdd(I->lower, J->lower);
    up = nAdd(I->upper, J->upper);

    nNormalize(lo);
    nNormalize(up);

    return new interval(lo, up);
}

interval* intervalSubtract(interval *I, interval *J) {
    number lo, up;
    lo = nSub(I->lower, J->upper);
    up = nSub(I->upper, J->lower);

    nNormalize(lo);
    nNormalize(up);

    return new interval(lo, up);
}

bool intervalEqual(interval *I, interval *J) {
    return nEqual(I->lower, J->lower) && nEqual(I->upper, J->upper);
}

// ckeck if zero is contained in an interval
bool intervalContainsZero(interval *I) {
    number n = nMult(I->lower, I->upper);
    bool result = !nGreaterZero(n);
    // delete helper number
    nDelete(&n);

    return result;
}

interval* intervalPower(interval *I, int p) {
    number lo = nInit(0), up = nInit(0);

    nPower(I->lower, p, &lo);
    nPower(I->upper, p, &up);

    // should work now
    if (p % 2 == 1) {
        return new interval(lo, up);
    } else if (p == 0) {
        nDelete(&lo);
        nDelete(&up);
        return new interval(nInit(1));
    } else {
        number minn, maxn;
        if (nGreater(lo, up)) {
            minn = up;
            maxn = lo;
        } else {
            minn = lo;
            maxn = up;
        }

        if (intervalContainsZero(I)) {
            nDelete(&minn);
            return new interval(nInit(0), maxn);
        } else {
            return new interval(minn, maxn);
        }
    }
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
    if (result->Data() != NULL) {
        delete (interval*) result->Data();
    }

    switch(op) {
        case '+':
        {
            if (i1->Typ() != intervalID || i2->Typ() != intervalID) {
                Werror("syntax: <interval> + <interval>");
                return TRUE;
            }
            interval *I1, *I2;
            I1 = (interval*) i1->Data();
            I2 = (interval*) i2->Data();

            RES = intervalAdd(I1, I2);
            break;
        }
        case '-':
        {
            if (i1->Typ() != intervalID || i2->Typ() != intervalID) {
                Werror("syntax: <interval> - <interval>");
                return TRUE;
            }
            interval *I1, *I2;
            I1 = (interval*) i1->Data();
            I2 = (interval*) i2->Data();

            RES = intervalSubtract(I1, I2);
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

                delete I2inv;
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

            RES = intervalPower(I, p);
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

            bool isEq = intervalEqual(I1, I2);
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

    result->rtyp = intervalID;
    result->data = (void*) RES;
    return FALSE;
}

/*
 * BOX FUNCTIONS
 */

void* box_Init(blackbox *b) {
    box *B = new box();
    return (void*) B;
}

void* box_Copy(blackbox *b, void *d) {
    box *B = (box*) d;
    return (void*) new box(*B);
}

void box_Destroy(blackbox *b, void *d) {
    delete (box*) d;
}

char* box_String(blackbox *b, void *d) {
    blackbox *b_i = getBlackboxStuff(intervalID);
    int i, n = currRing->N;
    box *B = (box*) d;

    if (B == NULL || B->intervals == NULL) {
        return omStrDup("ooo");
    }

    StringSetS(interval_String(b_i, (void*) B->intervals[0]));

    for (i = 1; i < n; i++) {
        // interpret box as cartesian product, hence use " x "
        StringAppendS(" x ");
        StringAppendS(interval_String(b_i, (void*) B->intervals[i]));
    }
    return StringEndS();
}

// assigning values to intervals
BOOLEAN box_Assign(leftv result, leftv args) {
    assume(result->Typ() == boxID);
    box *RES;

    // destroy data of result if it exists
    if (result != NULL && result->Data() != NULL && result->Typ() == boxID) {
        // "B=B" (same pointers)
        if (result->Data() == args->Data()) {
            return FALSE;
        }
        delete (box*) result->Data();
    }

    /*
     * Allow assignments of the form
     *
     *      B = C,
     *      B = l,
     *
     * where B, C boxes, l list of intervals
     */

    if (args->Typ() == boxID) {
        box *B = (box*) args->Data();
        RES = new box(*B);
    } else if (args->Typ() == LIST_CMD) {
        RES = new box();
        lists l = (lists) args->Data();

        int i, m = lSize(l), n = currRing->N;
        // minimum
        int M = m > n ? n : m;

        for (i = 0; i <= M; i++) {
            if (l->m[i].Typ() != intervalID) {
                Werror("list contains non-intervals");
                return TRUE;
            }
            // delete interval before overwriting it.
            delete RES->intervals[i];
            RES->intervals[i] = new interval(*((interval*) l->m[i].Data()));
        }
    } else {
        Werror("Input not supported: first argument not box, list, or interval");
        return TRUE;
    }

    if (result->rtyp == IDHDL) {
        IDDATA((idhdl)result->data) = (char*) RES;
    } else {
        result->rtyp = boxID;
        result->data = (void*) RES;
    }

    return FALSE;
}

BOOLEAN box_Op2(int op, leftv result, leftv b1, leftv b2) {
    if (b1 == NULL || b1->Typ() != boxID) {
        Werror("first argument is not box but type(%d), second is type(%d)", b1->Typ(), b2->Typ());
        return TRUE;
    }

    box *B1 = (box*) b1->Data();
    int n = currRing->N;

    box *RES;
    switch(op) {
        case '[':
        {
            if (b2 == NULL || b2->Typ() != INT_CMD) {
                Werror("second argument not int");
                return TRUE;
            }
            if (result->Data() != NULL) {
                delete (interval*) result->Data();
            }

            int i = (int)(long) b2->Data();

            if (i < 1 || i > n) {
                Werror("index out of bounds");
                return TRUE;
            }

            result->rtyp = intervalID;
            result->data = (void*) new interval(*(B1->intervals[i-1]));
            return FALSE;
        }
        case '-':
        {
            if (b2 == NULL || b2->Typ() != boxID) {
                Werror("second argument not box");
            }
            if (result->Data() != NULL) {
                delete (box*) result->Data();
            }

            box *B2 = (box*) b2->Data();
            RES = new box();
            int i;
            for (i = 0; i < n; i++) {
                delete RES->intervals[i];
                RES->intervals[i] = intervalSubtract(B1->intervals[i], B2->intervals[i]);
            }

            result->rtyp = boxID;
            result->data = (void*) RES;
            return FALSE;
        }
        case EQUAL_EQUAL:
        {
            if (b2 == NULL || b2->Typ() != boxID) {
                Werror("second argument not box");
            }
            box *B2 = (box*) b2->Data();
            int i;
            bool res = true;
            for (i = 0; i < n; i++) {
                if (!intervalEqual(B1->intervals[i], B2->intervals[i])) {
                    res = false;
                    break;
                }
            }

            result->rtyp = INT_CMD;
            result->data = (void*) res;
            return FALSE;
        }
        default:
            return blackboxDefaultOp2(op, result, b1, b2);
    }
}

BOOLEAN box_OpM(int op, leftv result, leftv args) {
    switch(op) {
        case INTERSECT_CMD:
        {
            if (result->Data() != NULL && result->Typ() == boxID) {
                delete (box*) result->Data();
            }

            int i, n = currRing->N;
            if (args->Typ() != boxID) {
                Werror("can only intersect boxes");
                return TRUE;
            }
            box *B = (box*) args->Data();
            number lowerb[n], upperb[n];

            // do not copy, use same pointers, copy at the end
            for (i = 0; i < n; i++) {
                lowerb[i] = B->intervals[i]->lower;
                upperb[i] = B->intervals[i]->upper;
            }

            args = args->next;
            while(args != NULL) {
                if (args->Typ() != boxID) {
                    Werror("can only intersect boxes");
                    return TRUE;
                }

                B = (box*) args->Data();
                for (i = 0; i < n; i++) {
                    if (nGreater(B->intervals[i]->lower, lowerb[i])) {
                        lowerb[i] = B->intervals[i]->lower;
                    }
                    if (nGreater(upperb[i], B->intervals[i]->upper)) {
                        upperb[i] = B->intervals[i]->upper;
                    }

                    if (nGreater(lowerb[i], upperb[i])) {
                        result->rtyp = INT_CMD;
                        result->data = (void*) (-1);
                        return FALSE;
                    }
                }
                args = args->next;
            }

            // now copy the numbers
            box *RES = new box();
            for (i = 0; i < n; i++) {
                delete RES->intervals[i];
                RES->intervals[i] = new interval(nCopy(lowerb[i]), nCopy(upperb[i]));
            }

            result->rtyp = boxID;
            result->data = (void*) RES;
            return FALSE;
        }
        default:
            return blackboxDefaultOpM(op, result, args);
    }
}

BOOLEAN boxSet(leftv result, leftv args) {
    assume(result->Typ() == boxID);
    if (result != NULL || result->Data() != NULL) {
        delete (box*) result->Data();
    }

    if (args == NULL || args->Typ() != boxID ||
            args->next == NULL || args->next->Typ() != INT_CMD ||
            args->next->next == NULL ||
            args->next->next->Typ() != intervalID) {
        Werror("syntax: boxSet(<box>, <int>, <interval>)");
        return TRUE;
    }
    int n = currRing->N;
    box *B = (box*) args->Data();
    int i = (int)(long) args->next->Data();
    interval *I = (interval*) args->next->next->Data();

    if (i < 1 || i > n) {
        Werror("index out of range");
        return TRUE;
    }

    box *RES = new box(*B);

    // replace interval, free space
    delete RES->intervals[i-1];
    RES->intervals[i-1] = new interval(*I);

    result->rtyp = boxID;
    result->data = (void*) RES;
    return FALSE;
}

/*
 * POLY FUNCTIONS
 */

BOOLEAN evalPolyAtBox(leftv result, leftv args) {
    assume(result->Typ() == intervalID);
    if (result->Data() != NULL) {
        delete (box*) result->Data();
    }

    if (args == NULL || args->Typ() != POLY_CMD ||
            args->next == NULL || args->next->Typ() != boxID) {
        Werror("syntax: evalPolyAtBox(<poly>, <box>)");
        return TRUE;
    }

    int i, pot, n = currRing->N;
    poly p = (poly) args->Data();
    box *B = (box*) args->next->Data();

    interval *tmp, *tmpPot, *tmpMonom, *RES = new interval();

    while(p != NULL) {
        tmpMonom = new interval(nInit(1));

        for (i = 1; i <= n; i++) {
            pot = pGetExp(p, i);

            tmpPot = intervalPower(B->intervals[i-1], pot);
            tmp = intervalMultiply(tmpMonom, tmpPot);

            delete tmpMonom;
            delete tmpPot;

            tmpMonom = tmp;
        }

        tmp = intervalScalarMultiply(p->coef, tmpMonom);
        delete tmpMonom;
        tmpMonom = tmp;

        tmp = intervalAdd(RES, tmpMonom);
        delete RES;
        delete tmpMonom;

        RES = tmp;

        p = p->next;
    }

    result->rtyp = intervalID;
    result->data = (void*) RES;
    return FALSE;
}

/*
 * INIT MODULE
 */

extern "C" int mod_init(SModulFunctions* psModulFunctions) {
    blackbox *b_iv = (blackbox*)omAlloc0(sizeof(blackbox));
    blackbox *b_bx = (blackbox*)omAlloc0(sizeof(blackbox));

    b_iv->blackbox_Init    = interval_Init;
    b_iv->blackbox_Copy    = interval_Copy;
    b_iv->blackbox_destroy = interval_Destroy;
    b_iv->blackbox_String  = interval_String;
    b_iv->blackbox_Assign  = interval_Assign;
    b_iv->blackbox_Op2     = interval_Op2;

    intervalID = setBlackboxStuff(b_iv, "interval");

    b_bx->blackbox_Init    = box_Init;
    b_bx->blackbox_Copy    = box_Copy;
    b_bx->blackbox_destroy = box_Destroy;
    b_bx->blackbox_String  = box_String;
    b_bx->blackbox_Assign  = box_Assign;
    b_bx->blackbox_Op2     = box_Op2;
    b_bx->blackbox_OpM     = box_OpM;

    boxID = setBlackboxStuff(b_bx, "box");

    // debug
    Print("Created type interval with id %d\n", intervalID);
    Print("Created type box with id %d\n", boxID);

    // add additional functions
    psModulFunctions->iiAddCproc("interval.lib", "bounds", FALSE, bounds);
    psModulFunctions->iiAddCproc("interval.lib", "length", FALSE, length);
    psModulFunctions->iiAddCproc("interval.lib", "boxSet", FALSE, boxSet);
    psModulFunctions->iiAddCproc("interval.lib", "evalPolyAtBox", FALSE, evalPolyAtBox);

    return MAX_TOK;
}

// vim: spell spelllang=en
