#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#include "gem5/m5ops.h" // Important for stats

// Function for Givens-Rotation
void apply_givens(double &a, double &b, double &c, double &s) {
    double r = std::hypot(a, b);
    if (r == 0) {
        c = 1.0; s = 0.0;
    } else {
        c = a / r;
        s = -b / r;
        a = r;
        b = 0.0;
    }
}

int main() {
    #ifndef MATRIX_SIZE
        #define MATRIX_SIZE 100
    #endif

    int N = MATRIX_SIZE;

    // Statt als Pointer-chasing, lieber als 1D vector initialisieren, sodass
    // die Daten zusammenhängend im Speicher liegen.
    std::vector<std::vector<double>> R(N, std::vector<double>(N, 1.0));
    std::vector<double> b(N, 1.0);

    std::cout << "Starting QR-Givens for N = " << N << "..." << std::endl;

    // --- GEM5 START MEASUREMENT ---
    // m5_reset_stats(0, 0);

    for (int j = 0; j < N; j++) {
        for (int i = N - 1; i > j; i--) {
            double c, s;
            apply_givens(R[i-1][j], R[i][j], c, s);

            // Row update
            for (int k = j + 1; k < N; k++) {
                double temp = c * R[i-1][k] - s * R[i][k];
                R[i][k] = s * R[i-1][k] + c * R[i][k];
                R[i-1][k] = temp;
            }
            // b-vector update
            double temp_b = c * b[i-1] - s * b[i];
            b[i] = s * b[i-1] + c * b[i];
            b[i-1] = temp_b;
        }
    }

    // --- GEM5 END MEASUREMENT ---
    // m5_dump_stats(0, 0);

    std::cout << "QR-Givens finished." << std::endl;
    return 0;
}
