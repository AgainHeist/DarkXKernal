#pragma once
#include "../other/memory.hpp"
#include <string>
#include <sstream>
#include "protect/oxorany.h"
#include "protect/oxorany_include.h"

class MonoDictionary {
public:
    uint64_t address;

    MonoDictionary(uint64_t addr) : address(addr) {}

    uint64_t getValues() const {
        return rpm<uint64_t>(address + 0x28) + 0x20;
    }
    
    int getNumValues() const {
        return rpm<int>(address + 0x38);
    }
};

inline uint64_t string2Offset(const char *c) {
    std::string str(c);
    if (str.empty()) return 0;
    
    if (str.size() > 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        str = str.substr(2);
    }
    
    uint64_t result = 0;
    std::stringstream ss;
    ss << std::hex << str;
    ss >> result;
    
    return result;
}

inline void getUTF8(char* dst, uint64_t addr) {
    if (addr < 0x10000000) {
        dst[0] = '\0';
        return;
    }

    int stringLen = rpm<int>(addr + 0x10);
    if (stringLen <= 0 || stringLen > 64) {
        stringLen = 32; 
    }

    unsigned char raw[128]; 
    for (int i = 0; i < (stringLen * 2); i++) {
        raw[i] = rpm<unsigned char>(addr + 0x14 + i);
    }

    int j = 0;
    for (int i = 0; i < (stringLen * 2); i += 2) {
        unsigned short unicode = raw[i] | (raw[i + 1] << 8);
        if (unicode == 0) break;
        if (unicode < 0x80) {
            dst[j++] = (char)unicode;
        } else if (unicode < 0x800) {
            dst[j++] = (char)((unicode >> 6) | 0xC0);
            dst[j++] = (char)((unicode & 0x3F) | 0x80);
        } else if (unicode >= 0xD800 && unicode <= 0xDBFF) { 
            i += 2;
            unsigned short low = raw[i] | (raw[i + 1] << 8);
            unsigned int codepoint = 0x10000 + ((unicode - 0xD800) << 10) + (low - 0xDC00);
            dst[j++] = (char)((codepoint >> 18) | 0xF0);
            dst[j++] = (char)(((codepoint >> 12) & 0x3F) | 0x80);
            dst[j++] = (char)(((codepoint >> 6) & 0x3F) | 0x80);
            dst[j++] = (char)((codepoint & 0x3F) | 0x80);
        } else {
            dst[j++] = (char)((unicode >> 12) | 0xE0);
            dst[j++] = (char)(((unicode >> 6) & 0x3F) | 0x80);
            dst[j++] = (char)((unicode & 0x3F) | 0x80);
        }
        if (j >= 60) break; 
    }
    dst[j] = '\0';
}

inline const char* GetWeaponName(int wp) {
    switch (wp) {
        case 1: return oxorany("HAND");
        case 2: return oxorany("M4A1");
        case 80: return oxorany("M4A1-LV1");
        case 81: return oxorany("M4A1-LV2");
        case 82: return oxorany("M4A1-LV3");
        case 12: return oxorany("SCAR");
        case 178: return oxorany("SCAR-LV1");
        case 179: return oxorany("SCAR-LV2");
        case 180: return oxorany("SCAR-LV3");
        case 14: return oxorany("GROZA");
        case 70: return oxorany("GROZA-X");
        case 24: return oxorany("FAMAS");
        case 67: return oxorany("FAMAS-LV1");
        case 130: return oxorany("FAMAS-LV2");
        case 131: return oxorany("FAMAS-LV3");
        case 39: return oxorany("PLASMA");
        case 46: return oxorany("AUG");
        case 6: return oxorany("AK-47");
        case 28: return oxorany("XM8");
        case 33: return oxorany("AN94");
        case 47: return oxorany("PARAFAL");
        case 57: return oxorany("KINGFISHER");
        case 73: return oxorany("G36");
        case 99: return oxorany("SHIELD GUN");
        case 11: return oxorany("M14");
        case 63: return oxorany("M14-LV1");
        case 126: return oxorany("M14-LV2");
        case 127: return oxorany("M14-LV3");
        case 18: return oxorany("SKS");
        case 26: return oxorany("SVD");
        case 72: return oxorany("SVD-Y");
        case 48: return oxorany("WOODPECKER");
        case 89: return oxorany("AC80");
        case 19: return oxorany("M249");
        case 71: return oxorany("M249-X");
        case 30: return oxorany("M60");
        case 61: return oxorany("M60-LV1");
        case 122: return oxorany("M60-LV2");
        case 123: return oxorany("M60-LV3");
        case 54: return oxorany("KORD");
        case 7: return oxorany("UMP");
        case 8: return oxorany("MP5");
        case 60: return oxorany("MP5-LV1");
        case 120: return oxorany("MP5-LV2");
        case 121: return oxorany("MP5-LV3");
        case 15: return oxorany("MP40");
        case 32: return oxorany("P90");
        case 35: return oxorany("CG15");
        case 43: return oxorany("THOMSON");
        case 49: return oxorany("VECTOR");
        case 88: return oxorany("MAC10");
        case 150: return oxorany("PP19");
        case 5: return oxorany("M1014");
        case 184: return oxorany("M1014-LV1");
        case 185: return oxorany("M1014-LV2");
        case 186: return oxorany("M1014-LV3");
        case 29: return oxorany("SPAS12");
        case 41: return oxorany("M1887");
        case 119: return oxorany("M1887-X");
        case 50: return oxorany("MAG-7");
        case 86: return oxorany("CHARGE BUSTER");
        case 181: return oxorany("TROGON");
        case 21002: return oxorany("M590");
        case 4: return oxorany("AWM");
        case 65: return oxorany("AWM-Y");
        case 30060: return oxorany("AWM-INFINITE");
        case 21: return oxorany("KAR98K");
        case 64: return oxorany("KAR98K-LV1");
        case 128: return oxorany("KAR98K-LV2");
        case 129: return oxorany("KAR98K-LV3");
        case 45: return oxorany("M82B");
        case 75: return oxorany("M24");
        case 197: return oxorany("VSK94");
        case 13: return oxorany("VSS");
        case 3: return oxorany("USP");
        case 9: return oxorany("DESERT EAGLE");
        case 10: return oxorany("G18");
        case 20: return oxorany("M1873");
        case 25: return oxorany("M500");
        case 55: return oxorany("M1917");
        case 56: return oxorany("USP-2");
        case 58: return oxorany("MINI UZI");
        case 16: return oxorany("PAN");
        case 17: return oxorany("PARANG");
        case 27: return oxorany("BAT");
        case 34: return oxorany("KATANA");
        case 51: return oxorany("SCYTHE");
        case 93: return oxorany("HEALING PISTOL");
        case 21001: return oxorany("HEALING PISTOL-Y");
        case 78: return oxorany("HEALING SNIPER");
        case 100: return oxorany("FGL-24");
        case 1201: return oxorany("GLOO WALL");
        case 617: return oxorany("GLOO MELTER");
        case 601: return oxorany("GRENADE");
        default: return oxorany("Unknown");
    }
}