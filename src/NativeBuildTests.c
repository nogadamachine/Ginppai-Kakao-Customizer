#include "NativeBuildProfile.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    const char *versions[]={"26.7.3","26.8.0","26.9.2"};
    const uint8_t ids[][16]={
        {0xfa,0xab,0x71,0xb6,0x29,0x25,0x3d,0x3e,0x81,0x7f,0xc6,0x61,0x42,0xb3,0x77,0x78},
        {0xf4,0xb1,0x11,0xb1,0x95,0x83,0x3b,0xb9,0x98,0x1b,0xbf,0x70,0x2c,0x6b,0x93,0xe3},
        {0x26,0x2e,0x04,0x39,0x8a,0xcc,0x30,0xc4,0x97,0x7d,0xf0,0xe7,0x54,0xd0,0x37,0xc7}
    };
    for(unsigned i=0;i<3;i++) {
        assert(KCProfileForBuild(versions[i],ids[i])==(KCBuildProfile)(i+1));
        for(unsigned j=0;j<3;j++)if(i!=j)assert(KCProfileForBuild(versions[i],ids[j])==KCBuildUnsupported);
        for(unsigned byte=0;byte<16;byte++) {
            uint8_t changed[16];memcpy(changed,ids[i],16);changed[byte]^=1;
            assert(KCProfileForBuild(versions[i],changed)==KCBuildUnsupported);
        }
        assert(KCProfileForBuild("26.9.3",ids[i])==KCBuildUnsupported);
    }
    assert(KCProfileForBuild(NULL,ids[0])==KCBuildUnsupported);
    assert(KCProfileForBuild(versions[0],NULL)==KCBuildUnsupported);
    puts("Exact version/UUID and appearance-only build boundaries passed.");
}
