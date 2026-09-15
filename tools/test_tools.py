import struct
import unittest
import hashlib
import plistlib
import tempfile
from pathlib import Path
from unittest.mock import patch as mock_patch
from prepare_ipa import commands, inject, country_patch, custom_branding
import native_patch

def sample():
    data=bytearray(0x9000)
    struct.pack_into('<IIIIIIII',data,0,0xFEEDFACF,0x100000C,0,2,0,0,0,0)
    return data

class InjectionTests(unittest.TestCase):
    def test_add_then_repeat_does_not_duplicate(self):
        data=sample();name='@executable_path/Frameworks/Test.dylib'
        result=inject(data,name)
        self.assertEqual(len(result),len(data))
        self.assertEqual(len(list(commands(result))),1)
        self.assertEqual(inject(result,name),result)
        self.assertEqual(data[0x8000:],result[0x8000:])
    def test_refuses_encryption(self):
        data=sample();struct.pack_into('<II',data,16,1,24)
        struct.pack_into('<IIIIII',data,32,0x2c,24,0x8000,0x1000,1,0)
        with self.assertRaisesRegex(ValueError,'Encrypted'): inject(data,'Test')
    def test_refuses_nonzero_padding(self):
        data=sample();data[32]=1
        with self.assertRaisesRegex(ValueError,'header space'): inject(data,'Test')

class BrandingTests(unittest.TestCase):
    def test_all_locales_change_without_changing_executable_or_bundle_id(self):
        with tempfile.TemporaryDirectory() as folder:
            app=Path(folder);locale=app/'ko.lproj';locale.mkdir()
            original={'CFBundleIdentifier':'com.iwilab.KakaoTalk','CFBundleExecutable':'KakaoTalk','CFBundleName':'Original'}
            (app/'Info.plist').write_bytes(plistlib.dumps(original))
            (locale/'InfoPlist.strings').write_text('"CFBundleDisplayName" = "Original"; "NSCameraUsageDescription" = "Camera";')
            custom_branding(app,'Ginppai Kakao')
            info=plistlib.loads((app/'Info.plist').read_bytes());localized=plistlib.loads((locale/'InfoPlist.strings').read_bytes())
            self.assertEqual(info['CFBundleIdentifier'],original['CFBundleIdentifier'])
            self.assertEqual(info['CFBundleExecutable'],original['CFBundleExecutable'])
            self.assertEqual(info['CFBundleDisplayName'],'Ginppai Kakao')
            self.assertEqual(localized['CFBundleDisplayName'],'Ginppai Kakao')
            self.assertEqual(localized['NSCameraUsageDescription'],'Camera')
    def test_invalid_name_and_bad_locale_leave_original_untouched(self):
        with tempfile.TemporaryDirectory() as folder:
            app=Path(folder);original=plistlib.dumps({'CFBundleName':'Original'});(app/'Info.plist').write_bytes(original)
            for name in ['', '  ', 'first\nsecond','x'*81]:
                with self.assertRaises(ValueError): custom_branding(app,name)
            locale=app/'en.lproj';locale.mkdir();(locale/'InfoPlist.strings').write_bytes(b'not a plist')
            with self.assertRaises(Exception): custom_branding(app,'New')
            self.assertEqual((app/'Info.plist').read_bytes(),original)

class NativeCallbackTests(unittest.TestCase):
    def fixture(self):
        data=sample()
        struct.pack_into('<II',data,16,2,224)
        struct.pack_into('<II',data,32,0x19,152)
        data[40:46]=b'__TEXT'
        struct.pack_into('<QQQQ',data,56,native_patch.BASE,0x9000,0,0x9000)
        struct.pack_into('<III',data,88,5,5,1)
        data[104:110]=b'__text'
        struct.pack_into('<QQI',data,136,native_patch.BASE+0x8000,0x1000,0x8000)
        struct.pack_into('<II',data,184,0x19,72)
        data[192:198]=b'__DATA'
        struct.pack_into('<QQQQ',data,208,0x1083e7000,0x1000,0,0)
        struct.pack_into('<III',data,240,3,3,0)
        struct.pack_into('<I',data,0x8200,0xaa1e03e0)
        return data
    def apply_fixture(self,data):
        digest=hashlib.sha256(data[0x8000:]).hexdigest()
        with mock_patch.object(native_patch,'VERIFIED_TEXT_HASHES',{digest}), mock_patch.object(native_patch,'SAVE_ENTRY',native_patch.BASE+0x8200):
            return native_patch.patch(data,feature_patches=False)
    def test_native_callback_changes_only_reserved_bytes_and_entry(self):
        original=self.fixture();result=self.apply_fixture(original)
        changed={i for i,(a,b) in enumerate(zip(original,result)) if a!=b}
        allowed=set(range(0x7400,0x7424))|set(range(0x77f0,0x77f8))|set(range(0x8200,0x8204))
        self.assertTrue(changed<=allowed)
        self.assertEqual(result[0x7000:0x7400],original[0x7000:0x7400])
        self.assertEqual(struct.unpack_from('<I',result,0x741c)[0],0xaa1e03e0)
        self.assertEqual(len(result),len(original))
    def test_callback_refuses_a_second_install(self):
        with self.assertRaisesRegex(ValueError,'already installed'):
            self.apply_fixture(self.apply_fixture(self.fixture()))
    def test_callback_refuses_occupied_code_space(self):
        data=self.fixture();data[0x7400]=1
        with self.assertRaisesRegex(ValueError,'not empty'):self.apply_fixture(data)
    def test_callback_refuses_readonly_data_segment(self):
        data=self.fixture();struct.pack_into('<I',data,244,1)
        with self.assertRaisesRegex(ValueError,'readable and writable'):self.apply_fixture(data)
    def test_branch_validates_alignment_and_reach(self):
        self.assertEqual(native_patch.branch(0x1000,0xffc),0x17ffffff)
        for target in [0x1001,0x1000+(1<<27)]:
            with self.assertRaises(ValueError):native_patch.branch(0x1000,target)
    def test_refuses_foreign_binary_country_patch(self):
        with self.assertRaisesRegex(ValueError,'verified'): country_patch(sample())
    def test_refuses_livecontainer_transformed_binary(self):
        data=sample();struct.pack_into('<I',data,12,6)
        with self.assertRaisesRegex(ValueError,'LiveContainer'): inject(data,'Test')
    def test_keeps_first_section_intact(self):
        data=sample();struct.pack_into('<II',data,16,1,152)
        struct.pack_into('<II',data,32,0x19,152)
        struct.pack_into('<I',data,32+64,1)
        struct.pack_into('<I',data,32+72+48,184)
        with self.assertRaisesRegex(ValueError,'header space'): inject(data,'Test')

