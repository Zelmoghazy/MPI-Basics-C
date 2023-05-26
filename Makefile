all:
	@mpicc mpi.c -o mpi
	@mpirun -n $(n) ./mpi
clean:
	@rm mpi