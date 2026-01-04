/*
 * hardfault_handler.cpp
 */
/**
 * Using Cortex-M3/M4/M7 Fault Exceptions
 * MDK Tutorial
 * AN209, Summer 2017, V 5.0
 */

#include <cstdio>
#include <cstdint>

#include "gd32.h"

extern "C" void HardFault_Handler()
{
    __asm volatile(
        "TST LR, #4\n"
        "ITE EQ\n"
        "MRSEQ R0, MSP\n"
        "MRSNE R0, PSP\n"
        "MOV R1, LR\n"
        "B HardfaultHandler\n");
}

extern "C" void HardfaultHandler(uint32_t* hardfault_args, uint32_t lr_value)
{
    uint32_t cfsr;
    uint32_t bus_fault_address;
    uint32_t memmanage_fault_address;

    bus_fault_address = SCB->BFAR;
    memmanage_fault_address = SCB->MMFAR;
    cfsr = SCB->CFSR;

    const auto kStackedR0 = hardfault_args[0];
    const auto kStackedR1 = hardfault_args[1];
    const auto kStackedR2 = hardfault_args[2];
    const auto kStackedR3 = hardfault_args[3];
    const auto kStackedR12 = hardfault_args[4];
    const auto kStackedLr = hardfault_args[5];
    const auto kStackedPc = hardfault_args[6];
    const auto kStackedPsr = hardfault_args[7];

    printf("[HardFault]\n");
    printf("- Stack frame:\n");
    printf(" R0  = %x\n", (unsigned int)kStackedR0);
    printf(" R1  = %x\n", (unsigned int)kStackedR1);
    printf(" R2  = %x\n", (unsigned int)kStackedR2);
    printf(" R3  = %x\n", (unsigned int)kStackedR3);
    printf(" R12 = %x\n", (unsigned int)kStackedR12);
    printf(" LR  = %x\n", (unsigned int)kStackedLr);
    printf(" PC  = %x\n", (unsigned int)kStackedPc);
    printf(" PSR = %x\n", (unsigned int)kStackedPsr);
    printf("- FSR/FAR:\n");
    printf(" CFSR = %x\n", (unsigned int)cfsr);
    printf(" HFSR = %x\n", (unsigned int)SCB->HFSR);
    printf(" DFSR = %x\n", (unsigned int)SCB->DFSR);
    printf(" AFSR = %x\n", (unsigned int)SCB->AFSR);
    if (cfsr & 0x0080)
    {
        printf(" MMFAR = %x\n", (unsigned int)memmanage_fault_address);
    }
    if (cfsr & 0x8000)
    {
        printf(" BFAR = %x\n", (unsigned int)bus_fault_address);
    }
    printf("- Misc\n");
    printf(" LR/EXC_RETURN= %x\n", lr_value);

    while (1);
}
