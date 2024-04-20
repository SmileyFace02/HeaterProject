#ifndef MACROS_H
#define MACROS_H

#define issigned(t) (((t)(-1)) < ((t) 0))

#define umaxof(t) (((0x1ULL << ((sizeof(t) * 8ULL) - 1ULL)) - 1ULL) | \
                    (0xFULL << ((sizeof(t) * 8ULL) - 4ULL)))

#define smaxof(t) (((0x1ULL << ((sizeof(t) * 8ULL) - 1ULL)) - 1ULL) | \
                    (0x7ULL << ((sizeof(t) * 8ULL) - 4ULL)))

#define maxof(t) ((unsigned long long) (issigned(t) ? smaxof(t) : umaxof(t)))

#endif