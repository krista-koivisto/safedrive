#ifndef ENCRYPT_H
#define ENCRYPT_H

#include <stdio.h>
#include <gpgme.h>
#include <string.h>
#include <openssl/sha.h>

#include "config.h"
#include "debug.h"

#define OUTPUT_BLOCK_SIZE 512

void sha1(unsigned char *data, size_t size, unsigned char *ret, int return_raw);
void encrypt_file(SConfig *conf, char *in_file, char *out_file);

#endif

