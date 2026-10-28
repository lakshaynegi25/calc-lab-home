// Lakshay Negi 241ADB131
// https://github.com/lakshaynegi25/calc-lab-home
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: https://chatgpt.com/share/6ae0f970-e340-83eb-8027-ca3202731b0d

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <dirent.h>
#include <sys/stat.h>

// AI (ChatGPT, review): prompt "I'm confused about the Value structure in my
// code. Why do we need is_float, and how does the calculator decide between
// integer and decimal division?"
// The flag preserves integer arithmetic unless a decimal value is involved.
typedef struct {
    long long integer;
    double decimal;
    int is_float;
} Value;

static const char *error_at;

static double as_double(Value value) {
    return value.is_float ? value.decimal : (double)value.integer;
}

void skip_spaces(const char **p) {
    while (**p == ' ' || **p == '\t') {
        (*p)++;
    }
}

// AI (ChatGPT, review): prompt "Can you explain how my parse_number function
// reads integers and decimals using strtoll and strtod? I'm confused about how
// it rejects invalid number formats."
// Checking the digits first prevents forbidden decimal forms from being accepted.
// A missing number is reported where an operand was expected.
// At the end of a line, this is the newline or EOF position.
int parse_number(const char **p, Value *result) {
    skip_spaces(p);

    if (!isdigit((unsigned char)**p)) {
        error_at = *p;
        return 0;
    }

    const char *start = *p;
    const char *end_digits = start;
    while (isdigit((unsigned char)*end_digits)) {
        end_digits++;
    }

    if (*end_digits == '.' && isdigit((unsigned char)end_digits[1])) {
        const char *end_fraction = end_digits + 1;
        while (isdigit((unsigned char)*end_fraction)) {
            end_fraction++;
        }

        // Scientific notation is forbidden; report its first invalid letter.
        if (*end_fraction == 'e' || *end_fraction == 'E') {
            error_at = end_fraction;
            return 0;
        }

        errno = 0;
        double number = strtod(start, NULL);
        if (!isfinite(number)) {
            error_at = start;
            return 0;
        }
        result->decimal = number;
        result->integer = 0;
        result->is_float = 1;
        *p = end_fraction;
        return 1;
    }

    char *end;
    errno = 0;
    long long number = strtoll(start, &end, 10);
    if (errno == ERANGE) {
        error_at = start;
        return 0;
    }

    result->integer = number;
    result->decimal = 0.0;
    result->is_float = 0;
    *p = end;
    return 1;
}

int parse_expression(const char **p, Value *result);

// AI (ChatGPT, review): prompt "Can you explain how parse_primary handles
// nested parentheses and detects a missing closing bracket?"
// A parenthesized expression reuses the full parser so nesting has no special rule.
int parse_primary(const char **p, Value *result) {
    skip_spaces(p);

    if (**p == '(') {
        (*p)++;
        if (!parse_expression(p, result)) {
            return 0;
        }
        skip_spaces(p);
        // If ')' is missing, the pointer reaches the newline or end of input.
       // That position is used to report the missing closing parenthesis.
        if (**p != ')') {
            error_at = *p;
            return 0;
        }
        (*p)++;
        return 1;
    }

    return parse_number(p, result);
}

// AI (ChatGPT, review): prompt "Why does parse_power call itself recursively?
// How does this make 2**3**2 equal 512 instead of 64?"
// Recursing on the exponent makes ** group from the right: 2**(3**2).
int parse_power(const char **p, Value *result) {
    Value base;
    if (!parse_primary(p, &base)) {
        return 0;
    }
    skip_spaces(p);

    if ((*p)[0] == '*' && (*p)[1] == '*') {
        const char *op_at = *p;
        (*p) += 2;
        skip_spaces(p);
        const char *exp_at = *p;
        Value exponent;

        // The right operand is parsed as a power, not just a number.
        if (!parse_power(p, &exponent)) {
            return 0;
        }

        if (base.is_float || exponent.is_float) {
            double value = pow(as_double(base), as_double(exponent));
            if (!isfinite(value)) {
                error_at = op_at;
                return 0;
            }
            result->decimal = value;
            result->integer = 0;
            result->is_float = 1;
            return 1;
        }

        if (exponent.integer < 0) {
            error_at = exp_at;
            return 0;
        }

        long long answer = 1;
        long long factor = base.integer;
        unsigned long long exp = (unsigned long long)exponent.integer;
        while (exp > 0) {
            if (exp % 2 == 1 && __builtin_mul_overflow(answer, factor, &answer)) {
                error_at = op_at;
                return 0;
            }
            exp /= 2;
            if (exp > 0 && __builtin_mul_overflow(factor, factor, &factor)) {
                error_at = op_at;
                return 0;
            }
        }
        result->integer = answer;
        result->decimal = 0.0;
        result->is_float = 0;
        return 1;
    }

    *result = base;
    return 1;
}

