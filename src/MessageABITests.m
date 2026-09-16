#import <Foundation/Foundation.h>
typedef struct { uint64_t first, second; } KCTestSwiftString;
extern KCTestSwiftString __attribute__((swiftcall)) KCGinppaiNativeString(NSString *text);
static KCTestSwiftString __attribute__((swiftcall)) testGetter(__attribute__((swift_context)) void *context) {
    return KCGinppaiNativeString((__bridge NSString *)context);
}
KCTestSwiftString __attribute__((swiftcall)) KCMessageStringABIRoundtrip(NSString *text) {
    return testGetter((__bridge void *)text);
}

// Class-bound Swift existentials contain the object and its witness table.
typedef struct { void *object, *witness; } KCTestNativeRecord;
extern KCTestNativeRecord __attribute__((swiftcall)) KCABIGetRecord(__attribute__((swift_context)) void *);
extern void __attribute__((swiftcall)) KCABISetRecord(KCTestNativeRecord,__attribute__((swift_context)) void *);
void __attribute__((swiftcall)) KCRecordABICopy(id source, id destination) {
    KCABISetRecord(KCABIGetRecord((__bridge void *)source),(__bridge void *)destination);
}
