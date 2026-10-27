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
    TOK_EOF,
    TOK_EOL,
    TOK_ERROR,
    TOK_NUMBER,
    TOK_PLUS,
    TOK_MINUS,
    TOK_MUL,
    TOK_DIV,
    TOK_POW,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_HASH,
} TokenType;

typedef struct {
    int is_float;
    long long i;
    double f;
} Val;

typedef struct {
    TokenType type;
    Val value; 
    size_t position;
} Token;

typedef struct {
    const char* text; 
    size_t index;     
} Lexer;

Token next_token(Lexer* lex) {
    Token tok;
    
    // this loop (the one line below) was written by Gemini 3.1 Pro.
    // prompt: how would I make a loop that makes lexer skip spaces, tabs or returns?
    while (lex->text[lex->index] == ' ' || lex->text[lex->index] == '\t' || lex->text[lex->index] == '\r') {
        lex->index++;
    }

    tok.position = lex->index + 1;
    char c = lex->text[lex->index];

    if (c == '\0') {
        tok.type = TOK_EOF;
        return tok;
    }
    if (c == '\n') {
        tok.type = TOK_EOL;
        lex->index++;
        return tok;
    }

    // Handling operators and parentheses: as soon as lexer reaches one of the operators it just skips it because i wrote index++
    if (c == '+') { tok.type = TOK_PLUS;   lex->index++; return tok; }
    if (c == '-') { tok.type = TOK_MINUS;  lex->index++; return tok; }
    if (c == '/') { tok.type = TOK_DIV;    lex->index++; return tok; }
    if (c == '(') { tok.type = TOK_LPAREN; lex->index++; return tok; }
    if (c == ')') { tok.type = TOK_RPAREN; lex->index++; return tok; }
    if (c == '#') { tok.type = TOK_HASH;   lex->index++; return tok; }

    // handling '*' and '**' 
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
        tok.type = TOK_NUMBER;
        tok.value.is_float = 0;
        tok.value.i = 0;

        while (isdigit(lex->text[lex->index])) {
            tok.value.i = tok.value.i * 10 + (lex->text[lex->index] - '0');
            lex->index++;
        }

        if (lex->text[lex->index] == '.') {
            size_t dot_pos = lex->index + 1;
            lex->index++;
            if (!isdigit(lex->text[lex->index])) {
                tok.type = TOK_ERROR;
                tok.position = dot_pos;
                return tok;
            }
            tok.value.is_float = 1;
            double fraction = 1.0;
            tok.value.f = (double)tok.value.i;
            while (isdigit(lex->text[lex->index])) {
                fraction /= 10.0;
                tok.value.f += (lex->text[lex->index] - '0') * fraction;
                lex->index++;
            }
        } else {
            tok.value.f = (double)tok.value.i;
        }

        if (lex->text[lex->index] == 'e' || lex->text[lex->index] == 'E') {
            tok.type = TOK_ERROR;
            tok.position = lex->index + 1;
            lex->index++;
            return tok;
        }

        return tok;
    }

    if (c == '.') {
        tok.type = TOK_ERROR;
        lex->index++;
        return tok;
    }

    tok.type = TOK_ERROR;
    lex->index++;
    return tok;
}

typedef struct {
    Lexer lex;
    Token current;
    int has_error;
    size_t error_pos;
} Parser;

void advance(Parser* p) {
    p->current = next_token(&p->lex);
}

void set_error(Parser* p, size_t pos) {
    if (!p->has_error) { 
        p->has_error = 1;
        p->error_pos = pos;
    }
}

Val parse_expr(Parser* p);
Val parse_term(Parser* p);
Val parse_power(Parser* p);
Val parse_primary(Parser* p);

