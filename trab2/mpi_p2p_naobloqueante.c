// MPI Matrix Multiplication using Point-to-Point Non-Blocking Communication (instrumentado)
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  // para memcpy

void initialize_matrices(int n, double* A, double* B, double* C) {
    for (int i = 0; i < n * n; i++) {
        A[i] = i % 100;
        B[i] = (i % 100) + 1;
        C[i] = 0.0;
    }
}

int main(int argc, char* argv[]) {
    int rank, size, n = atoi(argv[1]);
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    double *A, *B, *C;
    A = (double*)malloc(n * n * sizeof(double));
    B = (double*)malloc(n * n * sizeof(double));
    C = (double*)malloc(n * n * sizeof(double));

    if (rank == 0) {
        initialize_matrices(n, A, B, C);
    }

    double *local_A = (double*)malloc((n * n / size) * sizeof(double));
    double *local_C = (double*)malloc((n * n / size) * sizeof(double));

    MPI_Request request;
    double comm_time = 0.0, comp_time = 0.0;
    double t_start = 0.0, t_end = 0.0;

    if (rank == 0) {
        t_start = MPI_Wtime();
    }

    // Distribuição não-bloqueante de A
    if (rank == 0) {
        double t0 = MPI_Wtime();
        for (int i = 1; i < size; i++) {
            MPI_Isend(A + i * (n * n / size),
                      n * n / size, MPI_DOUBLE,
                      i, 0, MPI_COMM_WORLD, &request);
        }
        comm_time += MPI_Wtime() - t0;
        for (int i = 0; i < n * n / size; i++) {
            local_A[i] = A[i];
        }
    } else {
        double t0 = MPI_Wtime();
        MPI_Irecv(local_A, n * n / size, MPI_DOUBLE,
                  0, 0, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, MPI_STATUS_IGNORE);
        comm_time += MPI_Wtime() - t0;
    }

    // Broadcast não-bloqueante de B
    double t0 = MPI_Wtime();
    MPI_Ibcast(B, n * n, MPI_DOUBLE, 0, MPI_COMM_WORLD, &request);
    MPI_Wait(&request, MPI_STATUS_IGNORE);
    comm_time += MPI_Wtime() - t0;

    // Computação local
    t0 = MPI_Wtime();
    for (int i = 0; i < n / size; i++) {
        for (int j = 0; j < n; j++) {
            local_C[i * n + j] = 0.0;
            for (int k = 0; k < n; k++) {
                local_C[i * n + j] += local_A[i * n + k] * B[k * n + j];
            }
        }
    }
    comp_time += MPI_Wtime() - t0;

    // Reunião não-bloqueante de C
    if (rank == 0) {
        for (int i = 0; i < n * n / size; i++) {
            C[i] = local_C[i];
        }
        for (int i = 1; i < size; i++) {
            double t1 = MPI_Wtime();
            MPI_Irecv(C + i * (n * n / size),
                      n * n / size, MPI_DOUBLE,
                      i, 1, MPI_COMM_WORLD, &request);
            MPI_Wait(&request, MPI_STATUS_IGNORE);
            comm_time += MPI_Wtime() - t1;
        }
    } else {
        double t1 = MPI_Wtime();
        MPI_Isend(local_C, n * n / size, MPI_DOUBLE,
                  0, 1, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, MPI_STATUS_IGNORE);
        comm_time += MPI_Wtime() - t1;
    }

    if (rank == 0) {
        t_end = MPI_Wtime();
        printf("Total exec: %.6f  Comm: %.6f  Comp: %.6f\n",
               t_end - t_start, comm_time, comp_time);
    }

    free(A); free(B); free(C);
    free(local_A); free(local_C);
    MPI_Finalize();
    return 0;
}
