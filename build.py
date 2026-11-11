#!/usr/bin/env python3
"""Build public arm64 iOS dylib without a personal signing identity."""
import os
import hashlib
import json
import re
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parent
BUILD = ROOT / 'build'
BUILD.mkdir(exist_ok=True)
SDK = subprocess.check_output(['xcrun', '--sdk', 'iphoneos', '--show-sdk-path'], text=True).strip()
ENV = {**os.environ, 'SDKROOT': SDK}
version = json.loads((ROOT/'package.json').read_text())['version']
if not re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+(?:-[a-z0-9.]+)?', version):
    raise ValueError('Invalid package version.')
build_id = hashlib.sha256(b''.join(p.read_bytes() for p in sorted((ROOT/'src').glob('*')) if p.suffix in ['.m', '.inc', '.h', '.swift'])).hexdigest()[:16]
target = ['-sdk', SDK, '-target', 'arm64-apple-ios17.0']
NATIVE_MODULES = BUILD/'native-modules'
NATIVE_MODULES.mkdir(exist_ok=True)
subprocess.run(['xcrun','swiftc',*target,'-parse-as-library','-emit-module','-module-name','TalkSharedBase',str(ROOT/'src/NativeChatTypes.swift'),'-o',str(NATIVE_MODULES/'TalkSharedBase.swiftmodule')],check=True,env=ENV)
subprocess.run(['xcrun','swiftc',*target,'-parse-as-library','-emit-module','-module-name','TalkAppBase',str(ROOT/'src/NativeUnreadTypes.swift'),'-o',str(NATIVE_MODULES/'TalkAppBase.swiftmodule')],check=True,env=ENV)
subprocess.run(['xcrun', 'swiftc', *target, '-O', '-whole-module-optimization', '-parse-as-library', '-emit-object',
                '-module-name', 'KCMedia', '-I', str(NATIVE_MODULES), str(ROOT/'src/MediaResolver.swift'), str(ROOT/'src/MessageCore.swift'), str(ROOT/'src/PhotoPrivacy.swift'), str(ROOT/'src/MoreTabRendering.swift'), str(ROOT/'src/NavigationAdPolicy.swift'), str(ROOT/'src/ChannelNavigation.swift'), '-o', str(BUILD/'MediaResolver.o')], check=True, env=ENV)
subprocess.run(['xcrun', 'clang', '-arch', 'arm64', '-isysroot', SDK, '-miphoneos-version-min=17.0',
                '-fobjc-arc', '-fblocks', '-O2', '-Wall', '-Wextra', '-Wno-unused-parameter',
                '-DKC_BUILD_ID="'+build_id+'"',
                '-DKC_VERSION="'+version+'"',
                '-Wno-unused-function', '-Wno-deprecated-declarations', '-c', str(ROOT/'src/KakaoCustomizer.m'),
                '-o', str(BUILD/'KakaoCustomizer.o')], check=True, env=ENV)
output = BUILD/'GinppaiKakaoCustomizer.dylib'
frameworks = ['UIKit', 'Foundation', 'CoreData', 'CoreGraphics', 'QuartzCore', 'CoreMedia', 'Photos', 'AVFoundation', 'ImageIO', 'UniformTypeIdentifiers']
native_symbols=['_$s14TalkSharedBase4ChatCMa','_$s14TalkSharedBase4ChatCMn',
                '_$s14TalkSharedBase15NSEUserSettingsCMa','_$s14TalkSharedBase15NSEUserSettingsC6sharedACvgZ',
                '_$s14TalkSharedBase15NSEUserSettingsC24excludeBadgeForMutedChatSbvg',
                '_$s11TalkAppBase17UnreadCountHelperCMa','_$s11TalkAppBase17UnreadCountHelperC6sharedACvgZ',
                '_$s11TalkAppBase17UnreadCountHelperC08openChatdE0Sivg']
subprocess.run(['xcrun', 'swiftc', *target, '-emit-library', str(BUILD/'MediaResolver.o'), str(BUILD/'KakaoCustomizer.o'),
                *[arg for f in frameworks for arg in ['-framework', f]], *[arg for symbol in native_symbols for arg in ['-Xlinker','-U','-Xlinker',symbol]], '-Xlinker', '-install_name', '-Xlinker',
                '@rpath/GinppaiKakaoCustomizer.dylib', '-o', str(output)], check=True, env=ENV)
# Declaration modules must never ship a second native implementation. Weak
# references keep other supported app builds loadable when an accessor is absent.
symbol_lines=subprocess.check_output(['nm','-m',str(output)],text=True).splitlines()
for symbol in native_symbols:
    matches=[line for line in symbol_lines if line.split() and line.split()[-1]==symbol]
    if matches and any('(undefined) weak external' not in line for line in matches):
        raise RuntimeError('Native declaration was defined or strongly linked: '+symbol)
for line in symbol_lines:
    symbol=line.split()[-1] if line.split() else ''
    if symbol.startswith(('_$s14TalkSharedBase4ChatC','_$s14TalkSharedBase15NSEUserSettingsC','_$s11TalkAppBase17UnreadCountHelperC')) and '(undefined)' not in line:
        raise RuntimeError('Native declaration implementation leaked into output: '+symbol)
subprocess.run(['codesign', '--force', '--sign', '-', '--identifier', 'com.nogadamachine.ginppai.kakaocustomizer', str(output)], check=True)
print(output)
