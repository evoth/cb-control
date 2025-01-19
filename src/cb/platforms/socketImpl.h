#if defined(_WIN32)
#include "windows/socketImpl.h"
#elif defined(ESP32)
#include "esp32/socketImpl.h"
#endif

// TODO: This should really be done with separate implementation files instead
// of doing it backwards like this