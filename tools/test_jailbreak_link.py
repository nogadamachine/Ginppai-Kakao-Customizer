#!/usr/bin/env python3
"""Exercise the hook-provider dependency with the real macOS dynamic loader."""
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import unittest
from unittest.mock import patch

import jailbreak_link as link


class JailbreakLinkTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if os.uname().sysname != 'Darwin':
            raise unittest.SkipTest('Requires the macOS dynamic loader and clang.')
        cls.temp = tempfile.TemporaryDirectory(prefix='ginppai-loader-test-')
        cls.root = Path(cls.temp.name)
        cls.env = dict(os.environ)
        clt = Path('/Library/Developer/CommandLineTools')
        if (clt/'usr/bin/clang').exists():
            cls.env['DEVELOPER_DIR'] = str(clt)
            cls.clang = str(clt/'usr/bin/clang')
        else:
            cls.clang = shutil.which('clang')
        cls.sdk = subprocess.check_output(['xcrun', '--sdk', 'macosx', '--show-sdk-path'],
                                          env=cls.env, text=True).strip()
        framework = cls.root/'Frameworks/CydiaSubstrate.framework'
        framework.mkdir(parents=True)
        cls.compile('provider', 'int MSHookFunction(void) { return 73; }',
                    framework/'CydiaSubstrate', '-dynamiclib', '-Wl,-install_name,'+link.LIBRARY)
        cls.compile('plugin', '#include <dlfcn.h>\nint available(void) {\n'
                    'int (*hook)(void) = dlsym(RTLD_DEFAULT, "MSHookFunction");\n'
                    'return hook ? hook() : 0; }', cls.root/'original.dylib',
                    '-dynamiclib', '-Wl,-headerpad,0x1000')
        cls.compile('loader', '#include <dlfcn.h>\n#include <stdio.h>\n'
                    'int main(int argc, char **argv) {\n'
                    'void *p = dlopen(argv[1], RTLD_LAZY);\n'
                    'if (!p) { puts(dlerror()); return 2; }\n'
                    'int (*test)(void) = dlsym(p, "available");\n'
                    'if (!test) return 3; printf("%d\\n", test()); return 0; }',
                    cls.root/'loader')
        cls.original = (cls.root/'original.dylib').read_bytes()

    @classmethod
    def compile(cls, name, code, output, *flags):
        source = cls.root/(name+'.c')
        source.write_text(code)
        subprocess.run([cls.clang, '-isysroot', cls.sdk, str(source), '-o', str(output), *flags],
                       env=cls.env, check=True)

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def test_provider_becomes_available_without_other_tweaks(self):
        def probe(path):
            return subprocess.check_output([str(self.root/'loader'), str(path)], text=True).strip()
        self.assertEqual(probe(self.root/'original.dylib'), '0')
        # Substitute only the framework search directory; use the production dependency name.
        with patch.dict(link.RPATHS, {'rootful': [str(self.root/'Frameworks')]}):
            linked = link.link_provider(self.original, 'rootful')
            self.assertEqual(link.link_provider(linked, 'rootful'), linked)
        path = self.root/'linked.dylib'
        path.write_bytes(linked)
        subprocess.run(['codesign', '--force', '--sign', '-', str(path)], check=True, capture_output=True)
        self.assertEqual(probe(path), '73')
        self.assertEqual(probe(self.root/'original.dylib'), '0')

    def test_preserves_payload_and_rejects_insufficient_padding(self):
        linked = link.link_provider(self.original, 'rootless')
        new_end = 32 + struct.unpack_from('<I', linked, 20)[0]
        self.assertEqual(linked[new_end:], self.original[new_end:])
        self.assertEqual(link.link_provider(linked, 'rootless'), linked)
        damaged = bytearray(self.original)
        old_end = 32 + struct.unpack_from('<I', damaged, 20)[0]
        damaged[old_end] = 1
        with self.assertRaisesRegex(ValueError, 'padding'):
            link.link_provider(bytes(damaged), 'rootless')

    def test_rejects_wrong_or_truncated_inputs(self):
        for data in (b'', b'not a Mach-O', self.original[:40]):
            with self.assertRaises(ValueError):
                link.link_provider(data, 'rootless')
        with self.assertRaises(ValueError):
            link.link_provider(self.original, 'roothide')


if __name__ == '__main__':
    unittest.main(verbosity=2)
