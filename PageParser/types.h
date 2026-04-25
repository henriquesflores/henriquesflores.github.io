#include <stdint.h> 

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef size_t usize;
typedef i32 b32;

#define CAST(type, object) ((type) object)
#define ARRAY_LEN(x) (sizeof(x) / sizeof(x[0]))

#define local_persist static;
#define global_variable static; 
#define internal static;

#define KILO(x) (1024L * x)
#define MEGA(x) (1024L * KILO(x))
#define GIGA(x) (1024L * MEGA(x))