// Definitions and maths utilities
// Maybe move maths stuff somewhere else in future

#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <stdint.h>
#include <cmath>

// types
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef s32 b32;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef intptr_t intptr;
typedef uintptr_t uintptr;

typedef float r32;
typedef float f32;
typedef double r64;
typedef double f64;

#if DEBUG
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif

#define pi32 3.14159265359f

#define TRUE 1
#define FALSE 0

r32 absolute(r32 val)
{
    return val < 0.0f ? -val : val;
}

void clamp(r32 low, r32* val, r32 high) {
    if (*val < low) *val = low;
    if (*val > high) *val = high;
}

struct Real3 {
    r32 x;
    r32 y;
    r32 z;
    
    r32 length_sq()
    {
        return x*x + y*y + z*z;
    }
    
    r32 length()
    {
        return sqrtf(x*x + y*y + z*z);
    }
    
    r32 distance(Real3 other)
    {
        return sqrtf((x - other.x)*(x - other.x) + (y - other.y)*(y - other.y) + (z - other.z)*(z - other.z));
    }
    
    void normalize()
    {
        r32 l = length();
        Assert(l > 0);
        if (l > 0)
        {
            x /= l;
            y /= l;
            z /= l;
        }
    }
    
    Real3 operator*(r32 mult)
    {
        return { x*mult, y*mult, z*mult };
    }
    
    Real3 operator-(Real3 other)
    {
        return { x - other.x, y - other.y, z - other.z };
    }
    
    r32 dot(Real3 other)
    {
        return (x*other.x + y*other.y + z*other.z);
    }
    
    Real3 cross(Real3 other)
    {
        Real3 product;
        product.x = y*other.z - z*other.y;
        product.y = z*other.x - x*other.z;
        product.z = x*other.y - y*other.x;
        return product;
    }
};

struct Real4
{
    r32 x;
    r32 y;
    r32 z;
    r32 w;
    
    r32 dot(Real4 other)
    {
        return x*other.x + y*other.y + z*other.z + w*other.w;
    }
    
    r32 norm()
    {
        return sqrt(x*x + y*y + z*z + w*w);
    }
    
    void normalize()
    {
        r32 n = norm();
        Assert(n > 0);
        if (n > 0)
        {
            x /= n;
            y /= n;
            z /= n;
            w /= n;
        }
    }
    
    bool isNormalized()
    {
        r32 n = norm();
        return absolute(n - 1) < 1e-4;
    }
    
    Real3 xyz()
    {
        return { x, y, z };
    }
};

Real4 quaternion(Real3 dir, r32 angle)
{
    Real4 q;
    
    dir.normalize();
    r32 s = sinf(angle*0.5);
    q.x = dir.x*s;
    q.y = dir.y*s;
    q.z = dir.z*s;
    
    r32 c = cosf(angle*0.5);
    q.w = c;
    
    return q;
}

struct Mat3x3
{
    r32 elems[9];
    
    r32& elem(u32 row, u32 col)
    {
        Assert(row < 3 && col < 3 && row >= 0 && col >= 0);
        return elems[3*row + col];
    }
    
    static Mat3x3 identity()
    {
        Mat3x3 mat;
        memset(mat.elems, 0, sizeof(elems[0])*9);
        mat.elem(0, 0) = 1.0f;
        mat.elem(1, 1) = 1.0f;
        mat.elem(2, 2) = 1.0f;
        
        return mat;
    }
    
    static Mat3x3 rotation(Real3 axis, r32 angle_rad)
    {
        return rotation(quaternion(axis, angle_rad));
    }
    
    static Mat3x3 rotation(Real4 quaternion)
    {
        Assert(quaternion.isNormalized());
        r32 x = quaternion.x;
        r32 y = quaternion.y;
        r32 z = quaternion.z;
        r32 w = quaternion.w;
        Mat3x3 mat = {
            w*w + x*x - y*y - z*z, 2*x*y - 2*w*z,         2*x*z + 2*w*y,
            2*x*y + 2*w*z,         w*w - x*x + y*y - z*z, 2*y*z - 2*w*x,
            2*x*z - 2*w*y,         2*y*z + 2*w*x,         w*w - x*x - y*y + z*z
        };
        return mat;
    }
    
