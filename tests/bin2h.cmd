1>2# : ^
'''
@echo off
python "%~f0" %*
exit /b
rem ^
'''
import argparse


def _to_hex(s):
    l = []
    for i in xrange(0, len(s), 4):
        d3 = ord(s[i + 3]) if i + 3 < len(s) else 0
        d2 = ord(s[i + 2]) if i + 2 < len(s) else 0
        d1 = ord(s[i + 1]) if i + 1 < len(s) else 0
        d = (d3 << 24) | (d2 << 16) | (d1 << 8) | (ord(s[i]) << 0)
        l.append(hex(d))
    return ', '.join(l)


def convert_file(input, output):
    with open(input) as f:
        contents = f.read()
    contents = _to_hex(contents)
    with open(output, 'w') as f:
        f.write(contents)

if __name__ == '__main__':        
    parser = argparse.ArgumentParser(description='Convert binary file to C++ include')
    parser.add_argument('input', help='path to input file')
    parser.add_argument('output', help='path to output file')
    args = parser.parse_args()
    convert_file(args.input, args.output)
