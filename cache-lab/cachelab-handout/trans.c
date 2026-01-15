/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]) {
  int v1, v2, v3, v4, v5, v6, v7, v8;
  if (M == 32) {
    for (int i = 0; i < N; i += 8) {
      for (int j = 0; j < M; j += 8) {
        for (int k = 0; k < 8; ++k) {
          v1 = A[i + k][j];
          v2 = A[i + k][j + 1];
          v3 = A[i + k][j + 2];
          v4 = A[i + k][j + 3];
          v5 = A[i + k][j + 4];
          v6 = A[i + k][j + 5];
          v7 = A[i + k][j + 6];
          v8 = A[i + k][j + 7];
          B[j + k][i] = v1;
          B[j + k][i + 1] = v2;
          B[j + k][i + 2] = v3;
          B[j + k][i + 3] = v4;
          B[j + k][i + 4] = v5;
          B[j + k][i + 5] = v6;
          B[j + k][i + 6] = v7;
          B[j + k][i + 7] = v8;
        }
        for (int k = 0; k < 8; ++k) {
          for (int l = 0; l < k; ++l) {
            v1 = B[j + k][i + l];
            B[j + k][i + l] = B[j + l][i + k];
            B[j + l][i + k] = v1;
          }
        }
      }
    }
  } else if (M == 64) {
    for (int i = 0; i < N; i += 4) {
      for (int j = 0; j < M; j += 4) {
        for (int k = 0; k < 4; ++k) {
          v1 = A[i + k][j];
          v2 = A[i + k][j + 1];
          v3 = A[i + k][j + 2];
          v4 = A[i + k][j + 3];
          B[j + k][i] = v1;
          B[j + k][i + 1] = v2;
          B[j + k][i + 2] = v3;
          B[j + k][i + 3] = v4;
        }
        for (int k = 0; k < 4; ++k) {
          for (int l = 0; l < k; ++l) {
            v1 = B[j + k][i + l];
            B[j + k][i + l] = B[j + l][i + k];
            B[j + l][i + k] = v1;
          }
        }
      }
    }
  } else if (M == 61) {
    for (int i = 0; i < 64; i += 8) {
      for (int j = 0; j < 56; j += 8) {
        for (int k = 0; k < 8; ++k) {
          v1 = A[i + k][j];
          v2 = A[i + k][j + 1];
          v3 = A[i + k][j + 2];
          v4 = A[i + k][j + 3];
          v5 = A[i + k][j + 4];
          v6 = A[i + k][j + 5];
          v7 = A[i + k][j + 6];
          v8 = A[i + k][j + 7];
          B[j + k][i] = v1;
          B[j + k][i + 1] = v2;
          B[j + k][i + 2] = v3;
          B[j + k][i + 3] = v4;
          B[j + k][i + 4] = v5;
          B[j + k][i + 5] = v6;
          B[j + k][i + 6] = v7;
          B[j + k][i + 7] = v8;
        }
        for (int k = 0; k < 8; ++k) {
          for (int l = 0; l < k; ++l) {
            v1 = B[j + k][i + l];
            B[j + k][i + l] = B[j + l][i + k];
            B[j + l][i + k] = v1;
          }
        }
      }
    }
    for (int i = 0; i < 65; i += 5) {
      for (int k = 0; k < 5; ++k) {
        v1 = A[i + k][56];
        v2 = A[i + k][57];
        v3 = A[i + k][58];
        v4 = A[i + k][59];
        v5 = A[i + k][60];
        B[56 + k][i] = v1;
        B[56 + k][i + 1] = v2;
        B[56 + k][i + 2] = v3;
        B[56 + k][i + 3] = v4;
        B[56 + k][i + 4] = v5;
      }
      for (int k = 0; k < 5; ++k) {
        for (int l = 0; l < k; ++l) {
          v1 = B[56 + k][i + l];
          B[56 + k][i + l] = B[56 + l][i + k];
          B[56 + l][i + k] = v1;
        }
      }
    }
    for (int i = 65; i < 67; ++i) {
      v1 = A[i][56];
      v2 = A[i][57];
      v3 = A[i][58];
      v4 = A[i][59];
      v5 = A[i][60];
      B[56][i] = v1;
      B[57][i] = v2;
      B[58][i] = v3;
      B[59][i] = v4;
      B[60][i] = v5;
    }
    for (int j = 0; j < 54; j += 3) {
      for (int k = 0; k < 3; ++k) {
        v1 = A[64 + k][j];
        v2 = A[64 + k][j + 1];
        v3 = A[64 + k][j + 2];
        B[j + k][64] = v1;
        B[j + k][65] = v2;
        B[j + k][66] = v3;
      }
      for (int k = 0; k < 3; ++k) {
        for (int l = 0; l < k; ++l) {
          v1 = B[j + k][64 + l];
          B[j + k][64 + l] = B[j + l][64 + k];
          B[j + l][64 + k] = v1;
        }
      }
    }
    for (int i = 64; i < 67; ++i) {
      v1 = A[i][54];
      v2 = A[i][55];
      B[54][i] = v1;
      B[55][i] = v2;
    }
  }
}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

/*
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N]) {
  int i, j, tmp;

  for (i = 0; i < N; i++) {
    for (j = 0; j < M; j++) {
      tmp = A[i][j];
      B[j][i] = tmp;
    }
  }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions() {
  /* Register your solution function */
  registerTransFunction(transpose_submit, transpose_submit_desc);

  /* Register any additional transpose functions */
  registerTransFunction(trans, trans_desc);
}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N]) {
  int i, j;

  for (i = 0; i < N; i++) {
    for (j = 0; j < M; ++j) {
      if (A[i][j] != B[j][i]) {
        return 0;
      }
    }
  }
  return 1;
}