    Real3 row(u32 n)
    {
        Assert(n < 3 && n >= 0);
        Real3 ret = { elems[3*n], elems[3*n + 1], elems[3*n + 2] };
        return ret;
    }
    
    Real3 col(u32 n)
    {
        Assert(n < 3 && n >= 0);
        Real3 ret = { elems[n], elems[n + 3], elems[n + 6] };
        return ret;
    }
    
    void mult(Mat3x3& right)
    {
        Mat3x3 ret;
        for (u32 x = 0; x < 3; ++x) {
            for (u32 y = 0; y < 3; ++y) {
                ret.elems[x + 3*y] = row(y).dot(right.col(x));
            }
        }
        *this = ret;
    }
    
    Real3 mult(Real3 v)
    {
        Real3 ret;
        ret.x = row(0).dot(v);
        ret.y = row(1).dot(v);
        ret.z = row(2).dot(v);
        return ret;
    }
    
    Mat3x3 transposition()
    {
        Mat3x3 transpose = *this;
        
        r32 e01 = transpose.elem(0, 1);
        r32 e02 = transpose.elem(0, 2);
        r32 e12 = transpose.elem(1, 2);
        transpose.elem(0, 1) = transpose.elem(1, 0);
        transpose.elem(0, 2) = transpose.elem(2, 0);
        transpose.elem(1, 2) = transpose.elem(2, 1);
        transpose.elem(1, 0) = e01;
        transpose.elem(2, 0) = e02;
        transpose.elem(2, 1) = e12;
        
        return transpose;
    }
};

// we use row  major representation, column vectors
struct Mat4x4
{
    r32 elems[16];
    
    r32& elem(u32 row, u32 col)
    {
        Assert(row < 4 && col < 4 && row >= 0 && col >= 0);
        return elems[4*row + col];
    }
    
    void copy_to(r32* ptr)
    {
        memcpy(ptr, elems, sizeof(elems[0])*16);
    }
    
    static Mat4x4 identity()
    {
        Mat4x4 mat;
        memset(mat.elems, 0, sizeof(elems[0])*16);
        mat.elem(0, 0) = 1.0f;
        mat.elem(1, 1) = 1.0f;
        mat.elem(2, 2) = 1.0f;
        mat.elem(3, 3) = 1.0f;
        
        return mat;
    }
    
    static Mat4x4 from(Mat3x3 rotation, Real3 translation)
    {
        Mat4x4 mat;
        mat.set_to(rotation, translation);
        return mat;
    }
    void set_to(Mat3x3 rotation, Real3 translation)
    {
        r32 elems_assign[16] = {
            rotation.elem(0,0), rotation.elem(0, 1), rotation.elem(0, 2), translation.x,
            rotation.elem(1,0), rotation.elem(1, 1), rotation.elem(1, 2), translation.y,
            rotation.elem(2,0), rotation.elem(2, 1), rotation.elem(2, 2), translation.z,
            0.0f, 0.0f, 0.0f, 1.0f
        };
        memcpy(elems, elems_assign, sizeof(r32)*16);
    }
    
    static Mat4x4 rotation(Mat3x3 rotation)
    {
        Mat4x4 mat;
        mat.set_to(rotation);
        return mat;
    }
    void set_to(Mat3x3 rotation)
    {
        r32 elems_assign[16] = {
            rotation.elem(0,0), rotation.elem(0, 1), rotation.elem(0, 2), 0.0f,
            rotation.elem(1,0), rotation.elem(1, 1), rotation.elem(1, 2), 0.0f,
            rotation.elem(2,0), rotation.elem(2, 1), rotation.elem(2, 2), 0.0f,0.0f, 0.0f, 0.0f, 1.0f
        };
        
        memcpy(elems, elems_assign, sizeof(r32)*16);
    }
    
