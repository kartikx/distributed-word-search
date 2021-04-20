#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int comp(const void* a, const void* b) { return (*(int*)a - *(int*)b); }

void printArr(int arr[], int n, int id) {
#pragma omp critical
    {
        printf("[%d]: ", id);
        for (int i = 0; i < n; i++) {
            printf("%d ", arr[i]);
        }
        printf("\n");
    }
}

void printSharedRecv(int sharedRecvBuffer[4][8], int* sharedRecvCount, int numThreads, int id) {
#pragma omp critical
    {
        printf("[%d]: ", id);
        for (int j = 0; j < sharedRecvCount[id]; j++) {
            printf("%d ", sharedRecvBuffer[id][j]);
        }
        printf("\n");
    }
}

void printFinalOutput(int localArr[], int n, int id, int numThreads, FILE* fptr) {
    for (int i = 0; i < numThreads; i++) {
#pragma omp barrier
        if (id == i) {
            for (int j = 0; j < n; j++) {
                fprintf(fptr, "%d ", localArr[j]);
            }
        }
    }
#pragma omp barrier
    if (id == 0)
        fprintf(fptr, "\n");
}

int main(int argc, char* argv[]) {
    int numThreads, numCount;
    int localSize, localStart;

    if (argc < 3) {
        printf("Incorrect usage, invoke ./a.out {numThreads} {inputFile}\n");
        exit(1);
    }

    numThreads = atoi(argv[1]);

    // Shared.
    int *globalNums, *sharedRecvCount, **sharedRecvBuffer;
    // printf("%d\n", numThreads);

    omp_set_num_threads(numThreads);
    int hc_dim = round(log(numThreads) / log(2));
    if (pow((double)2, (double)hc_dim) != (double)numThreads) {
        if (omp_get_thread_num() == 0)
            printf("Number of processes provided is not a power of 2, invalid hypercube\n");
        exit(1);
    }

    int maxGroups = pow(2, hc_dim - 1);
    int* medianBuffer = (int*)malloc(sizeof(int) * maxGroups);

    // First, T0 will store the contents of the file into a shared
    // globalNums array.
    if (omp_get_thread_num() == 0) {
        char* fName = argv[2];
        FILE* fptr = fopen(fName, "r");
        if (fptr != NULL) {
            fscanf(fptr, "%d", &numCount);
            globalNums = (int*)malloc(sizeof(int) * numCount);
            for (int i = 0; i < numCount; i++) {
                fscanf(fptr, "%d", &globalNums[i]);
            }
            fclose(fptr);
        } else {
            printf("File does not exist\n");
            exit(1);
        }

        // for (int i = 0; i < numCount; i++) {
        //     printf("%d ", globalNums[i]);
        // }
        // printf("---\n");
        // exit(0);
    }

    // For some reason, having it to be exactly numThreads is giving segfault.
    sharedRecvBuffer = (int**)malloc(sizeof(int) * numThreads * 2);
    for (int i = 0; i < numThreads; i++) {
        sharedRecvBuffer[i] = (int*)malloc(sizeof(int) * numCount * 2);
    }
    sharedRecvCount = (int*)malloc(sizeof(int) * numThreads);

    // Parallel Sequential Quick Sort on each part.
#pragma omp parallel private(localSize, localStart) shared(sharedRecvBuffer, sharedRecvCount)
    {
        int id = omp_get_thread_num();
        clock_t startTime = clock();

        // ! Currently using localArray, not a scalable solution.
        int* localArray = malloc(sizeof(int) * numCount);
        int* mergeBuf = malloc(sizeof(int) * numCount);
        localSize = (numCount / omp_get_num_threads());
        localStart = localSize * id;

        for (int i = 0; i < localSize; i++) {
            localArray[i] = globalNums[i + localStart];
        }

        // if (id == 0) {
        //     free(globalNums);
        // }

        qsort(localArray, localSize, sizeof(int), comp);

        // #pragma omp critical
        // { printArr(A + localStart, localSize, omp_get_thread_num()); }

        // printf("[%d] %d %d\n", id, localSize, localStart);
        // hc_dim = 1;
        for (int iter = 0; iter < hc_dim; iter++) {
            int color = pow(2, iter) * id / numThreads;

            if (id % ((int)pow(2, hc_dim - iter)) == 0) {
                // printf("Local Size: %d\n", localSize);
                medianBuffer[color] = localArray[localSize / 2];
                // printf("Median [%d] %d\n", id, medianBuffer[color]);
                // printf("---------\n");
            }

#pragma omp barrier

            // Everyone finds their pair process and pivot.
            if (id == 0)
                printf("--------\n");
            int median = medianBuffer[color];
            // printf("[%d] %d\n", id, median);
            // Now everyone has the median value.
            int sz = numThreads / (int)pow(2, iter);
            int base_process = id - (id % sz);
            int pair_process = base_process + (id + (sz >> 1)) % sz;

            int pivot = 0;
            while (pivot < localSize && localArray[pivot] < median) pivot++;
            int localPivot = pivot;

#pragma omp barrier
            // printf("[%d] %d\n", id, localPivot);

            // ! Is this needed?
            // #pragma omp critical
            if ((id - base_process) >= (sz >> 1)) {
                // printf("[%d] Upper\n", id);
                // Upper half needs to write it's lower half into pair process.
                for (int i = 0; i < localPivot && i < localSize; i++) {
                    sharedRecvBuffer[pair_process][i] = localArray[i];
                }

                sharedRecvCount[pair_process] = localPivot;
                // printf("Upper: [%d] Wrote %d values into [%d]\n", id, localPivot, pair_process);
            } else {
                // Lower half needs to write it's upper half into pair process.
                for (int i = localPivot; i < localSize; i++) {
                    // printf("%d\n", sharedRecvBuffer[pair_process][i - localPivot]);
                    // printf("%d\n", localArray[i]);
                    sharedRecvBuffer[pair_process][i - localPivot] = localArray[i];
                }

                sharedRecvCount[pair_process] = localSize - localPivot;
                // printf("Lower: [%d] Wrote %d values into [%d]\n", id, localSize - localPivot,
                //        pair_process);
            }

#pragma omp barrier
            // sleep(1);
            int i, j = 0, k = 0, i_end;
            int recvCount = sharedRecvCount[id];
            // printArr(sharedRecvBuffer[id], recvCount, id);
            // Upper Half.
            if ((id - base_process) >= (sz >> 1)) {
                i = localPivot, i_end = localSize;

                // Modifying local_n since number of elements changed.
                // This updated value is going to be used in later iterations.
                localSize = localSize - pivot + recvCount;
            } else {
                i = 0, i_end = localPivot;
                localSize = pivot + recvCount;
            }

            while (i < i_end && j < recvCount) {
                if (localArray[i] < sharedRecvBuffer[id][j])
                    mergeBuf[k++] = localArray[i++];
                else
                    mergeBuf[k++] = sharedRecvBuffer[id][j++];
            }
            while (i < i_end) mergeBuf[k++] = localArray[i++];
            while (j < recvCount) mergeBuf[k++] = sharedRecvBuffer[id][j++];

            for (int c = 0; c < localSize; c++) localArray[c] = mergeBuf[c];

            // printArr(localArray, localSize, id);
        }

        // Everyone to store the final localArray in their sharedRecvBuffer.
        sharedRecvCount[id] = localSize;
        for (int c = 0; c < localSize; c++) sharedRecvBuffer[id][c] = localArray[c];

        clock_t endTime = clock();
        // Stop timer here.
        // printf("\t%d\n", localSize);

        // #pragma omp critical
        // printFinalOutput(localArray, localSize, id, numThreads, stdout);
        if (id == 0) {
            FILE* outFptr = fopen("outputFile.txt", "w");
            if (outFptr == NULL)
                outFptr = stdout;

            // printf("Printing...\n");
            for (int i = 0; i < numThreads; i++) {
                for (int c = 0; c < sharedRecvCount[i]; c++) {
                    fprintf(outFptr, "%d ", sharedRecvBuffer[i][c]);
                }
                // printf("\n");
            }
            fprintf(stdout, "Final output stored in outputFile.txt\n");
            fclose(outFptr);

            fprintf(stdout, "Time elapsed %lf\n", ((double)(endTime - startTime)) / CLOCKS_PER_SEC);
        }
    }

    return 0;
}