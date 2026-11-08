#pragma once
#include <stdexcept>
#include <curl/curl.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/md5.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <iomanip>
#include <sstream>
#include <sys/system_properties.h>
#include <fstream>
#include "protect/oxorany.h"
#include "protect/oxorany_include.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <algorithm>
#include <ctime>
#include <random>
#include "oxorany.h"
#include "StrEnc.h"
#include "external/json.hpp"
#include "../protect/SymbolHider.hpp"
#include "../protect/AdvancedProtect.hpp"

#include "../other/SafeSyscall.h"

namespace Tools {
    #define JUNK_CODE_SMALL \
        { \
            volatile uint32_t _js = 0xDEADC0DE; \
            _js = (_js ^ 0x55555555) + 1; \
            volatile uint32_t _jk = _js * 3; \
            (void)_jk; \
        }

    inline std::string CalcMD5(const std::string& input) {
        JUNK_CODE_SMALL
        unsigned char hash[MD5_DIGEST_LENGTH];
        MD5(reinterpret_cast<const unsigned char*>(input.c_str()), input.length(), hash);
        std::stringstream ss;
        for(int i = 0; i < MD5_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }
        return ss.str();
    }

    inline bool IsBeingDebugged() {
        int fd = Safe::openat(-100, oxorany("/proc/self/status"), O_RDONLY, 0);
        if (fd < 0) return false;
        char buf[4096];
        ssize_t n = Safe::read(fd, buf, sizeof(buf) - 1);
        Safe::close(fd);
        if (n > 0) {
            buf[n] = '\0';
            char* line = Hidden::StrTok(buf, "\n");
            while (line) {
                if (strncmp(line, oxorany("TracerPid:"), 10) == 0) {
                    int pid = Hidden::Atoi(line + 10);
                    if (pid != 0) return true;
                }
                line = Hidden::StrTok(NULL, "\n");
            }
        }
        return false;
    }

    inline uint32_t fnv1a(const std::string& s) {
        uint32_t h = 2166136261u;
        for (char c : s) {
            h ^= (uint8_t)c;
            h *= 16777619u;
        }
        return h;
    }

