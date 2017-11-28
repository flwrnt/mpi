#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Matrix_
{
	int rows;
	int cols;
	int* data;
} Matrix;

Matrix createMatrix(int size)
{
	Matrix result;
	result.rows = size;
	result.cols = size;
	result.data = (int*) malloc(size*size*sizeof(int));

  if (!result.data) printf("matrix allocation error\n");

	return result;
}

MPI_Datatype createLine(int cols)
{
	MPI_Datatype result;
	MPI_Type_contiguous(cols, MPI_INT, &result);
	MPI_Type_commit(&result);

	return result;
}

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
	while (fscanf(stdin, "%d", &right->data[i]) != EOF) {
		i++;
  }

	return size;
}

void displayMatrix(Matrix *matrix){
	printf("[ ");
	for (int i=0; i<matrix->cols*matrix->rows; i++) 
		printf("%d ", matrix->data[i]);
	printf("]\n");
}

int main(int argc, char** argv) {

 	MPI_Init(&argc, &argv);

	int world_size, wrank;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

    Matrix left, right, res;
	int n, range, rest = 0;
	int executionArray[world_size];
	int displs[world_size];
	double Tbegin, Tend;

	if (wrank == 0) {
		n = init(&left, &right);
	}

	MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

	if (world_size > n) {
		if (wrank == 0) printf("to many process compare to size\n");
		MPI_Finalize();
		return 0;
	}

	Tbegin = MPI_Wtime(); 

	range = n / world_size;
	MPI_Datatype Line = createLine(n);

	if (wrank != 0) {
		left = createMatrix(n);
		right = createMatrix(n);
	}else{
        rest = n % world_size;
		
		displs[0] = 0;
		executionArray[0] = range;
		
		if(0 < rest)
			executionArray[0] += 1;

		for(int i = 1; i < world_size; i++){
        	executionArray[i] = range;

			if(i < rest)
				executionArray[i] += 1;

        	displs[i] =  displs[i - 1] + executionArray[i - 1];
   		 }
	}

	MPI_Bcast(&executionArray, world_size, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&displs, world_size, MPI_INT, 0, MPI_COMM_WORLD);

	res = createMatrix(n);

	MPI_Bcast(left.data, n, Line, 0, MPI_COMM_WORLD);
	MPI_Bcast(right.data, n, Line, 0, MPI_COMM_WORLD);

	int * resultLine = (int*) malloc(n*n*sizeof(int));
	int index = 0;

	for (int j=0; j<executionArray[wrank]; j++) {
		for (int col=0; col<left.cols; ++col, ++index)
		{
			int result = 0;
			for (int i=0; i<left.rows; ++i)
				result += left.data[(displs[wrank]+j)*left.cols + i] * right.data[col + i*left.cols];

			resultLine[index] = result;
		}
	}

	MPI_Gatherv(resultLine, executionArray[wrank], Line, res.data, executionArray, displs, Line, 0, MPI_COMM_WORLD);

	Tend = MPI_Wtime(); 

	if (wrank == 0) {
		printf("Parallel : %f\n", Tend - Tbegin);
		// displayMatrix(&res);

		Matrix result = createMatrix(n);
		Tbegin = MPI_Wtime();
		for (int row=0; row<left.rows; ++row)
		{
			for (int col=0; col<left.cols; ++col)
			{
				int one = 0;
				for (int i=0; i<left.rows; ++i)
					one += left.data[row*left.cols + i] * right.data[col + i*right.cols];

				result.data[row*result.cols + col] = one;
			}
		}

		printf("Serial : %f\n", MPI_Wtime()-Tbegin);

		// Comparaison des résultats.
		int success = 1;
		for (int i=0; i<result.rows*result.cols; ++i) {
			if (res.data[i] != result.data[i]) {
				success = 0;
				break;
			}
		}
		printf("Succès ? %d.\n", success);

		free(res.data);
		free(result.data);
	}
	MPI_Type_free(&Line);
	free(left.data);
	free(right.data);
	free(resultLine);
	
	MPI_Finalize();

	return 0;
}