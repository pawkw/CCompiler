#include "compiler.h"
#include <string.h>
#include "helpers/vector.h"
#include "helpers/buffer.h"

#define LEX_GETC_IF(buffer, c, expr)    \
    for(c = peekc(); expr; c = peekc()) \
    {                                   \
        buffer_write(buffer, c);        \
        nextc();                        \
    }

const char* token_names[8] = {"Identifier",  \
                            "Keyword",      \
                            "Operator",     \
                            "Symbol",       \
                            "Number",       \
                            "String",       \
                            "Comment",      \
                            "Newline"};

static struct lex_process* lex_process;
static struct token temp_token;
struct token* read_next_token();

static struct pos lex_file_position()
{
    return lex_process->pos;
}

struct token* token_create(struct token* _token)
{
    memcpy(&temp_token, _token, sizeof(struct token));
    temp_token.pos = lex_file_position();
    return &temp_token;
}

static char nextc()
{
    char c = lex_process->function->next_char(lex_process);
    lex_process->pos.col += 1;
    if(c == '\n')
    {
        lex_process->pos.line += 1;
        lex_process->pos.col = 1;
    }

    return c;
}

static char peekc()
{
    return lex_process->function->peek_char(lex_process);
}

static void pushc(char c)
{
    lex_process->function->push_char(lex_process, c);
}

static struct token* lexer_last_token()
{
    return vector_back_or_null(lex_process->token_vec);
}

// Set the flag on the last token to indicate that there is trailing white space.
static struct token* handle_white_space()
{
    struct token* last_token = lexer_last_token();

    if(last_token)
    {
        last_token->whitespace = true;
    }

    nextc();
    return read_next_token();
}

const char* read_number_string()
{
    const char* num = NULL;
    struct buffer* buffer = buffer_create();
    char c = peekc();
    LEX_GETC_IF(buffer, c, (c >= '0' && c <= '9'));
    buffer_write(buffer, 0x00);
    return buffer_ptr(buffer);
}

unsigned long long read_number()
{
    const char* s = read_number_string();
    return atoll(s);
}

struct token* token_make_number_for_value(unsigned long number)
{
    return token_create(&(struct token){.type = TOKEN_TYPE_NUMBER, .llnum = number});
}

struct token* token_make_number()
{
    return token_make_number_for_value(read_number());
}

struct token* read_next_token()
{
    struct token* token = NULL;
    char c = peekc();
    
    switch (c)
    {
        case EOF:
            /* File done. */
            break;
        
        case ' ':
        case '\t':
            token = handle_white_space();
            break;

        NUMERIC_CASE:
            token = token_make_number();
            break;
            
        default:
            compiler_error(lex_process->compiler, "Unexpected token");
    }

    return token;
}

void print_token(struct token* token)
{
    printf("Token type: %s -> ", token_names[token->type]);
    switch(token->type)
    {
        case(TOKEN_TYPE_IDENTIFIER):
        case(TOKEN_TYPE_KEYWORD):
        case(TOKEN_TYPE_STRING):
            printf(" %s", token->sval);
            break;
        case(TOKEN_TYPE_NUMBER):
            printf(" %lld", token->llnum);
            break;
        default:
            printf("Unknown token.");
            break;
    }
    if(token->whitespace)
        printf(" Whitespace follows.");
    if(token->between_brackets)
        printf(" Is between brackets");
    printf("\n");
}

int lex(struct lex_process* process)
{
    process->current_expression_count = 0;
    process->parentheses_buffer = NULL;
    lex_process = process;
    process->pos.filename = process->compiler->cfile.abs_path;

    struct token* token = read_next_token();
    while(token)
    {
        vector_push(process->token_vec, token);
        token = read_next_token();
    }

    if(process->token_vec)
    {
        for(int x = 0; x < vector_count(process->token_vec); x++)
        {
            print_token(vector_at(process->token_vec, x));
        }

    }
    return LEXICAL_ANALYSIS_ALL_OK;
}