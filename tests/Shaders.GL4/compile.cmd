1>2# : ^
'''
@echo off
echo normal 
echo batch code
echo Switch to python
python "%~f0" %*
exit /b
rem ^
'''
import os

def _to_hex(s):
    
    l = []
    for i in xrange(0, len(s), 4):
        d3 = ord(s[i + 3]) if i + 3 < len(s) else 0
        d2 = ord(s[i + 2]) if i + 2 < len(s) else 0
        d1 = ord(s[i + 1]) if i + 1 < len(s) else 0
        d = (d3 << 24) | (d2 << 16) | (d1 << 8) | (ord(s[i]) << 0)
        l.append(hex(d))
    l.append(hex(0))
    return ', '.join(l)

def _process_dir(path):
    for each in os.listdir(path):
        if each.lower().endswith(('.vsh', '.psh', '.csh')):
            with open(os.path.join(path, each)) as f:
                contents = f.read()
            contents = _to_hex(contents)
            with open(os.path.join(path, each[0:-1] + '.h'), 'w') as f:
                f.write(contents)
            with open(os.path.join(path, each[0:-1] + '.patched.h'), 'w') as f:
                f.write(contents)

_process_dir(os.path.dirname(__file__))