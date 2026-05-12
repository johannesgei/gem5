#include <stdint.h>
#include <stdio.h>

// Wir definieren eine Sektion namens ".pim_buffer"
__attribute__((section(".pim_buffer,\"aw\",@progbits #")))
volatile uint32_t pim_signal = 0;

int main() {
    printf("Virtuelle Adresse von pim_signal: %p\n", (void*)&pim_signal);
    pim_signal = 1;
    return 0;
}
