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
    // printf("%d\n", numLines);
    // exit(0);

    printContents(fileContents);

    // for (int i = 0; i < numWords; i++) searchWord(fileContents, 0, numLines, wordSet[i]);

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int local_n, start_offset;

    int maxLocalSize = (int)ceil((float)numWords / size);

    // Stores the words alloted to this process.
    char* localWordSet[maxLocalSize];

    // Stores the corresponding global indices into wordSet for each local word.
    int wordIndex[maxLocalSize];

    int localWordCount = 0;

    // Stores total number of processes that have been allotted the same word
    // as this process. Only useful when each process gets only 1 word.
    int totalProcessesForWord;

    double start_time, elapsed_time, max_time;

    // start timer
    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();

    // Some processes get more than one word.
    if (size < numWords) {
        for (int i = 0; i < numWords; i++) {
            if (i % size == rank) {
                localWordSet[localWordCount] = wordSet[i];
                wordIndex[localWordCount] = i;
                localWordCount++;
            }
        }

        // This word is only present in this process.
        // Hence this process will need to search in entire file.
        totalProcessesForWord = 1;
    } else {
        localWordSet[localWordCount] = wordSet[rank % numWords];
        wordIndex[localWordCount] = rank % numWords;
        totalProcessesForWord = ceil((size - rank % numWords) / (float)numWords);
        localWordCount++;
    }

    if (totalProcessesForWord == 1) {
        start_offset = 0;
        local_n = numLines;
    } else {
        local_n = (int)ceil(numLines / (float)totalProcessesForWord);
        start_offset = 0 + local_n * (rank / numWords);
    }

    // printAllottedWords(localWordSet, localWordCount, totalProcessesForWord, wordIndex, rank,
    //                    start_offset, local_n, size);

    // printLocalContents(fileContents, start_offset, local_n, rank, size);

    int sendbuf;

    int foundWords[numWords];
    memset(foundWords, 0, numWords * sizeof(int));
    int foundCount = 0;

    // Perform Local Search and send messages to P0
    for (int i = 0; i < localWordCount; i++) {
        int local_ans = searchWord(fileContents, numLines, start_offset, local_n, localWordSet[i]);
        sendbuf = wordIndex[i];
        if (local_ans == true) {
            // printf("P%d found %s\n", rank, localWordSet[i]);

            if (rank != 0) {
                MPI_Send(&sendbuf, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                printf("P%d sent\n", rank);
            } else {
                if (foundWords[wordIndex[i]] == 0) {
                    foundCount += 1;
                }
                foundWords[wordIndex[i]] = 1;
            }
        } else {
            // All indices returned in the message should be between [0 .. MAX_WORDSET - 1]
            // A value greater indicates that the word wasn't found.
            sendbuf += MAX_WORDSET;
            if (rank != 0) {
                MPI_Send(&sendbuf, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                // printf("P%d sent\n", rank);
            }
        }
    }

    // P0 receives messages.
    if (rank == 0) {
        int totalMessages;
        if (size <= numWords) {
            totalMessages = numWords - localWordCount;
        } else {
            totalMessages = size - 1;
        }

        // printf("P0 expects %d messages\n", totalMessages);

        int recv;
        for (int i = 0; i < totalMessages; i++) {
            MPI_Recv(&recv, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // printf("P0 recvd %d\n", recv);
            if (recv < MAX_WORDSET && foundWords[recv] == 0) {
                foundCount += 1;
                foundWords[recv] = 1;
            }
        }

        // printf("Expecting %d, Foundcount is: %d\n", numWords, foundCount);

        if (option == OR && foundCount > 0) {
            printf("Found atleast one word\n");
            printf("Query successful\n");
        } else if (option == AND && foundCount == numWords) {
            printf("Found all %d words\n", numWords);
            printf("Query successful\n");
        } else {
            printf("Query unsuccessful\n");
        }
    }

    elapsed_time = MPI_Wtime() - start_time;

    MPI_Reduce(&elapsed_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("Time taken: %lfs\n", max_time);
    }

    // printf("P%d exited\n", rank);

    MPI_Finalize();
}