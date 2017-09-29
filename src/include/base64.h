#ifndef BASE64_H
#define BASE64_H

#include <stddef.h>
#include <stdint.h>

int b64_decode(char const *src, uint8_t *target, size_t targsize);
int b64_encode(uint8_t const *src, size_t srclength, char *target, size_t targsize);

#endif
