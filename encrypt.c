#include "encrypt.h"

static gpgme_error_t _passphrase_cb(void *pass, const char *uid_hint, const char *passphrase_info, int prev_was_bad, int fd)
{
	ssize_t pass_len = strlen((char*)pass);
    unsigned long long written = 0;
	
	char *passphrase = malloc(sizeof(char) * pass_len + 1);
	strncpy(passphrase, pass, pass_len + 1);

    while (written < pass_len)
    {
        ssize_t bytes_written = gpgme_io_write(fd, &passphrase[0] + written, pass_len - written);
		
		if (bytes_written == 0)
            break;
			
        written += bytes_written;
    }
	
    gpgme_io_write(fd, "\n", 1);
	
    return GPG_ERR_NO_ERROR;
}

void _init_gpgme(gpgme_ctx_t *ctx, SConfig *conf)
{
	gpgme_check_version(NULL);
	gpgme_new(ctx);
	gpgme_set_protocol(*ctx, GPGME_PROTOCOL_OpenPGP);
	gpgme_set_armor(*ctx, 0);
	
	gpgme_set_passphrase_cb(*ctx, NULL, NULL);
	gpgme_set_pinentry_mode(*ctx, GPGME_PINENTRY_MODE_LOOPBACK);
	gpgme_set_passphrase_cb(*ctx, _passphrase_cb, conf->passphrase);
}

void _clean_gpgme(gpgme_ctx_t *ctx)
{
	gpgme_release (*ctx);
}

void _encrypt_file(gpgme_ctx_t *ctx, char *in_file, char *out_file)
{
	gpgme_encrypt_flags_t flags = GPGME_ENCRYPT_ALWAYS_TRUST | GPGME_ENCRYPT_SYMMETRIC;
	gpgme_data_t in, out;
	
	// Read data from the input file
	char *data = 0;
	unsigned long long size = read_file(in_file, &data);
	
	if (size == 0)
		return;
	
	// Encrypt
	gpgme_data_new (&out);
	gpgme_data_new_from_mem (&in, data, size, 0);
	gpgme_op_encrypt(*ctx, NULL, flags, in, out);
	gpgme_op_encrypt_result (*ctx);
	
	// Write to file
	int ret = gpgme_data_seek(out, 0, SEEK_SET);
	char buf[OUTPUT_BLOCK_SIZE + 1];
	
	ret = gpgme_data_read(out, buf, OUTPUT_BLOCK_SIZE);
	
	if (write_binary_file(out_file, buf, ret, 0) != 0)
		while((ret = gpgme_data_read(out, buf, OUTPUT_BLOCK_SIZE)) > 0)
			write_binary_file(out_file, buf, ret, 1);
	
	gpgme_data_release (in);
	gpgme_data_release (out);
}

void sha1(unsigned char *data, size_t size, unsigned char *ret, int return_raw)
{
	ret[0] = 0;
	unsigned char hash[SHA_DIGEST_LENGTH + 1];
	SHA1(data, size, hash);
	
	if (return_raw)
	{
		memcpy(ret, hash, SHA_DIGEST_LENGTH);
	}
	else
	{
		for(int i = 0; i < SHA_DIGEST_LENGTH; i++)
			sprintf(&ret[strlen(ret)], "%x", hash[i]);
	}
}

void encrypt_file(SConfig *conf, char *in_file, char *out_file)
{
	gpgme_ctx_t ctx;
	
	debug_print("Encrypting '%s' => '%s'.", DEBUG_INFO, 1, in_file, out_file);
	
	_init_gpgme(&ctx, conf);
	_encrypt_file(&ctx, in_file, out_file);
	_clean_gpgme(&ctx);
}

