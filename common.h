#ifndef COMMON_H
#define COMMON_H

#include <QObject>
#include <QDateTime>
#include <QLoggingCategory>

#define TEXT_(x) #x
#define TEXT(x) TEXT_(x)

#define WATCH_OUT(debuger, name, time_ms)                                                          \
    do {                                                                                           \
        static clock_t start = clock();                                                            \
        double diff = (clock() - start);                                                           \
        if (diff > time_ms) {                                                                      \
            debuger << TEXT(name) << ": " << diff << " ms";                                        \
        }                                                                                          \
        start = clock();                                                                           \
    } while (false)

#endif // COMMON_H
