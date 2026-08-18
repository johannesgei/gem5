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
    double alpha = 2.5;
    
    // Speicherallokation im gem5-System
    double* X = (double*)malloc(N * elemBytes);
    double* Y = (double*)malloc(N * elemBytes);

    if (X == NULL || Y == NULL) {
        return -1;
    }

    // Initialisierung
    for (uint64_t i = 0; i < N; i++) {
        X[i] = (double)(i * 0.5);
        Y[i] = (double)(i * 1.2);
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