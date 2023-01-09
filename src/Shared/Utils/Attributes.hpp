#ifndef SRC_SHARED_UTILS_ATTRIBUTES_HPP_
#define SRC_SHARED_UTILS_ATTRIBUTES_HPP_

#ifdef __GNUC__
#	define WOS_ALWAYS_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#	define WOS_ALWAYS_INLINE inline __forceinline
#else
#	define WOS_ALWAYS_INLINE inline
#endif

#endif
