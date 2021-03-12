#include "utils.h"

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "search_parallel.h"

// Print contents of entire file.
void printContents(char** fileContents) {
    /**
     * The moment fileContents[i] == NULL
     * indicates no more strings afterwards.
     */
    for (int i = 0; fileContents[i] != NULL; i++) {
        printf("%s\n", fileContents[i]);
    }
}

// Print contents of a local region of a file.
void printLocalContents(char** fileContents, int start, int size, int rank, int nP) {
    for (int i = 0; i < nP; i++) {
        MPI_Barrier(MPI_COMM_WORLD);
        if (i == rank) {
            printf("P%d[\n", rank);
            for (int i = 0; i < size && fileContents[start + i] != NULL; i++) {
                printf("%s\n", fileContents[start + i]);
            }
            printf("]\n");
        }
    }
}

// Detects whether the given character is a valid alphanumeric.
// If yes, it returns false, indicating that the corresponding substring
// is not a complete word.
int isValidWord(char* wordPtr) {
    char ch = *wordPtr;
    if (ch >= 'A' && ch <= 'Z' || ch >= 'a' && ch <= 'z' || ch >= '0' && ch <= '9')
        return false;
    return true;
}

// Searches the fileContents in a given interval for the given word.
// Only returns true if complete word is found, incomplete matches rejected.
int searchWord(char** fileContents, int totalLines, int start, int size, char* word) {
    char* wordPtr;

    // Need to find complete occurence of word in curr String.
    for (int i = 0; i < size && start + i < totalLines && fileContents[start + i] != NULL; i++) {
        wordPtr = fileContents[start + i];

        while ((wordPtr = strstr(wordPtr, word)) != NULL) {
            if (wordPtr == fileContents[start + i] || isValidWord(wordPtr - 1)) {
                // printf("Found %s\n", word);
                return true;
            }
            wordPtr += strlen(word);
        }
    }

    return false;
}

// Read contents of file and store it.
int readFile(FILE* fp, char** fileContents) {
    int counter = 0;

    char buffer[MAX_LINE];

    /**
     * Fgets will read until MAX_LINE - 1 or /n
     * This may lead to the problem of disproportionate
     * lines.
     */
    while (fgets(buffer, MAX_LINE - 1, fp)) {
        fileContents[counter] = malloc(strlen(buffer) * sizeof(char));
        buffer[strcspn(buffer, "\n")] = '\0';
        strcpy(fileContents[counter], buffer);
        counter++;
    }

    // Alternative approach tried, gave Seg Faults for some reason.

    // while (fread(buffer, sizeof(char), MAX_LINE, fp) != 0) {
    //     fileContents[counter] = malloc(strlen(buffer) * sizeof(char));
    //     strcpy(fileContents[counter], buffer);
    //     counter++;
    // }

    return counter;
}

// Read argv[] and store words in an array.
void initWordSet(char* wordSet[], int numWords, char* argv[]) {
    for (int i = 0; i < numWords; i++) {
        // printf("Query Word: %s\n", argv[i + 3]);
        wordSet[i] = malloc(strlen(argv[i + 3]) * sizeof(char));
        strcpy(wordSet[i], argv[i + 3]);
    }
}

void printAllottedWords(char* localWordSet[], int localWordCount, int totalProcessesForWord,
                        int* wordIndex, int rank, int start_offset, int local_n, int nP) {
    for (int i = 0; i < nP; i++) {
        MPI_Barrier(MPI_COMM_WORLD);
        if (i == rank) {
            printf("P%d: ", rank);

            for (int i = 0; i < localWordCount; i++) {
                printf("%s ", localWordSet[i]);
            }

            printf("[ ");
            for (int i = 0; i < localWordCount; i++) {
                printf("%d ", wordIndex[i]);
            }
            printf("] ");

            printf("[%d] ", totalProcessesForWord);

            printf("%d %d\n", start_offset, local_n);
        }
    }
}

void printArray(int foundWords[], int numWords, int rank) {
    printf("P%d [ ", rank);

    for (int i = 0; i < numWords; i++) {
        printf("%d ", foundWords[i]);
    }

    printf("]\n");
}

int getArraySum(int foundWords[], int numWords) {
    int sum = 0;
    for (int i = 0; i < numWords; i++) {
        sum += foundWords[i];
    }

    return sum;
}