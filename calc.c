 // Lakshay Negi 241ADB131
 // https://github.com/lakshaynegi25/calc-lab-home
 // Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
 // Resources: ChatGPT (OpenAI), setup assistance; conversation link to be added

 #include <stdio.h>


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

    int ch;

    while ((ch = fgetc(file)) != EOF) {
        putchar(ch);
    }

    fclose(file);
    return 0;
}
