#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Nutzung: %s <N> <ElementBytes>\n", argv[0]);
        return 1;
    }

    uint64_t N = strtoull(argv[1], NULL, 10);
    uint64_t elemBytes = strtoull(argv[2], NULL, 10);
    _Float16 alpha = 2.5;
    
    // Speicherallokation im gem5-System
    _Float16* X = (_Float16*)malloc(N * elemBytes);
    _Float16* Y = (_Float16*)malloc(N * elemBytes);

    if (X == NULL || Y == NULL) {
        return -1;
    }

    // Initialisierung
    for (uint64_t i = 0; i < N; i++) {
        X[i] = (_Float16)(i * 0.5);
        Y[i] = (_Float16)(i * 1.2);
    }

    // --- START DER MESSRELEVANTEN ZONE ---
    for (uint64_t i = 0; i < N; i++) {
        Y[i] = alpha * X[i] + Y[i];
        __asm__ __volatile__("" : : "g"(X), "g"(Y) : "memory");
    }
    __asm__ __volatile__("" : : "g"(Y) : "memory");
    // --- ENDE DER MESSRELEVANTEN ZONE ---

    // if (N > 0) {
    //     printf("Berechnung abgeschlossen. Letztes Element Y[%lu] = %f\n", N-1, Y[N-1]);
    // }

    free(X);
    free(Y);
    return 0;
}