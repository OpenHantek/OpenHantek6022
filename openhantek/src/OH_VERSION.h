// define the version that is shown on top of the program
// if undefined (for development commits) the build will be shown by OpenHantek

//#define OH_VERSION "v2.17-rc1"


# ifdef OH_VERSION
#   undef VERSION
#   define VERSION OH_VERSION
# else
#   include "OH_BUILD.h"
#   ifdef OH_BUILD
#     undef VERSION
#     define VERSION OH_BUILD
#   endif
# endif
