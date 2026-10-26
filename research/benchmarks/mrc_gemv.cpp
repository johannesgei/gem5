#include <iostream>
#include <vector>

// MRC combining: x = H^T * y  (K users, Nt antennas, one subcarrier tile)
constexpr int Nt = 64;
constexpr int K = 16;
constexpr int Nsc = 256; // footprint knob; H does not fit in a 256 KiB L2

int main() {
    std::cout << "Starting MRC GEMV kernel..." << std::endl;

    std::vector<float> H(Nsc * Nt * K, 1.5f);
    std::vector<float> y(Nsc * Nt, 2.5f);
    std::vector<float> x(Nsc * K, 0.0f);

    for (int sc = 0; sc < Nsc; ++sc) {
        const float *Hs = H.data() + sc * Nt * K;
        const float *ys = y.data() + sc * Nt;
        float *xs = x.data() + sc * K;
        for (int i = 0; i < K; ++i) {
            float acc = 0.0f;
            for (int n = 0; n < Nt; ++n) {
                acc += Hs[n * K + i] * ys[n];
            }
            xs[i] = acc;
        }
    }

    std::cout << "Kernel completed successfully. x[0] = " << x[0] << std::endl;
    return 0;
}
