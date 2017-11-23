#include <mpi.h>

int main(int argc, char** argv) {

 	MPI_Init(&argc, &argv);

	int world_size, wrank;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

  

	MPI_Finalize();
	return 0;
}