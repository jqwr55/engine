#pragma once
#include <types.h>

template<typename T> T max(T a, T b) {
    return (a > b ? a : b);
}
template<typename T> T min(T a, T b) {
    return (a < b ? a : b);
}
template<typename T> T Abs(T a) {
    return ( a < 0 ? -a : a);
}

struct VectorBase {
    static void* (*AllocateAligned)(void* client,u32 alignment,u32 size);
    static void* (*Allocate)(void* client,u32 size);
    static void (*Free)(void* client,void* mem);
    static void* client;
};

decltype(VectorBase::Allocate) VectorBase::Allocate = nullptr;
decltype(VectorBase::AllocateAligned) VectorBase::AllocateAligned = nullptr;
decltype(VectorBase::Free) VectorBase::Free = nullptr;
decltype(VectorBase::client) VectorBase::client = nullptr;

template<typename T> struct Vector : public VectorBase {
    
    T* ptr = nullptr;
    u32 cap = 0;
    u32 count = 0;

    void PushFront(T t) {
        if( cap > count + 1) {
            for(u32 i = count ; i > 0 ; i--) {
                ptr[i] = ptr[i-1];
            }
        }
        else {
            T* tmp = (T*)Allocate(client , sizeof(T) * (count + 1) * 2 );
            MemCpy(ptr , tmp+1 , count * sizeof(T) );
            Free(client , ptr);
            ptr = tmp;
            cap = (count + 1) * 2;
        }

        count++;
        ptr[0] = t;
    }
    void PushBack(T t) {
        if( cap < count + 1 ) {
            T* tmp = (T*)Allocate(client , sizeof(T) * (count + 1) * 2 );
            MemCpy(ptr , tmp ,  sizeof(T) * count );
            Free(client , ptr);
            ptr = tmp;
            cap = (count + 1) * 2;
        }
        ptr[count++] = t;
    }
    T PopBack() {
        return ptr[--count];
    }
    void Shrink() {
        if( cap > count ) {
            T* tmp = (T*)Allocate(client , sizeof(T) * count );
            MemCpy(ptr , tmp ,  sizeof(T) * count );
            Free(client , ptr);
            ptr = tmp;
            cap = count;
        }
    }
    void ShrinkNoBranch() {
        T* tmp = (T*)Allocate(client , sizeof(T) * count );
        MemCpy(ptr , tmp ,  sizeof(T) * count );
        Free(client , ptr);
        ptr = tmp;
        cap = count;
    }
    void free() {
        Free(client , ptr);
        ptr = nullptr;
        cap = 0;
        count = 0;
    }
    void Clear() {
        count = 0;
    }

};


void GenerateMissingTextureData(void* mem) {
    ((u32*)mem)[0] = UINT32_MAX;
    ((u32*)mem)[1] = 0;
    ((u32*)mem)[2] = UINT32_MAX;
    ((u32*)mem)[3] = 0;
}