Val do_math(Parser* p, Val left, Val right, TokenType op, size_t op_pos) {
    Val res;
    res.is_float = left.is_float || right.is_float;

    double lf = left.is_float ? left.f : (double)left.i;
    double rf = right.is_float ? right.f : (double)right.i;

    if (op == TOK_PLUS) {
        if (res.is_float) res.f = lf + rf;
        else res.i = left.i + right.i;
    } else if (op == TOK_MINUS) {
        if (res.is_float) res.f = lf - rf;
        else res.i = left.i - right.i;
    } else if (op == TOK_MUL) {
        if (res.is_float) res.f = lf * rf;
        else res.i = left.i * right.i;
    } else if (op == TOK_DIV) {
        if (res.is_float) res.f = lf / rf;
        else res.i = left.i / right.i; 
    } else if (op == TOK_POW) {
        if (res.is_float) {
            res.f = pow(lf, rf);
        } else {
            if (right.i < 0) {
                set_error(p, op_pos);
                return left;
            }
            if (right.i == 0) res.i = 1;
            else {
                long long base = left.i;
                long long exp = right.i;
                long long r = 1;
                while (exp > 0) {
                    if (exp % 2 == 1) r *= base;
                    base *= base;
                    exp /= 2;
                }
                res.i = r;
            }
        }
    }

    if (res.is_float && (isnan(res.f) || isinf(res.f))) {
        set_error(p, op_pos);
    }
    return res;
}

Val parse_primary(Parser* p) {
    Val err_val = {0, 0, 0.0};
    if (p->has_error) return err_val;

    if (p->current.type == TOK_NUMBER) {
        Val val = p->current.value;
        advance(p);
        return val;
    } 
    
    if (p->current.type == TOK_LPAREN) {
        advance(p); 
        
        Val val = parse_expr(p);
        
        if (p->current.type == TOK_RPAREN) {
            advance(p);
        } else {
            set_error(p, p->current.position); 
        }
        return val;
    }

    set_error(p, p->current.position);
    return err_val;
}

Val parse_power(Parser* p) {
    Val left = parse_primary(p);

    if (p->has_error) return left;

    if (p->current.type == TOK_POW) {
        size_t op_pos = p->current.position;
        advance(p);
        
        Val right = parse_power(p); 
        
        if (!p->has_error) {
            left = do_math(p, left, right, TOK_POW, op_pos);
        }
    }

    return left;
}

Val parse_term(Parser* p) {
    Val left = parse_power(p);

    while (!p->has_error && (p->current.type == TOK_MUL || p->current.type == TOK_DIV)) {
        TokenType op = p->current.type;
        size_t op_pos = p->current.position;
        advance(p); 

        size_t right_pos = p->current.position; 
        Val right = parse_power(p);
        
        if (!p->has_error) {
            if (op == TOK_DIV && ((right.is_float && right.f == 0.0) || (!right.is_float && right.i == 0))) {
                set_error(p, right_pos);
            } else {
                left = do_math(p, left, right, op, op_pos);
            }
        }
    }
    return left;
}

Val parse_expr(Parser* p) {
    Val left = parse_term(p);

    while (!p->has_error && (p->current.type == TOK_PLUS || p->current.type == TOK_MINUS)) {
        TokenType op = p->current.type;
        size_t op_pos = p->current.position;
        advance(p); 
        
        Val right = parse_term(p);
        
        if (!p->has_error) {
            left = do_math(p, left, right, op, op_pos);
        }
    }
    return left;
}

