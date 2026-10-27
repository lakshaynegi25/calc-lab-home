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
#include <math.h>

// Preserve integer division unless an operand is a decimal number.
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

int parse_primary(const char **p, Value *result) {
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

        // Parsing the exponent recursively makes ** right-associative.
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
            if (total.integer == LLONG_MIN && number.integer == -1) {
                error_at = op_at;
                return 0;
            }
            total.integer /= number.integer;
        }
    }

    *result = total;
    return 1;
}

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

    // Save results temporarily so errors discard earlier answers.
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
            Value value;
            if (!evaluate_basic(line, &value)) {
                size_t position = offset + (size_t)(error_at - line) + 1;
                printf("ERROR:%zu\n", position);
                fclose(results);
                fclose(file);
                return 0;
            }
            if (value.is_float) {
                fprintf(results, "%.15g\n", value.decimal);
            } else {
                fprintf(results, "%lld\n", value.integer);
            }
        }
        offset += strlen(line);
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
