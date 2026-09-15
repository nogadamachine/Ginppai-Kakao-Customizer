#!/usr/bin/env python3
"""Build public arm64 iOS dylib without a personal signing identity."""
import os
import hashlib
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parent
BUILD = ROOT / 'build'
BUILD.mkdir(exist_ok=True)
SDK = subprocess.check_output(['xcrun', '--sdk', 'iphoneos', '--show-sdk-path'], text=True).strip()
ENV = {**os.environ, 'SDKROOT': SDK}
build_id = hashlib.sha256(b''.join(p.read_bytes() for p in sorted((ROOT/'src').glob('*')) if p.suffix in ['.m', '.inc', '.swift'])).hexdigest()[:16]
target = ['-sdk', SDK, '-target', 'arm64-apple-ios17.0']
subprocess.run(['xcrun', 'swiftc', *target, '-O', '-whole-module-optimization', '-parse-as-library', '-emit-object',
                '-module-name', 'KCMedia', str(ROOT/'src/MediaResolver.swift'), str(ROOT/'src/MessageCore.swift'), str(ROOT/'src/PhotoPrivacy.swift'), '-o', str(BUILD/'MediaResolver.o')], check=True, env=ENV)
subprocess.run(['xcrun', 'clang', '-arch', 'arm64', '-isysroot', SDK, '-miphoneos-version-min=17.0',
                '-fobjc-arc', '-fblocks', '-O2', '-Wall', '-Wextra', '-Wno-unused-parameter',
                '-DKC_BUILD_ID="'+build_id+'"',
                '-Wno-unused-function', '-Wno-deprecated-declarations', '-c', str(ROOT/'src/KakaoCustomizer.m'),
                '-o', str(BUILD/'KakaoCustomizer.o')], check=True, env=ENV)
output = BUILD/'GinppaiKakaoCustomizer.dylib'
frameworks = ['UIKit', 'Foundation', 'CoreData', 'CoreGraphics', 'QuartzCore', 'CoreMedia', 'Photos', 'AVFoundation', 'ImageIO', 'UniformTypeIdentifiers']
subprocess.run(['xcrun', 'swiftc', *target, '-emit-library', str(BUILD/'MediaResolver.o'), str(BUILD/'KakaoCustomizer.o'),
                *[arg for f in frameworks for arg in ['-framework', f]], '-Xlinker', '-install_name', '-Xlinker',
                '@rpath/GinppaiKakaoCustomizer.dylib', '-o', str(output)], check=True, env=ENV)
subprocess.run(['codesign', '--force', '--sign', '-', '--identifier', 'com.nogadamachine.ginppai.kakaocustomizer', str(output)], check=True)
print(output)
