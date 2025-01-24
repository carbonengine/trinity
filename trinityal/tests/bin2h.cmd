1>2# : ^
'''
@echo off
python "%~f0" %*
exit /b
rem ^
'''
import argparse


def _hex(d):
    return '0x{:0>2x}'.format(d)


def _to_hex(s):
    l = ''
    for i in range(0, len(s), 1):
        if not isinstance(s[i], int):
            d = ord(s[i])
        else:
            d = s[i]
        if l:
            l += ', '
        if i and i % 16 == 0:
            l += '\n'
        l += _hex(d)
    return l


def convert_file(input, output):
    with open(input, 'rb') as f:
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
