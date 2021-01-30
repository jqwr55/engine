#pragma once
#include <cstdint>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/scalar_multiplication.hpp>
#include <immintrin.h>

#ifdef __GNUC__
    #define _RESTRICT __restrict__
#else
    #ifdef _MSC_VER
        #define _RESTRICT __restrict
    #endif
#endif

#ifdef __GNUC__
    #define _FORCE_INLINE __attribute__((always_inline))
#else
    #ifdef _MSC_VER
        #define _FORCE_INLINE __builtin_trap();
    #endif
#endif



typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

static_assert(sizeof(f32) == 4);
static_assert(sizeof(f64) == 8);

typedef i8 byte;

constexpr u64 KILO_BYTES = 1024;
constexpr u64 MEGA_BYTES = 1024 * KILO_BYTES;
constexpr u64 GIGA_BYTES = 1024 * MEGA_BYTES;
constexpr u64 TERA_BYTES = 1024 * GIGA_BYTES;

constexpr u32 MILI_SEC = 1000;
constexpr u32 MICRO_SEC = 1000 * MILI_SEC; 
constexpr u32 NANO_SEC = 1000 * MICRO_SEC;