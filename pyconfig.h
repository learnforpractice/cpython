#if defined(__APPLE__) && defined(__MACH__)
    #include "pyconfig_darwin.h"
#elif defined(__linux__)
    #include "pyconfig_linux.h"
#else
   #error Not Supported Platform
#endif
