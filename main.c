#include <stdio.h>
#include "helpers/vector.h"
#include "compiler.h"

int main()
{
    int result = compile_file("./test.c", "test", 0);
    if(result == COMPILER_FILE_COMPILED_OK)
        printf("Successfully compiled.\n");
    else if(result == COMPILER_FAILED_WITH_ERRORS)
        printf("Compile failed.\n");
    else
        printf("Unknown response from compile_file.\n");

    return result;
}