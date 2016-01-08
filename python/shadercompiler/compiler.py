import os
import subprocess

COMPILER_PATH = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'carbon', 'tools', 'shadercompiler',
                                             'shadercompiler.exe'))


class GLESExtensionOption(object):
    WARN = 'w'
    ENABLE = 'e'
    DISABLE = 'd'


CompilerError = subprocess.CalledProcessError


def compile_shader(effect_path, output_path, threads=None, warnings=True, defines=None, optimizations=3, clip_planes=1,
                   gles_emulate_samplers=False, avoid_flow_control=False,
                   gles_extension_option=GLESExtensionOption.WARN, gles_extensions=None):
    args = [COMPILER_PATH, '/single', '/O%s' % optimizations, '/clipPlanes', str(clip_planes),
            '/E%s' % gles_extension_option]
    if threads is not None:
        args.extend(('/threads', str(threads)))
    if not warnings:
        args.append('/no_warnings')
    for name, value in (defines or {}).iteritems():
        args.extend(('/define', name, str(value)))
    if gles_emulate_samplers:
        args.append('/GS')
    if avoid_flow_control:
        args.append('/Gfa')
    for name, value in gles_extensions or {}:
        args.append('/E%s%s' % (value, name))
    args.append(effect_path)
    args.append(output_path)
    return subprocess.check_output(args)
