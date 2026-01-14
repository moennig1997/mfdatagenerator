#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include "config.h"
#include "parser.h"
#include "generator.h"

void print_usage(const char *prog_name) {
    fprintf(stderr, "Usage: %s -d <definition_file> -o <output_file>\n", prog_name);
}

int main(int argc, char *argv[]) {
    int opt;
    char *def_file = NULL;
    char *out_file = NULL;

    while ((opt = getopt(argc, argv, "d:o:h")) != -1) {
        switch (opt) {
            case 'd':
                def_file = optarg;
                break;
            case 'o':
                out_file = optarg;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    if (!def_file || !out_file) {
        fprintf(stderr, "Error: Definition file and output file are required.\n");
        print_usage(argv[0]);
        return 1;
    }

    // Initialize random seed
    srand((unsigned int)time(NULL));

    Config config;
    memset(&config, 0, sizeof(Config));

    // Set defaults
    config.newline = NEWLINE_LF;
    config.rdw_endian = RDW_ENDIAN_BIG;
    // Default fill pattern logic will be handled if not overwritten by parser

    if (parse_definition(def_file, &config) != 0) {
        fprintf(stderr, "Error parsing definition file.\n");
        return 1;
    }

    FILE *f_out = fopen(out_file, "wb");
    if (!f_out) {
        fprintf(stderr, "DEBUG: trying to open '%s'\n", out_file);
        perror("Error opening output file");
        free_config(&config);
        return 1;
    }

    if (generate_data(&config, f_out) != 0) {
        fprintf(stderr, "Error generating data.\n");
        fclose(f_out);
        free_config(&config);
        return 1;
    }

    fclose(f_out);
    free_config(&config);
    printf("Data generation completed successfully.\n");
    return 0;
}
