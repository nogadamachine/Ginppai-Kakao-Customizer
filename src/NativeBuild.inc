// Exact build profiles. Addresses are unslid and independently checked against
// exports, native instructions, Swift vtables and the on-device capability probes.
#import <mach-o/loader.h>
static NSUInteger kcNativeProfile;
static BOOL kcSelectNativeBuild(void) {
    NSString *version=[NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
    for(uint32_t i=0;i<_dyld_image_count();i++) {
        const char *name=_dyld_get_image_name(i);
        if(!name || ![[NSString stringWithUTF8String:name].lastPathComponent isEqualToString:@"KakaoTalk"])continue;
        const struct mach_header_64 *h=(const void *)_dyld_get_image_header(i);
        if(h->magic!=MH_MAGIC_64)return NO;
        const uint8_t *cursor=(const void *)(h+1),*end=cursor+h->sizeofcmds;
        for(uint32_t j=0;j<h->ncmds && cursor+sizeof(struct load_command)<=end;j++) {
            const struct load_command *c=(const void *)cursor;
            if(c->cmdsize<sizeof(*c) || cursor+c->cmdsize>end)return NO;
            if(c->cmd==LC_UUID && c->cmdsize>=sizeof(struct uuid_command)) {
                const uint8_t oldID[]={0xfa,0xab,0x71,0xb6,0x29,0x25,0x3d,0x3e,0x81,0x7f,0xc6,0x61,0x42,0xb3,0x77,0x78};
                const uint8_t newID[]={0xf4,0xb1,0x11,0xb1,0x95,0x83,0x3b,0xb9,0x98,0x1b,0xbf,0x70,0x2c,0x6b,0x93,0xe3};
                const uint8_t *uuid=((const struct uuid_command *)c)->uuid;
                if([version isEqualToString:@"26.7.3"] && memcmp(uuid,oldID,16)==0)kcNativeProfile=1;
                if([version isEqualToString:@"26.8.0"] && memcmp(uuid,newID,16)==0)kcNativeProfile=2;
                return kcNativeProfile!=0;
            }
            cursor+=c->cmdsize;
        }
    }
    return NO;
}
static uintptr_t KCAddress(uintptr_t legacy) {
    if(kcNativeProfile==1)return legacy;
    if(kcNativeProfile!=2)return 0;
    switch(legacy) {
        case 0x100297d64:return 0x10029a7a0;
        case 0x10078c178:return 0x10077693c;
        case 0x10078c17c:return 0x100776940;
        case 0x1007b752c:return 0x1007a1994;
        case 0x1007b7530:return 0x1007a1998;
        case 0x1008133d0:return 0x1007f9324;
        case 0x100b153b4:return 0x100ae9ba4;
        case 0x100d91268:return 0x100d6639c;
        case 0x100dbcb94:return 0x100d921dc;
        case 0x1017bec68:return 0x1017b54a8;
        case 0x1017becc8:return 0x1017b5508;
        case 0x1017beccc:return 0x1017b550c;
        case 0x1017becdc:return 0x1017b551c;
        case 0x10461fc74:return 0x104722260;
        case 0x10461fca8:return 0x104722298;
        case 0x10461fea8:return 0x1047224a0;
        case 0x10461feec:return 0x1047224e4;
        case 0x104621174:return 0x10472374c;
        case 0x104631934:return 0x104733e44;
        case 0x104679d4c:return 0x10477dc84;
        case 0x104694bfc:return 0x1047987e4;
        case 0x104694c00:return 0x1047987e8;
        case 0x104695010:return 0x104798bf8;
        case 0x104695014:return 0x104798bfc;
        case 0x1046957dc:return 0x1047993c0;
        case 0x1046957e0:return 0x1047993c4;
        case 0x1046acee0:return 0x1047b1d10;
        case 0x1046ade0c:return 0x1047b29cc;
        case 0x104de6b68:return 0x104efcdb8;
        case 0x104e12ed8:return 0x104f29174;
        case 0x104e13388:return 0x104f29624;
        case 0x1083e7f00:return 0x10856ff00;
        case 0x1083e7f08:return 0x10856ff08;
        case 0x1083e7f10:return 0x10856ff10;
        default:return 0;
    }
}
static NSUInteger KCMessageSlot(NSUInteger legacy) {return kcNativeProfile==2?legacy-8:legacy;}
static NSUInteger KCTypingSlot(void) {return kcNativeProfile==2?0xc80:0xc90;}
