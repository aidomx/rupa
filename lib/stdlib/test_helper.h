#pragma once

#if defined(RUPA_PACKAGE_H)

/**
 * @brief Register test helper functions (assert, assertEq) in the environment.
 *
 * These functions are only available in test mode and provide runtime
 * assertion capabilities for execution-based tests.
 *
 * @param env Environment to register functions in.
 */
void testHelperInit(RuntimeEnv *env);

/**
 * @brief Reset assertion counters for a new test run.
 */
void testHelperReset(void);

/**
 * @brief Get the number of failed assertions.
 * @return Number of assertion failures since last reset.
 */
int testHelperFailures(void);

#endif