    static Mat4x4 translation(Real3 translation)
    {
        Mat4x4 mat;
        mat.set_to(translation);
        return mat;
    }
    void set_to(Real3 translation)
    {
        r32 elems_assign[16] = {
            1.0f, 0.0f, 0.0f, translation.x,
            0.0f, 1.0f, 0.0f, translation.y,
            0.0f, 0.0f, 1.0f, translation.z,
            0.0f, 0.0f, 0.0f, 1.0f
        };
        memcpy(elems, elems_assign, sizeof(r32)*16);
    }
    
    Real4 row(u32 n)
    {
        Assert(n < 4 && n >= 0);
        Real4 ret = { elems[4*n], elems[4*n + 1], elems[4*n + 2], elems[4*n + 3] };
        return ret;
    }
    
    Real4 col(u32 n)
    {
        Assert(n < 4 && n >= 0);
        Real4 ret = { elems[n], elems[n + 4], elems[n + 8], elems[n + 12] };
        return ret;
    }
    
    void mult(Mat4x4& right)
    {
        Mat4x4 ret;
        for (u32 x = 0; x < 4; ++x) {
            for (u32 y = 0; y < 4; ++y) {
                ret.elems[x + 4*y] = row(y).dot(right.col(x));
            }
        }
        *this = ret;
    }
    
    Real4 mult(Real4 proj_point)
    {
        Real4 ret;
        ret.x = row(0).dot(proj_point);
        ret.y = row(1).dot(proj_point);
        ret.z = row(2).dot(proj_point);
        ret.w = row(3).dot(proj_point);
        return ret;
    }
    
    Real3 mult_point(Real3 point)
    {
        return mult({ point.x, point.y, point.z, 1 }).xyz();
    }
    
    Real3 mult_vector(Real3 vector)
    {
        return mult({ vector.x, vector.y, vector.z, 0 }).xyz();
    }
    
    Mat4x4 transposition()
    {
        Mat4x4 transpose = *this;
        
        r32 e01 = transpose.elem(0, 1);
        r32 e02 = transpose.elem(0, 2);
        r32 e03 = transpose.elem(0, 3);
        r32 e12 = transpose.elem(1, 2);
        r32 e13 = transpose.elem(1, 3);
        r32 e23 = transpose.elem(2, 3);
        transpose.elem(0, 1) = transpose.elem(1, 0);
        transpose.elem(0, 2) = transpose.elem(2, 0);
        transpose.elem(0, 3) = transpose.elem(3, 0);
        transpose.elem(1, 2) = transpose.elem(2, 1);
        transpose.elem(1, 3) = transpose.elem(3, 1);
        transpose.elem(2, 3) = transpose.elem(3, 2);
        transpose.elem(1, 0) = e01;
        transpose.elem(2, 0) = e02;
        transpose.elem(3, 0) = e03;
        transpose.elem(2, 1) = e12;
        transpose.elem(3, 1) = e13;
        transpose.elem(3, 2) = e23;
        return transpose;
    }
};

// a 2d vector of real32s
struct Real2
{
    r32 x;
    r32 y;
    
    r32 length_sq()
    {
        return x*x + y*y;
    }
    
    r32 length()
    {
        return sqrtf(x*x + y*y);
    }
    
    r32 distance(Real2 other)
    {
        return sqrtf((x - other.x)*(x - other.x) + (y - other.y)*(y - other.y));
    }
    
    void normalize()
    {
        r32 l = length();
        if (l > 0)
        {
            x /= l;
            y /= l;
        }
    }
    
    Real2 operator*(r32 mult)
    {
        return { x*mult, y*mult };
    }
    
    Real2 operator-(Real2 other)
    {
        return { x - other.x, y - other.y };
    }
    
    r32 dot(Real2 other)
    {
        return (x*other.x + y*other.y);
    }
};

inline Real2 operator+(Real2 a, Real2 b) {
    Real2 added;
    added.x = a.x + b.x;
    added.y = a.y + b.y;
    
    return added;
}

inline Real2 operator*(r32 factor, Real2 vect) {
    Real2 mult;
    mult.x = factor*vect.x;
    mult.y = factor*vect.y;
    
    return mult;
}

inline Real3 operator+(Real3 a, Real3 b) {
    Real3 added;
    added.x = a.x + b.x;
    added.y = a.y + b.y;
    added.z = a.z + b.z;
    
    return added;
}

