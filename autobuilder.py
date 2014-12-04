import argparse
import os
import subprocess
from P4 import P4
import _winreg

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


def build_project(project, target='build', msbuild_version='4.0'):
    subprocess.check_call([get_msbuild_path(msbuild_version), '/t:' + target, '/property:BuildCores=2',
                           '/p:Platform=Win32', '/verbosity:m', '/nologo', project])


def submit_with_description(p4, cl_description):
    cl = create_changelist(p4, cl_description)
    try:
        p4.run_reopen('-c', cl, *COMPILED_FILE_PATHS)
        p4.run_submit('-f', 'revertunchanged', '-c', cl)
    except:
        p4.run_revert('-c', cl)
        p4.run_change('-d', cl)
        raise


def main():
    parser = argparse.ArgumentParser(description='Builds effect projects and submits results')
    parser.add_argument('--target', choices=('build', 'rebuild'), default='build', help='msbuild target action')
    parser.add_argument('--password', help='perforce password')
    parser.add_argument('--cl_desc', help='CL description')
    parser.add_argument('project', nargs='+')

    args = parser.parse_args()
    p4 = initialize_p4(args.password)
    try:
        for each in args.project:
            build_project(each)
        submit_with_description(p4, args.cl_desc or DEFAULT_CL_DESCRIPTION)
    except:
        p4.run_revert('-c', 'default', *COMPILED_FILE_PATHS)
        raise


if __name__ == '__main__':
    main()