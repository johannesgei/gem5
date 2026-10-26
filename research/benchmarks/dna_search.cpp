#include <iostream>
#include <string>
#include <vector>

// Streaming DNA comparison search (not a full BLAST pipeline).
constexpr int L = 1 << 20; // 1 MiB reference, beyond a 256 KiB L2
constexpr int Q = 8;

int main() {
    std::cout << "Starting DNA comparison-search kernel..." << std::endl;

    std::string bases = "ACGT";
    std::vector<char> ref(L);
    for (int i = 0; i < L; ++i) {
        ref[i] = bases[i & 3];
    }
    std::vector<char> query(ref.begin() + L / 3, ref.begin() + L / 3 + Q);

    int best_pos = -1;
    int best_d = Q + 1;
    for (int i = 0; i <= L - Q; ++i) {
        int d = 0;
        for (int j = 0; j < Q; ++j) {
            d += (ref[i + j] != query[j]);
        }
        if (d < best_d) {
            best_d = d;
            best_pos = i;
        }
    }

    std::cout << "Kernel completed successfully. best_pos = " << best_pos
              << " dist = " << best_d << std::endl;
    return 0;
}
