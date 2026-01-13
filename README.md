# Mainframe Data Generator Tool

## Outline
This tool is a C-based command-line utility designed to generate test data for mainframe systems. It supports creating files with various characteristics suitable for mainframe environments, including fixed-length and variable-length records, EBCDIC-friendly structures (via binary mode), and specific layout controls like Record Descriptor Words (RDW).

The tool reads a definition file containing generation parameters and produces an output file with the generated data.

## Usage
```bash
./generator -d <definition_file> -o <output_file>
```

- `-d <definition_file>`: Path to the configuration file defining data characteristics.
- `-o <output_file>`: Path where the generated data will be saved.

## Parameter Specification
The definition file is a text file with `KEY=VALUE` pairs. Lines starting with `#` are comments.

| Key | Values | Description |
| :--- | :--- | :--- |
| `FILE_TYPE` | `FIXED` | Fixed-length records. |
| | `VARIABLE` | Variable-length records (random length up to `RECORD_LENGTH`). |
| `RECORD_LENGTH` | *Integer* | Maximum length of the record (data payload) in bytes. |
| `DATA_MODE` | `TEXT` | Generates text data. Padded with spaces. |
| | `BINARY` | Generates binary data. Padded with nulls (0x00). |
| `NEWLINE` | `LF`, `CRLF`, `CR` | Appends newline character(s) after each record (mainly for FIXED/TEXT mode). |
| | `NONE` | No newline delimiters. |
| `RECORD_COUNT` | *Integer* | Number of records to generate. |
| `RDW_ENDIAN` | `BIG` | Big-endian RDW (Mainframe standard). |
| | `LITTLE` | Little-endian RDW. |
| `FILL_PATTERN` | `RANDOM` | Random content based on `DATA_MODE` and `TEXT_TYPE`. |
| | `0xHH` | Fill with specific hex byte (e.g., `0x00`, `0xFF`). |
| | *String* | Fill with a specific looping string (e.g., `ABC`). |
| `TEXT_TYPE` | `ASCII` | Random printable ASCII characters. |
| | `JAPANESE` | Random Shift-JIS characters (including 2-byte Kanji/Hiragana/Katakana). |

## Output Specification

### Fixed Length (`FILE_TYPE=FIXED`)
Records are written sequentially with a constant size.
- **Structure**: `[DATA]` followed by `[NEWLINE]` (if configured).
- **Size**: `RECORD_LENGTH` (+ newline size).

### Variable Length (`FILE_TYPE=VARIABLE`)
Records are written with a 4-byte Record Descriptor Word (RDW) prefixing the data.
- **Structure**: `[RDW][DATA]`
- **RDW Format**: 4 bytes. `LL LL 00 00`
    - `LL LL`: Total record length (Data + 4 bytes for RDW) as a 16-bit integer.
    - Endianness is controlled by `RDW_ENDIAN`.
- **Data Size**: Randomly selected between 1 and `RECORD_LENGTH`.
- **Note**: Newlines differ from standard text files; usually, `NEWLINE` should be `NONE` for strict variable-length mainframe simulation unless specifically testing text parsing.

### Fill Patterns
- **Random ASCII**: Printable characters from 0x20 to 0x7E.
- **Random Japanese**: Valid Shift-JIS 2-byte characters (Hiragana, Katakana, Kanji Level 1) interspersed with ASCII.
- **Hex Fill**: The entire record is filled with the specified byte.
- **String Pattern**: The string is repeated to fill the record length.
