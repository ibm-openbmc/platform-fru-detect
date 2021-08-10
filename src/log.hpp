/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

#include <cstdio>

#define LOG_DEBUG 0

#define LOG_PREFIX_FMT "%-60s\t: "

#define log_error(fmt, ...) fprintf(stderr, LOG_PREFIX_FMT fmt, __PRETTY_FUNCTION__, ##__VA_ARGS__)
#define log_warn(fmt, ...) fprintf(stderr, LOG_PREFIX_FMT fmt, __PRETTY_FUNCTION__, ##__VA_ARGS__)
#define log_info(fmt, ...) fprintf(stderr, LOG_PREFIX_FMT fmt, __PRETTY_FUNCTION__, ##__VA_ARGS__)

#if LOG_DEBUG
#define log_debug(fmt, ...) fprintf(stderr, LOG_PREFIX_FMT fmt, __PRETTY_FUNCTION__, ##__VA_ARGS__)
#else
#define log_debug(...)
#endif
