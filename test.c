#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 600

int readFile(FILE* fp, char** fileContents) {
    int counter = 0;

    char buffer[MAX_LINE];

    // while (fgets(buffer, MAX_LINE - 1, fp)) {
    //     fileContents[counter] = malloc(strlen(buffer) * sizeof(char));
    //     buffer[strcspn(buffer, "\n")] = '\0';
    //     strcpy(fileContents[counter], buffer);
    //     counter++;
    // }

    while (fread(buffer, sizeof(char), MAX_LINE, fp) != 0) {
        fileContents[counter] = malloc(strlen(buffer) * sizeof(char));
        strcpy(fileContents[counter], buffer);
        counter++;
        // printf("%d\n", counter);
    }

    return counter;
}

int main() {
    FILE* fp = fopen("miniFile.txt", "r");

    char* fileContents[10];

    int num = readFile(fp, fileContents);

    for (int i = 0; i < num; i++) {
        printf("%d\n%s\n", i, fileContents[i]);
    }
}