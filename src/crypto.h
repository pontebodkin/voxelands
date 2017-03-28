#ifndef _CRYPTO_H_
#define _CRYPTO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

uint32_t hash(char* str);
char* base64_encode(char* str);
char* base64_decode(char* str);

#ifdef __cplusplus
}
#endif

#endif
