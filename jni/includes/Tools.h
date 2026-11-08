#pragma once

#include <string>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/md5.h>

namespace Tools
{
    std::string CalcSHA256(std::string s);
}
