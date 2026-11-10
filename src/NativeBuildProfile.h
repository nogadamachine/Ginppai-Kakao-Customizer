// Build identity is a safety boundary: appearance support grants no native addresses.
#ifndef KC_NATIVE_BUILD_PROFILE_H
#define KC_NATIVE_BUILD_PROFILE_H
#include <stdint.h>
#include <string.h>

typedef enum {
    KCBuildUnsupported = 0,
    KCBuildNative2673 = 1,
    KCBuildNative2680 = 2,
    KCBuildAdsShopping2692 = 3
} KCBuildProfile;

static KCBuildProfile KCProfileForBuild(const char *version, const uint8_t uuid[16]) {
    static const uint8_t v2673[] = {0xfa,0xab,0x71,0xb6,0x29,0x25,0x3d,0x3e,0x81,0x7f,0xc6,0x61,0x42,0xb3,0x77,0x78};
    static const uint8_t v2680[] = {0xf4,0xb1,0x11,0xb1,0x95,0x83,0x3b,0xb9,0x98,0x1b,0xbf,0x70,0x2c,0x6b,0x93,0xe3};
    static const uint8_t v2692[] = {0x26,0x2e,0x04,0x39,0x8a,0xcc,0x30,0xc4,0x97,0x7d,0xf0,0xe7,0x54,0xd0,0x37,0xc7};
    if (!version || !uuid) return KCBuildUnsupported;
    if (!strcmp(version,"26.7.3") && !memcmp(uuid,v2673,16)) return KCBuildNative2673;
    if (!strcmp(version,"26.8.0") && !memcmp(uuid,v2680,16)) return KCBuildNative2680;
    if (!strcmp(version,"26.9.2") && !memcmp(uuid,v2692,16)) return KCBuildAdsShopping2692;
    return KCBuildUnsupported;
}
#endif