// Parsing powers first gives ** higher precedence than * and /.
// Errors involving an operator use its position; division by zero uses the operand.
int parse_term(const char **p, Value *result) {
    Value total;
    if (!parse_power(p, &total)) {
        return 0;
    }

    while (1) {
        skip_spaces(p);
        char op = **p;
        if (op != '*' && op != '/') {
            break;
        }
        const char *op_at = *p;
        (*p)++;
        skip_spaces(p);
        const char *operand_at = *p;

        Value number;
        if (!parse_power(p, &number)) {
            return 0;
        }

        if (op == '/' && (number.is_float ? number.decimal == 0.0 : number.integer == 0)) {
            error_at = operand_at;
            return 0;
        }

        if (total.is_float || number.is_float) {
            double left = as_double(total);
            double right = as_double(number);
            double value = op == '*' ? left * right : left / right;
            if (!isfinite(value)) {
                error_at = op_at;
                return 0;
            }
            total.decimal = value;
            total.integer = 0;
            total.is_float = 1;
        } else if (op == '*') {
            if (__builtin_mul_overflow(total.integer, number.integer, &total.integer)) {
                error_at = op_at;
                return 0;
            }
        } else {
            if (total.integer == (-9223372036854775807LL - 1LL) &&
             number.integer == -1) {
                error_at = op_at;
                return 0;
            }
            total.integer /= number.integer;
        }
    }

    *result = total;
    return 1;
}

// AI (ChatGPT, review): prompt "Why does my calculator use separate
// parse_expression and parse_term functions? Explain simply how this makes
// multiplication happen before addition."
// A term is fully evaluated before +/- combines it with the next term.
int parse_expression(const char **p, Value *result) {
    Value total;
    if (!parse_term(p, &total)) {
        return 0;
    }

    while (1) {
        skip_spaces(p);
        char op = **p;
        if (op != '+' && op != '-') {
            break;
        }
        const char *op_at = *p;
        (*p)++;
        Value number;
        if (!parse_term(p, &number)) {
            return 0;
        }

        if (total.is_float || number.is_float) {
            double left = as_double(total);
            double right = as_double(number);
            double value = op == '+' ? left + right : left - right;
            if (!isfinite(value)) {
                error_at = op_at;
                return 0;
            }
            total.decimal = value;
            total.integer = 0;
            total.is_float = 1;
        } else if (op == '+') {
            if (__builtin_add_overflow(total.integer, number.integer, &total.integer)) {
                error_at = op_at;
                return 0;
            }
        } else {
            if (__builtin_sub_overflow(total.integer, number.integer, &total.integer)) {
                error_at = op_at;
                return 0;
            }
        }
    }

    *result = total;
    return 1;
}

int evaluate_basic(const char *line, Value *result) {
    const char *p = line;
    error_at = NULL;
    if (!parse_expression(&p, result)) {
        return 0;
    }

    skip_spaces(&p);
    if (*p != '\n' && *p != '\r' && *p != '\0') {
        error_at = p;
        return 0;
    }
    return 1;
}



