from setuptools import setup
import os
import sys

if sys.platform == 'win32':
    import py2exe
    extra_options = {
        'setup_requires': ['py2exe'],
        'options': {
            'py2exe': {
                'dll_excludes': ['MSVCP90.dll', 'HID.DLL', 'w9xpopen.exe'],
                'dist_dir': os.path.join('..', 'fxinfo', 'Windows')
            }
        },
        'zipfile': None,
        'windows': [
            {
                'script': 'main.py',
                'dest_base': 'fxinfo'
            }
        ]
    }
elif sys.platform == 'darwin':
    extra_options = {
        'setup_requires': ['py2app'],
        'app': ['main.py'],
        'options': {
            'py2app': {
                'argv_emulation': True,
                'dist_dir': os.path.join('..', 'fxinfo', 'macOS')
            }
        }
    }
    PLATFROM_DIR = 'macOS'
else:
    raise RuntimeError('unsupported platform')

setup(
    name='fxinfo',
    **extra_options
)