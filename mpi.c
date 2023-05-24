#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <mpi.h>

#define DATA 0
#define RESULT 1

int getRandom(int m, int n){
	int a = 106, c = 1283;
	
	return (a * n + c) % m;
}

int main(int argc,char **argv) {

  Args ins__args;
  parseArgs(&ins__args, &argc, argv);

  int INITIAL_NUMBER = ins__args.start; 
  int FINAL_NUMBER = ins__args.stop;
  int N = 100;
  struct timeval ins__tstart, ins__tstop;

  int myrank, nproc;
  
  MPI_Init(&argc,&argv);

  // obtain my rank
  MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
  // and the number of processes
  MPI_Comm_size(MPI_COMM_WORLD,&nproc);

  if (myrank == 0) {
    gettimeofday(&ins__tstart, NULL);
    MPI_Request r;
    MPI_Status s;
    int start = 1;
    int *tab = (int*)malloc(sizeof(int) * FINAL_NUMBER);
    int *tab2 = malloc(sizeof(int) * FINAL_NUMBER);

    for (int i = 0; i < FINAL_NUMBER; i++) {
      tab[i] = 0;
    }

    // Master process
    MPI_Request send_request;


    for(int i=1; i < nproc; i++){
      MPI_Send (&start, 1, MPI_INTEGER, i, DATA, MPI_COMM_WORLD);
    }

    for(int i=1; i < nproc; i++){
      MPI_Isend(&start, 1, MPI_INTEGER, i, DATA, MPI_COMM_WORLD, &send_request);
    }

    for(int j = 0, number = (j+(nproc-1)*2) * N; number < INITIAL_NUMBER; j++, number = (j+(nproc-1)*2) * N){
      MPI_Recv(tab2, FINAL_NUMBER, MPI_INTEGER, MPI_ANY_SOURCE, RESULT, MPI_COMM_WORLD, &s);
      MPI_Isend(&start, 1, MPI_INTEGER, s.MPI_SOURCE, DATA, MPI_COMM_WORLD, &send_request);

      for (int i = 0; i < FINAL_NUMBER; i++) {
        tab[i] += tab2[i];
      }
    }

    // for every slave recieve 2 last results
    for(int j = 2; j < nproc*2; j++){
      MPI_Recv(tab2, FINAL_NUMBER, MPI_INTEGER, MPI_ANY_SOURCE, RESULT, MPI_COMM_WORLD, &s);
      for (int i = 0; i < FINAL_NUMBER; i++) {
        tab[i] += tab2[i];
      }
    }

    start = 0;
    for(int i=1; i < nproc; i++){
      MPI_Isend(&start, 1, MPI_INTEGER, i, DATA, MPI_COMM_WORLD, &send_request);
      MPI_Wait(&send_request, &s);
    }
    // WaitAll would probably be better

    gettimeofday(&ins__tstop, NULL);
    ins__printtime(&ins__tstart, &ins__tstop, ins__args.marker);

    for (int i = 0; i < FINAL_NUMBER; i++){
      printf("%d - %d\n", i, tab[i]);
    }

    free(tab);
    free(tab2);
  } 
  else {
    int n = myrank * 7;
    int start = 1;
    int start_second = 0;
    int *tab2 = malloc(sizeof(int) * FINAL_NUMBER);
    MPI_Request r;
    MPI_Status s;

    MPI_Recv (&start, 1, MPI_INTEGER, MPI_ANY_SOURCE, DATA, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Slave process

    do{
      MPI_Irecv(&start_second, 1, MPI_INTEGER, MPI_ANY_SOURCE, DATA,MPI_COMM_WORLD, &r);

      for (int i = 0; i < FINAL_NUMBER; i++){
        tab2[i] = 0;
      }
      for (int i = 0; i < N; i++) {
        int randomNumber = getRandom(FINAL_NUMBER, n);
        tab2[randomNumber]++;
        n = randomNumber;
      }
      MPI_Request send_request;
      MPI_Isend(tab2, FINAL_NUMBER, MPI_INTEGER, 0, RESULT, MPI_COMM_WORLD, &send_request);
      MPI_Wait(&r, &s);
      start = start_second;
    }while(start);

    free(tab2);
  }
  
  MPI_Finalize();
}
