/*
 * stack_debug.h
 */

#ifndef STACK_DEBUG_H_
#define STACK_DEBUG_H_


#include <stdint.h>

extern "C" {
void stack_debug_print(uint32_t);
}

#define STACK_DEBUG_PRINT 											\
		do {														\
			stack_debug_print(__LINE__);							\
		} while (0)

#endif /* STACK_DEBUG_H_ */
