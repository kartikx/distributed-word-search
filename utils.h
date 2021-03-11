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
                        int rank, int start_offset, int local_n, int nP);

#endif