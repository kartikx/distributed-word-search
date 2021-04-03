#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

int comp(const void* a, const void* b) { return (*(int*)a - *(int*)b); }

void printArr(int arr[], int n, int id) {
    printf("[%d]: ", id);
    for (int i = 0; i < n; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
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

int main() {
    int numThreads;
    numThreads = 4;

    int A[] = {4, 3, 2, 1, 0, 5, 8, 6};
    int numCount = 8;

    int hc_dim = round(log(numThreads) / log(2));

    omp_set_num_threads(numThreads);

    int localSize, localStart;

    // Shared.
    int median;

    int sharedRecvBuffer[numThreads][numCount];
    int sharedRecvCount[numThreads];

    // Parallel Sequential Quick Sort on each part.
#pragma omp parallel private(localSize, localStart) shared(sharedRecvBuffer, sharedRecvCount)
    {
        int id = omp_get_thread_num();

        // ! Currently using localArray, not a scalable solution.
        int localArray[numCount];
        localSize = (numCount / omp_get_num_threads());
        localStart = localSize * omp_get_thread_num();

        for (int i = 0; i < localSize; i++) {
            localArray[i] = A[i + localStart];
        }

        qsort(localArray, localSize, sizeof(int), comp);

        // #pragma omp critical
        // { printArr(A + localStart, localSize, omp_get_thread_num()); }

        // printf("[%d] %d %d\n", id, localSize, localStart);
        hc_dim = 1;
        for (int iter = 0; iter < hc_dim; iter++) {
            if (id == 0) {
                // printf("Local Size: %d\n", localSize);
                median = localArray[localSize / 2];
            }

#pragma omp barrier
            //             printf("[%d] %d\n", id, median);

            // Now everyone has the median value.
            int sz = numThreads / (int)pow(2, iter);
            int base_process = id - (id % sz);
            int pair_process = base_process + (id + (sz >> 1)) % sz;

            int pivot = 0;
            while (pivot < (localStart + localSize) && A[pivot] < median) pivot++;
            int localPivot = pivot - localStart;

#pragma omp barrier
            // printf("[%d] %d\n", id, localPivot);

            // ! Is this needed?
            // #pragma omp critical
            if (id >= (sz >> 1)) {
                // printf("[%d] Upper\n", id);
                // Upper half needs to write it's lower half into pair process.
                for (int i = 0; i < localPivot; i++) {
                    sharedRecvBuffer[pair_process][i] = A[localStart + i];
                }

                sharedRecvCount[pair_process] = localPivot;
                // printf("1. [%d] Wrote %d values into [%d]\n", id, localPivot, pair_process);
            } else {
                // Lower half needs to write it's upper half into pair process.
                for (int i = localPivot; i < localSize; i++) {
                    sharedRecvBuffer[pair_process][i - localPivot] = A[localStart + i];
                }

                sharedRecvCount[pair_process] = localSize - localPivot;
                // printf("2. [%d] Wrote %d values into [%d]\n", id, localSize - localPivot,
                //    pair_process);
            }

#pragma omp barrier
            printf("[%d] %d\n", id, sum);

#pragma omp barrier
            {
                if (id == 0) {
                    printf("-----------\n");
                }
            }

            localSize <<= 1;
        }
    }

    return 0;
}