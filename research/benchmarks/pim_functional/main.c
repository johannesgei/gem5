#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// Nur als Fallback
#define DEFAULT_N 1024 * 1024 // 1 Million Elemente
#define DEFAULT_ELEMENT_SIZE sizeof(double) // 8 Byte

// // Definition der Register-Struktur
typedef struct {
    volatile uint64_t size;       // Offset 0x00
    volatile uint64_t elem_bytes; // Offset 0x08
    volatile uint64_t command;    // Offset 0x10
} PIMRegisters;

// // Hier wird die Sektion real im Binary verankert
__attribute__((section(".pim_regs,\"aw\",@progbits #")))
volatile PIMRegisters pim_regs;

int main(int argc, char* argv[]) {
    uint64_t n_elements = DEFAULT_N;
    uint64_t elem_bytes = DEFAULT_ELEMENT_SIZE;

    if (argc > 1) {n_elements = strtoull(argv[1], NULL, 10); }
    if (argc > 2) {elem_bytes = strtoull(argv[2], NULL, 10); }

    printf("[CPU] Sende Parameter via struct (N=%lu, Bytes=%ld)...\n", n_elements, elem_bytes);

    // Werte dynamisch in die PIM-Register schreiben
    pim_regs.size       = n_elements;
    pim_regs.elem_bytes = elem_bytes;
    __sync_synchronize(); // Speicher-Barriere

    pim_regs.command    = 1; // PIM_CMD_AXPY
    __sync_synchronize();

    printf("[CPU] Parameter erfolgreich übermittelt.\n");

    printf("[CPU] Warte auf Hardware-Antwort (Blockierendes Lesen)...\n");
    volatile uint64_t status = pim_regs.command;

    printf("[CPU] PIM fertig (Status: %lu). Beende Programm.\n", status);

    return 0;
}