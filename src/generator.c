#include "generator.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void fill_buffer(unsigned char *buffer, size_t length, const Config *config) {
    if (config->fill_type == FILL_PATTERN_RANDOM) {
        if (config->data_mode == DATA_MODE_TEXT) {
            if (config->text_type == TEXT_TYPE_JAPANESE) {
                size_t i = 0;
                while (i < length) {
                    // Decide if we want a 2-byte char or 1-byte char
                    // 70% chance of 2-byte if space allows
                    int make_multibyte = (length - i >= 2) && ((rand() % 100) < 70);
                    
                    if (make_multibyte) {
                        // Shift-JIS 2-byte construction
                        // We will select valid characters based on JIS X 0208 rows to avoid undefined areas.
                        // JIS X 0208 Allocation:
                        // Row 1: Symbols
                        // Row 4: Hiragana
                        // Row 5: Katakana
                        // Row 16-47: Level 1 Kanji
                        
                        int row, cell;
                        int type = rand() % 100;
                        
                        if (type < 10) { 
                            // Symbols (Row 1)
                            row = 1;
                            cell = (rand() % 60) + 1; // Limit to common symbols
                        } else if (type < 20) {
                            // Hiragana (Row 4)
                            row = 4;
                            cell = (rand() % 83) + 1;
                        } else if (type < 30) {
                            // Katakana (Row 5)
                            row = 5;
                            cell = (rand() % 86) + 1;
                        } else {
                            // Kanji Level 1 (Rows 16-47)
                            row = 16 + (rand() % 32);
                            if (row == 47) {
                                cell = (rand() % 51) + 1; // Row 47 ends at cell 51
                            } else {
                                cell = (rand() % 94) + 1;
                            }
                        }

                        // Convert JIS X 0208 (row, cell) to Shift-JIS
                        unsigned char b1, b2;
                        
                        // Calculate first byte
                        if (row <= 62) {
                            b1 = 0x81 + (row - 1) / 2;
                        } else {
                            b1 = 0xE0 + (row - 63) / 2;
                        }

                        // Calculate second byte
                        if (row % 2 != 0) {
                            // Odd row
                            b2 = 0x40 + (cell - 1);
                            if (b2 >= 0x7F) b2++;
                        } else {
                            // Even row
                            b2 = 0x9F + (cell - 1);
                        }
                        
                        buffer[i++] = b1;
                        buffer[i++] = b2;
                    } else {
                        // Random printable ASCII (0x20 - 0x7E)
                        buffer[i++] = (rand() % (0x7E - 0x20 + 1)) + 0x20;
                    }
                }
            } else {
                for (size_t i = 0; i < length; i++) {
                    // Random printable ASCII (0x20 - 0x7E)
                    buffer[i] = (rand() % (0x7E - 0x20 + 1)) + 0x20;
                }
            }
        } else {
            for (size_t i = 0; i < length; i++) {
                buffer[i] = rand() % 256;
            }
        }
    } else if (config->fill_type == FILL_PATTERN_HEX) {
        memset(buffer, config->single_byte_fill, length);
    } else if (config->fill_type == FILL_PATTERN_STRING && config->string_pattern) {
        size_t pat_len = strlen(config->string_pattern);
        if (pat_len > 0) {
            for (size_t i = 0; i < length; i++) {
                buffer[i] = config->string_pattern[i % pat_len];
            }
        } else {
            // Empty string pattern, fallback to default
            memset(buffer, (config->data_mode == DATA_MODE_TEXT) ? 0x20 : 0x00, length);
        }
    } else {
        // Default
        memset(buffer, (config->data_mode == DATA_MODE_TEXT) ? 0x20 : 0x00, length);
    }
}

