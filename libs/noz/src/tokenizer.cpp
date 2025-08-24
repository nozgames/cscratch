//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <noz/tokenizer.h>
#include <string.h>
#include <ctype.h>

void tokenizer_init(tokenizer_t* tok, const char* input)
{
    tok->input = input;
    tok->position = 0;
    tok->length = input ? strlen(input) : 0;
    tok->line = 1;
    tok->column = 1;
}

bool tokenizer_has_more(tokenizer_t* tok)
{
    return tok && tok->position < tok->length;
}

char tokenizer_peek(tokenizer_t* tok)
{
    if (!tokenizer_has_more(tok))
    {
        return '\0';
    }
    return tok->input[tok->position];
}

char tokenizer_next(tokenizer_t* tok)
{
    if (!tokenizer_has_more(tok))
    {
        return '\0';
    }
    
    char c = tok->input[tok->position++];
    
    if (c == '\n')
    {
        tok->line++;
        tok->column = 1;
    }
    else
    {
        tok->column++;
    }
    
    return c;
}

bool tokenizer_expect(tokenizer_t* tok, char expected)
{
    tokenizer_skip_whitespace(tok);
    if (tokenizer_peek(tok) != expected)
    {
        return false;
    }
    tokenizer_next(tok);
    return true;
}

void tokenizer_skip_whitespace(tokenizer_t* tok)
{
    while (tokenizer_has_more(tok) && isspace(tokenizer_peek(tok)) && tokenizer_peek(tok) != '\n')
    {
        tokenizer_next(tok);
    }
}

void tokenizer_skip_to_newline(tokenizer_t* tok)
{
    while (tokenizer_has_more(tok) && tokenizer_peek(tok) != '\n')
    {
        tokenizer_next(tok);
    }
    if (tokenizer_peek(tok) == '\n')
    {
        tokenizer_next(tok);
    }
}

bool tokenizer_read_line(tokenizer_t* tok, text_t* line)
{
    if (!tokenizer_has_more(tok))
    {
        return false;
    }
    
    text_clear(line);
    
    while (tokenizer_has_more(tok))
    {
        char c = tokenizer_peek(tok);
        if (c == '\n')
        {
            tokenizer_next(tok);  // Consume newline
            break;
        }
        
        if (c == '\r')
        {
            tokenizer_next(tok);  // Skip \r
            if (tokenizer_peek(tok) == '\n')
            {
                tokenizer_next(tok);  // Consume \n
            }
            break;
        }
        
        // Add character to line
        char ch_str[2] = {tokenizer_next(tok), '\0'};
        text_append(line, ch_str);
    }
    
    return true;
}

bool tokenizer_read_until(tokenizer_t* tok, char delimiter, text_t* result)
{
    text_clear(result);
    
    while (tokenizer_has_more(tok))
    {
        char c = tokenizer_peek(tok);
        if (c == delimiter || c == '\n' || c == '\r')
        {
            break;
        }
        
        char ch_str[2] = {tokenizer_next(tok), '\0'};
        text_append(result, ch_str);
    }
    
    return result->length > 0;
}

bool tokenizer_read_quoted_string(tokenizer_t* tok, text_t* result)
{
    text_clear(result);
    
    char quote_char = tokenizer_peek(tok);
    if (quote_char != '"' && quote_char != '\'')
    {
        return false;
    }
    
    tokenizer_next(tok); // Skip opening quote
    
    while (tokenizer_has_more(tok))
    {
        char c = tokenizer_next(tok);
        
        if (c == quote_char)
        {
            // End of string
            return true;
        }
        
        if (c == '\\' && tokenizer_has_more(tok))
        {
            // Escape sequence
            char escaped = tokenizer_next(tok);
            switch (escaped)
            {
                case 'n':  c = '\n'; break;
                case 't':  c = '\t'; break;
                case 'r':  c = '\r'; break;
                case '\\': c = '\\'; break;
                case '"':  c = '"';  break;
                case '\'': c = '\''; break;
                default:   c = escaped; break;
            }
        }
        
        char ch_str[2] = {c, '\0'};
        text_append(result, ch_str);
    }
    
    // Unterminated string
    return false;
}

