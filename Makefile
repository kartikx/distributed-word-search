utils.o: utils.c utils.h
	mpicc -c utils.c

search_parallel.o: search_parallel.c search_parallel.h
	mpicc -c search_parallel.c

program: utils.o search_parallel.o
	mpicc -o exec utils.o search_parallel.o -lm