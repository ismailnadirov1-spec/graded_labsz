// Ismayil Nadirov 251ADB088
//
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: Gemini AI (In some blocks to fix bugs, I added comments where exactly is it used)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

/* opening the file and reading it without any newline characters
by using "rb" */
char* read_entire_file(const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if(!file) {
        fprintf(stderr, "failed to open input file: %s\n", filepath);
        exit(1);
    }


    /*fileno(file) gets raw POSIX file descriptor and fstat fills 
    struct stat with metadata about the file on disk*/
    struct stat st;
    if (fstat(fileno(file), &st) !=0) {
        fprintf(stderr, "failed to get file stats.\n");
        fclose(file);
        exit(1);
    }

    //using malloc to request a single block of memory from the heap
    //using size+1 to have exact file size plus 1 byte for a null term
    size_t size = st.st_size;
    char* buffer = malloc(size+1);
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed for fiile.\n");
        fclose(file);
        exit(1);
    }

    if (fread(buffer, 1, size, file) != size && ferror(file)) {
        fprintf(stderr, "failed to read file contents.\n");
        free(buffer);
        fclose(file);
        exit(1);
    }

    buffer[size] = '\0';
    fclose(file);
    return buffer;
}



int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input.txt>\n", argv[0]);
        return 1;
    }
    char* input_data = read_entire_file(argv[1]);
    printf ("successfully loaded %zu bytes.\n", strlen(input_data));
    free (input_data);
    return 0;
}