import logging
import os
import shutil
import subprocess
import tempfile
import threading

RGA_PATH_KEY = 'RgaPath'
RGA_TARGETS_KEY = 'RgaTargets'

_RGA_PATH = r"c:\Program Files\GPUOpen\Radeon GPU Analyzer\rga.exe"

_log = logging.getLogger('rga')


def _TableToTree(lines):
    result = {}
    statNames = lines[0].strip().split(',')[1:]
    for each in lines[1:]:
        tokens = each.strip().split(',')
        result[tokens[0]] = dict(zip(statNames, tokens[1:]))
    return result


def _GetRgaStats(stageInfo, onComplete, onFail, rgaPath, targetPlatforms):
    dirPath = tempfile.mkdtemp()
    try:
        inPath = os.path.join(dirPath, 'sh.txt')
        with open(inPath, 'w') as f:
            f.write(stageInfo['original']['source'])
        args = [rgaPath, '-s', 'hlsl', '-f', stageInfo['original']['entryPoint'], '-p', stageInfo['profile'],
                '--DXFlags', str((1 << 12) | (1 << 15))]
        for each in targetPlatforms:
            args.extend(['-c', each])
        args.extend(['--set-adapter', '0', '-a', os.path.join(dirPath, 'rga.txt'), inPath])
        _log.debug('Running RGA with arguments: %s', ' '.join(args))

        startupinfo = subprocess.STARTUPINFO()
        startupinfo.dwFlags = subprocess.CREATE_NEW_CONSOLE | 1
        startupinfo.wShowWindow = 0

        output = subprocess.check_output(args, stderr=subprocess.STDOUT, startupinfo=startupinfo)
        _log.debug('RGA output: %s', output)
        with open(os.path.join(dirPath, '%s_rga.txt' % stageInfo['original']['entryPoint'])) as f:
            lines = f.readlines()
        onComplete(_TableToTree(lines))
    except BaseException as e:
        _log.exception('Exception when running RGA')
        onFail(e)
    finally:
        shutil.rmtree(dirPath)


def GetRgaDefaultPath():
    return _RGA_PATH


def GetRgaPlatforms(rgaPath):
    return subprocess.check_output([rgaPath, '-l', '-s', 'hlsl'])


def GetRgaStats(stageInfo, onComplete, onFail, rgaPath='', targetPlatforms=()):
    if not rgaPath:
        return False
    t = threading.Thread(target=_GetRgaStats, args=(stageInfo, onComplete, onFail, rgaPath, targetPlatforms))
    t.daemon = True
    t.start()
    return True