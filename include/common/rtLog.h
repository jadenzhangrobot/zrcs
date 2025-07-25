
#pragma once

#ifdef REALTIME
#include <rtdk.h>  // Xenomai real-time development kit
#define RT_PRINTF rt_printf
#else
#include <stdio.h>
#define RT_PRINTF printf
#endif

#define INFO_PRINT(fmt, ...) \
            do { RT_PRINTF(fmt, ##__VA_ARGS__); } while (0)

#define ERROR_PRINT(fmt, ...) \
            do { RT_PRINTF("[ERROR] %s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__); } while (0)

#define WARN_PRINT(fmt, ...) \
            do { RT_PRINTF("[WARN] %s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__); } while (0)
// clang-format on

