#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "lo_pass.h"

filtbank *create_filtbank(int n_chan, filtpar *par)
{
    filtbank *bank = (filtbank*)calloc(sizeof(*bank), 1);
    bank->n_chan = n_chan;
    bank->w = (int64_t*)calloc(n_chan*2, sizeof(*bank->w));
    bank->par = par;
    return bank;
}

void filter_data(filtbank *bank, int32_t *input, int32_t *output, int n_samp)
{
    const int32_t *b = bank->par->b;

#pragma omp parallel for shared(input, output)
    for (int ic=0; ic<bank->n_chan; ic++) {
        int64_t *w = bank->w + ic*2;
        int32_t *src = input  + ic*n_samp;
        int32_t *dst = output + ic*n_samp;
        for (int i=0; i<n_samp; i++) {
            int64_t c = (-w[1] * b[0] + w[0] * b[1]) >> bank->par->b_bits;
            int64_t W = (*(src++) << bank->par->p_bits) - c;
            int64_t y = w[0] + (w[1]<<1) + W;
            *(dst++) = y >> bank->par->shift;
            w[0] = w[1];
            w[1] = W;
        }
    }
}

void multi_filter_data(filtbank *banks, int n_bank, int32_t *input, int32_t *output, int n_samp)
{
//#pragma omp parallel for shared(input, output)
    for (int ic=0; ic<banks[0].n_chan; ic++) {
        int32_t *src = input  + ic*n_samp;
        int32_t *dst = output + ic*n_samp;
        for (int i=0; i<n_samp; i++) {
            int32_t x = *(src++);
            for (int ib=0; ib<n_bank; ib++) {
                const filtbank *bank = &banks[ib];
                const int32_t *b = bank->par->b;
                int64_t *w = bank->w + ic*2;
                int64_t c = (-w[1] * b[0] + w[0] * b[1]) >> bank->par->b_bits;
                int64_t W = (x << bank->par->p_bits) - c;
                w[0] = w[1];
                w[1] = W;
                x = (w[0] + (w[1]<<1) + W) >> bank->par->shift;
            }
            *(dst++) = x;
        }
    }
}

filtpar *mce_filter()
{
    filtpar *pars = (filtpar*)calloc(3, sizeof(*pars));
    filtpar p1 = {{32092, 15750}, 14, 3, 3+2+0 };
    filtpar p2 = {{31238, 14895}, 14, 3, 3+2+7};
    pars[0] = p1;
    pars[1] = p2;

    return pars;
}
