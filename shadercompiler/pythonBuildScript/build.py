# Copyright (c) 2023 CCP ehf.

#!/usr/bin/env python3

# This is a example script to show how shaders should be compiled on a project using carbon-trinty. Changes are recommended to be made to suite the project requirements

import argparse
import json
import logging
import multiprocessing
import os
import re
import subprocess
import sys
import threading
from pathlib import Path
import yaml

BUILDER_DIR = Path(__file__).resolve().parent
BRANCH_DIR = BUILDER_DIR.parents[2]
SHADER_COMPILER = (BUILDER_DIR / "Windows" / "ShaderCompiler.exe" if sys.platform == 'win32' else
                   BUILDER_DIR / "macOS" / "ShaderCompiler") if sys.platform in ('win32', 'darwin') else None
if SHADER_COMPILER is None:
    raise RuntimeError('Unsupported platform')

ARGS_PATH = BUILDER_DIR / 'shadercompiler.args'

SHADER_MODELS = {'lo': 3, 'hi': 4, 'depth': 5}
PLATFORMS = {'dx11': 2, 'dx12': 6, 'metal': 10}
SUPPORTED_PLATFORMS = ('metal',) if sys.platform == 'darwin' else ('dx11', 'dx12', 'metal') if sys.platform == 'win32' else ()


def expand_directories(path):
    path = Path(path)
    if path.is_dir():
        for f in path.rglob('*.fx'):
            yield f.resolve()
    elif path.suffix.lower() == '.fx':
        yield path.resolve()


def flatten_paths(paths):
    for each in paths:
        path = Path(each)
        if not path.exists():
            continue
        if path.suffix.lower() == '.code-workspace':
            with path.open() as f:
                workspace = json.load(f)
            for folder in workspace.get('folders', []):
                if 'path' in folder:
                    folder_path = path.parent / folder['path']
                    yield from expand_directories(folder_path)
        else:
            yield from expand_directories(each)


def get_output_file(path, sm, platform):
    path = Path(path)
    if path.suffix.lower() != '.fx':
        print(f'warning: "{path}" is not an .fx file')
        return str(path)
    out_name = path.with_suffix(f'.sm_{sm}').resolve().as_posix().lower()
    return out_name.replace('/effect/', f'/effect.{platform.lower()}/')


class WorkItem:
    def __init__(self, src, platform, sm, compiler, warnings, extra_args, staging):
        self.src = src
        self.platform = platform
        self.sm = sm
        self.compiler = compiler
        self.warnings = warnings
        self.compiled = get_output_file(src, sm, platform)
        self.dest = self.compiled
        self.permutations = 0
        self.extra_args = extra_args
        self.staging = staging
        if self.staging:
            self.dest = Path(self.staging, Path(self.compiled).relative_to(BRANCH_DIR))

    def get_command_line(self):
        args = [str(self.compiler), '/single', '/O3']
        if not self.warnings:
            args.append('/no_warnings')
        return args + ['/define', 'SHADERMODEL', str(SHADER_MODELS[self.sm]),
                       '/define', 'PLATFORM', str(PLATFORMS[self.platform]), str(self.src), str(self.dest)] + self.extra_args

    def get_mtime_line(self):
        return f'{self.src} {self.compiled} SHADERMODEL {SHADER_MODELS[self.sm]} PLATFORM {PLATFORMS[self.platform]}'

    def get_permutations(self):
        output = subprocess.check_output([self.compiler, '/single', '/permutations', self.src])
        try:
            desc = yaml.safe_load(output) or {}
            count = 1
            for each in desc.values():
                count *= len(each['options'])
            return max(count, 1)
        except Exception:
            return 1

    def __str__(self):
        return f'{self.src}, {self.platform}, {self.sm}'


def get_outputs(path, platforms, shader_models, compiler, warnings, extra_args, staging):
    for platform in platforms:
        for sm in shader_models:
            yield WorkItem(path, platform, sm, compiler, warnings, extra_args, staging)


