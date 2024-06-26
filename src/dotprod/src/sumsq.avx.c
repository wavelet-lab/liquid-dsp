/*
 * Copyright (c) 2007 - 2015 Joseph Gaeddert
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

//
// sumsq.mmx.c : floating-point sum of squares (MMX)
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "liquid.internal.h"

// include proper SIMD extensions for x86 platforms
// NOTE: these pre-processor macros are defined in config.h

#if HAVE_MMX
#include <mmintrin.h>   // MMX
#endif

#if HAVE_SSE
#include <xmmintrin.h>  // SSE
#endif

#if HAVE_SSE2
#include <emmintrin.h>  // SSE2
#endif

#if HAVE_SSE3
#include <pmmintrin.h>  // SSE3
#endif

#if HAVE_AVX
#include <immintrin.h>  // AVX
#endif

// sum squares, basic loop
//  _v      :   input array [size: 1 x _n]
//  _n      :   input length
float liquid_sumsqf(float *      _v,
                    unsigned int _n)
{
    // first cut: ...
    __m256 v;   // input vector
    __m256 s;   // dot product
    __m256 sum = _mm256_setzero_ps(); // load zeros into sum register

    // t = 8*(floor(_n/8))
    unsigned int t = (_n >> 3) << 3;

    //
    unsigned int i;
    for (i=0; i<t; i+=8) {
        // load inputs into register (unaligned)
        v = _mm256_loadu_ps(&_v[i]);

        // compute multiplication
        s = _mm256_mul_ps(v, v);
       
        // parallel addition
        sum = _mm256_add_ps( sum, s );
    }

    // aligned output array
    float total;

    // fold down into single value
    __m256 z = _mm256_setzero_ps();
    sum = _mm256_hadd_ps(sum, z);
    sum = _mm256_hadd_ps(sum, z);
    __m128 ssum = _mm_add_ps(_mm256_extractf128_ps(sum, 0), _mm256_extractf128_ps(sum, 1));

    // unload single (lower value)
    _mm_store_ss(&total, ssum);

    // cleanup
    for (; i<_n; i++)
        total += _v[i] * _v[i];

    // set return value
    return total;
}

// sum squares, basic loop
//  _v      :   input array [size: 1 x _n]
//  _n      :   input length
float liquid_sumsqcf(float complex * _v,
                     unsigned int    _n)
{
    // simple method: type cast input as real pointer, run double
    // length sumsqf method
    float * v = (float*) _v;
    return liquid_sumsqf(v, 2*_n);
}
