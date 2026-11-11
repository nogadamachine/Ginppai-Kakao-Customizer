// One read-only, UUID- and full-body-verified native presentation setter.
// This calls the original Swift setter so native Observation notifications are
// preserved. It never rewrites code, a witness table or a Swift stored field.
#import <mach-o/dyld.h>
#import <mach-o/loader.h>
#import <dlfcn.h>
#include <math.h>
_Static_assert(sizeof(CGFloat)==sizeof(double), "More 26.9.2 padding requires arm64 CGFloat ABI");
typedef void KCMoreHomePaddingSetter(CGFloat value,id __attribute__((swift_context)) context) __attribute__((swiftcall));

static const struct mach_header_64 *KCMHPVerifiedImage(const uint8_t **function) {
    if(!kcAppearanceOnlyBuild || ![NSBundle.mainBundle.bundleIdentifier isEqualToString:@"com.iwilab.KakaoTalk"] ||
       ![[NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"] isEqualToString:@"26.9.2"])return NULL;
    static const uint8_t uuid[16]={0x26,0x2e,0x04,0x39,0x8a,0xcc,0x30,0xc4,0x97,0x7d,0xf0,0xe7,0x54,0xd0,0x37,0xc7};
    // LC_FUNCTION_STARTS: 0x105b05f38 .. 0x105b05fb8, 128 bytes.
    // SHA256: de4303b96a4b07f7ed83d5a88243097b3c64c0682d6d04d86fbfb66118d652a5
    static const uint8_t body[128]={
        0xff,0x43,0x02,0xd1,0xe9,0x23,0x05,0x6d,0xf6,0x57,0x06,0xa9,0xf4,0x4f,0x07,0xa9,
        0xfd,0x7b,0x08,0xa9,0xfd,0x03,0x02,0x91,0x08,0x1c,0xa0,0x4e,0x80,0x42,0x00,0x91,
        0xe1,0xe3,0x00,0x91,0x22,0x00,0x80,0x52,0x35,0x66,0x94,0x96,0x80,0x0a,0x40,0xfd,
        0x00,0x20,0x68,0x1e,0x61,0x00,0x00,0x54,0x88,0x02,0x01,0xfc,0x0b,0x00,0x00,0x14,
        0xee,0x35,0x00,0x94,0xf3,0x03,0x00,0xaa,0xf4,0x13,0x00,0xf9,0xe8,0x17,0x00,0xfd,
        0x01,0x00,0x00,0x90,0x21,0xe0,0x3e,0x91,0xe2,0x43,0x00,0x91,0x15,0x00,0x80,0xd2,
        0x1c,0x00,0x00,0x94,0x23,0x99,0x0d,0x94,0xfd,0x7b,0x48,0xa9,0xf4,0x4f,0x47,0xa9,
        0xf6,0x57,0x46,0xa9,0xe9,0x23,0x45,0x6d,0xff,0x43,0x02,0x91,0xc0,0x03,0x5f,0xd6,
    };
    const struct mach_header_64 *match=NULL;const uint8_t *address=NULL;
    for(uint32_t index=0;index<_dyld_image_count();index++) {
        const char *name=_dyld_get_image_name(index);
        if(!name || ![[NSString stringWithUTF8String:name].lastPathComponent isEqualToString:@"KakaoTalk"])continue;
        const struct mach_header_64 *header=(const void *)_dyld_get_image_header(index);
        if(!header || header->magic!=MH_MAGIC_64 || header->sizeofcmds>1024*1024 || header->ncmds>4096)return NULL;
        BOOL identity=NO,executable=NO;const uint8_t *cursor=(const void *)(header+1),*end=cursor+header->sizeofcmds;
        for(uint32_t command=0;command<header->ncmds;command++) {
            if(cursor+sizeof(struct load_command)>end)return NULL;
            const struct load_command *load=(const void *)cursor;
            if(load->cmdsize<sizeof(*load) || load->cmdsize>(uintptr_t)(end-cursor))return NULL;
            if(load->cmd==LC_UUID && load->cmdsize>=sizeof(struct uuid_command))identity=!memcmp(((const struct uuid_command *)load)->uuid,uuid,sizeof(uuid));
            if(load->cmd==LC_SEGMENT_64 && load->cmdsize>=sizeof(struct segment_command_64)) {
                const struct segment_command_64 *segment=(const void *)load;
                if(!memcmp(segment->segname,"__TEXT",7) && segment->vmaddr==UINT64_C(0x100000000) && segment->fileoff==0 &&
                   segment->filesize>=UINT64_C(0x5b05fb8) && segment->vmsize>=UINT64_C(0x5b05fb8) &&
                   (segment->initprot&(VM_PROT_READ|VM_PROT_EXECUTE))==(VM_PROT_READ|VM_PROT_EXECUTE))executable=YES;
            }
            cursor+=load->cmdsize;
        }
        if(!identity || !executable || match)return NULL;
        const uint8_t *candidate=(const uint8_t *)header+UINT64_C(0x5b05f38);
        Dl_info info={0};if(!dladdr(candidate,&info) || info.dli_fbase!=(const void *)header || memcmp(candidate,body,sizeof(body)))return NULL;
        match=header;address=candidate;
    }
    if(!match)return NULL;if(function)*function=address;return match;
}
static NSDictionary *KCMHPState(id controller) {
    if(![NSStringFromClass(object_getClass(controller)) isEqualToString:@"KakaoTalk.MoreTabHomeViewController"])return nil;
    Class bridge=NSClassFromString(@"KCChannelNavigationBridge");SEL selector=NSSelectorFromString(@"moreHomePaddingState:");
    Method method=class_getClassMethod(bridge,selector);char result[32],argument[32];
    if(!method || method_getNumberOfArguments(method)!=3)return nil;
    method_getReturnType(method,result,sizeof(result));method_getArgumentType(method,2,argument,sizeof(argument));
    if(strcmp(result,"@") || argument[0]!='@')return nil;
    id state=((id(*)(id,SEL,id))objc_msgSend)(bridge,selector,controller);
    return [state isKindOfClass:NSDictionary.class] && [state[@"padding"] isKindOfClass:NSNumber.class]?state:nil;
}
static NSNumber *KCMHPPadding(id context) {
    Class bridge=NSClassFromString(@"KCChannelNavigationBridge");SEL selector=NSSelectorFromString(@"moreContextPadding:");
    Method method=class_getClassMethod(bridge,selector);char result[32],argument[32];
    if(!method || method_getNumberOfArguments(method)!=3)return nil;
    method_getReturnType(method,result,sizeof(result));method_getArgumentType(method,2,argument,sizeof(argument));
    if(strcmp(result,"@") || argument[0]!='@')return nil;
    id value=((id(*)(id,SEL,id))objc_msgSend)(bridge,selector,context);
    return [value isKindOfClass:NSNumber.class]?value:nil;
}
static BOOL KCMoreHomeSetNativePadding(id controller,id context,CGFloat expected,CGFloat target) {
    if(!NSThread.isMainThread || !isfinite(expected) || !isfinite(target) || expected<56 || expected>256 || target<56 || target>256 || fabs(fabs(target-expected)-50)>=0.5)return NO;
    const uint8_t *address=NULL;const struct mach_header_64 *header=KCMHPVerifiedImage(&address);if(!header)return NO;
    if(![NSStringFromClass(object_getClass(context)) isEqualToString:@"WalletUI.WalletHomeLayoutContext"])return NO;
    Dl_info contextImage={0};if(!dladdr((__bridge const void *)object_getClass(context),&contextImage) || contextImage.dli_fbase!=(const void *)header)return NO;
    NSDictionary *state=KCMHPState(controller);NSNumber *before=state[@"padding"];
    if(state[@"context"]!=context || !isfinite(before.doubleValue) || fabs(before.doubleValue-expected)>=0.5)return NO;
    KCMoreHomePaddingSetter *setter=(KCMoreHomePaddingSetter *)(uintptr_t)address;
    setter(target,context);
    NSNumber *after=KCMHPPadding(context);
    return after && isfinite(after.doubleValue) && fabs(after.doubleValue-target)<0.5;
}
