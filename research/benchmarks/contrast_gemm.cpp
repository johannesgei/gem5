#include <iostream>
#include <vector>

// Contrast kernel: tiny GEMM that fits in L1, repeated.
// Expected PIM-suitability loser (high arithmetic intensity).
constexpr int N = 24;
constexpr int Reps = 200;

int main() {
    std::cout << "Starting in-cache contrast GEMM kernel..." << std::endl;

    std::vector<float> A(N * N, 1.5f);
    std::vector<float> B(N * N, 2.5f);
    std::vector<float> C(N * N, 0.0f);

    for (int r = 0; r < Reps; ++r) {
        for (int i = 0; i < N; ++i) {
            for (int k = 0; k < N; ++k) {
                for (int j = 0; j < N; ++j) {
                    C[i * N + j] += A[i * N + k] * B[k * N + j];
                }
            }
        }
    }

    std::cout << "Kernel completed successfully. C[0] = " << C[0] << std::endl;
    return 0;
}
