#include <iostream>
#include <vector>

// LS channel estimation analog: h[nt] = sum_sc Y[sc, nt] * p[sc]
constexpr int Nt = 32;
constexpr int Nsc = 16384; // Y is Nsc*Nt; sized to miss a 256 KiB L2

int main() {
    std::cout << "Starting LS channel estimation kernel..." << std::endl;

    std::vector<float> Y(Nsc * Nt, 1.5f);
    std::vector<float> p(Nsc, 0.5f);
    std::vector<float> h(Nt, 0.0f);

    for (int n = 0; n < Nt; ++n) {
        float acc = 0.0f;
        for (int sc = 0; sc < Nsc; ++sc) {
            acc += Y[sc * Nt + n] * p[sc];
        }
        h[n] = acc;
    }

    std::cout << "Kernel completed successfully. h[0] = " << h[0] << std::endl;
    return 0;
}