/* opening the file and reading it without any newline characters
by using "rb" */
char* read_entire_file(const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if(!file) {
        fprintf(stderr, "failed to open input file: %s\n", filepath);
        exit(1);
    }

    /* fileno(file) gets raw POSIX file descriptor and fstat fills 
    struct stat with metadata about the file on disk */
    struct stat st;
    if (fstat(fileno(file), &st) != 0) {
        fprintf(stderr, "failed to get file stats.\n");
        fclose(file);
        exit(1);
    }

    // using malloc to request a single block of memory from the heap
    // using size+1 to have exact file size plus 1 byte for a null term
    size_t size = st.st_size;
    char* buffer = malloc(size + 1);
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

void get_basename(const char* path, char* base) {
    const char* last_slash = strrchr(path, '/');
    if (!last_slash) last_slash = path;
    else last_slash++;
    strcpy(base, last_slash);
    
    char* ext = strrchr(base, '.');
    if (ext && strcmp(ext, ".txt") == 0) {
        *ext = '\0';
    }
}

void ensure_dir(const char* dir) {
    struct stat st = {0};
    if (stat(dir, &st) == -1) {
        mkdir(dir, 0777);
    }
}

void evaluate_file(const char* in_filepath, const char* out_dir) {
    char base[256];
    get_basename(in_filepath, base);
    
    char out_filepath[512];
    sprintf(out_filepath, "%s/%s_ismayil_nadirov_251ADB088.txt", out_dir, base);
    
    char* input_data = read_entire_file(in_filepath);

    Parser p;
    p.lex.text = input_data;
    p.lex.index = 0;
    p.has_error = 0;
    p.error_pos = 0;

    advance(&p);

    size_t result_capacity = 1000;
    Val* results = malloc(result_capacity * sizeof(Val));
    if (!results) {
        fprintf(stderr, "Memory allocation failed for results.\n");
        free(input_data);
        exit(1);
    }
    
    size_t result_count = 0;

    while (p.current.type != TOK_EOF) {
        if (p.current.type == TOK_EOL) {
            advance(&p);
            continue;
        }

        if (p.current.type == TOK_HASH) {
            while (p.current.type != TOK_EOL && p.current.type != TOK_EOF) {
                advance(&p);
            }
            continue;
        }

        Val val = parse_expr(&p);

        if (!p.has_error) {
            if (p.current.type != TOK_EOL && p.current.type != TOK_EOF) {
                set_error(&p, p.current.position);
            }
        }

        if (p.has_error) break;

        if (result_count == result_capacity) {
            result_capacity *= 2;
            results = realloc(results, result_capacity * sizeof(Val));
        }
        results[result_count++] = val;

        if (p.current.type == TOK_EOL) {
            advance(&p);
        }
    }

    FILE* out_file = fopen(out_filepath, "w");
    if (!out_file) {
        fprintf(stderr, "Cannot write to output file: %s\n", out_filepath);
        free(results);
        free(input_data);
        exit(1);
    }

    if (p.has_error) {
        fprintf(out_file, "ERROR:%zu\n", p.error_pos);
    } else {
        for (size_t i = 0; i < result_count; i++) {
            if (results[i].is_float) {
                fprintf(out_file, "%.15g\n", results[i].f);
            } else {
                fprintf(out_file, "%lld\n", results[i].i);
            }
        }
    }

    fclose(out_file);
    free(results);
    free(input_data);
}

void evaluate_dir(const char* dirpath, const char* out_dir) {
    DIR* dir;
    struct dirent* ent;
    if ((dir = opendir(dirpath)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            size_t len = strlen(ent->d_name);
            if (len > 4 && strcmp(ent->d_name + len - 4, ".txt") == 0) {
                char in_filepath[512];
                sprintf(in_filepath, "%s/%s", dirpath, ent->d_name);
                evaluate_file(in_filepath, out_dir);
            }
        }
        closedir(dir);
    } else {
        fprintf(stderr, "Could not open directory %s\n", dirpath);
        exit(1);
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [-d DIR] [-o OUTDIR] [input.txt]\n", argv[0]);
        return 1;
    }

    const char* in_path = NULL;
    const char* out_dir = NULL;
    int is_dir = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--dir") == 0) {
            is_dir = 1;
            if (i + 1 < argc && argv[i+1][0] != '-') {
                in_path = argv[++i];
            } else {
                fprintf(stderr, "Missing argument for -d\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output-dir") == 0) {
            if (i + 1 < argc && argv[i+1][0] != '-') {
                out_dir = argv[++i];
            } else {
                fprintf(stderr, "Missing argument for -o\n");
                return 1;
            }
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return 1;
        } else {
            if (!in_path) in_path = argv[i];
            else {
                fprintf(stderr, "Too many inputs provided.\n");
                return 1;
            }
        }
    }

    if (!in_path) {
        fprintf(stderr, "No input provided.\n");
        return 1;
    }

    char default_out_dir[512];
    if (!out_dir) {
        char base[256];
        get_basename(in_path, base);
        sprintf(default_out_dir, "%s_ismayil_251ADB088", base);
        out_dir = default_out_dir;
    }

    ensure_dir(out_dir);

    if (is_dir) {
        evaluate_dir(in_path, out_dir);
    } else {
        evaluate_file(in_path, out_dir);
    }

    return 0;
}