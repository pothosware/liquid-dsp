/*
 * Copyright (c) 2007, 2009 Joseph Gaeddert
 * Copyright (c) 2007, 2009 Virginia Polytechnic Institute & State University
 *
 * This file is part of liquid.
 *
 * liquid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * liquid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with liquid.  If not, see <http://www.gnu.org/licenses/>.
 */

//
// P/N synchronizer
//

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

//#include "liquid.h"

struct PNSYNC(_s) {
    unsigned int n; // sequence length
    WINDOW() sym;   // received symbols
    WINDOW() sym2;  // received symbols squared
    DOTPROD() dp;   // dot product
    TO rxy;         // cross correlation
};

PNSYNC() PNSYNC(_create)(unsigned int _n, TC * _v)
{
    PNSYNC() fs = (PNSYNC()) malloc(sizeof(struct PNSYNC(_s)));
    fs->n = _n;

    fs->sym  = WINDOW(_create)(fs->n);
    fs->sym2 = WINDOW(_create)(fs->n);

    TC h[fs->n];
    memmove(h, _v, (fs->n)*sizeof(TC));

    // compute signal energy and normalize
    unsigned int i;
    float e=0.0f;
    for (i=0; i<fs->n; i++)
        e += ABS(h[i]) * ABS(h[i]);

    e = sqrtf(e/(fs->n));

    for (i=0; i<fs->n; i++)
        h[i] /= e;

    fs->dp = DOTPROD(_create)(h,fs->n);

    return fs;
}

// TODO : test this method
PNSYNC() PNSYNC(_create_msequence)(unsigned int _g)
{
    unsigned int m = liquid_msb_index(_g) - 1;

    // create/initialize msequence
    msequence ms = msequence_create(m);
    msequence_init(ms, m, _g, 1);

    PNSYNC() fs = (PNSYNC()) malloc(sizeof(struct PNSYNC(_s)));
    fs->n = msequence_get_length(ms);

    fs->sym  = WINDOW(_create)(fs->n);
    fs->sym2 = WINDOW(_create)(fs->n);

    TC h[fs->n];

    unsigned int i;
    for (i=0; i<fs->n; i++)
        h[i] = msequence_advance(ms) ? 1.0f : -1.0f;

    msequence_destroy(ms);

    fs->dp = DOTPROD(_create)(h,fs->n);

    return fs;
}

void PNSYNC(_destroy)(PNSYNC() _fs)
{
    WINDOW(_destroy)(_fs->sym);
    WINDOW(_destroy)(_fs->sym2);
    DOTPROD(_destroy)(_fs->dp);
    free(_fs);
}

void PNSYNC(_print)(PNSYNC() _fs)
{

}

void PNSYNC(_correlate)(PNSYNC() _fs, TI _sym, TO *_y)
{
    // push symbol into buffers
    WINDOW(_push)(_fs->sym,  _sym);
    WINDOW(_push)(_fs->sym2, ABS(_sym)*ABS(_sym));

    // compute dotprod, energy
    TI * r2;
    unsigned int i;
    WINDOW(_read)(_fs->sym2, &r2);
    float e=0.0f;
    for (i=0; i<_fs->n; i++)
        e += r2[i];
    e /= _fs->n;

    TI * r;
    WINDOW(_read)(_fs->sym, &r);
    DOTPROD(_execute)(_fs->dp, r, &(_fs->rxy));
    _fs->rxy /= (sqrtf(e) * _fs->n);

    *_y = _fs->rxy;
}

