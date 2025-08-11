#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#ifdef WINDOWS
#include "Windows.h"
#include "Process.h"
// #include "synchapi.h"
// #include "processthreadsapi.h"
#endif

#ifdef UNIX
#include<unistd.h>
#endif

typedef uint32_t u32;
typedef int64_t i64;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint8_t u8;
typedef uint16_t u16;

typedef float f32;
typedef double f64;

void SleepMilliseconds(i32 milliseconds)
{
#ifdef WINDOWS
    Sleep(milliseconds);
#endif

#ifdef UNIX
    usleep(milliseconds);
#endif
}


#define lengthof(x) (sizeof(x) / sizeof(x[0]))
