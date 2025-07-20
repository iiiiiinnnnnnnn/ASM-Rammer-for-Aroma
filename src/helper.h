#pragma once

#include <notifications/notifications.h>

#ifdef _MSC_VER

namespace std {
    template<typename T>
    struct span {
        span(...) {}
    };
}

#define socketclose(x)

#endif

// exnotifications.h - Header for extended notifications macros

#define OSFatalf(fmt, ...) do { \
    char __exnoti_buf[256]; \
    snprintf(__exnoti_buf, sizeof(__exnoti_buf), fmt, ##__VA_ARGS__); \
    OSFatal(__exnoti_buf); \
} while (0)

#define NotificationInfof(fmt, ...) do { \
    char __exnoti_buf[256]; \
    snprintf(__exnoti_buf, sizeof(__exnoti_buf), fmt, ##__VA_ARGS__); \
    NotificationModule_AddInfoNotification(__exnoti_buf); \
} while (0)

#define NotificationErrorf(fmt, ...) do { \
    char __exnoti_buf[256]; \
    snprintf(__exnoti_buf, sizeof(__exnoti_buf), fmt, ##__VA_ARGS__); \
    NotificationModule_AddErrorNotification(__exnoti_buf); \
} while (0)