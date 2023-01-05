/* get_x509_fp.cpp
   Copyright (C) 2023 Markus Uhlin. All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   - Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.

   - Neither the name of the author nor the names of its contributors may be
     used to endorse or promote products derived from this software without
     specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE. */

#include "common.h"

#include <openssl/pem.h>

#include <cstring>
#include <stdexcept>
#include <string>

#include "filePred.h"
#include "get_x509_fp.hpp"
#include "printtext.h"

x509_fingerprint::x509_fingerprint()
{
	this->bio = nullptr;
	this->cert = nullptr;
	this->alg = nullptr;
	BZERO(&this->md[0], sizeof this->md);
	this->md_len = 0;
}

x509_fingerprint::x509_fingerprint(const char *path)
{
	if (!file_exists(path))
		throw std::runtime_error("file non-existent");
	else if (!is_regular_file(path))
		throw std::runtime_error("not a regular file");
	else if ((this->bio = BIO_new(BIO_s_file())) == nullptr)
		throw std::runtime_error("out of memory");
	else if (!BIO_read_filename(this->bio, path) ||
	    (this->cert = PEM_read_bio_X509(this->bio, nullptr, nullptr,
	    nullptr)) == nullptr)
		throw std::runtime_error("read error");
	this->alg = EVP_sha1();
	if (!X509_digest(this->cert, this->alg, &this->md[0], &this->md_len))
		throw std::runtime_error("unable to get the digest");
}

x509_fingerprint::~x509_fingerprint()
{
	if (this->bio)
		BIO_free_all(this->bio);
	if (this->cert)
		X509_free(this->cert);
}

void
x509_fingerprint::show_fp(void)
{
	bool		error = false;
	std::string	str("");

	for (unsigned int i = 0; i < this->md_len; i++) {
		char	buf[3] = { '\0' };
		int	ret;

		if ((ret = snprintf(buf, sizeof buf, "%02X", this->md[i])) < 0 ||
		    static_cast<size_t>(ret) >= sizeof buf) {
			error = true;
			break;
		}

		str.append(&buf[0]);
	}

	if (error) {
		printtext_print("err", "error showing the fingerprint");
		return;
	}

	printtext_print("success", "fingerprint (%s): %s",
	    (this->alg == EVP_sha1() ? "sha1" : "unknown"),
	    str.c_str());
}
