#ifndef BJN_FBR_UTILS_H
#define BJN_FBR_UTILS_H

#include <string.h>

#if _MSC_VER >= 1600 || __cplusplus >= 201103L
#include <type_traits>
#endif

namespace bjn_sky
{
    size_t decryptData(char *buffer, char *decrypted, size_t size);

    template <typename T>
    void zeroOut(T &val)
    {
#if _MSC_VER >= 1600 || __cplusplus >= 201103L
        // force a compilation error if we passed in a pointer...
        // Unfortunately, the only way to do this is with C++11, but it isn't ubiquitous yet.
        static_assert(!std::is_pointer<T>::value, "Zeroing out a pointer is probably not what you want...");
#endif
        memset(&val, 0, sizeof(T));
    }
}
#endif // BJN_FBR_UTILS_H
