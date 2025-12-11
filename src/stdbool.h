/**
 * @file    stdbool.h
 * @brief   Boolean type for STM32
 *          This is a minimal stub for IntelliSense
 */

#ifndef _STDBOOL_H
#define _STDBOOL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Boolean type */
#ifndef __cplusplus
#define bool    _Bool
#define true    1
#define false   0
#else
/* C++ has bool, true, false as keywords */
#endif

#define __bool_true_false_are_defined   1

#ifdef __cplusplus
}
#endif

#endif /* _STDBOOL_H */

