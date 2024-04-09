#pragma once

#include <stddef.h> /* for ptrdiff_t below */

typedef struct { const char *p; ptrdiff_t n; } _GoString_;
typedef _GoString_ GoString;
typedef unsigned char GoUint8;
typedef int GoInt32;

/*
  static assertion to make sure the file is being used on architecture
  at least with matching size of GoInt.
*/
typedef char _check_for_64_bit_pointer_matching_GoInt[sizeof(void*)==64/8 ? 1:-1];


#ifdef __cplusplus
extern "C" {
#endif

extern char* getAdGroupsForCSasJSON(char* p0, char* p1, char* p2, GoUint8 p3, GoUint8 p4, char* p5, char* p6, GoInt32 p7);

extern char* getAdGroupsAllJSONCSharp(char* p0);

extern char* getAdGroupsCachedCSharp(char* p0, char* p1, char* p2, GoUint8 p3, GoUint8 p4, char* p5, char* p6);

extern char* getAdGroups_forCSharp(char* p0, char* p1, char* p2, GoUint8 p3, GoUint8 p4, char* p5);

extern char* getAdGroups_extern(GoString p0, GoString p1, GoString p2, GoUint8 p3, GoUint8 p4, GoString p5);

extern char* getAdGroupsWithCaching(GoString p0, GoString p1, GoString p2, GoUint8 p3, GoUint8 p4, GoString p5, GoString p6);

extern char* getAdGroupsAsJSON(GoString p0, GoString p1, GoString p2, GoUint8 p3, GoUint8 p4, GoString p5, GoString p6, GoInt32 p7);

extern char* getAdGroupsAllJSON(GoString p0);

#ifdef __cplusplus
}
#endif
