#!/usr/bin/python
import argparse
import json
import logging
import multiprocessing
import os
import re
import subprocess
import sys
import threading
import yaml


if sys.platform == 'win32':
    SHADER_COMPILER = os.path.join(os.path.dirname(__file__), "Windows", "ShaderCompiler.exe")
elif sys.platform == 'darwin':
    SHADER_COMPILER = os.path.join(os.path.dirname(__file__), "macOS", "ShaderCompiler")
else:
    raise RuntimeError('unsupported platform')


SHADER_MODELS = {'lo': 3, 'hi': 4, 'depth': 5}
PLATFORMS = {'dx11': 2, 'dx12': 6, 'metal': 10}
if sys.platform == 'darwin':
    SUPPORTED_PLATFORMS = ('metal',)
elif sys.platform == 'win32':
    SUPPORTED_PLATFORMS = 'dx11', 'dx12', 'metal'
else:
    SUPPORTED_PLATFORMS = ()


def _expand_directories(path):
    if os.path.isdir(path):
        for root, dirs, files in os.walk(path):
            for f in files:
                if f.lower().endswith('.fx'):
                    yield os.path.join(root, f)
    elif path.lower().endswith('.fx'):
        yield os.path.abspath(path)


def flatten_paths(paths):
    for each in paths:
        if not os.path.exists(each):
            continue
        if each.lower().endswith('.code-workspace'):
            with open(each) as f:
                workspace = json.load(f)
            for folder in workspace.get('folders', []):
                if 'path' in folder:
                    for x in _expand_directories(os.path.abspath(os.path.join(os.path.dirname(each), folder['path']))):
                        yield x
        else:
            for x in _expand_directories(each):
                yield x


def get_output_file(path, sm, platform):
    if not path.lower().endswith('.fx'):
        print 'warning: \'%s\' is not an .fx file' % path
        return path

    out_name = os.path.abspath(path[:-2] + 'sm_' + sm).lower()
    return out_name.replace(os.path.sep + 'effect' + os.path.sep,
                            os.path.sep + 'effect.' + platform.lower() + os.path.sep)


class WorkItem(object):
    def __init__(self, src, platform, sm, compiler, warnings):
        self.src = src
        self.platform = platform
        self.sm = sm
        self.compiler = compiler
        self.warnings = warnings
        self.dest = get_output_file(src, sm, platform)
        self.permutations = 0

    def get_command_line(self):
        args = [self.compiler, '/single', '/O3']
        if not self.warnings:
            args.append('/no_warnings')
        return args + ['/define', 'SHADERMODEL', str(SHADER_MODELS[self.sm]),
                       '/define', 'PLATFORM', str(PLATFORMS[self.platform]), self.src, self.dest]

    def get_mtime_line(self):
        return '%s %s SHADERMODEL %s PLATFORM %s' % (self.src, self.dest, SHADER_MODELS[self.sm],
                                                     PLATFORMS[self.platform])

    def get_permutations(self):
        output = subprocess.check_output([self.compiler, '/single', '/permutations', self.src])
        try:
            desc = yaml.safe_load(output) or {}
            count = 1
            for each in desc.values():
                count *= len(each['options'])
            return max(count, 1)
        except:
            return 1

    def __str__(self):
        return '%s, %s, %s' % (self.src, self.platform, self.sm)


def get_outputs(path, platforms, shader_models, compiler, warnings):
    for platform in platforms:
        for sm in shader_models:
            yield WorkItem(path, platform, sm, compiler, warnings)


class WorkItemProcessor(object):
    def __init__(self):
        self._pending = []
        """:type: list[WorkItem]"""
        self._permutations = {}
        """:type: dict[str, int]"""
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
                t = threading.Thread(target=self._worker, args=(item,))
                t.start()
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
                tc = self._thread_count
            if tc == 0 or item.permutations + tc <= self.max_thread_count:
                return self._pending.pop(i)
        return None

    def _worker(self, item):
        logging.info('Building %s', item)
        try:
            #logging.debug('Spawning %s', ' '.join(item.get_command_line()))
            try:
                os.makedirs(os.path.dirname(item.dest))
            except OSError:
                pass
            p = subprocess.Popen(item.get_command_line(), stdout=subprocess.PIPE)
            stdout, _ = p.communicate()
            if p.returncode != 0:
                self._has_errors = True
                logging.info('Errors when building %s', item)
                if not stdout:
                    stdout = '%s: error: ShaderCompiler returned non-zero exit code without producing any output' % item.src
            if stdout:
                with self._print_mutex:
                    print str(item)
                    print stdout
                    sys.stdout.flush()
        except BaseException:
            self._has_errors = True
            raise
        finally:
            with self._mutex:
                self._thread_count -= item.permutations
            logging.debug('Finished building %s', item)
            self._process_done.set()


