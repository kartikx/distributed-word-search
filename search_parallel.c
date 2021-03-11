#include "search_parallel.h"

#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

int main(int argc, char* argv[]) {
    char* fileContents[MAX_FILE];

    char* wordSet[MAX_WORDSET];
    int numLines, numWords;

    int option = 0;

    int size, rank, totalAns = 0;

    // Check number of arguments provided.
    if (argc < 4) {
        printf("Incorrect usage\n");
        printf("Use `./a.out file.txt AND hello my` to use\n");
        exit(1);
    }

    // Check the value of option and set it.
    if (strcmp(argv[2], "AND") == 0) {
        option = AND;
    } else if (strcmp(argv[2], "OR") == 0) {
        option = OR;
    } else {
        printf("Option provided is neither AND nor OR, exiting\n");
        exit(1);
    }

    // Check number of Query words provided.
    numWords = argc - 3;
    if (numWords > MAX_WORDSET) {
        printf("Number of query words provided exceeds limit, only first %d will be considered\n",
               MAX_WORDSET);
        numWords = MAX_WORDSET;
    }

    // Store the Query Words in an array.
    initWordSet(wordSet, numWords, argv);

    FILE* fptr = fopen(argv[1], "r");
    if (fptr == NULL) {
        printf("No such file exists: %s\n", argv[1]);
    }

    // Store contents of file and return total number of "lines"
    numLines = readFile(fptr, fileContents);

    // printContents(fileContents);

    // for (int i = 0; i < numWords; i++) searchWord(fileContents, 0, numLines, wordSet[i]);

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int local_n, start_offset;

    char* localWordSet[(int)ceil((float)numWords / size)];
    int localWordCount = 0;
    int totalProcessesForWord;

    // Some processes get more than one word.
    if (size < numWords) {
        for (int i = 0; i < numWords; i++) {
            if (i % size == rank) localWordSet[localWordCount++] = wordSet[i];
        }

        // This word is only present in this process.
        // Hence this process will need to search in entire file.
        totalProcessesForWord = 1;
    } else {
        localWordSet[localWordCount++] = wordSet[rank % numWords];
        totalProcessesForWord = ceil((size - rank % numWords) / (float)numWords);
    }

    if (totalProcessesForWord == 1) {
        start_offset = 0;
        local_n = numLines;
    } else {
        local_n = (int)ceil(numLines / (float)totalProcessesForWord);
        start_offset = 0 + local_n * (rank / numWords);
    }

    printAllottedWords(localWordSet, localWordCount, totalProcessesForWord, rank, start_offset,
                       local_n, size);

    // printLocalContents(fileContents, start_offset, local_n, rank, size);

    for (int i = 0; i < localWordCount; i++) {
        int local_ans = searchWord(fileContents, numLines, start_offset, local_n, localWordSet[i]);

        if (local_ans == 1) {
            printf("P%d found %s\n", rank, localWordSet[i]);
        }
    }

    MPI_Finalize();
}