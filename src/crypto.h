#ifndef _CRYPTO_H_
#define _CRYPTO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

uint32_t hash(const char* str);
char* base64_encode(const char* str);
char* base64_decode(const char* str);

#ifdef __cplusplus
}
#endif

#endif
