#include <string.h>
#include <assert.h>
#include "compiler.h"
#include "helpers/vector.h"
#include "helpers/buffer.h"

#define LEX_GETC_IF(buffer, c, expr)     \
    for (c = peekc(); expr; c = peekc()) \
    {                                    \
        buffer_write(buffer, c);         \
        nextc();                         \
    }

const char *token_names[8] = {"Identifier",
                              "Keyword",
                              "Operator",
                              "Symbol",
                              "Number",
                              "String",
                              "Comment",
                              "Newline"};

static struct lex_process *lex_process;
static struct token temp_token;
struct token *read_next_token();

static struct pos lex_file_position()
{
    return lex_process->pos;
}

struct token *token_create(struct token *_token)
{
    memcpy(&temp_token, _token, sizeof(struct token));
    temp_token.pos = lex_file_position();
    return &temp_token;
}

static char nextc()
{
    char c = lex_process->function->next_char(lex_process);
    lex_process->pos.col += 1;
    if (c == '\n')
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

static struct token *lexer_last_token()
{
    return vector_back_or_null(lex_process->token_vec);
}

// Set the flag on the last token to indicate that there is trailing white space.
static struct token *handle_white_space()
{
    struct token *last_token = lexer_last_token();

    if (last_token)
    {
        last_token->whitespace = true;
    }

    nextc();
    return read_next_token();
}

const char *read_number_string()
{
    const char *num = NULL;
    struct buffer *buffer = buffer_create();
    char c = peekc();
    LEX_GETC_IF(buffer, c, (c >= '0' && c <= '9'));
    buffer_write(buffer, 0x00);
    return buffer_ptr(buffer);
}

unsigned long long read_number()
{
    const char *s = read_number_string();
    return atoll(s);
}

struct token *token_make_number_for_value(unsigned long number)
{
    return token_create(&(struct token){.type = TOKEN_TYPE_NUMBER, .llnum = number});
}

struct token *token_make_number()
{
    return token_make_number_for_value(read_number());
}

static struct token *token_make_string(char start_delim, char end_delim)
{
    struct buffer *buffer = buffer_create();

    assert(nextc() == start_delim);
    char c = nextc();
    for (; c != end_delim && c != EOF; c = nextc())
    {
        if (c == '\\')
        {
            // Handle an escaped character.
            continue;
        }
        buffer_write(buffer, c);
    }
    buffer_write(buffer, 0x00);

    return token_create(&(struct token){.type = TOKEN_TYPE_STRING, .sval = buffer_ptr(buffer)});
}

static bool operator_treated_as_one(char op)
{
    return op == '(' || op == '[' || op == ',' || op == '.' || op == '*' || op == '?';
}

static bool is_single_operator(char op)
{
    return op == '+' ||
           op == '-' ||
           op == '/' ||
           op == '*' ||
           op == '=' ||
           op == '>' ||
           op == '<' ||
           op == '|' ||
           op == '&' ||
           op == '^' ||
           op == '%' ||
           op == '~' ||
           op == '!' ||
           op == '(' ||
           op == '[' ||
           op == ',' ||
           op == '.' ||
           op == '?';
}

bool operator_valid(const char *operator)
{
    return S_EQ(operator, "+") ||
           S_EQ(operator, "-") ||
           S_EQ(operator, "*") ||
           S_EQ(operator, "/") ||
           S_EQ(operator, "!") ||
           S_EQ(operator, "^") ||
           S_EQ(operator, "+=") ||
           S_EQ(operator, "-=") ||
           S_EQ(operator, "*=") ||
           S_EQ(operator, "/=") ||
           S_EQ(operator, ">>") ||
           S_EQ(operator, "<<") ||
           S_EQ(operator, ">=") ||
           S_EQ(operator, "<=") ||
           S_EQ(operator, ">") ||
           S_EQ(operator, "<") ||
           S_EQ(operator, "||") ||
           S_EQ(operator, "&&") ||
           S_EQ(operator, "|") ||
           S_EQ(operator, "&") ||
           S_EQ(operator, "++") ||
           S_EQ(operator, "--") ||
           S_EQ(operator, "=") ||
           S_EQ(operator, "!=") ||
           S_EQ(operator, "==") ||
           S_EQ(operator, "->") ||
           S_EQ(operator, "(") ||
           S_EQ(operator, "[") ||
           S_EQ(operator, ",") ||
           S_EQ(operator, ".") ||
           S_EQ(operator, "...") ||
           S_EQ(operator, "?") ||
           S_EQ(operator, "%");
}

void read_op_flush_back_keep_first(struct buffer *buffer)
{
    const char *data = buffer_ptr(buffer);
    int len = buffer->len;
    for (int i = len - 1; i >= 1; i--)
    {
        if (data[i] == 0x00)
            continue;

        pushc(data[i]);
    }
}

const char *read_operator()
{
    bool single_operator = true;
    char operator= nextc();
    struct buffer *buffer = buffer_create();
    buffer_write(buffer, operator);

    if (!operator_treated_as_one(operator))
    {
        operator= peekc();
        if (is_single_operator(operator))
        {
            buffer_write(buffer, operator);
            nextc();
            single_operator = false;
        }
    }

    // NULL terminator
    buffer_write(buffer, 0x00);
    char *ptr = buffer_ptr(buffer);
    if (!single_operator)
    {
        if (!operator_valid(ptr))
        {
            read_op_flush_back_keep_first(buffer);
            ptr[1] = 0x00;
        }
    }
    else if (!operator_valid(ptr))
    {
        compiler_error(lex_process->compiler, "The operator %s is not valid.\n", ptr);
    }

    return ptr;
}

static void lex_new_expression()
{
    lex_process->current_expression_count++;
    if (lex_process->current_expression_count == 1)
    {
        lex_process->parentheses_buffer = buffer_create();
    }
}

bool lex_is_in_expression()
{
    return lex_process->current_expression_count > 0;
}

static struct token *token_make_operator_or_string()
{
    char operator = peekc();
    if(operator == '<')
    {
        struct token* last_token = lexer_last_token();
        if(token_is_keyword(last_token, "include"))
        {
            return token_make_string('<', '>');
        }
    }

    struct token* token = token_create(&(struct token){.type=TOKEN_TYPE_OPERATOR, .sval=read_operator()});
    if(operator == '(')
    {
        lex_new_expression();
    }
    return token;
}

struct token *read_next_token()
{
    struct token *token = NULL;
    char c = peekc();

    switch (c)
    {
    case EOF:
        /* File done. */
        break;

    case '"':
        token = token_make_string('"', '"');
        break;

    case ' ':
    case '\t':
        token = handle_white_space();
        break;

    NUMERIC_CASE:
        token = token_make_number();
        break;

    OPERATOR_CASE_EXCLUDING_DIVISION:
        token = token_make_operator_or_string();
        break;

    default:
        compiler_error(lex_process->compiler, "Unexpected token");
    }

    return token;
}

void print_token(struct token *token)
{
    printf("Token type: %s -> ", token_names[token->type]);
    switch (token->type)
    {
    case (TOKEN_TYPE_IDENTIFIER):
    case (TOKEN_TYPE_KEYWORD):
    case (TOKEN_TYPE_STRING):
        printf(" \"%s\"", token->sval);
        break;
    case (TOKEN_TYPE_NUMBER):
        printf(" %lld", token->llnum);
        break;
    case (TOKEN_TYPE_OPERATOR):
        printf(" %s", token->sval);
        break;
    default:
        printf("Unknown token.");
        break;
    }
    if (token->whitespace)
        printf(" Whitespace follows.");
    if (token->between_brackets)
        printf(" Is between brackets");
    printf("\n");
}

int lex(struct lex_process *process)
{
    process->current_expression_count = 0;
    process->parentheses_buffer = NULL;
    lex_process = process;
    process->pos.filename = process->compiler->cfile.abs_path;

    struct token *token = read_next_token();
    while (token)
    {
        vector_push(process->token_vec, token);
        token = read_next_token();
    }

    if (process->token_vec)
    {
        for (int x = 0; x < vector_count(process->token_vec); x++)
        {
            print_token(vector_at(process->token_vec, x));
        }
    }
    return LEXICAL_ANALYSIS_ALL_OK;
}