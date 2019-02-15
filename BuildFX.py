import subprocess
import os
import sys
import multiprocessing
import tempfile
import threading
from P4 import P4, P4Exception
import Queue

SHADER_COMPILER = os.path.dirname(__file__) + "\\ShaderCompiler.exe"
SHADER_MODELS = {'lo': 3, 'hi': 4, 'depth': 5}
PLATFORMS = {'dx9': 1, 'dx11': 2, 'gles2': 3}
OPTION_PREFIX = '/'
ARG_FILE_PREFIX = '@'


def print_usage():
    print "Usage: BuildFX [<global_options>] [<path1_options>] path1 [[<path2_options>] path2...]"
    print "Global Options:"
    print "  %sincremental  Only build out of date files" % OPTION_PREFIX
    print "  %sskipped      Print files skipped when building with %sincremental" % (OPTION_PREFIX, OPTION_PREFIX)
    print "  %scores=n      Specify number of cores to use" % OPTION_PREFIX
    print "  %soutput=dir   Override output directory (used for testing)"
    print "Path Options:"
    print "  %ssm=sm        Compile shader model (sm is lo, hi or depth)" % OPTION_PREFIX
    print "  %splatform=p   Compile for platform (p is dx9, dx11 or gles2)" % OPTION_PREFIX
    print "  %soptimize=o   Optimization level 0..3.  3 is default" % OPTION_PREFIX
    print "If no shader model is specified, all supported shader models are built"


def get_output_file(path, sm, platform):
    if not path.lower().endswith('.fx'):
        print 'warning: \'%s\' is not an .fx file' % path
        return path

    out_name = os.path.abspath(path[:-2] + 'sm_' + sm).lower()
    return out_name.replace('\\effect\\', '\\effect.' + platform.lower() + '\\')


def get_modified_files(mtime_queue, mtime_commands, queue, out_files, print_skipped):
    std_in = tempfile.TemporaryFile('w+')
    std_in.write('\n'.join(mtime_queue))
    std_in.seek(0)

    cmd_path = '%s /mtime' % SHADER_COMPILER
    sp = subprocess.Popen(cmd_path, stdin=std_in, stdout=subprocess.PIPE, universal_newlines=True)
    for i in iter(sp.stdout.readline, b''):
        i = i.strip()
        if i in mtime_commands:
            out_files.append(i)
            queue.put(mtime_commands[i])
            del mtime_commands[i]
    sp.communicate()
    if print_skipped:
        for i in mtime_commands:
            cmd_line, path, platform, sm, out_f = mtime_commands[i]
            print '%s for %s, %s' % (os.path.basename(path), platform, sm)
            print "Output is up to date\n"


def get_incremental_command(compile_command):
    return ('%s %s SHADERMODEL %s PLATFORM %s' %
            (compile_command[1], compile_command[4], SHADER_MODELS[compile_command[3]],
             PLATFORMS[compile_command[2]]))


def prepare_compile_batch(path, sm, platform, optimization=3, check_mode_dir=None):
    out_name = get_output_file(path, sm, platform)

    if check_mode_dir:
        e = out_name.index('\\effect.')
        out_name = os.path.join(check_mode_dir, out_name[e + 1:])

    out_dir = os.path.dirname(out_name)
    if not os.path.exists(out_dir):
        os.makedirs(out_dir)
    
    cmd_line = ("%s /novalidate /single /define SHADERMODEL %s /define PLATFORM %s /O%s %s %s" %
                (SHADER_COMPILER, SHADER_MODELS[sm], PLATFORMS[platform], optimization, path, out_name))

    compile_command = (cmd_line, path, platform, sm, out_name)
    return compile_command


GLOBAL_OPTIONS = {'incremental': lambda x: x is None, 'skipped': lambda x: x is None, 'cores': lambda x: int(x) > 0,
                  'output': lambda x: x is not None}
FILE_OPTIONS = {'sm': lambda x: x in ('lo', 'hi', 'depth'), 'platform': lambda x: x in ('dx9', 'dx11', 'gles2'),
                'optimize': lambda x: 0 <= int(x) <= 3}


def _get_option_name_value(option):
    option = option[len(OPTION_PREFIX):]
    try:
        index = option.index('=')
    except ValueError:
        return option, None
    return option[:index], option[index + 1:]


def _normalize_file_options(options):
    options['sm'] = list(set(options.get('sm', ['lo', 'hi', 'depth'])))
    options['platform'] = list(set(options.get('platform', ['dx9', 'dx11', 'gles2'])))
    options['optimize'] = int(options.get('optimize', ['3'])[-1])
    return options