class WorkItemProcessor:
    def __init__(self):
        self._pending = []
        self._permutations = {}
        self.max_thread_count = multiprocessing.cpu_count()
        self._thread_count = 0
        self._has_errors = False
        self._process_done = threading.Event()
        self._mutex = threading.Lock()
        self._print_mutex = threading.Lock()

    def process(self, items):
        self._pending = list(items)
        self._thread_count = 0
        self._has_errors = False
        self._process_done.clear()

        while self._pending:
            item = self._next()
            if item:
                with self._mutex:
                    self._thread_count += item.permutations
                threading.Thread(target=self._worker, args=(item,)).start()
            else:
                self._process_done.wait()
                self._process_done.clear()

        logging.debug('Waiting for the last shaders to compile')
        while True:
            with self._mutex:
                if self._thread_count <= 0:
                    break
            self._process_done.wait()
            self._process_done.clear()

        logging.debug('Done building all files')
        return not self._has_errors

    def _next(self):
        for i in range(len(self._pending)):
            item = self._pending[i]
            if item.permutations == 0:
                if item.src not in self._permutations:
                    self._permutations[item.src] = item.get_permutations()
                item.permutations = self._permutations[item.src]

            with self._mutex:
                if self._thread_count == 0 or item.permutations + self._thread_count <= self.max_thread_count:
                    return self._pending.pop(i)

        return None

    def _worker(self, item):
        logging.info(f'Building {item}')
        try:
            logging.debug(f'Spawning {" ".join(str(line) for line in item.get_command_line())}')
            os.makedirs(Path(item.dest).parent, exist_ok=True)
            p = subprocess.Popen(item.get_command_line(), stdout=subprocess.PIPE)
            stdout, _ = p.communicate()
            stdout = stdout.decode("utf-8")
            if p.returncode != 0:
                self._has_errors = True
                logging.error(f'Errors when building {item}', extra={'output': stdout or 'ShaderCompiler returned non-zero exit code without producing any output'})
                if not stdout:
                    stdout = f'{item.src}: error: ShaderCompiler returned non-zero exit code without producing any output'
            if stdout and p.returncode == 0:
                logging.warning(f'Warnings when building {item}', extra={'output': stdout})
        except Exception:
            self._has_errors = True
            raise
        finally:
            with self._mutex:
                self._thread_count -= item.permutations
            logging.debug(f'Finished building {item}')
            self._process_done.set()


