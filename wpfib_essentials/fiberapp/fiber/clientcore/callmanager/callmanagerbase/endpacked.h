#ifdef POSIX
#undef PACKED
#endif

#ifdef WIN32
#pragma pack(pop)
#undef PACKED
#endif
