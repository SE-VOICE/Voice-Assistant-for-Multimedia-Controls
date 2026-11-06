#!/usr/bin/env python3
"""Run existing host suites and measure portable C++ / optional Python coverage.

Requires macOS Xcode command-line tools, Homebrew openssl@3, Python coverage,
and Unity installed by `pio test -d firmware -e native`. Does not flash hardware.
Outputs evidence only; no production source or tests are modified.
"""
import argparse
import datetime
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    build = Path(tempfile.mkdtemp(prefix='vamc-coverage-'))
    commands = []

    def run(command, *, env=None, log=None):
        commands.append(command)
        result = subprocess.run(command, cwd=repo, env=env, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if result.stderr:
            with (output / 'tool-diagnostics.log').open('a') as diagnostics:
                diagnostics.write(str(command) + '\n' + result.stderr + '\n')
        if log:
            (output / log).write_text((result.stdout + result.stderr).replace(str(repo), '.'))
        if result.returncode:
            raise RuntimeError(f'Exit {result.returncode}: {command}\n{result.stdout}{result.stderr}')
        return result.stdout

    tools = {name: run(['xcrun', '--find', name]).strip()
             for name in ['clang', 'clang++', 'llvm-cov', 'llvm-profdata']}
    sdk = run(['xcrun', '--sdk', 'macosx', '--show-sdk-path']).strip()
    brew = shutil.which('brew') or '/opt/homebrew/bin/brew'
    openssl = Path(run([brew, '--prefix', 'openssl@3']).strip())
    unity = repo / 'firmware/.pio/libdeps/native/Unity/src'
    if not (unity / 'unity.c').exists():
        raise RuntimeError('Run PlatformIO native tests first to install Unity.')
    firmware = repo / 'firmware'
    library_sources = sorted((firmware / 'lib').rglob('*.cpp'))
    includes = [firmware / 'include', unity, openssl / 'include']
    includes += sorted(p for p in (firmware / 'lib').iterdir() if p.is_dir())
    flags = ['-std=c++11', '-O0', '-g', '-isysroot', sdk,
             '-fprofile-instr-generate', '-fcoverage-mapping']
    for include in includes:
        flags += ['-I', str(include)]
    unity_object = build / 'unity.o'
    run([tools['clang'], '-isysroot', sdk, '-c', str(unity / 'unity.c'),
         '-I', str(unity), '-o', str(unity_object)])
    objects = [unity_object]
    for index, source in enumerate(library_sources):
        obj = build / f'library-{index}.o'
        run([tools['clang++'], *flags, '-c', str(source), '-o', str(obj)])
        objects.append(obj)
    suites, binaries, profiles = [], [], []
    for test in sorted((firmware / 'test').glob('*/test_main.cpp')):
        name = test.parent.name
        binary = build / name
        test_object = build / f'{name}.o'
        # Do not instrument test fixtures/main: identical fixture symbol names
        # in separate executables can otherwise contaminate merged profiles.
        test_flags = [f for f in flags if f not in
                      ['-fprofile-instr-generate', '-fcoverage-mapping']]
        run([tools['clang++'], *test_flags, '-c', str(test), '-o', str(test_object)])
        run([tools['clang++'], '-isysroot', sdk, '-fprofile-instr-generate',
             str(test_object), *map(str, objects),
             '-L', str(openssl / 'lib'), '-lcrypto', '-o', str(binary)])
        profile = build / f'{name}.profraw'
        environment = dict(os.environ, LLVM_PROFILE_FILE=str(profile))
        transcript = run([str(binary)], env=environment, log=f'{name}.log')
        match = re.search(r'(\d+) Tests (\d+) Failures (\d+) Ignored', transcript)
        if not match:
            raise RuntimeError(f'Unity summary missing for {name}')
        tests, failures, ignored = map(int, match.groups())
        suites.append(dict(suite=name, tests=tests, failures=failures, ignored=ignored))
        binaries.append(binary)
        profiles.append(profile)
        print(f'{name}: {tests} tests, {failures} failures', flush=True)
    indexed = build / 'all.profdata'
    run([tools['llvm-profdata'], 'merge', '-sparse', *map(str, profiles), '-o', str(indexed)])
    # Every binary links exactly the same instrumented library objects. Their
    # merged profiles give the union across suites; one binary supplies the
    # mapping for those shared library objects without counting them 15 times.
    # Explicit denominator: all 12 portable implementation .cpp files. Inline
    # header-only logic (including reset_button.h) is tested but not measured.
    sources = library_sources
    coverage_args = [str(binaries[0]), f'-instr-profile={indexed}', *map(str, sources)]
    run([tools['llvm-cov'], 'report', *coverage_args], log='firmware-coverage-summary.txt')
    raw = run([tools['llvm-cov'], 'export', *coverage_args])
    data = json.loads(raw)
    (output / 'firmware-coverage.json').write_text(raw.replace(str(repo) + '/', ''))
    run([tools['llvm-cov'], 'show', '-show-branches=count', '-show-line-counts-or-regions',
         *coverage_args], log='firmware-annotated-coverage.txt')
    measured = {Path(f['filename']).resolve() for f in data['data'][0]['files']}
    inventory = {'compiled_cpp': [str(p.relative_to(repo)) for p in library_sources],
                 'mapped_files': [str(p.relative_to(repo)) for p in sorted(measured)],
                 'headers_excluded_from_percentage': [str(p.relative_to(repo))
                     for p in sorted((firmware / 'lib').rglob('*.h'))],
                 'excluded': ['firmware/src (ESP32-only adapters and entry point)',
                              'firmware/examples', 'firmware/test', 'Unity',
                              'OpenSSL/framework/toolchain internals',
                              'unimplemented wake-word/offline-model functionality']}
    (output / 'coverage-scope.json').write_text(json.dumps(inventory, indent=2) + '\n')

    prototype = 'companion/laptop_prototype'
    environment = dict(os.environ, COVERAGE_FILE=str(build / '.coverage'))
    run([sys.executable, '-m', 'coverage', 'run', '--branch', f'--source={prototype}',
         '--omit=*/tests/*', '-m', 'unittest', 'discover', '-s', f'{prototype}/tests', '-v'],
        env=environment, log='python-tests.log')
    run([sys.executable, '-m', 'coverage', 'report', '-m'], env=environment,
        log='python-coverage-summary.txt')
    run([sys.executable, '-m', 'coverage', 'json', '-o', str(output / 'python-coverage.json')],
        env=environment)
    run([sys.executable, '-m', 'coverage', 'html', '-d', str(output / 'python-html')],
        env=environment)
    versions = {name: run([path, '--version']).splitlines()[0] for name, path in tools.items()}
    versions['python'] = sys.version
    versions['coverage'] = run([sys.executable, '-m', 'coverage', '--version']).strip()
    manifest = dict(timestamp_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
                    source_commit=run(['git', 'rev-parse', 'HEAD']).strip(),
                    source_status=run(['git', 'status', '--short']), versions=versions,
                    suites=suites, totals=data['data'][0]['totals'],
                    commands=commands, temporary_build_directory=str(build))
    (output / 'run-manifest.json').write_text(json.dumps(manifest, indent=2).replace(str(repo), '.') + '\n')
    print(json.dumps(manifest['totals'], indent=2), flush=True)


if __name__ == '__main__':
    main()
