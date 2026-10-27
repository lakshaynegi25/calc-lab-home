 // Lakshay Negi 241ADB131
 // https://github.com/lakshaynegi25/calc-lab-home
 // Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
 // Resources: ChatGPT (OpenAI), setup assistance; conversation link to be added



#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

void skip_spaces(const char **p) {
    while (**p == ' ' || **p == '\t') {
        (*p)++;
    }
}

int parse_number(const char **p, long long *result) {
    skip_spaces(p);

    if (!isdigit((unsigned char)**p)) {
        return 0;
    }

    char *end;
    errno = 0;
    long long number = strtoll(*p, &end, 10);

    if (errno == ERANGE) {
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
        (*p) += 2;

        long long exponent;

        if (!parse_power(p, &exponent) || exponent < 0) {
            return 0;
        }

        long long answer = 1;
        unsigned long long exp = (unsigned long long)exponent;

        while (exp > 0) {
            if (exp % 2 == 1) {
                if (__builtin_mul_overflow(answer, base, &answer)) {
                    return 0;
                }
            }

            exp /= 2;

            if (exp > 0) {
                if (__builtin_mul_overflow(base, base, &base)) {
                    return 0;
                }
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

        (*p)++;

        long long number;
        if (!parse_power(p, &number)) {
            return 0;
        }

        if (op == '*') {
            if (__builtin_mul_overflow(total, number, &total)) {
                return 0;
            }
        } else {
            if (number == 0 ||
                (total == LLONG_MIN && number == -1)) {
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

        (*p)++;

        long long number;
        if (!parse_term(p, &number)) {
            return 0;
        }

        if (op == '+') {
            if (__builtin_add_overflow(total, number, &total)) {
                return 0;
            }
        } else {
            if (__builtin_sub_overflow(total, number, &total)) {
                return 0;
            }
        }
    }

    *result = total;
    return 1;
}

int evaluate_basic(const char *line, long long *result) {
    const char *p = line;

    if (!parse_expression(&p, result)) {
        return 0;
    }

    skip_spaces(&p);
    return *p == '\n' || *p == '\r' || *p == '\0';
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

    char line[4096];

    while (fgets(line, sizeof(line), file)) {
        long long result;
        const char *p = line;

        while (isspace((unsigned char)*p)) {
            p++;
        }

        if (*p == '\0') {
            continue;
        }

        if (evaluate_basic(line, &result)) {
            printf("%lld\n", result);
        } else {
            printf("Not supported yet\n");
        }
    }

    fclose(file);
    return 0;
}