def get_fx_files(dir_path):
    for root, dirs, files in os.walk(dir_path):
        for f in files:
            if f.lower().endswith('.fx'):
                yield os.path.join(root, f)


def parse_arguments(arguments):
    global_options = {}
    file_options = {}
    files = []
    if not arguments:
        return {}, []
    for each in arguments:
        if each.startswith(ARG_FILE_PREFIX):
            arg_file = each[len(ARG_FILE_PREFIX):]
            try:
                f = open(arg_file)
            except OSError:
                raise ValueError('could not open arguments file %s' % arg_file)
            file_arguments = f.read().split()
            if file_arguments is not None:
                g, f = parse_arguments(file_arguments)
                global_options.update(g)
                files.extend(f)
        elif each.startswith(OPTION_PREFIX):
            name, value = _get_option_name_value(each)
            if name in GLOBAL_OPTIONS:
                try:
                    # noinspection PyCallingNonCallable
                    if not GLOBAL_OPTIONS[name](value):
                        raise ValueError('invalid value for option %s (%s)' % (name, each))
                except:
                    raise ValueError('invalid value for option %s (%s)' % (name, each))
                global_options[name] = value
            elif name in FILE_OPTIONS:
                try:
                    # noinspection PyCallingNonCallable
                    if not FILE_OPTIONS[name](value):
                        raise ValueError('invalid value for option %s (%s)' % (name, each))
                except:
                    raise ValueError('invalid value for option %s (%s)' % (name, each))
                file_options.setdefault(name, []).append(value)
            else:
                raise ValueError('invalid option %s' % each)
        else:
            file_options = _normalize_file_options(file_options)
            if os.path.isdir(each):
                for f in get_fx_files(each):
                    files.append((f, file_options))
            else:
                files.append((each, file_options))
            file_options = {}
    return global_options, files


def _worker(queue, has_errors):
    while True:
        cmd_line, path, platform, sm, out_f = queue.get()
        try:
            p = subprocess.Popen(cmd_line, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
            std_out, std_err = p.communicate()
            output = '%s for %s, %s' % (os.path.split(path)[1], platform, sm)
            if std_out:
                output += '\n' + std_out
            if std_err:
                output += '\n' + std_err
            print output
            sys.stdout.flush()
            if p.returncode != 0:
                has_errors[0] = True
        finally:
            queue.task_done()


def _ensure_files_writable(files):
    for each in files:
        if os.path.exists(each) and not os.access(each, os.W_OK):
            raise RuntimeError('error: could not open file %s for writing' % each)


def check_files_into_perforce(files):
    p4 = P4()
    try:
        p4.connect()
    except P4Exception:
        print "warning: perforce connection failed, working in local mode"
        return
    try:
        p4.run_edit(files)
    except P4Exception:
        _ensure_files_writable(files)
        try:
            p4.run_add("-tbinary+m", files)
        except P4Exception as e:
            if 'warning' in e.value.lower():
                return
            raise


def fill_work_queue(files, global_options, check_mode_dir):
    queue = Queue.Queue()
    out_files = []
    mtime_queue = []
    mtime_commands = {}
    for each, options in files:
        for pl in options['platform']:
            for smodel in options['sm']:
                compile_command = prepare_compile_batch(each, smodel, pl, options['optimize'], check_mode_dir)
                if 'incremental' in global_options:
                    mtime_command = get_incremental_command(compile_command)
                    mtime_queue.append(mtime_command)
                    mtime_commands[compile_command[4]] = compile_command
                else:
                    queue.put(compile_command)
                    out_files.append(compile_command[4])
    if 'incremental' in global_options:
        get_modified_files(mtime_queue, mtime_commands, queue, out_files, 'skipped' in global_options)
    return out_files, queue


def main():
    global_options, files = parse_arguments(sys.argv[1:])
    if not files:
        print_usage()
        return 1
    has_errors = [False]
    out_files, queue = fill_work_queue(files, global_options, global_options.get('output'))
    if not out_files:
        return 0
    if out_files and ('output' not in global_options):
        check_files_into_perforce(out_files)
    for i in range(int(global_options['cores']) if 'cores' in global_options else multiprocessing.cpu_count()):
        w = threading.Thread(target=_worker, args=(queue, has_errors))
        w.daemon = True
        w.start()
    queue.join()
    if has_errors[0]:
        return 2
    return 0


def _guarded_main():
    # noinspection PyBroadException
    try:
        return main()
    except BaseException as e:
        print "error: %s" % e
        return 1

if __name__ == '__main__':
    sys.exit(_guarded_main())
