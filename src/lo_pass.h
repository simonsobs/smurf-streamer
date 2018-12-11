#include <stdint.h>

typedef struct {
    int32_t b[2];
    int b_bits;
    int p_bits;
    int32_t shift;
} filtpar;

typedef struct {
    int n_chan;
    int64_t *w;  // (n_chan,2)
    filtpar *par;
} filtbank;

filtbank *create_filtbank(int n_chan, filtpar *par);
void filter_data(filtbank *bank, int32_t *input, int32_t *output, int n_samp);
filtpar *mce_filter();
void multi_filter_data(filtbank *banks, int n_bank, int32_t *input, int32_t *output, int n_samp);

