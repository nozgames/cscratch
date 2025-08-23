//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <noz/string.h>
#include <stdbool.h>
#include <stddef.h>

// Token types
typedef enum token_type
{
    token_type_none,           // No token / end of input
    token_type_identifier,     // Variable names, keywords
    token_type_number,         // Numeric literals
    token_type_string,         // String literals  
    token_type_operator,       // Operators (+, -, *, /, =, etc.)
    token_type_delimiter,      // Delimiters (, ), {, }, [, ], ;, :, etc.
    token_type_newline,        // Line endings
    token_type_whitespace,     // Spaces, tabs
    token_type_comment,        // Comments
    token_type_eof             // End of file
} token_type_t;

// Token structure
typedef struct token
{
    text_t value;
    size_t line;
    size_t column;
    token_type_t type;
} token_t;

// String tokenizer for parsing text
typedef struct tokenizer
{
    const char* input;
    size_t position;
    size_t length;
    size_t line;
    size_t column;
} tokenizer_t;

// Initialize tokenizer with input string
void tokenizer_init(tokenizer_t* tok, const char* input);

// Check if there are more characters to read
bool tokenizer_has_more(const tokenizer_t* tok);

// Peek at current character without advancing
char tokenizer_peek(const tokenizer_t* tok);

// Read and advance to next character
char tokenizer_next(tokenizer_t* tok);

// Expect a specific character and advance if found
bool tokenizer_expect(tokenizer_t* tok, char expected);

// Skip whitespace characters (except newlines)
void tokenizer_skip_whitespace(tokenizer_t* tok);

// Skip to end of current line
void tokenizer_skip_to_newline(tokenizer_t* tok);

// Read an entire line into text_t
bool tokenizer_read_line(tokenizer_t* tok, text_t* line);

// Read until a delimiter character is encountered
bool tokenizer_read_until(tokenizer_t* tok, char delimiter, text_t* result);

// Read a quoted string (handles escape sequences)
bool tokenizer_read_quoted_string(tokenizer_t* tok, text_t* result);

// Read an identifier (alphanumeric + underscore)
bool tokenizer_read_identifier(tokenizer_t* tok, text_t* result);

// Read a number (integer or float)
bool tokenizer_read_number(tokenizer_t* tok, text_t* result);

// Read a number and parse it as a float
bool tokenizer_read_number_as_float(tokenizer_t* tok, float* result);

// Read a vec3 in format (x, y, z)
bool tokenizer_read_vec3(tokenizer_t* tok, vec3_t* result);

// Skip a single-line comment (// or #)
void tokenizer_skip_line_comment(tokenizer_t* tok);

// Skip a block comment (/* ... */)
bool tokenizer_skip_block_comment(tokenizer_t* tok);

// Get current line number (1-based)
size_t tokenizer_get_line_number(const tokenizer_t* tok);

// Get current column number (1-based)
size_t tokenizer_get_column_number(const tokenizer_t* tok);

// Token-based parsing functions
// Initialize a token structure
void token_init(token_t* token);

// Clear a token structure
void token_clear(token_t* token);

// Read the next token from the tokenizer
bool tokenizer_next_token(tokenizer_t* tok, token_t* token);

// Peek at the next token without consuming it
bool tokenizer_peek_token(tokenizer_t* tok, token_t* token);

// Check if a token matches an expected type
bool token_is_type(const token_t* token, token_type_t type);

// Check if a token matches an expected string value
bool token_is_value(const token_t* token, const char* value);

// Get token type name for debugging
const char* token_type_name(token_type_t type);