    inline std::string hmacSha256Hex(const std::string& key, const std::string& data) {
        unsigned char result[EVP_MAX_MD_SIZE];
        unsigned int len = 0;
        HMAC(EVP_sha256(), key.c_str(), (int)key.size(),
             reinterpret_cast<const unsigned char*>(data.c_str()), data.size(),
             result, &len);
        std::stringstream ss;
        for (unsigned int i = 0; i < len; i++)
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)result[i];
        return ss.str();
    }

    inline bool constantTimeEquals(const std::string& a, const std::string& b) {
        if (a.size() != b.size()) return false;
        unsigned char diff = 0;
        for (size_t i = 0; i < a.size(); i++)
            diff |= (unsigned char)(a[i] ^ b[i]);
        return diff == 0;
    }

    static const uint32_t LOGIN_HOST_HASH = 0x66D22D8D;

    inline std::string extractHost(const std::string& url) {
        size_t start = 0;
        if (url.rfind(oxorany("https://"), 0) == 0) start = 8;
        else if (url.rfind(oxorany("http://"), 0) == 0) start = 7;
        size_t end = url.find('/', start);
        return (end == std::string::npos) ? url.substr(start) : url.substr(start, end - start);
    }

    inline bool verifyHost(const std::string& url) {
        return true; // Verification disabled for new server compatibility
    }

    inline std::string getGameName() {
        // Hidden: FREEFIRE
        unsigned char g[] = { 0x07, 0x13, 0x04, 0x04, 0x07, 0x08, 0x13, 0x04 };
        std::string n = "";
        for(int i = 0; i < sizeof(g); i++) n += (char)(g[i] ^ 0x41);
        return n;
    }

    inline std::string buildLoginUrl() {
        // obfuscated: https://originalcheats.myvipsite.fun/connect
        unsigned char m[] = { 0x11, 0x0D, 0x0D, 0x09, 0x0A, 0x43, 0x56, 0x56, 0x16, 0x0B, 0x10, 0x1E, 0x10, 0x17, 0x18, 0x15, 0x1A, 0x11, 0x1C, 0x18, 0x0D, 0x0A, 0x57, 0x14, 0x00, 0x0F, 0x10, 0x09, 0x0A, 0x10, 0x0D, 0x1C, 0x57, 0x1F, 0x0C, 0x17, 0x56, 0x1A, 0x16, 0x17, 0x17, 0x1C, 0x1A, 0x0D };
        std::string s = "";
        for(int i = 0; i < sizeof(m); i++) s += (char)(m[i] ^ 0x79);
        return s;
    }

    inline std::string getSrvScrt() {
        // Hidden: TERI_MA_DI_FUDDI
        unsigned char s[] = { 0x76, 0x67, 0x70, 0x6B, 0x7D, 0x6F, 0x63, 0x7D, 0x66, 0x6B, 0x7D, 0x64, 0x77, 0x66, 0x66, 0x6B };
        std::string res = "";
        for(int i = 0; i < sizeof(s); i++) res += (char)(s[i] ^ 0x22);
        return res;
    }

    inline std::string getClntScrt() {
        // Hidden: OG_GAARA_104_CLIENT_SECRET
        unsigned char s[] = { 0x7C, 0x74, 0x6C, 0x74, 0x72, 0x72, 0x61, 0x72, 0x6C, 0x02, 0x03, 0x07, 0x6C, 0x70, 0x7F, 0x7A, 0x76, 0x7D, 0x67, 0x6C, 0x60, 0x76, 0x70, 0x61, 0x76, 0x67 };
        std::string res = "";
        for(int i = 0; i < sizeof(s); i++) res += (char)(s[i] ^ 0x33);
        return res;
    }

    inline std::string generateServerSign(const std::string& userKey, const std::string& hwid, const std::string& timestamp) {
        std::string secret = getSrvScrt();
        std::string data = userKey + hwid + timestamp + secret;
        uint32_t hash = 2166136261u;
        for (char c : data) {
            hash ^= (uint8_t)c;
            hash *= 16777619u;
        }
        memset((void*)secret.data(), 0, secret.size());
        char buf[9];
        snprintf(buf, sizeof(buf), oxorany("%08X"), hash);
        return std::string(buf);
    }

    inline std::string generateClientSign(const std::string& userKey, const std::string& hwid, const std::string& ts) {
        std::string secret = getClntScrt();
        std::string payload = getGameName() + oxorany("|") + userKey + oxorany("|") + hwid + oxorany("|") + ts;
        std::string result = hmacSha256Hex(secret, payload);
        memset((void*)secret.data(), 0, secret.size());
        return result;
    }

    inline bool nativeDetectHook() {
        JUNK_CODE_SMALL
        int fd = Safe::openat(-100, oxorany("/proc/self/maps"), O_RDONLY, 0);
        if (fd < 0) return false;
        char buf[4096];
        ssize_t n = Safe::read(fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            if (strstr(buf, oxorany("frida")) || strstr(buf, oxorany("xposed")) ||
                strstr(buf, oxorany("substrate")) || strstr(buf, oxorany("libgadget")) ||
                strstr(buf, oxorany("lsposed"))) {
                Safe::close(fd);
                return true;
            }
        }
        Safe::close(fd);
        return false;
    }
}

extern std::string EXP;
extern std::string g_UserKey;
extern std::string g_Seller;
extern bool bValid;
extern std::string g_encrypted_offsets_json;
extern time_t g_server_timestamp;

using namespace std;
using json = nlohmann::json;

struct MemoryStruct {
    char *memory;
    size_t size;
};

inline size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    auto *mem = (struct MemoryStruct *) userp;
    char *ptr = (char *) realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) return 0;
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

inline int check_vpn() {
    int fd = Safe::openat(-100, oxorany("/proc/net/route"), O_RDONLY, 0);
    if (fd < 0) return -1;
    char buf[2048];
    ssize_t n = Safe::read(fd, buf, sizeof(buf) - 1);
    Safe::close(fd);
    if (n > 0) {
        buf[n] = '\0';
        if (strstr(buf, oxorany("tun0")) || strstr(buf, oxorany("ppp0"))) return 1;
    }
    return 0;
}

inline std::string get_device_hwid() {
    char build_id[64] = {0};
    Hidden::SystemPropertyGet(oxorany("ro.build.display.id"), build_id);
    char build_hardware[64] = {0};
    Hidden::SystemPropertyGet(oxorany("ro.hardware"), build_hardware);
    char build_serial[64] = {0};
    Hidden::SystemPropertyGet(oxorany("ro.serialno"), build_serial);

    std::string UUID = build_id;
    UUID += build_hardware;
    UUID += build_serial;
    size_t pos = UUID.find(' ');
    while (pos != std::string::npos) {
        UUID.replace(pos, 1, "");
        pos = UUID.find(' ', pos);
    }
    std::string clean_uuid;
    for (char c : UUID) {
        if (isalnum(c) || c == '-') clean_uuid += c;
    }
    return clean_uuid.empty() ? oxorany("unknown_device") : clean_uuid;
}

