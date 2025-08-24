//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <noz/string.h>
#include <stdbool.h>
#include <stddef.h>

// @token_type
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

// @token
typedef struct token
{
    text_t value;
    size_t line;
    size_t column;
    token_type_t type;
} token_t;

typedef struct tokenizer
{
    const char* input;
    size_t position;
    size_t length;
    size_t line;
    size_t column;
} tokenizer_t;

void tokenizer_init(tokenizer_t* tok, const char* input);
bool tokenizer_has_more(tokenizer_t* tok);
char tokenizer_peek(tokenizer_t* tok);
char tokenizer_next(tokenizer_t* tok);
bool tokenizer_expect(tokenizer_t* tok, char expected);
void tokenizer_skip_whitespace(tokenizer_t* tok);
void tokenizer_skip_to_newline(tokenizer_t* tok);
bool tokenizer_read_line(tokenizer_t* tok, text_t* line);
bool tokenizer_read_until(tokenizer_t* tok, char delimiter, text_t* result);
bool tokenizer_read_quoted_string(tokenizer_t* tok, text_t* result);
bool tokenizer_read_identifier(tokenizer_t* tok, text_t* result);
bool tokenizer_read_number(tokenizer_t* tok, text_t* result);
bool tokenizer_read_number_as_float(tokenizer_t* tok, float* result);
bool tokenizer_read_vec3(tokenizer_t* tok, vec3* result);
bool tokenizer_read_color(tokenizer_t* tok, color_t* result);
void tokenizer_skip_line_comment(tokenizer_t* tok);
bool tokenizer_skip_block_comment(tokenizer_t* tok);
size_t tokenizer_get_line_number(tokenizer_t* tok);
size_t tokenizer_get_column_number(tokenizer_t* tok);
bool tokenizer_next_token(tokenizer_t* tok, token_t* token);
bool tokenizer_peek_token(tokenizer_t* tok, token_t* token);

void token_init(token_t* token);
void token_clear(token_t* token);
bool token_is_type(token_t* token, token_type_t type);
bool token_is_value(token_t* token, const char* value);
const char* token_type_name(token_type_t type);