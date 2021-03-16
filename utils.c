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

// Reads and searches for words in the given subregion of the File.
void readLocalFile(FILE* fp, int start_offset, int local_n, char** wordSet, int* localAns,
                   int numWords, int rank) {
    /**
     * Seek to start_offset from SEEK_SET.
     * Read current word completely.
     * Check if you've exceeded the limit by comparing ftell() output.
     * It may be possible that you're starting from the end of a word, in which
     * case you should skip it.
     */

    fseek(fp, start_offset, SEEK_SET);
    long int limit = ftell(fp);
    // printf("Start is: %ld\n", limit);

    limit += (long int)local_n;

    // printf("End is: %ld\n", limit);

    if (fseek(fp, -1, SEEK_CUR) == 0 && isValidChar(fgetc(fp))) {
        char ch;
        while (isValidChar((ch = fgetc(fp))))
            ;
    }

    char buffer[MAX_LINE];

    // You had decided that on the last word of your entire block,
    // You would exceed your limit, which will happen.

    // I have skipped the current word.
    while (ftell(fp) < limit && fgets(buffer, MAX_LINE - 1, fp)) {
        int index = strlen(buffer) - 1;

        while (!feof(fp) && index > 0 && isValidChar(buffer[index])) {
            if (feof(fp))
                break;
            // printf("%c ", buffer[index]);
            buffer[index--] = '\0';
            fseek(fp, -1, SEEK_CUR);
        }

        if (!feof(fp)) {
            buffer[index] = '\0';
            // fseek(fp, 1, SEEK_CUR);
        }

        if (buffer[0] != '\0') {
            searchWordInBuffer(buffer, wordSet, localAns, numWords, rank);
            // printf("[%s]\n", buffer);
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

// Returns true if alphanumeric.
int isValidChar(char ch) {
    if (ch >= 'A' && ch <= 'Z' || ch >= 'a' && ch <= 'z' || ch >= '0' && ch <= '9')
        return true;
    return false;
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

// Takes in a string buffer, and searches for all the words in the charSet in it.
// Modifies the localAns bitVector, to be ultimately reduced.
// ! Must be passed in localAns as all 0s.
void searchWordInBuffer(char* buf, char** wordSet, int localAns[], int numWords, int rank) {
    char* wordPtr;
    char* word;

    // Need to find complete occurence of word in curr String.
    for (int i = 0; i < numWords; i++) {
        wordPtr = buf;
        word = wordSet[i];

        // This condition ensures that each time this function is invoked,
        // (for each buffer), then you consider only the words which were not
        // found in other buffers of the block.
        while (localAns[i] == 0 && (wordPtr = strstr(wordPtr, word)) != NULL) {
            if (wordPtr == buf || isValidWord(wordPtr - 1)) {
                // printf("P%d Found %s\n", rank, word);

                localAns[i] = 1;
                // break;
            }
            wordPtr += strlen(word);
        }
    }
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
        int index = strlen(buffer) - 1;

        // Push back characters if currently breaking a word.
        while (index > 0 && !isValidWord(&buffer[index])) {
            buffer[index--] = '\0';
            fseek(fp, -1, SEEK_CUR);
        }

        buffer[index] = '\0';

        // Append to fileContents if non-empty string.
        if (buffer[0] != '\0') {
            fileContents[counter] = malloc((index + 1) * sizeof(char));

            strcpy(fileContents[counter], buffer);
            counter++;
        }
    }

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