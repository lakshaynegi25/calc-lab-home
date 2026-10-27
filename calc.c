// Lakshay Negi 241ADB131
// https://github.com/lakshaynegi25/calc-lab-home
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources: ChatGPT (OpenAI), implementation assistance; add complete conversation link before submitting

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

static const char *error_at;

void skip_spaces(const char **p) {
    while (**p == ' ' || **p == '\t') {
        (*p)++;
    }
}

int parse_number(const char **p, long long *result) {
    skip_spaces(p);

    if (!isdigit((unsigned char)**p)) {
        error_at = *p;
        return 0;
    }

    const char *start = *p;
    char *end;
    errno = 0;
    long long number = strtoll(*p, &end, 10);

    if (errno == ERANGE) {
        error_at = start;
        return 0;
    }

    *p = end;
    *result = number;
    return 1;
}

int parse_expression(const char **p, long long *result);

int parse_primary(const char **p, long long *result) {
    skip_spaces(p);

    if (**p == '(') {
        (*p)++;
        if (!parse_expression(p, result)) {
            return 0;
        }
        skip_spaces(p);
        if (**p != ')') {
            error_at = *p;
            return 0;
        }
        (*p)++;
        return 1;
    }

    return parse_number(p, result);
}

int parse_power(const char **p, long long *result) {
    long long base;
    if (!parse_primary(p, &base)) {
        return 0;
    }
    skip_spaces(p);

    if ((*p)[0] == '*' && (*p)[1] == '*') {
        const char *op_at = *p;
        (*p) += 2;
        skip_spaces(p);
        const char *exp_at = *p;
        long long exponent;

        if (!parse_power(p, &exponent)) {
            return 0;
        }
        if (exponent < 0) {
            error_at = exp_at;
            return 0;
        }

        long long answer = 1;
        unsigned long long exp = (unsigned long long)exponent;
        while (exp > 0) {
            if (exp % 2 == 1 && __builtin_mul_overflow(answer, base, &answer)) {
                error_at = op_at;
                return 0;
            }
            exp /= 2;
            if (exp > 0 && __builtin_mul_overflow(base, base, &base)) {
                error_at = op_at;
                return 0;
            }
        }
        *result = answer;
        return 1;
    }

    *result = base;
    return 1;
}

int parse_term(const char **p, long long *result) {
    long long total;
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

        long long number;
        if (!parse_power(p, &number)) {
            return 0;
        }

        if (op == '*') {
            if (__builtin_mul_overflow(total, number, &total)) {
                error_at = op_at;
                return 0;
            }
        } else {
            if (number == 0) {
                error_at = operand_at;
                return 0;
            }
            if (total == LLONG_MIN && number == -1) {
                error_at = op_at;
                return 0;
            }
            total /= number;
        }
    }

    *result = total;
    return 1;
}

int parse_expression(const char **p, long long *result) {
    long long total;
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
        long long number;
        if (!parse_term(p, &number)) {
            return 0;
        }

        if (op == '+') {
            if (__builtin_add_overflow(total, number, &total)) {
                error_at = op_at;
                return 0;
            }
        } else {
            if (__builtin_sub_overflow(total, number, &total)) {
                error_at = op_at;
                return 0;
            }
        }
    }

    *result = total;
    return 1;
}

int evaluate_basic(const char *line, long long *result) {
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

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: ./calc input.txt\n");
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        perror("Cannot open input file");
        return 1;
    }

    // Store results temporarily so any error replaces the entire output.
    FILE *results = tmpfile();
    if (results == NULL) {
        perror("Cannot create temporary results file");
        fclose(file);
        return 1;
    }

    char line[4096];
    size_t offset = 0;
    while (fgets(line, sizeof(line), file)) {
        const char *p = line;
        while (*p == ' ' || *p == '\t') {
            p++;
        }

        if (*p != '\n' && *p != '\r' && *p != '\0' && *p != '#') {
            long long value;
            if (!evaluate_basic(line, &value)) {
                size_t position = offset + (size_t)(error_at - line) + 1;
                printf("ERROR:%zu\n", position);
                fclose(results);
                fclose(file);
                return 0;
            }
            fprintf(results, "%lld\n", value);
        }
        offset += strlen(line);  // Newlines and ignored comments still count.
    }

    if (ferror(file)) {
        perror("Error reading input");
        fclose(results);
        fclose(file);
        return 1;
    }

    rewind(results);
    int ch;
    while ((ch = fgetc(results)) != EOF) {
        putchar(ch);
    }

    fclose(results);
    fclose(file);
    return 0;
}
