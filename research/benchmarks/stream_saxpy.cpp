#include <iostream>
#include <vector>

// STREAM-like SAXPY: y = a*x + y (bandwidth ceiling)
constexpr int N = 1 << 17; // two float arrays ~1 MiB total, beyond 256 KiB L2

int main() {
    std::cout << "Starting STREAM SAXPY kernel..." << std::endl;

    std::vector<float> x(N, 1.5f);
    std::vector<float> y(N, 2.5f);
    const float a = 1.5f;

    for (int i = 0; i < N; ++i) {
        y[i] = a * x[i] + y[i];
    }

    std::cout << "Kernel completed successfully. y[0] = " << y[0] << std::endl;
    return 0;
}
