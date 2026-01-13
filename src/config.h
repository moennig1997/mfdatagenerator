#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>

typedef enum {
    FILE_TYPE_FIXED,
    FILE_TYPE_VARIABLE
} FileType;

typedef enum {
    DATA_MODE_TEXT,
    DATA_MODE_BINARY
} DataMode;

typedef enum {
    NEWLINE_LF,
    NEWLINE_CRLF,
    NEWLINE_CR,
    NEWLINE_NONE
} NewlineMode;

typedef enum {
    RDW_ENDIAN_BIG,
    RDW_ENDIAN_LITTLE
} RdwEndian;

typedef enum {
    FILL_PATTERN_STRING,
    FILL_PATTERN_HEX,    // For fixed single byte fill
    FILL_PATTERN_RANDOM
} FillPatternType;

typedef enum {
    TEXT_TYPE_ASCII,
    TEXT_TYPE_JAPANESE
} TextType;

typedef struct {
    FileType file_type;
    size_t record_length;
    DataMode data_mode;
    NewlineMode newline;
    size_t record_count;
    RdwEndian rdw_endian;
    
    FillPatternType fill_type;
    unsigned char single_byte_fill; // Used if fill_type == FILL_PATTERN_HEX
    char *string_pattern;           // Used if fill_type == FILL_PATTERN_STRING
    TextType text_type;             // Used if data_mode == DATA_MODE_TEXT && fill_type == FILL_PATTERN_RANDOM
} Config;

#endif // CONFIG_H
