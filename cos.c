#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

unsigned long long factorialTR(unsigned long long n, unsigned long long a){
    return (n<=1)?a:factorialTR((n - 1), (a * n));
}

unsigned long long factorial(unsigned long long n){
    return factorialTR(n, 1);
}

double cosine(double x, unsigned int start, unsigned int end){
    /* Limit values to -180 -> 180 degrees*/
    if(x > M_PI ){
        x = fmod(x,M_PI);
    }else if(x < -M_PI){
        x = -1 * x;
        x = -1 * fmod(x,M_PI);
    }
    double result = 0.0;
    int sign = 1;
    for (unsigned int i = start; i < end; i++)
    {
        result+=sign*(double)(pow(x,2.0*i)/factorial(2*i));
        sign = sign *-1;
    }
    return result;
}

int main(void){
    int num_procs;
    int rank;
    int k;     // maximum number of iterations
    double x;  // input value
    double result;
    double local_result;
    double start_time;
    double end_time;

    MPI_Init(NULL,NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if(rank == 0){
        printf("Enter the number of iterations: \n");
        scanf("%d",&k);
        printf("Enter the required value in degrees :\n");
        scanf("%lf",&x);
        x = x*(M_PI/180); //degree to radian conversion 
    }

    start_time = MPI_Wtime();
    /* Send input values to all processes */
    if(rank == 0){
        for (int rec = 1; rec < num_procs; rec++){
            MPI_Send(&k,1,MPI_DOUBLE,rec,1,MPI_COMM_WORLD); //tag 1
            MPI_Send(&x,1,MPI_DOUBLE,rec,2,MPI_COMM_WORLD); //tag 2
        }
    }else{
        /* Recieve from process of rank = 0 */
        MPI_Recv(&k,1,MPI_DOUBLE,0,1,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        MPI_Recv(&x,1,MPI_DOUBLE,0,2,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    }

    int iterations = (k+1) / num_procs; 
    unsigned int start = rank * iterations;
    unsigned int end;
    /* Last process takes the rest */
    if(rank == num_procs-1){
        end = k + 1;
    }else{
        end = start + iterations;
    }

    local_result = cosine(x,start,end);    

    if(rank != 0){
        MPI_Send(&local_result,1,MPI_DOUBLE,0,0,MPI_COMM_WORLD); //send result to rank = 0 ,tag = 0
    }else{
        result = local_result;
        for (int source = 1; source < num_procs; source++)
        {
            MPI_Recv(&local_result,1,MPI_DOUBLE,source,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            result+= local_result;
        }
        printf("Computed value of cosine(x) where x = %lf is = %lf\n",x*180/M_PI,result);
        printf("Number of iterations = %d\n",k);
        end_time = MPI_Wtime();
        printf("Time taken = %lf\n",end_time-start_time);
    }
    MPI_Finalize();
    return 0;
}