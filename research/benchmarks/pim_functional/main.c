#include <stdio.h>
#include <stdint.h>
// #include "pnm_short.h"

// // #define N 1024 * 1024 // 1 Million Elemente
#define N 1
#define ELEMENT_SIZE sizeof(double) // 8 Byte

// // Definition der Register-Struktur
typedef struct {
    volatile uint64_t size;       // Offset 0x00
    volatile uint64_t elem_bytes; // Offset 0x08
    volatile uint64_t command;    // Offset 0x10
} PIMRegisters;

// // Hier wird die Sektion real im Binary verankert
__attribute__((section(".pim_regs,\"aw\",@progbits #")))
volatile PIMRegisters pim_regs;

int main() {
    printf("[CPU] Starte PNM-Test mit Makros...\n");

    printf("[CPU] Sende Parameter via struct (N=%d, Bytes=%ld)...\n", N, ELEMENT_SIZE);

    pim_regs.size       = N;
    pim_regs.elem_bytes = ELEMENT_SIZE;

    __sync_synchronize(); // Speicher-Barriere

    pim_regs.command    = 1; // PNM_CMD_AXPY

    printf("[CPU] Parameter erfolgreich übermittelt.\n");

    return 0;
}