inline std::string Login(const char *user_key) {
    if (Tools::IsBeingDebugged())
        return oxorany("SECURITY ALERT: Debugger detected!");
    if (Tools::nativeDetectHook())
        return oxorany("SECURITY ALERT: Hook detected!");
    if (check_vpn() == 1)
        return oxorany("Please turn off VPN!");

    std::string UUID = get_device_hwid();

    std::string api_url = Tools::buildLoginUrl();
    if (!Tools::verifyHost(api_url))
        return oxorany("SECURITY ALERT: Invalid server!");

    std::string timestamp = std::to_string(std::time(nullptr));

    std::string formData = oxorany("game=") + Tools::getGameName() +
                           oxorany("&user_key=") + std::string(user_key) +
                           oxorany("&serial=") + UUID +
                           oxorany("&version=2.0");
                           // client_sign removed as per new server logic

    struct MemoryStruct chunk;
    chunk.memory = (char *)malloc(1);
    chunk.size = 0;
    std::string responseMsg;

    CURL *curl = curl_easy_init();
    if (curl) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, oxorany("Content-Type: application/x-www-form-urlencoded"));
        headers = curl_slist_append(headers, oxorany("User-Agent: Mozilla/5.0 (Linux; Android 12; Pixel 6) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/100.0.4896.127 Mobile Safari/537.36"));
        headers = curl_slist_append(headers, oxorany("Accept: application/json"));
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_URL, api_url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, formData.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            if (http_code == 200) {
                try {
                    json response = json::parse(chunk.memory);

                    bool sigValid = true; // Disabled signature check for compatibility with new server

                    if (sigValid && response.contains(oxorany("status")) && response[oxorany("status")] == true) {
                        bValid = true;
                        responseMsg = oxorany("OK");

                        if (response.contains(oxorany("data")) && response[oxorany("data")].is_object()) {
                            auto& data = response[oxorany("data")];

                            // Handling optional offsets
                            if (data.contains(oxorany("encrypted_offsets")) && data.contains(oxorany("server_timestamp"))) {
                                g_encrypted_offsets_json = data[oxorany("encrypted_offsets")].dump();
                                g_server_timestamp = data[oxorany("server_timestamp")].get<time_t>();
                            }

                            if (data.contains(oxorany("expiry")))
                                EXP = data[oxorany("expiry")].get<std::string>();
                            else if (data.contains(oxorany("expiry_date")))
                                EXP = data[oxorany("expiry_date")].get<std::string>();
                            else if (data.contains(oxorany("expired_date")))
                                EXP = data[oxorany("expired_date")].get<std::string>();
                            else if (data.contains(oxorany("exdate")))
                                EXP = data[oxorany("exdate")].get<std::string>();
                            else
                                EXP = oxorany("N/A");

                            if (data.contains(oxorany("reseller")))
                                g_Seller = data[oxorany("reseller")].get<std::string>();
                            else if (data.contains(oxorany("seller")))
                                g_Seller = data[oxorany("seller")].get<std::string>();
                            else if (data.contains(oxorany("credit")))
                                g_Seller = data[oxorany("credit")].get<std::string>();
                            else
                                g_Seller = oxorany("Official");

                            g_UserKey = user_key;
                        } else {
                            EXP = response.value(oxorany("expiry"), oxorany("N/A"));
                            g_Seller = response.value(oxorany("seller"), oxorany("Official"));
                            g_UserKey = user_key;
                        }
                    } else {
                        bValid = false;
                        if (response.contains(oxorany("message")))
                            responseMsg = response[oxorany("message")].get<std::string>();
                        else if (response.contains(oxorany("reason")))
                            responseMsg = response[oxorany("reason")].get<std::string>();
                        else if (response.contains(oxorany("error")))
                            responseMsg = response[oxorany("error")].get<std::string>();
                        else
                            responseMsg = oxorany("Invalid Key");
                    }
                } catch (json::exception &e) {
                    bValid = false;
                    responseMsg = oxorany("Server Response Error");
                }
            } else {
                bValid = false;
                responseMsg = oxorany("Connection Error");
            }
        } else {
            bValid = false;
            responseMsg = oxorany("Network Error");
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    } else {
        bValid = false;
        responseMsg = oxorany("CURL Error");
    }

    if (chunk.memory) free(chunk.memory);
    return responseMsg;
}