bool tokenizer_read_identifier(tokenizer_t* tok, text_t* result)
{
    text_clear(result);
    
    char c = tokenizer_peek(tok);
    if (!isalpha(c) && c != '_')
    {
        return false;
    }
    
    while (tokenizer_has_more(tok))
    {
        c = tokenizer_peek(tok);
        if (!isalnum(c) && c != '_')
        {
            break;
        }
        
        char ch_str[2] = {tokenizer_next(tok), '\0'};
        text_append(result, ch_str);
    }
    
    return result->length > 0;
}

bool tokenizer_read_number(tokenizer_t* tok, text_t* result)
{
    text_clear(result);
    
    char c = tokenizer_peek(tok);
    if (!isdigit(c) && c != '-' && c != '+' && c != '.')
    {
        return false;
    }
    
    // Handle sign
    if (c == '-' || c == '+')
    {
        char ch_str[2] = {tokenizer_next(tok), '\0'};
        text_append(result, ch_str);
        c = tokenizer_peek(tok);
    }
    
    bool has_digits = false;
    bool has_decimal = false;
    
    while (tokenizer_has_more(tok))
    {
        c = tokenizer_peek(tok);
        
        if (isdigit(c))
        {
            has_digits = true;
            char ch_str[2] = {tokenizer_next(tok), '\0'};
            text_append(result, ch_str);
        }
        else if (c == '.' && !has_decimal)
        {
            has_decimal = true;
            char ch_str[2] = {tokenizer_next(tok), '\0'};
            text_append(result, ch_str);
        }
        else
        {
            break;
        }
    }
    
    return has_digits && result->length > 0;
}

bool tokenizer_read_number_as_float(tokenizer_t* tok, float* result)
{
    assert(tok);
    assert(result);
    
    tokenizer_skip_whitespace(tok);
    
    text_t number;
    bool success = tokenizer_read_number(tok, &number);
    if (success)
    {
        *result = (float)atof(number.value);
    }
    
    return success;
}

bool tokenizer_read_vec3(tokenizer_t* tok, vec3* result)
{
    assert(tok);
    assert(result);
    
    bool success = true;
    success &= tokenizer_expect(tok, '(');
    success &= tokenizer_read_number_as_float(tok, &result->x);
    success &= tokenizer_expect(tok, ',');
    success &= tokenizer_read_number_as_float(tok, &result->y);
    success &= tokenizer_expect(tok, ',');
    success &= tokenizer_read_number_as_float(tok, &result->z);
    success &= tokenizer_expect(tok, ')');
    
    return success;
}

void tokenizer_skip_line_comment(tokenizer_t* tok)
{
    char c = tokenizer_peek(tok);
    
    // Check for // or #
    if (c == '#')
    {
        tokenizer_skip_to_newline(tok);
    }
    else if (c == '/' && tok->position + 1 < tok->length && tok->input[tok->position + 1] == '/')
    {
        tokenizer_skip_to_newline(tok);
    }
}

bool tokenizer_skip_block_comment(tokenizer_t* tok)
{
    if (tok->position + 1 >= tok->length || 
        tok->input[tok->position] != '/' || 
        tok->input[tok->position + 1] != '*')
    {
        return false;
    }
    
    tokenizer_next(tok); // Skip '/'
    tokenizer_next(tok); // Skip '*'
    
    while (tok->position + 1 < tok->length)
    {
        if (tok->input[tok->position] == '*' && tok->input[tok->position + 1] == '/')
        {
            tokenizer_next(tok); // Skip '*'
            tokenizer_next(tok); // Skip '/'
            return true;
        }
        tokenizer_next(tok);
    }
    
    // Unterminated comment
    return false;
}

size_t tokenizer_get_line_number(tokenizer_t* tok)
{
    return tok ? tok->line : 1;
}

size_t tokenizer_get_column_number(tokenizer_t* tok)
{
    return tok ? tok->column : 1;
}

// Token-based parsing functions