// AI (ChatGPT, review): prompt "Can you explain why my read_line_dynamic
// function uses realloc and doubles the buffer size? How does it help when
// reading very large files?"
// Doubling avoids repeatedly copying the entire accumulated line.
// This also allows lines much longer than a fixed-size fgets buffer.
static int read_line_dynamic(FILE *input, char **line, size_t *capacity,
                             size_t *length) {
    int ch;
    *length = 0;
    while ((ch = fgetc(input)) != EOF) {
        if (*length > (size_t)-1 - 2) return -1;
        size_t needed = *length + 2; // current character plus terminating NUL
        if (needed > *capacity) {
            size_t new_capacity = *capacity ? *capacity : 8192;
            while (new_capacity < needed) {
                if (new_capacity > (size_t)-1 / 2) {
                    new_capacity = needed;
                    break;
                }
                new_capacity *= 2;
            }
            char *new_line = realloc(*line, new_capacity);
            if (new_line == NULL) return -1;
            *line = new_line;
            *capacity = new_capacity;
        }
        (*line)[(*length)++] = (char)ch;
        if (ch == '\n') break;
    }
    if (ferror(input)) return -1;
    if (*length == 0) return 0; // EOF with no further line
    (*line)[*length] = '\0';
    return 1;
}

// AI (ChatGPT, review): prompt "Why does evaluate_file use a temporary file
// before saving the results? What happens if the calculator finds an error
// after processing some correct expressions?"
// A temporary file prevents partial answers from appearing before a later error.
// AI (ChatGPT, review): prompt "How does my calculator find the exact error
// position across multiple lines? Please explain error_at, offset, and why
// newline characters are counted."
// The current line pointer plus prior line lengths gives a 1-based file offset.
static int evaluate_file(FILE *input, FILE *output) {
    FILE *temporary = tmpfile();
    if (temporary == NULL) {
        perror("Cannot create temporary results file");
        return 0;
    }

    char *line = NULL;
    size_t capacity = 0;
    size_t line_length = 0;
    size_t offset = 0;
    int status;
    while ((status = read_line_dynamic(input, &line, &capacity,
                                       &line_length)) == 1) {
        const char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p != '\n' && *p != '\r' && *p != '\0' && *p != '#') {
            Value value;
            if (!evaluate_basic(line, &value)) {
                size_t position = offset + (size_t)(error_at - line) + 1;
                int ok = fprintf(output, "ERROR:%zu\n", position) >= 0;
                free(line);
                fclose(temporary);
                return ok;
            }
            int ok = value.is_float
                ? fprintf(temporary, "%.15g\n", value.decimal)
                : fprintf(temporary, "%lld\n", value.integer);
            if (ok < 0) {
                free(line);
                fclose(temporary);
                return 0;
            }
        }
        // Counting the full line includes newline and ignored comment characters.
        offset += line_length;
    }
    free(line);
    if (status < 0 || ferror(temporary)) {
        fprintf(stderr, "Unable to read input or allocate line memory\n");
        fclose(temporary);
        return 0;
    }
    rewind(temporary);
    int ch;
    while ((ch = fgetc(temporary)) != EOF) {
        if (fputc(ch, output) == EOF) {
            fclose(temporary);
            return 0;
        }
    }
    int ok = !ferror(temporary);
    if (fclose(temporary) != 0) ok = 0;
    return ok;
}

static char *copy_n(const char *s, size_t n) {
    char *out = malloc(n + 1);
    if (out == NULL) return NULL;
    memcpy(out, s, n);
    out[n] = '\0';
    return out;
}

static char *file_stem(const char *path) {
    size_t len = strlen(path);
    while (len && path[len - 1] == '/') len--;
    size_t from = len;
    while (from && path[from - 1] != '/') from--;
    if (len == from) return NULL;
    if (len - from >= 4 && !strncmp(path + len - 4, ".txt", 4)) len -= 4;
    return copy_n(path + from, len - from);
}

static char *join_path(const char *dir, const char *name) {
    size_t a = strlen(dir), b = strlen(name);
    int slash = a && dir[a - 1] != '/';
    if (a > (size_t)-1 - b - 2) return NULL;
    char *path = malloc(a + b + (size_t)slash + 1);
    if (path == NULL) return NULL;
    memcpy(path, dir, a);
    if (slash) path[a++] = '/';
    memcpy(path + a, name, b + 1);
    return path;
}

static char *default_output_dir(const char *path) {
    char *stem = file_stem(path);
    if (stem == NULL) return NULL;
    const char suffix[] = "_lakshay_241ADB131";
    char *name = malloc(strlen(stem) + sizeof(suffix));
    if (name) sprintf(name, "%s%s", stem, suffix);
    free(stem);
    return name;
}

