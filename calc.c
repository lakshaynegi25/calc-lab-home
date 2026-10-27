 // Lakshay Negi 241ADB131
 // https://github.com/lakshaynegi25/calc-lab-home
 // Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
 // Resources: ChatGPT (OpenAI), setup assistance; conversation link to be added


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

int evaluate_basic(const char *line, long long *result) {
    const char *p = line;
    char *end;

    while (*p == ' ' || *p == '\t') {
        p++;
    }

    if (!isdigit((unsigned char)*p)) {
        return 0;
    }

    long long total = strtoll(p, &end, 10);
    p = end;

    while (1) {
        while (*p == ' ' || *p == '\t') {
            p++;
        }

        if (*p == '\n' || *p == '\0') {
            *result = total;
            return 1;
        }

        char op = *p++;

        if (op != '+' && op != '-') {
            return 0;
        }

        while (*p == ' ' || *p == '\t') {
            p++;
        }

        if (!isdigit((unsigned char)*p)) {
            return 0;
        }

        long long number = strtoll(p, &end, 10);
        p = end;

        if (op == '+') {
            total += number;
        } else {
            total -= number;
        }
    }
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