void token_init(token_t* token)
{
    if (!token) return;
    
    token->type = token_type_none;
    text_init(&token->value);
    token->line = 0;
    token->column = 0;
}

void token_clear(token_t* token)
{
    if (!token) return;
    
    token->type = token_type_none;
    text_clear(&token->value);
    token->line = 0;
    token->column = 0;
}

static bool is_operator_char(char c)
{
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '=' ||
           c == '<' || c == '>' || c == '!' || c == '&' || c == '|' ||
           c == '^' || c == '%' || c == '~';
}

static bool is_delimiter_char(char c)
{
    return c == '(' || c == ')' || c == '{' || c == '}' || 
           c == '[' || c == ']' || c == ';' || c == ':' ||
           c == ',' || c == '.';
}

bool tokenizer_next_token(tokenizer_t* tok, token_t* token)
{
    if (!tok || !token)
    {
        return false;
    }
    
    token_clear(token);
    
    if (!tokenizer_has_more(tok))
    {
        token->type = token_type_eof;
        return false;
    }
    
    token->line = tok->line;
    token->column = tok->column;
    
    char c = tokenizer_peek(tok);
    
    // Skip whitespace and newlines
    if (isspace(c))
    {
        if (c == '\n')
        {
            tokenizer_next(tok);
            token->type = token_type_newline;
            text_set(&token->value, "\n");
        }
        else
        {
            while (tokenizer_has_more(tok) && isspace(tokenizer_peek(tok)) && tokenizer_peek(tok) != '\n')
            {
                char ch_str[2] = {tokenizer_next(tok), '\0'};
                text_append(&token->value, ch_str);
            }
            token->type = token_type_whitespace;
        }
        return true;
    }
    
    // Comments
    if (c == '#' || (c == '/' && tok->position + 1 < tok->length && tok->input[tok->position + 1] == '/'))
    {
        while (tokenizer_has_more(tok) && tokenizer_peek(tok) != '\n')
        {
            char ch_str[2] = {tokenizer_next(tok), '\0'};
            text_append(&token->value, ch_str);
        }
        token->type = token_type_comment;
        return true;
    }
    
    // Block comments
    if (c == '/' && tok->position + 1 < tok->length && tok->input[tok->position + 1] == '*')
    {
        text_append(&token->value, "/*");
        tokenizer_next(tok); // Skip '/'
        tokenizer_next(tok); // Skip '*'
        
        while (tok->position + 1 < tok->length)
        {
            if (tok->input[tok->position] == '*' && tok->input[tok->position + 1] == '/')
            {
                text_append(&token->value, "*/");
                tokenizer_next(tok); // Skip '*'
                tokenizer_next(tok); // Skip '/'
                break;
            }
            char ch_str[2] = {tokenizer_next(tok), '\0'};
            text_append(&token->value, ch_str);
        }
        token->type = token_type_comment;
        return true;
    }
    
    // Quoted strings
    if (c == '"' || c == '\'')
    {
        if (tokenizer_read_quoted_string(tok, &token->value))
        {
            token->type = token_type_string;
            return true;
        }
    }
    
    // Numbers
    if (isdigit(c) || c == '-' || c == '+' || c == '.')
    {
        if (tokenizer_read_number(tok, &token->value))
        {
            token->type = token_type_number;
            return true;
        }
    }
    
    // Identifiers
    if (isalpha(c) || c == '_')
    {
        if (tokenizer_read_identifier(tok, &token->value))
        {
            token->type = token_type_identifier;
            return true;
        }
    }
    
    // Operators
    if (is_operator_char(c))
    {
        char ch_str[2] = {tokenizer_next(tok), '\0'};
        text_set(&token->value, ch_str);
        token->type = token_type_operator;
        return true;
    }
    
    // Delimiters
    if (is_delimiter_char(c))
    {
        char ch_str[2] = {tokenizer_next(tok), '\0'};
        text_set(&token->value, ch_str);
        token->type = token_type_delimiter;
        return true;
    }
    
    // Unknown character
    char ch_str[2] = {tokenizer_next(tok), '\0'};
    text_set(&token->value, ch_str);
    token->type = token_type_none;
    return true;
}

