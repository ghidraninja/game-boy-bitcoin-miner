#include <string.h>

void memzero(void *const pnt, const size_t len) {
    memset(pnt, 0x0, len);
}