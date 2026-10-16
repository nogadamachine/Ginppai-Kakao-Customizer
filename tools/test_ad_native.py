import hashlib
import struct
import unittest
from pathlib import Path
from unittest.mock import patch
import ad_native_patch as ads


def fixture():
    b=bytearray(0x300000)
    struct.pack_into('<IIIIIIII',b,0,0xFEEDFACF,0x100000C,0,6,3,248,0,0)
    struct.pack_into('<II16s',b,32,0x1B,24,ads.SDK_UUID)
    struct.pack_into('<II16sQQQQIIII',b,56,0x19,152,b'__TEXT',0,0x300000,0,0x300000,5,5,1,0)
    struct.pack_into('<16s16sQQIIIIIIII',b,128,b'__text',b'__TEXT',0x4000,0x2FC000,0x4000,2,0,0,0,0,0,0)
    struct.pack_into('<II16sQQQQIIII',b,208,0x19,72,b'__DATA',0x500000,0x8000,0,0,3,3,0,0)
    for address,word in ads.ENTRIES:struct.pack_into('<I',b,address,word)
    return b


class AdNativeTests(unittest.TestCase):
    def apply(self,b):
        with patch.object(ads,'SDK_SHA256',hashlib.sha256(b).hexdigest()):return ads.patch(b)

    def test_only_reserved_padding_and_two_entries_change(self):
        b=fixture();p=self.apply(b)
        allowed=set(range(ads.CAVE,ads.MARKER+8))
        for entry,_ in ads.ENTRIES:allowed.update(range(entry,entry+4))
        self.assertEqual(len(b),len(p))
        self.assertTrue(all(x==y or i in allowed for i,(x,y) in enumerate(zip(b,p))))
        self.assertEqual(p[ads.MARKER:ads.MARKER+8],ads.MAGIC)

    def test_exact_original_fingerprint_is_required(self):
        with self.assertRaisesRegex(ValueError,'exact original'):ads.patch(fixture())

    def test_rejects_wrong_uuid_occupied_space_and_prologues(self):
        cases=[(40,b'X'),(ads.CAVE,b'X'),(ads.ENTRIES[0][0],b'XXXX')]
        for address,value in cases:
            with self.subTest(address=address):
                b=fixture();b[address:address+len(value)]=value
                with self.assertRaises(ValueError):self.apply(b)

    def test_rejects_slot_outside_writable_segment(self):
        b=fixture();struct.pack_into('<I',b,208+60,1)
        with self.assertRaisesRegex(ValueError,'layout'):self.apply(b)

    def test_rejects_overlapping_section(self):
        b=fixture();struct.pack_into('<I',b,208+4,152);struct.pack_into('<I',b,208+64,1)
        struct.pack_into('<QQ',b,280+32,ads.FLAGS,16)
        struct.pack_into('<I',b,20,328)
        with self.assertRaisesRegex(ValueError,'overlap'):self.apply(b)

    def test_second_patch_refused(self):
        with self.assertRaises(ValueError):self.apply(self.apply(fixture()))

    def test_switch_paths_and_callback_register_preservation(self):
        p=self.apply(fixture())
        # Decode and execute the trampoline's control flow independently of its
        # emitter. Stops at the displaced native prologue or the caller return.
        for route,(entry,prologue) in enumerate(ads.ENTRIES):
            cave=ads.CAVE+route*0x80
            for enabled in (False,True):
                for callback in (False,True):
                    with self.subTest(route=route,enabled=enabled,callback=callback):
                        pc=cave;sp=0x10000;x20=0xABCDEF;lr=0x876543;stack={};calls=[];w0=None
                        for _ in range(30):
                            word=struct.unpack_from('<I',p,pc)[0]
                            if word==prologue:
                                self.assertFalse(enabled)
                                jump=struct.unpack_from('<I',p,pc+4)[0]
                                imm=jump&0x3FFFFFF
                                if imm&(1<<25):imm-=1<<26
                                self.assertEqual(pc+4+imm*4,entry+4)
                                break
                            if word&0x7F000000==0x36000000:  # TBZ
                                imm=(word>>5)&0x3FFF
                                if imm&0x2000:imm-=0x4000
                                pc+=4 if enabled else imm*4;continue
                            if word&0x7F000000==0x34000000:  # CBZ x17
                                imm=(word>>5)&0x7FFFF
                                if imm&0x40000:imm-=0x80000
                                pc+=4 if callback else imm*4;continue
                            if word==0xA9BF7BF4:sp-=16;stack[sp]=(x20,lr)
                            elif word&0xFFE0001F==0x52800000:w0=(word>>5)&0xFFFF
                            elif word==0xD63F0220:
                                self.assertTrue(callback);calls.append(w0);lr=pc+4
                            elif word==0xA8C17BF4:(x20,lr)=stack.pop(sp);sp+=16
                            elif word==0xD65F03C0:
                                self.assertTrue(enabled);self.assertEqual((sp,x20,lr),(0x10000,0xABCDEF,0x876543));break
                            pc+=4
                        else:self.fail('Trampoline did not terminate')
                        self.assertEqual(calls,[route] if enabled and callback else [])


if __name__=='__main__':unittest.main()