bool tokenizer_peek_token(tokenizer_t* tok, token_t* token)
{
    if (!tok || !token)
    {
        return false;
    }
    
    // Save current position
    size_t saved_pos = tok->position;
    size_t saved_line = tok->line;
    size_t saved_column = tok->column;
    
    // Read next token
    bool result = tokenizer_next_token(tok, token);
    
    // Restore position
    tok->position = saved_pos;
    tok->line = saved_line;
    tok->column = saved_column;
    
    return result;
}

bool token_is_type(token_t* token, token_type_t type)
{
    return token && token->type == type;
}

bool token_is_value(token_t* token, const char* value)
{
    return token && value && strcmp(token->value.value, value) == 0;
}

const char* token_type_name(token_type_t type)
{
    switch (type)
    {
        case token_type_none:       return "none";
        case token_type_identifier: return "identifier";
        case token_type_number:     return "number";
        case token_type_string:     return "string";
        case token_type_operator:   return "operator";
        case token_type_delimiter:  return "delimiter";
        case token_type_newline:    return "newline";
        case token_type_whitespace: return "whitespace";
        case token_type_comment:    return "comment";
        case token_type_eof:        return "eof";
        default:                    return "unknown";
    }
}

bool tokenizer_read_color(tokenizer_t* tok, color_t* result)
{
    if (!tok || !result)
        return false;

    tokenizer_skip_whitespace(tok);
    
    // Handle hex colors: #RRGGBB or #RRGGBBAA
    if (tokenizer_peek(tok) == '#')
    {
        tokenizer_next(tok); // Skip #
        
        text_t hex_text;
        text_init(&hex_text);
        
        // Read hex digits
        while (tokenizer_has_more(tok))
        {
            char ch = tokenizer_peek(tok);
            if (isxdigit(ch))
            {
                char hex_str[2] = {ch, '\0'};
                text_append(&hex_text, hex_str);
                tokenizer_next(tok);
            }
            else
                break;
        }
        
        if (hex_text.length == 6) // #RRGGBB
        {
            unsigned int hex = (unsigned int)strtoul(hex_text.value, NULL, 16);
            result->r = ((hex >> 16) & 0xFF) / 255.0f;
            result->g = ((hex >> 8) & 0xFF) / 255.0f;
            result->b = (hex & 0xFF) / 255.0f;
            result->a = 1.0f;
            return true;
        }
        else if (hex_text.length == 8) // #RRGGBBAA
        {
            unsigned int hex = (unsigned int)strtoul(hex_text.value, NULL, 16);
            result->r = ((hex >> 24) & 0xFF) / 255.0f;
            result->g = ((hex >> 16) & 0xFF) / 255.0f;
            result->b = ((hex >> 8) & 0xFF) / 255.0f;
            result->a = (hex & 0xFF) / 255.0f;
            return true;
        }
        else if (hex_text.length == 3) // #RGB shorthand
        {
            unsigned int hex = (unsigned int)strtoul(hex_text.value, NULL, 16);
            result->r = ((hex >> 8) & 0xF) / 15.0f;
            result->g = ((hex >> 4) & 0xF) / 15.0f;
            result->b = (hex & 0xF) / 15.0f;
            result->a = 1.0f;
            return true;
        }
        return false;
    }

    // Handle rgba(r,g,b,a) format
    text_t identifier;
    text_init(&identifier);
    if (tokenizer_read_identifier(tok, &identifier))
    {
        if (text_equals_cstr(&identifier, "rgba"))
        {
            tokenizer_skip_whitespace(tok);
            if (tokenizer_peek(tok) != '(')
                return false;
            tokenizer_next(tok); // Skip (
            
            // Read r,g,b,a values
            float r, g, b, a;
            
            tokenizer_skip_whitespace(tok);
            if (!tokenizer_read_number_as_float(tok, &r)) return false;
            
            tokenizer_skip_whitespace(tok);
            if (tokenizer_peek(tok) != ',') return false;
            tokenizer_next(tok); // Skip ,
            
            tokenizer_skip_whitespace(tok);
            if (!tokenizer_read_number_as_float(tok, &g)) return false;
            
            tokenizer_skip_whitespace(tok);
            if (tokenizer_peek(tok) != ',') return false;
            tokenizer_next(tok); // Skip ,
            
            tokenizer_skip_whitespace(tok);
            if (!tokenizer_read_number_as_float(tok, &b)) return false;
            
            tokenizer_skip_whitespace(tok);
            if (tokenizer_peek(tok) != ',') return false;
            tokenizer_next(tok); // Skip ,
            
            tokenizer_skip_whitespace(tok);
            if (!tokenizer_read_number_as_float(tok, &a)) return false;
            
            tokenizer_skip_whitespace(tok);
            if (tokenizer_peek(tok) != ')') return false;
            tokenizer_next(tok); // Skip )
            
            result->r = r / 255.0f;
            result->g = g / 255.0f;
            result->b = b / 255.0f;
            result->a = a; // Alpha is typically 0-1 already
            return true;
        }
        else if (text_equals_cstr(&identifier, "rgb"))
        {
            tokenizer_skip_whitespace(tok);
            if (tokenizer_peek(tok) != '(')
                return false;
            tokenizer_next(tok); // Skip (
            
            // Read r,g,b values
            float r, g, b;
            
            tokenizer_skip_whitespace(tok);
            if (!tokenizer_read_number_as_float(tok, &r)) return false;
            
            tokenizer_skip_whitespace(tok);
            if (tokenizer_peek(tok) != ',') return false;
            tokenizer_next(tok); // Skip ,
            
            tokenizer_skip_whitespace(tok);
            if (!tokenizer_read_number_as_float(tok, &g)) return false;
            
            tokenizer_skip_whitespace(tok);
            if (tokenizer_peek(tok) != ',') return false;
            tokenizer_next(tok); // Skip ,
            
            tokenizer_skip_whitespace(tok);
            if (!tokenizer_read_number_as_float(tok, &b)) return false;
            
            tokenizer_skip_whitespace(tok);
            if (tokenizer_peek(tok) != ')') return false;
            tokenizer_next(tok); // Skip )
            
            result->r = r / 255.0f;
            result->g = g / 255.0f;
            result->b = b / 255.0f;
            result->a = 1.0f;
            return true;
        }
        else
        {
            // Handle predefined CSS colors
            struct ColorName { const char* name; color_t color; };
            static const ColorName predefined_colors[] = {
                {"black", {0.0f, 0.0f, 0.0f, 1.0f}},
                {"white", {1.0f, 1.0f, 1.0f, 1.0f}},
                {"red", {1.0f, 0.0f, 0.0f, 1.0f}},
                {"green", {0.0f, 0.5f, 0.0f, 1.0f}},
                {"blue", {0.0f, 0.0f, 1.0f, 1.0f}},
                {"yellow", {1.0f, 1.0f, 0.0f, 1.0f}},
                {"cyan", {0.0f, 1.0f, 1.0f, 1.0f}},
                {"magenta", {1.0f, 0.0f, 1.0f, 1.0f}},
                {"gray", {0.5f, 0.5f, 0.5f, 1.0f}},
                {"grey", {0.5f, 0.5f, 0.5f, 1.0f}},
                {"orange", {1.0f, 0.65f, 0.0f, 1.0f}},
                {"pink", {1.0f, 0.75f, 0.8f, 1.0f}},
                {"purple", {0.5f, 0.0f, 0.5f, 1.0f}},
                {"brown", {0.65f, 0.16f, 0.16f, 1.0f}},
                {"transparent", {0.0f, 0.0f, 0.0f, 0.0f}},
                {nullptr, {0.0f, 0.0f, 0.0f, 0.0f}}
            };

            for (const ColorName* color = predefined_colors; color->name != nullptr; color++)
            {
                if (text_equals_cstr(&identifier, color->name))
                {
                    *result = color->color;
                    return true;
                }
            }
        }
    }

    return false;
}