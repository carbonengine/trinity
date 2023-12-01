import os
import subprocess
import sys
from . import paths

_SHADER_COMPILER_DIR = os.path.join(os.path.dirname(__file__), '..', '..', 'carbon', 'tools', 'ShaderCompiler')
if sys.platform == 'win32':
    COMPILER_PATH = os.path.abspath(os.path.join(_SHADER_COMPILER_DIR, 'Windows', 'ShaderCompiler.exe'))
elif sys.platform == 'darwin':
    COMPILER_PATH = os.path.abspath(os.path.join(_SHADER_COMPILER_DIR, 'macOS', 'ShaderCompiler'))
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
    for name, value in (defines or {}).iteritems():
        args.extend(('/define', name, str(value)))
    if avoid_flow_control:
        args.append('/Gfa')
    args.append(effect_path)
    args.append(output_path)
    print args
    return subprocess.check_output(args)
