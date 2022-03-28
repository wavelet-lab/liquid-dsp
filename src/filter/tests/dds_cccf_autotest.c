/*
 * Copyright (c) 2007 - 2022 Joseph Gaeddert
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

#include "autotest/autotest.h"
#include "liquid.internal.h"

// check direct digital synthesis, both interpolator and decimator
void testbench_dds_cccf(unsigned int _num_stages,   // number of half-band stages
                        float        _fc,           // filter cut-off
                        float        _As)           // stop-band suppression
{
    float        tol  = 1;  // error tolerance [dB], yes, that's dB
    float        bw = 0.1f; // original pulse bandwidth
    unsigned int m    = 40; // pulse semi-length
    unsigned int r=1<<_num_stages;   // resampling rate (output/input)

    // create resampler
    dds_cccf q = dds_cccf_create(_num_stages,_fc,bw,_As);
    dds_cccf_set_scale(q, 1.0f/r);
    if (liquid_autotest_verbose)
        dds_cccf_print(q);

    unsigned int delay_interp = dds_cccf_get_delay_interp(q);
    float        delay_decim  = dds_cccf_get_delay_decim (q);
    unsigned int h_len = 2*r*m+1; // pulse length
    unsigned int num_samples = h_len + delay_interp + (unsigned int) delay_decim + 8;

    unsigned int i;
    float complex buf_0[num_samples  ]; // input
    float complex buf_1[num_samples*r]; // interpolated
    float complex buf_2[num_samples  ]; // decimated

    // generate the baseband signal (filter pulse)
    float h[h_len];
    float w = 0.36f * bw; // pulse bandwidth
    liquid_firdes_kaiser(h_len,w,_As,0.0f,h);
    for (i=0; i<num_samples; i++)
        buf_0[i] = i < h_len ? 2*w*h[i] : 0.0f;

    // run interpolation (up-conversion) stage
    for (i=0; i<num_samples; i++)
        dds_cccf_interp_execute(q, buf_0[i], &buf_1[r*i]);

    // clear DDS object
    dds_cccf_reset(q);
    dds_cccf_set_scale(q, r);

    // run decimation (down-conversion) stage
    for (i=0; i<num_samples; i++)
        dds_cccf_decim_execute(q, &buf_1[r*i], &buf_2[i]);

    // verify input spectrum
    autotest_psd_s regions_orig[] = {
      {.fmin=-0.5,    .fmax=-0.6*bw, .pmin= 0, .pmax=-_As+tol, .test_lo=0, .test_hi=1},
      {.fmin=-0.3*bw, .fmax=+0.3*bw, .pmin=-3, .pmax=+1,       .test_lo=1, .test_hi=1},
      {.fmin=+0.6*bw, .fmax=+0.5,    .pmin= 0, .pmax=-_As+tol, .test_lo=0, .test_hi=1},
    };
    liquid_autotest_validate_psd_signal(buf_0, num_samples, regions_orig, 3,
        liquid_autotest_verbose ? "autotest_dds_cccf_orig.m" : NULL);

    // verify interpolated spectrum
    float f1 = _fc-0.6*bw/r, f2 = _fc-0.3*bw/r, f3 = _fc+0.3*bw/r, f4 = _fc+0.6*bw/r;
    autotest_psd_s regions_interp[] = {
      {.fmin=-0.5, .fmax=f1,   .pmin= 0, .pmax=-_As+tol, .test_lo=0, .test_hi=1},
      {.fmin= f2,  .fmax=f3,   .pmin=-3, .pmax=+1,       .test_lo=1, .test_hi=1},
      {.fmin= f4,  .fmax=+0.5, .pmin= 0, .pmax=-_As+tol, .test_lo=0, .test_hi=1},
    };
    liquid_autotest_validate_psd_signal(buf_1, r*num_samples, regions_interp, 3,
        liquid_autotest_verbose ? "autotest_dds_cccf_interp.m" : NULL);

    // verify decimated spectrum (using same regions as original)
    liquid_autotest_validate_psd_signal(buf_2, num_samples, regions_orig, 3,
        liquid_autotest_verbose ? "autotest_dds_cccf_decim.m" : NULL);

    // destroy filter object
    dds_cccf_destroy(q);
}

// test different configurations
void autotest_dds_cccf_0(){ testbench_dds_cccf( 1, +0.0f, 60.0f); }

