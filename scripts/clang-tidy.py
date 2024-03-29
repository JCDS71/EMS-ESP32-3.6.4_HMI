#!/usr/bin/env python3

# copied from esphome
# run from Linux using ./scripts/clang-forrmat.py

import argparse
import multiprocessing
import os
import queue
import shutil
import subprocess
import sys
import tempfile
import threading
import click
import pexpect

sys.path.append(os.path.dirname(__file__))
from helpers import shlex_quote, get_output, \
    build_all_include, temp_header_file, filter_changed, load_idedata, src_files

def clang_options(idedata):
    cmd = [
        # target 32-bit arch (this prevents size mismatch errors on a 64-bit host)
        '-m32',
        # disable built-in include directories from the host
        '-nostdinc',
        '-nostdinc++',
        # allow to condition code on the presence of clang-tidy
        '-DCLANG_TIDY'
    ]

    # copy compiler flags, except those clang doesn't understand.
    cmd.extend(flag for flag in idedata['cxx_flags'].split(' ')
               if flag not in ('-free', '-fipa-pta', '-mlongcalls', '-mtext-section-literals'))

    # defines
    cmd.extend(f'-D{define}' for define in idedata['defines'])

    # add include directories, using -isystem for dependencies to suppress their errors
    for directory in idedata['includes']['toolchain']:
        cmd.extend(['-isystem', directory])
    for directory in sorted(set(idedata['includes']['build'])):
        dependency = "framework-arduino" in directory or "/libdeps/" in directory
        cmd.extend(['-isystem' if dependency else '-I', directory])

    return cmd

def run_tidy(args, options, tmpdir, queue, lock, failed_files):
    while True:
        path = queue.get()
        invocation = ['clang-tidy']

        if tmpdir is not None:
            invocation.append('--export-fixes')
            # Get a temporary file. We immediately close the handle so clang-tidy can
            # overwrite it.
            (handle, name) = tempfile.mkstemp(suffix='.yaml', dir=tmpdir)
            os.close(handle)
            invocation.append(name)

        if args.quiet:
            invocation.append('-quiet')

        invocation.append(os.path.abspath(path))
        invocation.append('--')
        invocation.extend(options)
        invocation_s = ' '.join(shlex_quote(x) for x in invocation)

        # Use pexpect for a pseudy-TTY with colored output
        output, rc = pexpect.run(invocation_s, withexitstatus=True, encoding='utf-8',
                                 timeout=15 * 60)
        if rc != 0:
            with lock:
                print()
                print("\033[0;32m************* File \033[1;32m{}\033[0m".format(path))
                print(output)
                print()
                failed_files.append(path)
        queue.task_done()

def progress_bar_show(value):
    if value is None:
        return ''

def split_list(a, n):
    k, m = divmod(len(a), n)
    return [a[i * k + min(i, m):(i + 1) * k + min(i + 1, m)] for i in range(n)]

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-j', '--jobs', type=int,
                        default=multiprocessing.cpu_count(),
                        help='number of tidy instances to be run in parallel.')
    parser.add_argument('files', nargs='*', default=[],
                        help='files to be processed (regex on path)')
    parser.add_argument('--fix', action='store_true', help='apply fix-its')
    parser.add_argument('-q', '--quiet', action='store_false',
                        help='run clang-tidy in quiet mode')
    parser.add_argument('-c', '--changed', action='store_true',
                        help='only run on changed files')
    parser.add_argument('--split-num', type=int, help='split the files into X jobs.',
                        default=None)
    parser.add_argument('--split-at', type=int, help='which split is this? starts at 1',
                        default=None)
    parser.add_argument('--all-headers', action='store_true',
                        help='create a dummy file that checks all headers')
    args = parser.parse_args()

    try:
        get_output('clang-tidy', '-version')
    except:
        print("""
        Oops. It looks like clang-tidy is not installed.

        Please check you can run "clang-tidy -version" in your terminal and install
        clang-tidy (v11 or v12) if necessary.

        Note you can also upload your code as a pull request on GitHub and see the CI check
        output to apply clang-tidy.
        """)
        return 1

    idedata = load_idedata("clang-tidy")
    options = clang_options(idedata)

    files = []
    for path in src_files(['.cpp']):
        files.append(os.path.relpath(path, os.getcwd()))

    if args.files:
        # Match against files specified on command-line
        file_name_re = re.compile('|'.join(args.files))
        files = [p for p in files if file_name_re.search(p)]

    if args.changed:
        files = filter_changed(files)

    files.sort()

    if args.split_num:
        files = split_list(files, args.split_num)[args.split_at - 1]

    if args.all_headers and args.split_at in (None, 1):
        build_all_include()
        files.insert(0, temp_header_file)

    tmpdir = None
    if args.fix:
        tmpdir = tempfile.mkdtemp()

    failed_files = []
    try:
        task_queue = queue.Queue(args.jobs)
        lock = threading.Lock()
        for _ in range(args.jobs):
            t = threading.Thread(target=run_tidy,
                                 args=(args, options, tmpdir, task_queue, lock, failed_files))
            t.daemon = True
            t.start()

        # Fill the queue with files.
        with click.progressbar(files, width=30, file=sys.stderr,
                               item_show_func=progress_bar_show) as bar:
            for name in bar:
                task_queue.put(name)

        # Wait for all threads to be done.
        task_queue.join()

    except KeyboardInterrupt:
        print()
        print('Ctrl-C detected, goodbye.')
        if tmpdir:
            shutil.rmtree(tmpdir)
        os.kill(0, 9)

    if args.fix and failed_files:
        print('Applying fixes ...')
        try:
            subprocess.call(['clang-apply-replacements-12', tmpdir])
        except:
            print('Error applying fixes.\n', file=sys.stderr)
            raise

    sys.exit(len(failed_files))


if __name__ == '__main__':
    main()