static void write_rdw(FILE *out, size_t record_len, RdwEndian endian) {
    // RDW is 4 bytes: LL LL 00 00
    // LL is total length including RDW itself (record_len + 4)
    unsigned short total_len = (unsigned short)(record_len + 4);
    unsigned char rdw[4] = {0, 0, 0, 0};

    if (endian == RDW_ENDIAN_BIG) {
        rdw[0] = (total_len >> 8) & 0xFF;
        rdw[1] = total_len & 0xFF;
    } else {
        rdw[0] = total_len & 0xFF;
        rdw[1] = (total_len >> 8) & 0xFF;
    }
    // Bytes 2 and 3 are reserved (0x00)

    fwrite(rdw, 1, 4, out);
}

int generate_data(const Config *config, FILE *output_file) {
    unsigned char *buffer = malloc(config->record_length);
    if (!buffer) {
        perror("Error allocating memory for record buffer");
        return 1;
    }

    const char *newline_str = NULL;
    size_t newline_len = 0;

    if (config->data_mode == DATA_MODE_TEXT) {
        switch (config->newline) {
            case NEWLINE_LF: newline_str = "\n"; newline_len = 1; break;
            case NEWLINE_CRLF: newline_str = "\r\n"; newline_len = 2; break;
            case NEWLINE_CR: newline_str = "\r"; newline_len = 1; break;
            case NEWLINE_NONE: newline_str = NULL; newline_len = 0; break;
        }
    }

    // For fixed length, if there is a newline, it's appended.
    // For variable length, usually newlines are NOT part of the structure unless payload contains them.
    // Specification says: "Even in FIXED mode... newline is appended after RECORD_LENGTH data".
    // We will assume for Variable mode, we just write the payload inside RDW, potentially with newline if requested, 
    // but typically variable length records in mainframe don't have newlines delimiters.
    // However, if the user requested TEXT mode with NEWLINE, we should probably include it in the payload or append it?
    // Mainframe variable (RECFM=V) does NOT have delimiters. The RDW handles separation.
    // If the user asks for text with newline in variable mode, it's ambiguous.
    // I will implementation: 
    // FIXED: [DATA][NEWLINE]
    // VARIABLE: [RDW][DATA]  (Ignoring newline for Variable unless user strongly implies otherwise, but let's stick to standard RECFM=V)
    // Actually, if they want 'TEXT' mode and 'LF', they might expect standard unix text file behavior if they view it as stream.
    // BUT the spec says "RDW is 4 bytes...". That implies binary structure (RECFM=V).
    // Let's assume Newline is ONLY for FIXED text files to make them viewable, or if they explicitly want line breaks in the payload.
    // For VARIABLE, I will NOT append newline outside the record. If it's inside, it increases length.
    // Let's keep it simple: Newline is appended to the data payload if configured.
    // If Variable, the RDW length includes the data + newline?
    // Let's stick to: FIXED adds newline AFTER data. VARIABLE adds newline to DATA (so RDW covers it)?
    // The safest bet for "Mainframe" style is:
    // Fixed: Just data. (If newline requested, it's basically a Line Sequential file).
    // Variable: RDW + Data. No "newline" delimiter usually.
    // I will follow the spec: "Even in FIXED mode... the newline is appended...". 
    // For Variable, I will assume Newline is NOT appended unless part of payload, but let's just warn or ignore for now if unclear.
    // Let's assume for this tool, NEWLINE is a separator for FIXED. For VARIABLE, we just output RDW+DATA.
    
    for (size_t i = 0; i < config->record_count; i++) {
        size_t current_len = config->record_length;

        if (config->file_type == FILE_TYPE_VARIABLE) {
            // Random length between 1 and MAX (config->record_length)
            current_len = (rand() % config->record_length) + 1;
        }

        fill_buffer(buffer, current_len, config);

        if (config->file_type == FILE_TYPE_VARIABLE) {
            write_rdw(output_file, current_len, config->rdw_endian);
            fwrite(buffer, 1, current_len, output_file);
        } else {
            fwrite(buffer, 1, current_len, output_file);
            if (newline_str) {
                fwrite(newline_str, 1, newline_len, output_file);
            }
        }
    }

    free(buffer);
    return 0;
}
