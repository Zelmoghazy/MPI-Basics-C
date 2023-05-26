#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

/* 
 * - No shared memory between processes in different machines only way of communication is through message passing
 * - Used in distributed systems and high performance computing
 * - Not targeted for shared memory machines but still works although rarely used as it degrades performance
 *   due to overhead of send and recieve of message passing
 * - Running as a set of processes No shared Variables
 * - All communication, synchronization require subroutine calls
 *
 * - MPI : Message Passing Interface
 *     -> Processes running on different CPUs communicate with each other
 *     -> Various communication patterns that are supported by MPI are abstracted in a "communicator"
 *     -> Communicator type: MPI_Comm -> A communicator is a group of processes that can communicate and interact with each other using MPI functions.
 *     -> Predefined default communicator: MPI_COMM_WORLD -> predefined constant representing the communicator that encompasses all processes in a parallel program.
 *     -> Number of concurrent processes to be run is defined at runtime
 *     -> MPI functions return error codes or MPI_SUCCESS it doesnt return values.
 * 
 * 
 *    - int MPI_Init(int *argc, char ***argv); : Configure our program to run MPI, must be called before any MPI functions
 *        -> takes the address of command line arguments (in/out argument), search for specific commands to MPI use it to configure and remove them.
 * 
 *    - int MPI_Finalize(); : Called at the end of the program, close communication and free allocated memory.
 * 
 *    - int MPI_Comm_size(MPI_Comm comm, int *size); : get the number of processes that are communicating inside our communicator
 *        -> takes a pointer to an integer int* size (out argument), we should define a variable size and pass its address to this function to be updated
 *    
 *    (Each process has its own unique identifier, called a "rank", which is used to distinguish it from other processes)
 * 
 *    - int MPI_Comm_rank(MPI_Comm comm, int *rank); : retrieve the rank of the calling process within a specified communicator.
 *        -> takes a pointer to an integer int* rank (out argument), we should define a variable rank and pass its address to this function to be updated
 * 
 *    (Transmit from one process to another process)
 * 
 *    - int MPI_Send(void *buf,             : Generic pointer to anything in memory to fetch and send
 *                   int count,             : number of primitive element in this buffer determined by datatype.
 *                   MPI_Datatype datatype, : constants defined like MPI_CHAR as an abstraction to actual data type that allows comm. bet. different machines.
 *                   int dest,              : rank of process which we want to transmit the buffer to.
 *                   int tag,               : Generic integer used to distinguish one type of a message from another to same destination.
 *                   MPI_Comm comm )        : Communicator -> MPI_COMM_WORLD.
 * 
 *    - int MPI_Recv(void *buf,             : Generic pointer to place in memory where you want to recieve the incoming data (out argument).
 *                   int count,             : number of primitive element in this buffer determined by datatype.
 *                   MPI_Datatype datatype, : constants defined like MPI_CHAR as an abstraction to actual data type that allows comm. bet. different machines.
 *                   int source,            : rank of process which we want to recieve from.
 *                   int tag,               : select kind of message I want to recieve from source.
 *                   MPI_Comm comm          : Communicator -> MPI_COMM_WORLD.
 *                   MPI_Status *status )   : Info about success or failure of communication from another process.
 * 
 *    (Reciever can get a message without knowing the amount of data, the sender MPI_ANY_SOURCE or the tag MPI_ANY_TAG)
 * 
 *    - typedef struct MPI_Status {
 *         int MPI_SOURCE; : rank of the process where the information came from
 *         int MPI_TAG;    : tag value sent by the sender
 *         int MPI_ERROR;  : indication that anything gone wrong
 *      };
 * 
 *      MPI_Status status;
 *      MPI_Recv(...,&status);
 *      tag = status.MPI_TAG;
 *      source = status.MPI_SOURCE
 *      error = status.MPI_ERROR;
 * 
 *      - MPI_WTIME() : returns the current time with a double float.
 *          -> to time a program segment :
 *             start_time = MPI_Wtime();
 *             end_time = MPI_Wtime();
 *             Time spent = end_time - start_time;
 * 
 */

double f(double x){
    return x*x;
}
/* Sum of trapezoid areas = base_length * [((f(x_0) + f(x_n))/2) + f(x1) + ... + f(x_n-1)] */

double Trap(double left_endpt, double right_endpt, int trap_count){
    double estimate;
    double x;
    double base_len = (right_endpt-left_endpt)/trap_count;

    estimate = (f(left_endpt) + f(right_endpt)) / 2.0;
    for (int i = 1; i < trap_count; i++)
    {
        x = left_endpt + i*base_len;
        estimate += f(x);
    }
    estimate = estimate * base_len;
    return estimate;
}

int main(void){
    int num_procs;
    int rank;
    int n = 1024;
    int local_n;
    double left_endpt;
    double right_endpt;
    double base_len ;
    double local_left_endpt;
    double local_right_endpt;
    double total_integration;
    double local_integration;

    MPI_Init(NULL,NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if(rank == 0){
        printf("Enter the starting point of integration :\n");
        scanf("%lf",&left_endpt);
        printf("Enter the ending point of integration :\n");
        scanf("%lf",&right_endpt);
    }

    if(rank == 0){
        for (int rec = 1; rec < num_procs; rec++){
            MPI_Send(&left_endpt,1,MPI_DOUBLE,rec,1,MPI_COMM_WORLD);
            MPI_Send(&right_endpt,1,MPI_DOUBLE,rec,2,MPI_COMM_WORLD);
        }
    }else{
        MPI_Recv(&left_endpt,1,MPI_DOUBLE,0,1,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        MPI_Recv(&right_endpt,1,MPI_DOUBLE,0,2,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    }

    base_len = (right_endpt - left_endpt) / n;
    local_n = n / num_procs; // depends on number of processes.

    local_left_endpt = left_endpt + rank * local_n * base_len; // partitition
    local_right_endpt = local_left_endpt + local_n * base_len;

    local_integration = Trap(local_left_endpt,local_right_endpt,local_n);

    if(rank != 0){
        MPI_Send(&local_integration,1,MPI_DOUBLE,0,0,MPI_COMM_WORLD); //send result to rank = 0 , tag = 0
    }else{
        total_integration = local_integration;
        for (int source = 1; source < num_procs; source++)
        {
            MPI_Recv(&local_integration,1,MPI_DOUBLE,source,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            total_integration+=local_integration;
        }
        printf("with n = %d trapezoids\n",n);
        printf("Estimation from %f to %f = %.5lf\n",left_endpt,right_endpt,total_integration);
    }
    MPI_Finalize();
    return 0;
}