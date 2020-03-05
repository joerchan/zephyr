/*
 * Copyright (c) 2019 - 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __STACK_SIZE_ANALYZER_H
#define __STACK_SIZE_ANALYZER_H
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup stack_analyzer Stack analyzer
 * @brief Module for analyzing stack usage in samples
 *
 * This module implements functions and the configuration that simplifies
 * stack analysis.
 * @{
 */

/**
 * @brief Stack analyzer callback function
 *
 * Callback function with stack statistics.
 *
 * @param name  The name of the thread or stringified address
 *              of the thread handle if name is not set.
 * @param size  The total size of the stack
 * @param used  Stack size in use
 */
typedef void (*thread_analyzer_cb)(const char *name, size_t size, size_t used);

/**
 * @brief Run the stack analyzer and return the results to the callback
 *
 * This function analyzes stacks for all threads and calls
 * a given callback on every thread found.
 *
 * @param cb The callback function handler
 */
void thread_analyzer_run(thread_analyzer_cb cb);

/**
 * @brief Run the stack analyzer and print the result
 *
 * This function runs the stack analyzer and prints the output in standard
 * form.
 */
void thread_analyzer_print(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __STACK_SIZE_ANALYZER_H */
