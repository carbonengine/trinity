import argparse
import os
import subprocess
from P4 import P4, P4Exception
import _winreg
import sys

DEFAULT_CL_DESCRIPTION = 'Compiled effects'
COMPILED_FILE_PATHS = '//....sm_depth', '//....sm_hi', '//....sm_lo'


def initialize_p4(password=None):
    p4 = P4()
    p4.connect()
    if password is not None:
        p4.password = password
    p4.run_login()
    return p4


def create_changelist(p4, description):
    change = p4.fetch_change()
    change._description = description
    change._files = []
    save_change_results = p4.save_change(change)
    change_number = save_change_results[0].split(' ')[1]
    return change_number


def get_msbuild_path(version='4.0'):
    with _winreg.OpenKey(_winreg.HKEY_LOCAL_MACHINE, 'SOFTWARE\\Microsoft\\MSBuild\\ToolsVersions\\' + version) as k:
        build_dir, _ = _winreg.QueryValueEx(k, 'MSBuildToolsPath')
        return os.path.join(build_dir, 'msbuild.exe')


def build_project(project, target='build', msbuild_version='4.0', toolset=None):
    toolset_args = ['/p:PlatformToolset=%s' % toolset] if toolset else []
    subprocess.check_call([get_msbuild_path(msbuild_version), '/t:' + target, '/property:BuildCores=2',
                           '/p:Platform=Win32', '/verbosity:q', '/nologo'] + toolset_args + [project])


def run_p4(p4, command, *args):
    try:
        p4.run(command, *args)
    except P4Exception as e:
        msg = e.value
        if 'Warnings during command execution' in msg:
            return
        raise


def submit_with_description(p4, cl_description):
    cl = create_changelist(p4, cl_description)
    try:
        run_p4(p4, 'reopen', '-c', cl, *COMPILED_FILE_PATHS)
        try:
            run_p4(p4, 'submit', '-f', 'revertunchanged', '-c', cl)
        except P4Exception as e:
            if 'No files to submit' in e.value:
                run_p4(p4, 'change', '-d', cl)
                return
            raise
    except:
        run_p4(p4, 'revert', '-c', cl, '*.*')
        run_p4(p4, 'change', '-d', cl)
        raise


def clean_directory(dir_path):
    for root, dirs, files in os.walk(dir_path):
        for each in files:
            file_path = os.path.join(root, each)
            if os.access(file_path, os.W_OK):
                print 'Removing writable file %s' % file_path
                os.unlink(file_path)


def delete_writable_files(dir_pattern):
    import glob
    for each in glob.glob(dir_pattern):
        clean_directory(each)


def main():
    parser = argparse.ArgumentParser(description='Builds effect projects and submits results')
    parser.add_argument('--target', choices=('build', 'rebuild'), default='build', help='msbuild target action')
    parser.add_argument('--password', help='perforce password')
    parser.add_argument('--cl_desc', help='CL description')
    parser.add_argument('--clean', action='append', default=[], help='Directory to clean before building')
    parser.add_argument('--toolset', default=None, help='Override MSBuild platform toolset')
    parser.add_argument('project', nargs='+')

    args = parser.parse_args()
    p4 = initialize_p4(args.password)
    # noinspection PyBroadException
    try:
        run_p4(p4, 'revert', *COMPILED_FILE_PATHS)
    except:
        pass
    for each in args.clean:
        delete_writable_files(each)
    sys.stdout.flush()
    try:
        for each in args.project:
            build_project(each, toolset=args.toolset)
        submit_with_description(p4, args.cl_desc or DEFAULT_CL_DESCRIPTION)
    except:
        run_p4(p4, 'revert', '-c', 'default', *COMPILED_FILE_PATHS)
        raise


if __name__ == '__main__':
    main()