def get_modified(compiler, work_items, extra_args):
    logging.info('Searching for out of date files')
    outputs = {x.compiled: x for x in work_items}
    p = subprocess.Popen([compiler, '/mtime', '/O3'] + extra_args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = p.communicate('\n'.join(x.get_mtime_line() for x in work_items).encode())
    for line in stdout.decode().split('\n'):
        line = line.strip()
        item = outputs.get(line, None)
        if item:
            yield item


def get_extra_args():
    try:
        with open(ARGS_PATH) as f:
            args = json.load(f)
    except (IOError, OSError):
        return []
    if 'metal' in args:
        return ['/metal', str((Path(ARGS_PATH).parent / args['metal']).resolve())]
    return []


def build(paths, vcs, incremental, platforms=SUPPORTED_PLATFORMS, shader_models=tuple(SHADER_MODELS),
          shader_compiler=SHADER_COMPILER, warnings=True, staging=''):
    files = set(flatten_paths(paths))
    logging.debug(f'Discovered {len(files)} files to build')
    if not files:
        return True

    extra_args = get_extra_args()
    if staging:
        os.makedirs(staging, exist_ok=True)

    work_items = []
    for each in files:
        work_items.extend(get_outputs(each, platforms or SUPPORTED_PLATFORMS, shader_models or SHADER_MODELS,
                                      shader_compiler, warnings, extra_args, staging))

    if incremental:
        work_items = list(get_modified(shader_compiler, work_items, extra_args))
        if not work_items:
            logging.info('All files are up to date')
            return True

    logging.debug(f'Starting build for {len(work_items)} outputs')
    outputs = [x.dest for x in work_items]
    vcs.pre_build(outputs)
    success = WorkItemProcessor().process(work_items)
    vcs.submit(outputs, success)
    return success


class Perforce:
    def __init__(self, action, cl_desc):
        self._action = action
        self._cl_desc = cl_desc
        self._cl = None

    def pre_build(self, paths):
        if self._action == 'none':
            return
        logging.info(f'Checking out {len(paths)} files in Perforce')
        self._p4('edit', paths)

        logging.debug('Creating new change list')
        self._cl = self._create_cl()
        logging.info(f'Created CL {self._cl}')
        logging.debug(f'Moving files to CL {self._cl}')
        self._p4('reopen', ['-c', self._cl] + paths)

    def submit(self, paths, success):
        if self._action == 'none':
            return
        if self._action == 'revert':
            self._revert(paths)
        else:
            try:
                action_verb = 'Submitting' if self._action == 'submit' else 'Saving'
                logging.info(f'{action_verb} {len(paths)} files to Perforce')
                logging.debug('Adding new files to Perforce')
                self._p4('add', ['-tbinary+m'] + paths)
                logging.debug(f'Moving new files to CL {self._cl}')
                self._p4('reopen', ['-c', self._cl] + paths)
                if self._action == 'submit':
                    if not success:
                        logging.debug(f'Reverting unchanged files in CL {self._cl}')
                        self._p4('revert', ['-a', '-c', self._cl])
                    logging.debug(f'Submitting CL {self._cl}')
                    self._p4('submit', ['-c', self._cl])
            except subprocess.CalledProcessError:
                logging.exception(f'Failed to {self._action} compiled files')
                self._revert(paths)

    def _revert(self, paths):
        logging.info(f'Reverting {len(paths)} files in Perforce')
        self._p4('revert', paths)
        logging.debug(f'Removing CL {self._cl}')
        self._p4('change', ['-d', self._cl])

    def _create_cl(self):
        desc = f"""
Change: new

Description:
\t{self._cl_desc}
"""
        p = subprocess.Popen(['p4', 'change', '-i'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = p.communicate(desc.encode())
        if p.returncode != 0:
            raise subprocess.CalledProcessError(p.returncode, 'p4', stderr.decode())
        match = re.match(r'.*Change (\d+) created.*', stdout.decode(), re.IGNORECASE | re.MULTILINE)
        if not match:
            raise subprocess.CalledProcessError(p.returncode, 'p4', stderr.decode())
        return match.group(1)

    def _p4(self, action, paths):
        args = ['p4', '-b', str(len(paths) + 1), '-x', '-', action]
        logging.debug(f'Running {" ".join(args)}')
        p = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = p.communicate('\n'.join(paths).encode())
        logging.debug(f'p4 output: stdout: ({stdout.decode()}), stderr: ({stderr.decode()})')
        if p.returncode != 0:
            logging.error(f'Error when running p4 {action}: {stderr.decode()}')
            raise subprocess.CalledProcessError(p.returncode, 'p4', stderr.decode())


def str2bool(v):
    if isinstance(v, bool):
        return v
    if v.lower() in ('yes', 'true', 't', 'y', '1'):
        return True
    elif v.lower() in ('no', 'false', 'f', 'n', '0'):
        return False
    else:
        raise argparse.ArgumentTypeError('Boolean value expected.')


_quote = {"'": "|'", "|": "||", "\n": "|n", "\r": "|r", '[': '|[', ']': '|]'}


def escape_value(value):
    return "".join(_quote.get(x, x) for x in value.strip().replace('\r', ''))


class OutputFormatter(logging.Formatter):
    def format(self, record):
        string = super().format(record)
        if hasattr(record, 'output'):
            string += f'\n{record.output}'
        return string


class TeamCityFormatter(logging.Formatter):
    def __init__(self):
        super().__init__('##teamcity[message text=\'%(message)s\' status=\'%(levelname)s\']')

    def format(self, record):
        record.message = record.getMessage()
        if record.exc_info:
            if not record.exc_text:
                record.exc_text = self.formatException(record.exc_info)
        if record.exc_text:
            try:
                _ = '' + record.exc_text
            except UnicodeError:
                record.exc_text = record.exc_text.decode(sys.getfilesystemencoding(), 'replace')

        tc_level = 'NORMAL'
        if record.levelno >= logging.ERROR:
            tc_level = 'ERROR'
        elif record.levelno >= logging.WARNING:
            tc_level = 'WARNING'
        output = getattr(record, 'output', '')
        message = '\n'.join((record.message, record.exc_text or '', output)).strip()

        s = '\n'.join((self._fmt % {'message': escape_value(x), 'levelname': tc_level}) for x in message.split('\n'))
        return s


def main(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument('--perforce', choices=('submit', 'save', 'revert', 'none'), default='save',
                        help='Perforce action for compiled files: submit CL, leave CL, revert CL, no perforce interaction')
    parser.add_argument('--rebuild', action='store_true', help='Rebuild shaders instead of doing incremental build')
    parser.add_argument('--cl', default='Compiled shaders', help='Perforce CL description')
    parser.add_argument('--log', choices=('critical', 'error', 'warning', 'info', 'debug'), default='error',
                        help='Logging verbosity')
    parser.add_argument('--compiler', default=SHADER_COMPILER, help='Override shader compiler executable location')
    parser.add_argument('--platform', action='append', default=[], help='Override platforms to build')
    parser.add_argument('--model', nargs='*', default=[], help='Override shader models to build')
    parser.add_argument('--warnings', type=str2bool, default=True, help='Emit compiler warnings (true by default)')
    parser.add_argument('--staging', default='', help='Path to the output staging directory to place compiled files (if not specified, files are modified in-place)')
    parser.add_argument('--teamcity', action='store_true', default=False, help='Use TeamCity formatter for logs')
    parser.add_argument('path', nargs='+', help='Path to .fx file, folder or VS Code workspace')

    args = parser.parse_args(argv)
    logging.basicConfig(level=args.log.upper(), stream=sys.stdout)
    if args.teamcity:
        logging.root.handlers[0].setFormatter(TeamCityFormatter())
    else:
        logging.root.handlers[0].setFormatter(OutputFormatter(logging.BASIC_FORMAT))

    return build(args.path, Perforce(args.perforce, args.cl), incremental=not args.rebuild, platforms=args.platform,
                 shader_models=args.model, shader_compiler=args.compiler, warnings=args.warnings, staging=args.staging)


if __name__ == '__main__':
    if not main(sys.argv[1:]):
        sys.exit(1)