class NativeFeatureTests(unittest.TestCase):
    def fixture(self):
        data=sample()
        struct.pack_into('<I',data,0x8200,0x540000AC)
        struct.pack_into('<I',data,0x8300,0xD10203FF)
        struct.pack_into('<I',data,0x8400,native_patch.COVER_PROLOGUE)
        struct.pack_into('<I',data,0x8500,native_patch.branch(native_patch.BASE+0x8500,native_patch.BASE+0x8600)|0x80000000)
        struct.pack_into('<I',data,0x8700,native_patch.MORE_TAB_PROLOGUE)
        return data
    def apply_fixture(self,data):
        with mock_patch.object(native_patch,'UNREAD_CLAMP',native_patch.BASE+0x8200), \
             mock_patch.object(native_patch,'UNREAD_RETURN',native_patch.BASE+0x8214), \
             mock_patch.object(native_patch,'UNREAD_CONTINUE',native_patch.BASE+0x8204), \
             mock_patch.object(native_patch,'BOOL_GETTERS',[(native_patch.BASE+0x8300,1)]), \
             mock_patch.object(native_patch,'COVER_ENTRY',native_patch.BASE+0x8400), \
             mock_patch.object(native_patch,'ALIMTALK_GETTER',native_patch.BASE+0x8600), \
             mock_patch.object(native_patch,'ALIMTALK_CALLSITES',[native_patch.BASE+0x8500]), \
             mock_patch.object(native_patch,'MORE_TAB_AVAILABLE',native_patch.BASE+0x8700):
            native_patch.patch_features(data)
        return data
    def test_changes_only_reserved_header_and_verified_instructions(self):
        original=self.fixture();result=self.apply_fixture(bytearray(original))
        changed={i for i,(a,b) in enumerate(zip(original,result)) if a!=b}
        allowed=set(range(0x7800,0x7818))|set(range(0x7820,0x783c))|set(range(0x7840,0x785c))|set(range(0x7ff0,0x7ff8))|set(range(0x8200,0x8204))|set(range(0x8300,0x8304))|set(range(0x8400,0x8404))
        allowed|=set(range(0x7860,0x7878))|set(range(0x8500,0x8504))
        allowed|=set(range(0x7880,0x78a8))|set(range(0x8700,0x8704))
        self.assertTrue(changed<=allowed)
        self.assertEqual(result[0x7400:0x7800],original[0x7400:0x7800])
        self.assertEqual(result[0x7834:0x7838],struct.pack('<I',0xD10203FF))
        self.assertEqual(result[0x7854:0x7858],struct.pack('<I',native_patch.COVER_PROLOGUE))
    def test_refuses_wrong_prologue_and_occupied_header(self):
        data=self.fixture();data[0x7800]=1
        with self.assertRaisesRegex(ValueError,'not empty'):self.apply_fixture(data)
        data=self.fixture();struct.pack_into('<I',data,0x8300,0xD65F03C0)
        with self.assertRaisesRegex(ValueError,'prologue'):self.apply_fixture(data)
        data=self.fixture();struct.pack_into('<I',data,0x8400,0xD65F03C0)
        with self.assertRaisesRegex(ValueError,'cover policy'):self.apply_fixture(data)
        data=self.fixture();struct.pack_into('<I',data,0x8700,0xD65F03C0)
        with self.assertRaisesRegex(ValueError,'More tab'):self.apply_fixture(data)
    def test_flag_branch_bounds(self):
        for target,bit in [(0x1001,1),(0x9000,1),(0x1004,32)]:
            with self.assertRaises(ValueError):native_patch.bit_branch(0x1000,target,bit)

if __name__=='__main__': unittest.main()
