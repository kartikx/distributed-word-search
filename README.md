# Parallel Search
A Parallel Algorithm to search a file for a list of query words. The work is distributed among a cluster of processes communicating with each other via Message Passing Interface.

To run the code:
```
make program
mpiexec -n 8 ./exec pg-being_ernest.txt AND envious clear liquid
```

For timing the algorithm you may run the timeProgram script:

```
sh timeProgram.sh
```
