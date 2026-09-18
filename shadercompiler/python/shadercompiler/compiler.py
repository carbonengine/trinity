# Copyright (c) 2023 CCP ehf.
import os
import subprocess
import sys
import json

BUILDER_DIR = os.path.dirname(os.path.abspath(__file__))


def _find_branch_dir(start):
    directory = start
    while True:
        if os.path.isfile(os.path.join(directory, 'carbon.json')):
            return directory
        parent = os.path.dirname(directory)
        if parent == directory:  # reached the filesystem root
            break
        directory = parent
    raise RuntimeError('could not locate carbon.json in any parent of %s' % start)


BRANCH_DIR = _find_branch_dir(BUILDER_DIR)

carbonjson = json.load(open(os.path.join(BRANCH_DIR, 'carbon.json')))

SHADER_COMPILER_PATH = os.path.join(BRANCH_DIR, "vendor", carbonjson['libraries']['carbon_trinity'], "shadercompiler",
                                    "bin")

if sys.platform == 'win32':
    COMPILER_PATH = os.path.abspath(os.path.join(SHADER_COMPILER_PATH, 'Windows', 'x64', 'v141', 'ShaderCompiler.exe'))
elif sys.platform == 'darwin':
    COMPILER_PATH = os.path.abspath(
        os.path.join(SHADER_COMPILER_PATH, 'macOS', 'universal', 'AppleClang', 'ShaderCompiler'))
else:
    raise RuntimeError('unsupported platform')


class GLESExtensionOption(object):
    WARN = 'w'
    ENABLE = 'e'
    DISABLE = 'd'


CompilerError = subprocess.CalledProcessError


def compile_shader(effect_path, output_path=None, platform=None, shader_model=None, threads=None, warnings=True,
                   defines=None, optimizations=3, avoid_flow_control=False):
    if output_path is None:
        if platform is None or shader_model is None:
            raise RuntimeError('cannot have both output_path and platform or shader_model None')
        output_path = paths.get_compiled_path(effect_path, shader_model, platform)
    try:
        os.makedirs(os.path.dirname(output_path))
    except OSError:
        pass
    args = [COMPILER_PATH, '/single', '/O%s' % optimizations]
    if threads is not None:
        args.extend(('/threads', str(threads)))
    if not warnings:
        args.append('/no_warnings')
    if platform is not None:
        args.extend(('/define', 'PLATFORM', str(platform)))
    if shader_model is not None:
        args.extend(('/define', 'SHADERMODEL', str(shader_model)))
    for name, value in (defines or {}).items():
        args.extend(('/define', name, str(value)))
    if avoid_flow_control:
        args.append('/Gfa')
    args.append(effect_path)
    args.append(output_path)
    print(args)
    return subprocess.check_output(args)