inline Real3 operator*(r32 factor, Real3 vect) {
    Real3 mult;
    mult.x = factor*vect.x;
    mult.y = factor*vect.y;
    mult.z = factor*vect.z;
    
    return mult;
}

// a 2d vector of int32s
struct Integer2
{
    s32 x;
    s32 y;
    
    s32 length_sq()
    {
        return x*x + y*y;
    }
    
    r32 length()
    {
        return sqrtf((r32)(x*x + y*y));
    }
    
    r32 distance(Integer2 other)
    {
        return sqrtf((float)((x - other.x)*(x - other.x) + (y - other.y)*(y - other.y)));
    }
};

inline Integer2 operator+(Integer2 a, Integer2 b) {
    Integer2 added;
    added.x = a.x + b.x;
    added.y = a.y + b.y;
    
    return added;
}

// arr<T> some dumb dynamic array implementation
//
// simple usage:
// arr<T> some_array(100);
// some_array.resize(200);
// some_array.memory[150] = some_t;
//
// constructors: optionally pass in starting size
// resize(int32) to change size, copying data over up to new size
//
// memory that hasn't been assigned should be zero
//
// .memory to access the actual c array ptr
// [] for assignment and to get elements
// .size for the size
// copy_from(arr<T>*) to become a copy of the other array with the memory actually copied over
// set(T) to set everything to the argument
// zero() to zero everything
// will deallocate when it goes out of scope so malloc it yourself if you want control
// look at source for other stuff
//
template <typename T>
struct arr {
    
    arr()
    {
        size = 0;
        memory = 0;
    }
    
    arr(s32 size_in)
    {
        Assert(size_in > 0);
        
        size = size_in;
        memory = (T*)malloc(sizeof(T)*size_in);
        zero();
    }
    
    // turn into a copy of other (with its memory contents copied to our memory)
    void copy_from(arr<T>* other)
    {
        resize(other->size);
        memcpy(memory, other->memory, sizeof(T)*size);
    }
    
    void resize(s32 size_new)
    {
        Assert(size_new >= 0);
        
        if (size_new == 0) {
            free(memory);
            memory = 0;
            return;
        }
        
        void* more_memory = malloc(sizeof(T)*size_new);
        
        if (memory) {
            if (size_new >= size) {
                // copy over
                memcpy(more_memory, memory, sizeof(T)*size);
            }
            else {
                // truncate
                memcpy(more_memory, memory, sizeof(T)*size_new);
            }
            
            free(memory);
            
            memory = (T*)more_memory;
        }
        else
        {
            memory = (T*)more_memory;
        }
        
        s32 size_old = size;
        size = size_new;
        zero_from(size_old);
    }
    
    void zero()
    {
        for (int x = 0; x < size*sizeof(T); ++x) {
            ((char*)memory)[x] = 0;
        }
    }
    
    void zero_from(s32 start_index)
    {
        for (int x = start_index*((s32)sizeof(T)); x < size*((s32)sizeof(T)); ++x) {
            ((char*)memory)[x] = 0;
        }
    }
    
    void set(T value) {
        for (int x = 0; x < size; ++x) {
            memory[x] = value;
        }
    }
    
    T& operator[](int n) {
        return memory[n];
    }
    
    u32 size_bytes() {
        return size*sizeof(T);
    }
    
    ~arr()
    {
        if (memory) {
            free(memory);
        }
    }
    
    s32 size;
    T* memory;
};


// list<T>, some dumb array-based list implementation
//
// simple usage:
// list<T> some_list;
// some_list.append(some_t);
// ...
//
// iterating:
// for (int x = 0; x < some_list.size; ++x) {
//        T element = some_list.get(x);
//
//        if (element == compare) {
//            some_list.remove(x);
//            --x;
//    }
// }
//
// constructors: optionally pass up to 2 s32s for starting capacity and reallocation increment
//
// append(T) to add to the end of the list
// remove(int32) to remove at an index
// T get(int32) to get the element at an index
// set(int32, T t) to set an index to t
// clear() to remove all elements
// compress(optional int32) to resize buffer to the number of elements actually used + optional int32 extra space
//
// the list will be compressed automatically when you remove a number of elements equal to the reallocation increment
//
// .size for number of elements
// .capacity for size of background buffer
//
// will deallocate when it goes out of scope so malloc it yourself if you want control
// look at source for other stuff
//
template<typename T>
struct list {
    
