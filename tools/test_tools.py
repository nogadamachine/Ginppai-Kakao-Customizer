import struct
import unittest
from prepare_ipa import commands, inject, country_patch

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

if __name__=='__main__': unittest.main()
