#ifndef GENERATOR_H
#define GENERATOR_H

#include <stdio.h>
#include "config.h"

// Generates data based on the configuration and writes it to the output file.
// Returns 0 on success, non-zero on error.
int generate_data(const Config *config, FILE *output_file);

#endif // GENERATOR_H
