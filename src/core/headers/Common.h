#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <thread>
#include <unordered_map>

#include <dlfcn.h>
#include <immintrin.h>

#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/scalar_multiplication.hpp>

#ifdef __GNUC__
    #define _RESTRICT __restrict__
#else
    #ifdef _MSC_VER
        #define _RESTRICT __restrict
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

constexpr u64 KILO_BYTES = 1000;
constexpr u64 MEGA_BYTES = 1000 * KILO_BYTES;
constexpr u64 GIGA_BYTES = 1000 * MEGA_BYTES;
constexpr u64 TERA_BYTES = 1000 * GIGA_BYTES;

constexpr u32 MILI_SEC = 1000;
constexpr u32 MICRO_SEC = 1000 * MILI_SEC; 
constexpr u32 NANO_SEC = 1000 * MICRO_SEC;

void AlsaLogCall(const char* function, const char* file, i32 line, i32 err);

#if 1
    #define LOGASSERT(x , y ) if( !(x) ) {std::cout << #x << " " << y << " " << __FILE__ << " " << __LINE__ << std::endl; __builtin_trap(); }
    #define ASSERT(x) if( !(x) ) __builtin_trap();
    #define ALSA_CALL(x) if(i32 err = x) AlsaLogCall(#x , __FILE__ , __LINE__,err)
#else
    #define ASSERT(x) x
    #define LOGASSERT(x , y)
    #define ALSA_CALL(x)
#endif

#define introspect(params)
#define LDCall(x) LDClearErrors(); x; ASSERT( LDLogCall(#x , __FILE__ , __LINE__) )

void LDClearErrors();
bool LDLogCall(const char* function, const char* file , int line);

void LDClearErrors() {
    while( const char* msg = dlerror() );
}
bool LDLogCall(const char* function, const char* file , int line) {
    bool r = true;
    while( const char* msg = dlerror() ) {
        std::cout << "[Error]["<< file<<"] at: " << line << " " << msg << " " << function << std::endl;
        r = false;
    }
    return r;
}


void MemSet(void* dst , byte val , u32 size) {
    for(u32 i = 0; i < size ; i++) {
        ((byte*)dst)[i] = val;
    }
}
void MemCpy(void* _RESTRICT src , void* _RESTRICT dst , u32 size) {
    for(u32 i = 0; i < size ; i++) {
        ((byte*)dst)[i] = ((byte*)src)[i];
    }
}
template<typename T> T max(T a, T b) {
    return (a > b ? a : b);
}
template<typename T> T min(T a, T b) {
    return (a < b ? a : b);
}
template<typename T> T Abs(T a) {
    return ( a < 0 ? -a : a);
}