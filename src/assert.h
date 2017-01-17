#ifndef __drool_assert_h
#define __drool_assert_h

#if DROOL_ENABLE_ASSERT
#include <assert.h>
#define drool_assert(x) assert(x)
#else
#define drool_assert(x)
#endif

#endif /* __drool_assert_h */
