/*
 * hardfault_handler.c
 */
/**
 * Using Cortex-M3/M4/M7 Fault Exceptions
 * MDK Tutorial
 * AN209, Summer 2017, V 5.0
 */

#include <stdio.h>

#include "gd32.h"

void HardFault_Handler() {
	__asm volatile(
			"TST LR, #4\n"
			"ITE EQ\n"
			"MRSEQ R0, MSP\n"
			"MRSNE R0, PSP\n"
			"MOV R1, LR\n"
			"B hardfault_handler\n"
	);
}

void hardfault_handler(unsigned long *hardfault_args, unsigned int lr_value) {
	unsigned long stacked_r0;
	unsigned long stacked_r1;
	unsigned long stacked_r2;
	unsigned long stacked_r3;
	unsigned long stacked_r12;
	unsigned long stacked_lr;
	unsigned long stacked_pc;
	unsigned long stacked_psr;
	unsigned long cfsr;
	unsigned long bus_fault_address;
	unsigned long memmanage_fault_address;

	bus_fault_address = SCB->BFAR;
	memmanage_fault_address = SCB->MMFAR;
	cfsr = SCB->CFSR;

	stacked_r0 = ((unsigned long) hardfault_args[0]);
	stacked_r1 = ((unsigned long) hardfault_args[1]);
	stacked_r2 = ((unsigned long) hardfault_args[2]);
	stacked_r3 = ((unsigned long) hardfault_args[3]);
	stacked_r12 = ((unsigned long) hardfault_args[4]);
	stacked_lr = ((unsigned long) hardfault_args[5]);
	stacked_pc = ((unsigned long) hardfault_args[6]);
	stacked_psr = ((unsigned long) hardfault_args[7]);

	printf("[HardFault]\n");
	printf("- Stack frame:\n");
	printf(" R0  = %x\n", (unsigned int) stacked_r0);
	printf(" R1  = %x\n", (unsigned int) stacked_r1);
	printf(" R2  = %x\n", (unsigned int) stacked_r2);
	printf(" R3  = %x\n", (unsigned int) stacked_r3);
	printf(" R12 = %x\n", (unsigned int) stacked_r12);
	printf(" LR  = %x\n", (unsigned int) stacked_lr);
	printf(" PC  = %x\n", (unsigned int) stacked_pc);
	printf(" PSR = %x\n", (unsigned int) stacked_psr);
	printf("- FSR/FAR:\n");
	printf(" CFSR = %x\n", (unsigned int) cfsr);
	printf(" HFSR = %x\n", (unsigned int) SCB->HFSR);
	printf(" DFSR = %x\n", (unsigned int) SCB->DFSR);
	printf(" AFSR = %x\n", (unsigned int) SCB->AFSR);
	if (cfsr & 0x0080) {
		printf(" MMFAR = %x\n", (unsigned int) memmanage_fault_address);
	}
	if (cfsr & 0x8000) {
		printf(" BFAR = %x\n", (unsigned int) bus_fault_address);
	}
	printf("- Misc\n");
	printf(" LR/EXC_RETURN= %x\n", lr_value);

	while (1)
		;
}
