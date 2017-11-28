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

	if (wrank == 0) {
  	    n = init(&left, &right);
	}

	MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

	if (world_size > n) {
		if (wrank == 0) printf("to many process compare to size\n");
		MPI_Finalize();
		return 0;
	}

	range = n / world_size;
	MPI_Datatype Line = createLine(n);

	if (wrank != 0) {
		left = createMatrix(n);
		right = createMatrix(n);
	}

	res = createMatrix(n);

	MPI_Bcast(left.data, n, Line, 0, MPI_COMM_WORLD);
	MPI_Bcast(right.data, n, Line, 0, MPI_COMM_WORLD);

	// displayMatrix(&left);
	// displayMatrix(&right);

	int * resultLine = (int*) malloc(n*n*sizeof(int));
	int index = 0;

	for (int j=0; j<range; j++) {
		for (int col=0; col<left.cols; ++col, ++index)
		{
			int result = 0;
			for (int i=0; i<left.rows; ++i)
				result += left.data[(range*wrank+j)*left.cols + i] * right.data[col + i*left.cols];

			resultLine[index] = result;
            //printf("%d ", resultLine[index]);
		}
	}

    if(wrank == world_size - 1){
        rest = n % world_size;

        for (int j=0; j<rest; j++) {
            for (int col=0; col<left.cols; ++col, ++index)
            {
                int result = 0;
                for (int i=0; i<left.rows; ++i)
                    result += left.data[(range*(wrank+1)+j)*left.cols + i] * right.data[col + i*left.cols];
                resultLine[index] = result;
                //printf("%d ", resultLine[index]);
            }
        }
    }

    int * rcounts = (int*) malloc(world_size*sizeof(int));
    int * displs = (int*) malloc(world_size*sizeof(int));


	MPI_Bcast(&rest, 1, MPI_INT, world_size-1, MPI_COMM_WORLD);


    for(int i = 0; i < world_size; i++){
        rcounts[i] = range;
        displs[i] =  i * range;
    }

    rcounts[world_size-1] = range+rest;

    if(wrank != world_size - 1){
        rest = 0;
    }

	MPI_Gatherv(resultLine, range+rest, Line, res.data, rcounts, displs, Line, 0, MPI_COMM_WORLD);

	if (wrank == 0) displayMatrix(&res);

	MPI_Finalize();
	return 0;
}