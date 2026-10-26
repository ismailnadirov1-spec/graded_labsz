// Ismayil Nadirov 251ADB088
//
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: Gemini AI (In some blocks to fix bugs, I added comments where exactly is it used)

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

typedef enum {
    TOK_EOF,     // file ending
    TOK_EOL,     // end of line
    TOK_ERROR,   // invalid character found
    TOK_NUMBER,  // integers and Floats (Grade 10)
    TOK_PLUS,    // +
    TOK_MINUS,   // -
    TOK_MUL,     // *
    TOK_DIV,     // /
    TOK_POW,     // **
    TOK_LPAREN,  // (
    TOK_RPAREN,   // )
    TOK_HASH,    // handling the # comment rule
} TokenType;

typedef struct {
    TokenType type;
    double value; // if it is a TOK_NUMBER we store the real math value here
    size_t position; //the 1 based character position where this token started
} Token;

typedef struct {
    const char* text; //pointwr to the file buffer
    size_t index; //current 0 based index in the buffer
} Lexer;

Token next_token(Lexer* lex) {
    Token tok;
    
    //this loop (the one line below) was written by Gemini 3.1 Pro.
    //prompt: how would I make a loop that makes lexer skip spaces, tabs or returns?
    while (lex->text[lex->index] == ' ' || lex->text[lex->index] == '\t' || lex->text[lex->index] == '\r') {
        lex->index++;
    }

    tok.position= lex->index + 1;
    char c = lex->text[lex->index];

    if(c == '\0') {
        tok.type = TOK_EOF;
        return tok;
    }
    if (c == '\n') {
        tok.type = TOK_EOL;
        lex->index++;
        return tok;
    }

    //Handling operators and parentheses: as soon as lexer reaches one of the operators it just skips it because i wrote index++
    if (c == '+') { tok.type = TOK_PLUS;   lex->index++; return tok; }
    if (c == '-') { tok.type = TOK_MINUS;  lex->index++; return tok; }
    if (c == '/') { tok.type = TOK_DIV;    lex->index++; return tok; }
    if (c == '(') { tok.type = TOK_LPAREN; lex->index++; return tok; }
    if (c == ')') { tok.type = TOK_RPAREN; lex->index++; return tok; }
    if (c == '#') { tok.type = TOK_HASH;   lex->index++; return tok; }

    //handling '*' and '**' 
    if (c == '*') {
        lex->index++;
        if (lex->text[lex->index] == '*') {
            tok.type = TOK_POW;
            lex->index++;
        } else {
            tok.type = TOK_MUL;
        }
        return tok;
    }

    if (isdigit(c)) {
        char* endptr;
        tok.type = TOK_NUMBER;
        // strtod handles reading the digits and decimals automatically
        tok.value = strtod(&lex->text[lex->index], &endptr);
        
        // advance the lexer index to wherever strtod stopped reading
        lex->index = endptr - lex->text;

        return tok;
    }
    tok.type = TOK_ERROR;
    lex->index++;
    return tok;
}



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
    printf("Successfully loaded %zu bytes.\n\n", strlen(input_data));

    Lexer lex;
    lex.text = input_data;
    lex.index = 0;
    Token tok;
    do {
        tok = next_token(&lex); 
        
        printf("Token Type: %2d | Position: %3zu", tok.type, tok.position);
        
        if (tok.type == TOK_NUMBER) {
            printf(" | Value: %.15g", tok.value);
        }
        
        printf("\n");
        
    } while (tok.type != TOK_EOF);
    free(input_data);
    return 0;
}