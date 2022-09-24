#ifndef PAWC_COMPILER
#define PAWC_COMPILER

#include <stdio.h>
#include <stdbool.h>

struct pos
{
    int line;
    int col;
    const char* filename;
};

enum
{
    TOKEN_TYPE_IDENTIFIER,
    TOKEN_TYPE_KEYWORD,
    TOKEN_TYPE_OPERATOR,
    TOKEN_TYPE_SYMBOL,
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_COMMENT,
    TOKEN_TYPE_NEWLINE
};

struct token
{
    int type;
    int flags;

    union
    {
        char cval;
        const char* sval;
        unsigned int inum;
        unsigned long lnum;
        unsigned long long llnum;
        void* any;
    };

    // True if there is whitespace between this and the following token.
    bool whitespace;

    // A pointer to the first token in the series of tokens within the brackets.
    // For debugging.
    const char* between_brackets; 

};

struct lex_process;
typedef char (*LEX_PROCESS_NEXT_CHAR)(struct lex_process* process);
typedef char (*LEX_PROCESS_PEEK_CHAR)(struct lex_process* process);
typedef void (*LEX_PROCESS_PUSH_CHAR)(struct lex_process* process, char c);

struct lex_process_functions
{
    LEX_PROCESS_NEXT_CHAR next_char;
    LEX_PROCESS_PEEK_CHAR peek_char;
    LEX_PROCESS_PUSH_CHAR push_char;
};

struct lex_process
{
    struct pos pos;
    struct vector* vec;
    struct compile_process* compiler;

    // Depth of expressions.
    // IE: ((x)) would have a count of 2.
    int current_expression_count;

    struct buffer* parentheses_buffer;
    struct lex_process_functions* function;

    // Private data that the lexer does not understand, but the programmer does.
    void* private;
};

enum
{
    COMPILER_FILE_COMPILED_OK,
    COMPILER_FAILED_WITH_ERRORS
};

struct compile_process
{
    // Compile flags.
    int flags;
    struct pos pos;

    struct compile_process_input_file
    {
        FILE *fp;
        const char* abs_path;
    } cfile;

    FILE* ofile;
};

// compiler.c
int compile_file(const char* filename, const char *out_filename, int flags);

// cprocess.c
struct compile_process *compile_process_create(const char *filename, const char *filename_out, int flags);
char compile_process_next_char(struct lex_process* lex_process);
char compile_process_peek_char(struct lex_process* lex_process);
void compile_process_push_char(struct lex_process* lex_process, char c);

#endif