def _get_modified(compiler, work_items):
    logging.info('Searching for out of date files')
    outputs = {x.dest: x for x in work_items}
    p = subprocess.Popen([compiler, '/mtime'], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    stdout, stderr = p.communicate('\n'.join(x.get_mtime_line() for x in work_items))
    for line in stdout.split('\n'):
        line = line.strip()
        item = outputs.get(line, None)
        if item:
            yield item


def build(paths, vcs, incremental, platforms=SUPPORTED_PLATFORMS, shader_models=tuple(SHADER_MODELS),
          shader_compiler=SHADER_COMPILER, warnings=True):
    files = set(flatten_paths(paths))
    logging.debug('Discovered %s files to build', len(files))
    if not files:
        return True

    work_items = []
    for each in files:
        work_items.extend(get_outputs(each, platforms or SUPPORTED_PLATFORMS, shader_models or SHADER_MODELS,
                                      shader_compiler, warnings))

    if incremental:
        work_items = list(_get_modified(shader_compiler, work_items))
        if not work_items:
            logging.info('All files are up to date')
            return True

    logging.debug('Starting build for %s outputs', len(work_items))
    outputs = [x.dest for x in work_items]
    vcs.pre_build(outputs)
    success = WorkItemProcessor().process(work_items)
    vcs.submit(outputs, success)
    return success


class Perforce(object):
    def __init__(self, action, cl_desc):
        self._action = action
        self._cl_desc = cl_desc
        self._cl = None

    def pre_build(self, paths):
        if self._action == 'none':
            return
        logging.info('Checking out %s files in perforce', len(paths))
        self._p4('edit', paths)

        logging.debug('Creating new change list')
        self._cl = self._create_cl()
        logging.info('Created CL %s', self._cl)
        logging.debug('Moving files to CL %s', self._cl)
        self._p4('reopen', ['-c', self._cl] + paths)

    def submit(self, paths, success):
        if self._action == 'none':
            return
        if self._action == 'revert':
            self._revert(paths)
        else:
            try:
                if self._action == 'submit':
                    logging.info('Submitting %s files to perforce', len(paths))
                else:
                    logging.info('Saving %s files to a perforce change list', len(paths))
                logging.debug('Adding new files to perforce')
                self._p4('add', ['-tbinary+m'] + paths)
                logging.debug('Moving new files to CL %s', self._cl)
                self._p4('reopen', ['-c', self._cl] + paths)
                if self._action == 'submit':
                    if not success:
                        logging.debug('Reverting unchanged files in CL %s', self._cl)
                        self._p4('revert', ['-a', '-c', self._cl])
                    logging.debug('Submitting CL %s', self._cl)
                    self._p4('submit', ['-c', self._cl])
            except subprocess.CalledProcessError:
                logging.exception('Failed to %s compiled files', self._action)
                self._revert(paths)

    def _revert(self, paths):
        logging.info('Reverting %s files in perforce', len(paths))
        self._p4('revert', paths)
        logging.debug('Removing CL %s', self._cl)
        self._p4('change', ['-d', self._cl])

    def _create_cl(self):
        desc = """
Change: new

Description:
\t%s
""" % self._cl_desc
        p = subprocess.Popen(['p4', 'change', '-i'], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        stdout, stderr = p.communicate(desc)
        if p.returncode != 0:
            raise subprocess.CalledProcessError(p.returncode, 'p4', stderr)
        match = re.match(r'.*Change (\d+) created.*', stdout, re.IGNORECASE | re.MULTILINE)
        if not match:
            raise subprocess.CalledProcessError(p.returncode, 'p4', stderr)
        return match.group(1)

    def _p4(self, action, paths):
        args = ['p4', '-b', str(len(paths) + 1), '-x', '-', action]
        logging.debug('Running %s', ' '.join(args))
        p = subprocess.Popen(args, stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = p.communicate('\n'.join(paths))
        logging.debug('p4 output: stdout: (%s), stderr: (%s)', stdout, stderr)
        if p.returncode != 0:
            logging.error('Error when running p4 %s: %s', action, stderr)
            raise subprocess.CalledProcessError(p.returncode, 'p4', stderr)


def str2bool(v):
    if isinstance(v, bool):
        return v
    if v.lower() in ('yes', 'true', 't', 'y', '1'):
        return True
    elif v.lower() in ('no', 'false', 'f', 'n', '0'):
        return False
    else:
        raise argparse.ArgumentTypeError('Boolean value expected.')


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
    parser.add_argument('path', nargs='+', help='Path to .fx file, folder or VS Code workspace')

    args = parser.parse_args(argv)
    logging.basicConfig(level=args.log.upper())
    return build(args.path, Perforce(args.perforce, args.cl), incremental=not args.rebuild, platforms=args.platform,
                 shader_models=args.model, shader_compiler=args.compiler, warnings=args.warnings)


if __name__ == '__main__':
    if not main(sys.argv[1:]):
        sys.exit(1)