static int create_output_dir(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) return 1;
        fprintf(stderr, "Output path is not a directory: %s\n", path);
        return 0;
    }
    if (mkdir(path, 0777) == 0) return 1;
    fprintf(stderr, "Cannot create output directory '%s': %s\n", path, strerror(errno));
    return 0;
}

static int process_file(const char *input_path, const char *outdir) {
    FILE *input = fopen(input_path, "r");
    if (input == NULL) {
        fprintf(stderr, "Cannot open input '%s': %s\n", input_path, strerror(errno));
        return 0;
    }

    char *stem = file_stem(input_path);
    const char suffix[] = "_lakshay_negi_241ADB131.txt";
    char *filename = NULL;
    if (stem) {
        filename = malloc(strlen(stem) + sizeof(suffix));
        if (filename) sprintf(filename, "%s%s", stem, suffix);
    }
    char *output_path = filename ? join_path(outdir, filename) : NULL;
    free(stem);
    free(filename);
    if (output_path == NULL) {
        fprintf(stderr, "Cannot construct output filename\n");
        fclose(input);
        return 0;
    }

    FILE *output = fopen(output_path, "w");
    if (output == NULL) {
        fprintf(stderr, "Cannot write '%s': %s\n", output_path, strerror(errno));
        free(output_path);
        fclose(input);
        return 0;
    }

    int ok = evaluate_file(input, output);
    if (fclose(output) != 0) ok = 0;
    if (fclose(input) != 0) ok = 0;
    if (!ok) fprintf(stderr, "Failed processing '%s'\n", input_path);
    free(output_path);
    return ok;
}

// AI (ChatGPT, review): prompt "Can you explain how argc and argv handle -d
// and -o in my calculator? I want to understand how the program processes one
// file or a whole directory."
// Options choose one input or a folder; outputs use the same naming rule.
int main(int argc, char *argv[]) {
    const char *input_path = NULL;
    const char *input_dir = NULL;
    const char *output_arg = NULL;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--dir")) {
            if (input_dir || i + 1 >= argc || argv[i + 1][0] == '-') {
                fprintf(stderr, "Invalid directory argument\n");
                return 1;
            }
            input_dir = argv[++i];
        } else if (!strcmp(argv[i], "-o") || !strcmp(argv[i], "--output-dir")) {
            if (output_arg || i + 1 >= argc || argv[i + 1][0] == '-') {
                fprintf(stderr, "Invalid output directory argument\n");
                return 1;
            }
            output_arg = argv[++i];
        } else if (argv[i][0] == '-' || input_path != NULL) {
            fprintf(stderr, "Unknown or extra argument: %s\n", argv[i]);
            return 1;
        } else {
            input_path = argv[i];
        }
    }
    if ((input_dir && input_path) || (!input_dir && !input_path)) {
        fprintf(stderr, "Usage: ./calc [-d DIR] [-o OUTDIR] [input.txt]\n");
        return 1;
    }

    char *default_dir = NULL;
    const char *outdir = output_arg;
    if (outdir == NULL) {
        default_dir = default_output_dir(input_dir ? input_dir : input_path);
        outdir = default_dir;
        if (outdir == NULL) {
            fprintf(stderr, "Cannot name output directory\n");
            return 1;
        }
    }

    if (!create_output_dir(outdir)) {
        free(default_dir);
        return 1;
    }

    int ok = 1;
    if (input_dir) {
        DIR *dir = opendir(input_dir);
        if (dir == NULL) {
            fprintf(stderr, "Cannot open directory '%s': %s\n", input_dir, strerror(errno));
            free(default_dir);
            return 1;
        }
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            size_t len = strlen(entry->d_name);
            if (len < 5 || strcmp(entry->d_name + len - 4, ".txt")) continue;
            char *path = join_path(input_dir, entry->d_name);
            if (path == NULL) { ok = 0; break; }
            struct stat st;
            if (stat(path, &st) == 0 && S_ISREG(st.st_mode)) {
                if (!process_file(path, outdir)) ok = 0;
            }
            free(path);
            if (!ok) break;
        }
        if (closedir(dir) != 0) ok = 0;
    } else {
        ok = process_file(input_path, outdir);
    }
    free(default_dir);
    return ok ? 0 : 1;
}
