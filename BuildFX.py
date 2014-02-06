import subprocess
import os
import sys
import multiprocessing
import tempfile
import time

pkgpath = os.path.join(os.path.dirname(__file__), '..', '..', '..', 'packages')
sys.path.append(pkgpath)
import ccpp4

runningOnJenkins = True if os.getenv('RUNNING_ON_JENKINS') else False
noPerforce = True if runningOnJenkins else False
incrementalBuild = False
printSkipped = False
coreCount = 0

additionalOptions = ''

SupportedShaderModels = {'SM_LO': 3, 'SM_HI':4, 'SM_DEPTH':5}
Platforms = {'DX9': 1, 'DX11': 2, 'GLES2': 3}

def PrintUsage():
    print "Usage: BuildFX [<options>] path1 [path2...]\nOptions:"
    print "  /SM_LO      Compile shader model low"
    print "  /SM_HI      Compile shader model high"
    print "  /SM_DEPTH   Compile shader model high with readable depth buffer support"
    print "  /DX9        Compile for DirectX 9"
    print "  /DX11       Compile for DirectX 11"
    print "  /GLES2      Compile for OpenGL ES 2"
    print "  /incr       Only build out of date files"
    print "  /ps         Print files skipped when building with /incr"
    print "  /O{0,1,2,3} Optimization level 0..3.  3 is default"
    print "  /MPn        Specify number of threads used to be n (0 for 1 thread per core)"
    print "\n If no shader model is specified, all supported shader models are built"

def GetFXFiles(w):
    for root, dirs, files in os.walk(w):
        for f in files:
            if f.lower().endswith('.fx') and not f.lower().endswith('_pool.fx'):
                yield root + "\\" + f

def BuildOutputPath(path, platform):
    abs = os.path.abspath(path)
    abs = abs.lower()
    if runningOnJenkins:
        abs = abs.replace('\\effect\\', '\\jenkins_build_result\\effect\\')
    abs = abs.replace('\\effect\\', '\\effect.' + platform.lower() + '\\')
    return abs
    
    
def GetOutputFile(path, sm, platform):
    if not path.endswith('.fx'):
        print "'" + path + "' is not an .fx file\n"
        return
        
    outName = path[:-2]
    outName += sm
    outName = BuildOutputPath(outName, platform)
    return outName



commandQueue = list()
mtimeQueue = list()
mtimeCommands = dict()
outFiles = list()

def CheckMTime():
    stdOut = tempfile.TemporaryFile('w+t')
    cmdPath = os.path.split(__file__)[0] + "\\ShaderCompiler.exe /mtime"
    if coreCount != 0:
        cmdPath += " /threads " + str(coreCount)
    sp = subprocess.Popen(cmdPath, stdin=subprocess.PIPE, stdout=stdOut, stderr=stdOut, universal_newlines=True)
    for i in mtimeQueue:
        sp.stdin.write(i)
        sp.stdin.flush()
    sp.stdin.close()
    sp.wait()
    sys.stdout.flush()

    stdOut.seek(0)
    outputs = stdOut.read().split("\n")
    

    for i in outputs:
        if i in mtimeCommands:
            commandQueue.append(mtimeCommands[i])
            outFiles.append(i)
            del mtimeCommands[i]
    if printSkipped:
        for i in mtimeCommands:
            cmdLine, path, platform, sm = mtimeCommands[i]
            print os.path.split(path)[1] + ' for ' + platform + ', ' + sm
            print "Output is up to date\n"
    
    
