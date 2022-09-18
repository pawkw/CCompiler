#ifndef PAWC_COMPILER
#define PAWC_COMPILER

#include <stdio.h>

enum {
    COMPILER_FILE_COMPILED_OK,
    COMPILER_FAILED_WITH_ERRORS
};

struct compile_process
{
    // Compile flags.
    int flags;

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

#endif