#!/usr/bin/env python3
from pathlib import Path
import subprocess
import sys
import tempfile

root=Path(__file__).resolve().parent
subprocess.run([sys.executable,str(root/'tools/test_tools.py')],check=True)
with tempfile.TemporaryDirectory(prefix='ginppai-test-') as temp:
    binary=Path(temp)/'resolver-tests'
    # The fixture types intentionally use KakaoTalk's real Swift module name.
    subprocess.run(['xcrun','swiftc','-module-name','Profile',str(root/'src/MediaResolver.swift'),
                    str(root/'src/ResolverTests.swift'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
