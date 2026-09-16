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
    message=Path(temp)/'message-tests'
    message_abi=Path(temp)/'message-abi.o'
    subprocess.run(['xcrun','clang','-fobjc-arc','-O2','-c',str(root/'src/MessageABITests.m'),'-o',str(message_abi)],check=True)
    subprocess.run(['xcrun','swiftc','-module-name','TalkAppBase',str(root/'src/MessageCore.swift'),
                    str(root/'src/MessageTests.swift'),str(message_abi),'-o',str(message)],check=True)
    subprocess.run([str(message)],check=True)
    privacy=Path(temp)/'photo-privacy-tests'
    subprocess.run(['xcrun','swiftc',str(root/'src/PhotoPrivacy.swift'),
                    str(root/'src/PhotoPrivacyTests.swift'),'-o',str(privacy)],check=True)
    subprocess.run([str(privacy)],check=True)
    rendering=Path(temp)/'more-rendering-tests'
    subprocess.run(['xcrun','swiftc',str(root/'src/MoreTabRendering.swift'),
                    str(root/'src/MoreRenderingTests.swift'),'-o',str(rendering)],check=True)
    subprocess.run([str(rendering)],check=True)
