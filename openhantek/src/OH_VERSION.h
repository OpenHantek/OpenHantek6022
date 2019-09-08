// define the version that is shown on top of the program
// if undefined the build date will be used by OpenHantek

#define OH_VERSION "v2.14"

#ifdef OH_VERSION
#undef VERSION
#define VERSION OH_VERSION
#endif
