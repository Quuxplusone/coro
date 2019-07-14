#!/usr/bin/env python

import argparse
import json
import os
import re
import requests
import sys


def preprocess_file(fname):
    result = ''
    try:
        with open(fname, 'r') as f:
            for line in f.readlines():
                m = re.match(r'#include <https://raw.githubusercontent.com/Quuxplusone/coro/master/(.*)>', line)
                if m is not None:
                    with open(os.path.dirname(fname) + '/../' + m.group(1)) as h:
                        result += h.read() + '\n'
                else:
                    result += line.rstrip() + '\n'
        return result
    except RuntimeError as e:
        raise RuntimeError(str(e) + ' in ' + fname)

def fetch_godbolt_include_option(library, version):
    info = requests.get(
        'https://godbolt.org/api/libraries/c++',
        headers={'Accept': 'application/json'},
    ).json()
    for row in info:
        if row['name'] == library:
            for v in row['versions']:
                if v['version'] == version:
                    assert len(v['path']) == 1
                    return '-I' + v['path'][0]

_include_options = None
def get_godbolt_include_options():
    global _include_options
    if _include_options is None:
        _include_options = [
            fetch_godbolt_include_option('range-v3', 'trunk'),
            fetch_godbolt_include_option('cppcoro', 'trunk'),
        ]
    return _include_options

def compile_on_godbolt(source_code, options):
    compiler_options = get_godbolt_include_options() + [
        '-O3',
        '-fcoroutines-ts',
        '-pthread',
        '-std=c++2a',
        '-stdlib=libc++',
    ]
    data = {
        'source': source_code,
        'options': {
            'compilerOptions': {},
            'userArguments': ' '.join(compiler_options),
            'filters': {
                'execute': options.run,
            },
        },
    }
    response = requests.post(
        'https://godbolt.org/api/compiler/clang_concepts/compile',
        json=data,
        headers={'Accept': 'application/json'},
    )
    result = response.json()
    compiler_return_code = result.get('code', 0)
    compiler_stderr = result.get('stderr', [])
    runtime_return_code = result.get('execResult', {}).get('code', 0)
    runtime_stdout = result.get('execResult', {}).get('stdout', [])
    runtime_stderr = result.get('execResult', {}).get('stderr', [])
    if compiler_return_code:
        print 'Compilation returned %d (not 0)!' % compiler_return_code
    if compiler_stderr:
        print '\n'.join(e['text'] for e in compiler_stderr)
    if runtime_return_code:
        print 'Linking and/or running returned %d (not 0)!' % runtime_return_code
    if runtime_stdout:
        print '\n'.join(e['text'] for e in runtime_stdout)
    if runtime_stderr:
        print '\n'.join(e['text'] for e in runtime_stderr)
    return compiler_return_code


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('fnames', nargs='*', metavar='FILE', help='File to compile on Godbolt Compiler Explorer')
    parser.add_argument('--run', action='store_true', help='Compile AND RUN on Godbolt Compiler Explorer')
    options = parser.parse_args()

    for fname in options.fnames:
        source_code = preprocess_file(os.path.abspath(fname))
        print '%s %s on Compiler Explorer...' % (('Running' if options.run else 'Compiling'), fname)
        status = compile_on_godbolt(source_code, options)
        if status:
            sys.exit(status)
