#ifndef PARSER_H
#define PARSER_H

#include "config.h"

// Parses the definition file and populates the Config struct.
// Returns 0 on success, non-zero on error.
int parse_definition(const char *filename, Config *config);

// Frees any allocated memory in the Config struct (e.g., string pattern).
void free_config(Config *config);

#endif // PARSER_H