def DoCompile():
    if coreCount == 0:
        pcount = multiprocessing.cpu_count()
    else:
        pcount = coreCount
    
    index = 0
    processes = [None for ii in range(pcount)]
    stdOut = [tempfile.TemporaryFile('w+t') for i in range(pcount)]
    hasErrors = False
    while not hasErrors and index < len(commandQueue):
        assigned = False
        for i in range(pcount):
            if processes[i] is None or processes[i].poll() is not None:
                if processes[i] is not None:
                    stdOut[i].flush()
                    stdOut[i].seek(0)
                    print stdOut[i].read()
                    if processes[i].returncode != 0:
                        hasErrors = True
                        stdOut[i].seek(0)
                        stdOut[i].truncate(0)
                        break
                stdOut[i].seek(0)
                stdOut[i].truncate(0)
                cmdLine, path, platform, sm = commandQueue[index]
                stdOut[i].write(os.path.split(path)[1] + ' for ' + platform + ', ' + sm + "\n")
                stdOut[i].flush()
                processes[i] = subprocess.Popen(cmdLine, stdout=stdOut[i], stderr=stdOut[i], universal_newlines=True)
                index += 1
                assigned = True
                if index >= len(commandQueue):
                    break
        if index < len(commandQueue) and not assigned:
            time.sleep(0.1)

    # Now wait until all worker processes finish
    while True:
        working = False
        for i in range(pcount):
            if processes[i] is not None:
                if processes[i].poll() is None:
                    working = True
                    break
                else:
                    if processes[i].returncode != 0:
                        hasErrors = True
                    stdOut[i].flush()
                    stdOut[i].seek(0)
                    print stdOut[i].read()
                    processes[i] = None

        if working:
            time.sleep(0.1)
        else:
            break
    return not hasErrors

def PrepareCompileBatch(path, sm, platform, extra=None):
    outName = GetOutputFile(path, sm, platform)
    outDir = os.path.dirname(outName)
    if not os.path.exists(outDir):
        os.makedirs(outDir)
    
    # Select the version of the fxc compiler to use
    cmdLine = os.path.split(__file__)[0] + "\\ShaderCompiler.exe /shaderStats /single"
    
    # Add any extra options
    if extra:
        cmdLine += " " + extra
    
    # Add the shader model definition
    cmdLine += " /define SHADERMODEL " + str(SupportedShaderModels[sm])
    
    # Add the platform definition
    cmdLine += " /define PLATFORM " + str(Platforms[platform])
    
    cmdLine += " " + additionalOptions
    
    # Add the input file
    cmdLine += " " + path
    
    # Add binary output
    cmdLine += " " + outName

    compileCommand = (cmdLine, path, platform, sm)
    
    if incrementalBuild:
        mtimeCommand = path + " " + outName + " 0 -1 SHADERMODEL " + str(SupportedShaderModels[sm]) + " PLATFORM " + str(Platforms[platform]) + "\n"
        mtimeQueue.append(mtimeCommand)
        mtimeCommands[outName] = compileCommand
    else:
        commandQueue.append(compileCommand)
        outFiles.append(outName)

    
if len(sys.argv) == 1:
    PrintUsage()
    exit(1)
    
paths = []
shaderModels = []
platforms = []
for narg in xrange(1, len(sys.argv)):
    arg = sys.argv[narg]
    if arg.startswith('/'):
        uarg = arg[1:]
        if uarg.upper() in SupportedShaderModels:
            shaderModels.append(uarg)
        elif uarg.upper() in Platforms:
            platforms.append(uarg)
        elif uarg == "incr":
            incrementalBuild = True
        elif uarg == "ps":
            printSkipped = True
        elif len(uarg) == 2 and uarg[0] == "O" and uarg[1] in ('0', '1', '2', '3'):
            additionalOptions += arg
        elif len(uarg) > 2 and uarg[0:2] == 'MP':
            coreCount = int(uarg[2:])
        else:
            print 'Unknown option:', arg
            exit(1)
    else:
        paths.append(arg)

if len(paths) == 0:
    PrintUsage()
    exit(1)

if len(shaderModels) == 0:
    shaderModels = SupportedShaderModels.keys()

if len(platforms) == 0:
    print "No platforms to build"
    exit(1)

files = []
for ipath in paths:
    if os.path.isfile(ipath):
        files.append(ipath)
    else:
        files.extend(GetFXFiles(ipath))

for each in files:
    for pl in platforms:
        for smodel in shaderModels:
            PrepareCompileBatch(each, smodel, pl)

if incrementalBuild:
    CheckMTime()
    
if len(outFiles):
    if not noPerforce:
        try:
            p4 = ccpp4.P4Init("")
        except:
            noPerforce = True
            print "Warning: perforce connection failed, working in local mode"

    try:
        p4.run_edit(outFiles)
    except ccpp4.p4exceptions.P4FilesNotOnClientWarning:
        p4.run_add("-tbinary+m", outFiles)
    except:
        if not noPerforce:
            print "Error: error checking out compiled files in Perforce"
            exit(1)

    if not DoCompile():
        exit(2)