    list()
    {
        capacity = 0;
        capacity_increment = 100;
        size = 0;
        assign = 0;
        unused = 0;
        
        last_index_got = -1;
        last_index_pos = -1;
    }
    
    list(s32 capacity_in)
    {
        data.resize(capacity_in);
        used.resize(capacity_in);
        used.set(FALSE);
        
        capacity = capacity_in;
        capacity_increment = 100;
        size = 0;
        assign = 0;
        unused = 0;
        
        last_index_got = -1;
        last_index_pos = -1;
    }
    
    list(s32 capacity_in, s32 capacity_increment_in)
    {
        data.resize(capacity_in);
        used.resize(capacity_in);
        used.set(FALSE);
        
        capacity = capacity_in;
        capacity_increment = capacity_increment_in;
        size = 0;
        assign = 0;
        unused = 0;
        
        last_index_got = -1;
        last_index_pos = -1;
    }
    
    void append(T item)
    {
        if (assign + 1 > capacity) {
            data.resize(capacity+capacity_increment);
            used.resize(capacity+capacity_increment);
            used.zero_from(capacity);
            
            capacity += capacity_increment;
        }
        
        size += 1;
        
        data.memory[assign] = item;
        used.memory[assign] = TRUE;
        
        assign += 1;
    }
    
    T remove(s32 index)
    {
        Assert(index < size);
        
        T removed = get(index);
        
        size -= 1;
        used.memory[last_index_pos] = FALSE;
        
        last_index_got = -1;
        last_index_pos = -1;
        
        unused += 1;
        if (unused > capacity_increment) {
            compress(capacity_increment);
        }
        
        return removed;
    }
    
    T& get(s32 index)
    {
        Assert(index < size);
        
        s32 current_index = -1;
        s32 passed = -1;
        
        if (last_index_got <= index) {
            current_index = last_index_pos;
            passed = last_index_got;
        }
        
        while (passed < index) {
            ++current_index;
            if (used.memory[current_index]) {
                ++passed;
            }
        }
        
        last_index_got = index;
        last_index_pos = current_index;
        return data.memory[current_index];
    }
    
    void set(s32 index, T value)
    {
        get(index);
        
        assert(last_index_got == index);
        data.memory[last_index_pos] = value;
    }
    
    void clear() {
        data = arr<T>();
        used = arr<b32>();
        
        capacity = 0;
        size = 0;
        assign = 0;
        unused = 0;
        
        last_index_got = -1;
        last_index_pos = -1;
    }
    
    void compress(s32 space)
    {
        if (size < capacity) {
            last_index_got = -1;
            last_index_pos = -1;
            
            arr<T> old_data;
            old_data.copy_from(&data);
            arr<b32> old_used;
            old_used.copy_from(&used);
            
            data.resize(size + space);
            used.resize(size + space);
            capacity = size + space;
            assign = size;
            
            used.set(FALSE);
            
            s32 current_index_get = 0;
            s32 next_index_set = 0;
            
            while (next_index_set < size) {
                if (old_used.memory[current_index_get]) {
                    used.memory[next_index_set] = TRUE;
                    data.memory[next_index_set] = old_data.memory[current_index_get];
                    ++next_index_set;
                }
                ++current_index_get;
            }
        }
    }
    
    void compress()
    {
        compress(0);
    }
    
    s32 last_index_got;        // the last index we searched for
    s32 last_index_pos;        // the position the last index we searched for was at
    s32 capacity;              // capacity of list
    s32 capacity_increment;    // increment when capacity is reached
    s32 size;                  // current number of elements in list
    s32 assign;                // next index to be assigned to
    s32 unused;
    arr<T> data;               // list data
    arr<b32> used;             // used indices in data
};
