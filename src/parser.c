#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 1024

static char *trim_whitespace(char *str) {
    char *end;

    // Trim leading space
    while(isspace((unsigned char)*str)) str++;

    if(*str == 0)  // All spaces?
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator character
    *(end+1) = 0;

    return str;
}

static int parse_fill_pattern(const char *value, Config *config) {
    if (strcmp(value, "RANDOM") == 0) {
        config->fill_type = FILL_PATTERN_RANDOM;
    } else if (strncmp(value, "0x", 2) == 0 && strlen(value) == 4) {
        // Hex byte
        unsigned int byte_val;
        if (sscanf(value, "0x%x", &byte_val) == 1) {
            config->fill_type = FILL_PATTERN_HEX;
            config->single_byte_fill = (unsigned char)byte_val;
        } else {
            return -1; // Parse error
        }
    } else {
        // String pattern
        config->fill_type = FILL_PATTERN_STRING;
        config->string_pattern = strdup(value);
    }
    return 0;
}

int parse_definition(const char *filename, Config *config) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening definition file");
        return 1;
    }

    char line[MAX_LINE_LENGTH];
    int line_num = 0;

    // Set defaults that might not be set by memset
    config->newline = NEWLINE_LF; 
    config->rdw_endian = RDW_ENDIAN_BIG;
    config->text_type = TEXT_TYPE_ASCII;
    // Default fill pattern is context dependent (space for text, null for binary)
    // We will set a marker to know if it was set by user, or handle in generator.
    // For now, let's say default is handled in generator if fill_type is 0 and string_pattern is NULL.
    // Actually, 0 is FILL_PATTERN_STRING. Let's rely on string_pattern being NULL.
    
    while (fgets(line, sizeof(line), file)) {
        line_num++;
        char *l = trim_whitespace(line);

        if (strlen(l) == 0 || l[0] == '#') {
            continue;
        }

        char *delimiter = strchr(l, '=');
        if (!delimiter) {
            fprintf(stderr, "Syntax error at line %d: Missing '='\n", line_num);
            fclose(file);
            return 1;
        }

        *delimiter = '\0';
        char *key = trim_whitespace(l);
        char *value = trim_whitespace(delimiter + 1);

        if (strcmp(key, "FILE_TYPE") == 0) {
            if (strcmp(value, "FIXED") == 0) config->file_type = FILE_TYPE_FIXED;
            else if (strcmp(value, "VARIABLE") == 0) config->file_type = FILE_TYPE_VARIABLE;
            else { fprintf(stderr, "Invalid FILE_TYPE at line %d: %s\n", line_num, value); fclose(file); return 1; }
        } else if (strcmp(key, "RECORD_LENGTH") == 0) {
            config->record_length = atoi(value);
            if (config->record_length <= 0) { fprintf(stderr, "Invalid RECORD_LENGTH at line %d\n", line_num); fclose(file); return 1; }
        } else if (strcmp(key, "DATA_MODE") == 0) {
            if (strcmp(value, "TEXT") == 0) config->data_mode = DATA_MODE_TEXT;
            else if (strcmp(value, "BINARY") == 0) config->data_mode = DATA_MODE_BINARY;
            else { fprintf(stderr, "Invalid DATA_MODE at line %d: %s\n", line_num, value); fclose(file); return 1; }
        } else if (strcmp(key, "NEWLINE") == 0) {
            if (strcmp(value, "LF") == 0) config->newline = NEWLINE_LF;
            else if (strcmp(value, "CRLF") == 0) config->newline = NEWLINE_CRLF;
            else if (strcmp(value, "CR") == 0) config->newline = NEWLINE_CR;
            else if (strcmp(value, "NONE") == 0) config->newline = NEWLINE_NONE;
            else { fprintf(stderr, "Invalid NEWLINE at line %d: %s\n", line_num, value); fclose(file); return 1; }
        } else if (strcmp(key, "RECORD_COUNT") == 0) {
            config->record_count = atoi(value);
            if (config->record_count <= 0) { fprintf(stderr, "Invalid RECORD_COUNT at line %d\n", line_num); fclose(file); return 1; }
        } else if (strcmp(key, "RDW_ENDIAN") == 0) {
            if (strcmp(value, "BIG") == 0) config->rdw_endian = RDW_ENDIAN_BIG;
            else if (strcmp(value, "LITTLE") == 0) config->rdw_endian = RDW_ENDIAN_LITTLE;
            else { fprintf(stderr, "Invalid RDW_ENDIAN at line %d: %s\n", line_num, value); fclose(file); return 1; }
        } else if (strcmp(key, "FILL_PATTERN") == 0) {
            if (parse_fill_pattern(value, config) != 0) {
                fprintf(stderr, "Invalid FILL_PATTERN at line %d: %s\n", line_num, value);
                fclose(file);
                return 1;
            }
        } else if (strcmp(key, "TEXT_TYPE") == 0) {
            if (strcmp(value, "ASCII") == 0) config->text_type = TEXT_TYPE_ASCII;
            else if (strcmp(value, "JAPANESE") == 0) config->text_type = TEXT_TYPE_JAPANESE;
            else { fprintf(stderr, "Invalid TEXT_TYPE at line %d: %s\n", line_num, value); fclose(file); return 1; }
        } else {
            fprintf(stderr, "Unknown key at line %d: %s\n", line_num, key);
            // Optionally ignore unknown keys or error out. For now, strict.
             fclose(file); return 1; 
        }
    }

    fclose(file);
    return 0;
}

void free_config(Config *config) {
    if (config->string_pattern) {
        free(config->string_pattern);
        config->string_pattern = NULL;
    }
}
