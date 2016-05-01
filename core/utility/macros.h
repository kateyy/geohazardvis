#pragma once


#if defined(NDEBUG)
#define DEBUG_ONLY(S)
#else
#define DEBUG_ONLY(S) S
#endif
