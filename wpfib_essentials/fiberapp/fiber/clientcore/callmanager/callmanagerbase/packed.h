#ifdef POSIX
#define PACKED __attribute__ ((__packed__))
#endif

#ifdef WIN32
#define PACKED
#pragma pack(push,1)
#endif
