#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>

void printContents(char** fileContents);
void printLocalContents(char** fileContents, int start, int size, int rank, int nP);
int isValidWord(char* wordPtr);
int searchWord(char** fileContents, int totalLines, int start, int size, char* word);
int readFile(FILE* fp, char** fileContents);
void initWordSet(char* wordSet[], int numWords, char* argv[]);
void printAllottedWords(char* localWordSet[], int localWordCount, int totalProcessesForWord,
                        int* wordIndex, int rank, int start_offset, int local_n, int nP);
void printArray(int foundWords[], int numWords, int rank);
int getArraySum(int foundWords[], int numWords);

void readLocalFile(FILE* fp, int start_offset, int local_n, char** wordSet, int* localAns,
                   int numWords, int rank);
int isValidChar(char ch);
void searchWordInBuffer(char* buf, char** wordSet, int localAns[], int numWords, int rank);

#endif