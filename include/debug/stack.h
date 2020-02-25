/**
 * @file debug/stack.h
 * Stack usage analysis helpers
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEBUG_STACK_H_
#define ZEPHYR_INCLUDE_DEBUG_STACK_H_

#include <logging/log.h>
#include <stdbool.h>

extern K_THREAD_STACK_DEFINE(z_main_stack, CONFIG_MAIN_STACK_SIZE);
extern K_THREAD_STACK_DEFINE(z_idle_stack, CONFIG_IDLE_STACK_SIZE);
extern K_THREAD_STACK_DEFINE(_interrupt_stack, CONFIG_ISR_STACK_SIZE);
extern K_THREAD_STACK_DEFINE(sys_work_q_stack,
			     CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE);

static inline size_t stack_unused_space_get(const char *stack, size_t size)
{
	size_t unused = 0;

	if (IS_ENABLED(CONFIG_INIT_STACKS)) {
		const unsigned char *checked_stack =
			(const unsigned char *)stack;

		if (IS_ENABLED(CONFIG_STACK_SENTINEL)) {
			/* First 4 bytes of the stack buffer reserved for the
			 * sentinel value, it won't be 0xAAAAAAAA for thread
			 * stacks.
			 */
			checked_stack += 4;
		}

		/* TODO Currently all supported platforms have stack growth down
		 * and there is no Kconfig option to configure it so this always
		 * build "else" branch.
		 * When support for platform with stack direction up
		 * (or configurable direction) is added this check should be
		 * confirmed that correct Kconfig option is used.
		 */
		if (IS_ENABLED(CONFIG_STACK_GROWS_UP)) {
			for (size_t i = size; i > 0; i--) {
				if (checked_stack[i - 1] == 0xaaU) {
					unused++;
				} else {
					break;
				}
			}
		} else {
			for (size_t i = 0; i < size; i++) {
				if (checked_stack[i] == 0xaaU) {
					unused++;
				} else {
					break;
				}
			}
		}
	}

	return unused;
}

__deprecated
static inline void stack_analyze(const char *name, const char *stack,
				 unsigned int size)
{
	if (IS_ENABLED(CONFIG_INIT_STACKS)) {
		LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

		unsigned int unused = stack_unused_space_get(stack, size);
		unsigned int pcnt = ((size - unused) * 100U) / size;

		LOG_INF("%s :\tunused %u\tusage %u / %u (%u %%)",
			name, unused, size - unused, size, pcnt);
	}
}

/**
 * @brief Analyze stacks.
 *
 * Use this macro to get information about stack usage.
 *
 * @param name Name of the stack
 * @param sym The symbol of the stack
 */
#define STACK_ANALYZE(name, sym) DEPRECATED_MACRO		\
	do {							\
		stack_analyze(name,				\
			      Z_THREAD_STACK_BUFFER(sym),	\
			      K_THREAD_STACK_SIZEOF(sym));	\
	} while (false)

static inline void log_stack_usage(const struct k_thread *thread)
{
#if defined(CONFIG_INIT_STACKS) && defined(CONFIG_THREAD_STACK_INFO)
	size_t unused, size = thread->stack_info.size;

	LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

	if (k_thread_stack_space_get(thread, &unused) == 0) {
		unsigned int pcnt = ((size - unused) * 100U) / size;
		const char *tname;

		tname = k_thread_name_get((k_tid_t)thread);
		if (tname == NULL) {
			tname = "unknown";
		}

		LOG_INF("%p (%s):\tunused %zu\tusage %zu / %zu (%u %%)",
			thread, log_strdup(tname), unused, size - unused, size,
			pcnt);
	}
#endif
}
#endif /* ZEPHYR_INCLUDE_DEBUG_STACK_H_ */
