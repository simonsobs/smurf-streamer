#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "lo_pass.h"

int main(int argc, char **argv) {

    filtpar *pars = mce_filter();

    if (argc != 5) {
        printf("This program requires exactly 4 arguments.\n");
        printf("  n_chan  n_samp  input_file output_file\n");
        return 1;
    }

    /* Load the input data */
    int n_chan = atoi(argv[1]);
    int n_samp = atoi(argv[2]);
    FILE *fin = fopen(argv[3], "r");

    int32_t *buf0 = calloc(n_samp*n_chan, sizeof(*buf0));
    int32_t *buf1 = calloc(n_samp*n_chan, sizeof(*buf1));

    fread(buf0, sizeof(*buf0), n_samp*n_chan, fin);

    /* Create filter */
    filtbank *bank1 = create_filtbank(n_chan, pars+0);
    filtbank *bank2 = create_filtbank(n_chan, pars+1);

    int32_t *output;
    if (0) {
        filter_data(bank1, buf0, buf1, n_samp);
        filter_data(bank2, buf1, buf0, n_samp);
        output = buf0;
    } else {
        filtbank banks[2];
        memcpy(&banks[0], bank1, sizeof(*bank1));
        memcpy(&banks[1], bank2, sizeof(*bank2));
        multi_filter_data(banks, 2, buf0, buf1, n_samp);
        output = buf1;
    }

    /* Write output. */
    FILE *fout = fopen(argv[4], "w");
    fwrite(output, sizeof(*output), n_samp*n_chan, fout);
    fclose(fout);

    return 0;
}
