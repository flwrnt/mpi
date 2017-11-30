#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Matrix_ {
	int rows;
	int cols;
	int* data;
} Matrix;

/**
 * create matrix & allocate memory 
 */
Matrix createMatrix(int size) {
	Matrix result;
	result.rows = size;
	result.cols = size;
	result.data = (int*) malloc(size*size*sizeof(int));

  if (!result.data) printf("matrix allocation error\n");

	return result;
}

/**
 * create Line type
 */
MPI_Datatype createLine(int cols) {
	MPI_Datatype result;
	MPI_Type_contiguous(cols, MPI_INT, &result);
	MPI_Type_commit(&result);

	return result;
}

/**
 * init left and right matrix from stdin
 */
int init(Matrix *left, Matrix *right) {
	int size, i = 0;
	char c;

	fscanf(stdin, "%d", &size);

	*left = createMatrix(size);
	*right = createMatrix(size);

	while (fscanf(stdin, "%d%c", &left->data[i], &c) != EOF) {
    if (c == '\n') break;
		i++;
  }
	i = 0;
	while (fscanf(stdin, "%d", &right->data[i]) != EOF) i++;

	return size;
}

/**
 * display matrix 
 */
void displayMatrix(Matrix *matrix){
	printf("[ ");
	for (int i=0; i<matrix->cols*matrix->rows; i++) 
		printf("%d ", matrix->data[i]);
	printf("]\n");
}

int main(int argc, char **argv) {

 	MPI_Init(&argc, &argv);

	int world_size, wrank;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

	Matrix left, right, final;
	int n, mult, rest = 0, display = 0, serial = 0;
	int executionArray[world_size];
	int displs[world_size];
	double begin;

	if (wrank == 0) {
		// get -p option to display matrix
		for (int i=0; i<argc; i++) {
			if (strchr(argv[i], '-') && strchr(argv[i], 'p'))
				display = 1;
			if (strchr(argv[i], '-') && strchr(argv[i], 's'))
				serial = 1;
		}
		// init left, right & result matrices
		n = init(&left, &right);
		final = createMatrix(n);
	}

	// broadcast size to other process
	MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

	if (world_size > n) {
		if (wrank == 0) printf("to many process compare to size\n");
		MPI_Finalize();
		return 0;
	}

	begin = MPI_Wtime(); 

	// get line count by process
	mult = n / world_size;
	// create Line MPI type
	MPI_Datatype Line = createLine(n);

	if (wrank != 0) {
		// allocate matrices for other processes
		left = createMatrix(n);
		right = createMatrix(n);
	}
	else{
		// calc rest && fill execution & offset arrays
		rest = n % world_size;
		
		for(int i = 0; i < world_size; i++){
			executionArray[i] = mult;

			if(i < rest)
				executionArray[i] += 1;

			displs[i] = i==0 ? 0 : displs[i-1] + executionArray[i-1];
		}
	}

	// broadcast execution & offset arrays
	MPI_Bcast(&executionArray, world_size, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&displs, world_size, MPI_INT, 0, MPI_COMM_WORLD);

	int * leftLines = (int*) malloc(executionArray[wrank]*n*sizeof(int));
	MPI_Scatterv(left.data, executionArray, displs, Line, leftLines, executionArray[wrank], Line, 0, MPI_COMM_WORLD);

	// broadcast right matrix
	MPI_Bcast(right.data, n, Line, 0, MPI_COMM_WORLD);

	// allocate result lines for each process
	int * resultLine = (int*) malloc(executionArray[wrank]*n*sizeof(int));
	int index = 0;

	// calculate lines by process
	for (int j=0; j<executionArray[wrank]; j++) {
		for (int col=0; col<left.cols; ++col, ++index) {
			int tmp = 0;
			for (int i=0; i<left.rows; ++i)
				tmp += leftLines[j*left.rows + i] * right.data[col + i*left.cols];

			resultLine[index] = tmp;
		}
	}

	// send back all results lines to process 0
	MPI_Gatherv(resultLine, executionArray[wrank], Line, final.data, executionArray, displs, Line, 0, MPI_COMM_WORLD);

	if (wrank == 0) {
		fprintf(stderr, "Parallel: \t%f sec\n", MPI_Wtime() - begin);
		if (display) displayMatrix(&final);
		 	if (serial) {
				// execute serial version to compare
				Matrix result = createMatrix(n);
				begin = MPI_Wtime();
				for (int row=0; row<left.rows; ++row) {
					for (int col=0; col<left.cols; ++col) {
						int tmp = 0;
						for (int i=0; i<left.rows; ++i)
							tmp += left.data[row*left.cols + i] * right.data[col + i*right.cols];

						result.data[row*result.cols + col] = tmp;
					}
				}
				fprintf(stderr, "Serial: \t%f sec\n", MPI_Wtime()-begin);

				// compare results
				int success = 1;
				for (int i=0; i<result.rows*result.cols; ++i) {
					if (final.data[i] != result.data[i]) {
						success = 0;
						break;
					}
				}
				fprintf(stderr, "Success: \t%s\n", success ? "true" : "false");
				free(result.data);
		 	}
		free(final.data);
	}
	MPI_Type_free(&Line);
	free(left.data);
	free(right.data);
	free(resultLine);
	
	MPI_Finalize();

	return 0;
}
