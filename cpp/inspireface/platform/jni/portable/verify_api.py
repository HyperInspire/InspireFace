#!/usr/bin/env python3
"""Verify public C declarations, generated Java methods, and exported JNI symbols agree."""
import argparse
import json
from pathlib import Path
import re
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--generated', type=Path, required=True)
    parser.add_argument('--library', type=Path, required=True)
    parser.add_argument('--nm', default='nm')
    args = parser.parse_args()
    header = (args.root / 'cpp/inspireface/c_api/inspireface.h').read_text()
    expected = set(re.findall(r'HYPER_CAPI_EXPORT\s+extern\s+\w+\s+(HF\w+)\s*\(', header))
    manifest = set(json.loads((args.generated / 'api-manifest.json').read_text()))
    java = (args.generated / 'java/com/insightface/sdk/inspireface/jni/Native.java').read_text()
    methods = set(re.findall(r'public static native \w+ (HF\w+)\(', java))
    test = (args.root / 'java/src/test/java/com/insightface/sdk/inspireface/jni/ContractTest.java').read_text()
    exercised = set(re.findall(r'\b(HF\w+)\(', test)) & expected
    symbols = subprocess.check_output([args.nm, '-g', str(args.library)], text=True)
    exported = set()
    prefix = 'Java_com_insightface_sdk_inspireface_jni_Native_'
    for line in symbols.splitlines():
        fields = line.split()
        if len(fields) >= 3 and fields[-2] in ('T', 'W'):
            symbol = fields[-1].lstrip('_')
            if symbol.startswith(prefix + 'HF'):
                exported.add(symbol[len(prefix):])
    for name, actual in [('manifest', manifest), ('Java declarations', methods),
                         ('native exports', exported), ('contract call sites', exercised)]:
        if actual != expected:
            raise RuntimeError(f'{name}: missing={sorted(expected-actual)}, unexpected={sorted(actual-expected)}')
    print(f'Portable JNI API parity: {len(expected)}/{len(expected)} declarations, exports, and contract call sites')


if __name__ == '__main__':
    main()
