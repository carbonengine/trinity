# Copyright (c) CCP 2011
 
"""
Compiles Tr2ShaderMaterial shaders into a packed compiled shader
file using ShaderCompiler tool. Ported from BuildTr2FX.py
"""

import subprocess
import os
import sys
import re
import yaml
import tempfile

p4Path = os.path.split(__file__)[0]
p4Path += '/../libs/sharedmodules'
sys.path.append(p4Path)

from P4 import P4

p4 = P4()
p4.connect()
p4.prog = "BuildTr2FXPacked"
p4.exception_level = 1


def PrintUsage():
    '''
    Prints program usage information
    '''
    print "Usage: BuildTr2FXPacked [<options>] path\nOptions:"
    print "  /no_warnings             Do not output compile warnings"
    print "  /threads <thread_count>  Limit worker thread count to <thread_count>"
    print "  /incr                    Only build out of date files"
    print "  /AnythingElse            Compile with the macro AnythingElse=1 defined"


def GetFiles(directory):
    '''
    Generator for all file names (returned as absolute paths) in 
    the given folder.
    '''
    for root, dirs, fileNames in os.walk(directory):
        for fileName in fileNames:
            yield os.path.abspath(root + fileName)


def GetFXFiles(root):
    '''
    Generator for all shader (.fx) file names (returned as absolute paths) in 
    the given folder.
    '''
    for filename in GetFiles(root):
        if filename.lower().endswith('.fx'):
            yield filename       


def FxFileToShaderName(fxFileName):
    '''
    Converts shader file name to high-level shader name.
    '''
    return os.path.split(fxFileName)[-1].split('.')[0]


def GetPermuteIndex(defineList, bits):
    '''
    Computes permute index from a list of defines.
    '''
    index = 0
    for d in defineList:
        if d.find("=1") > 0:  # '=1' isn't valid, hence > 0 rather than >= 0
            index |= bits.get(d[:-2], 0)
        else:                # assume situation flag behaviour
            index |= bits.get(d, 0)
    return index


def RunFXCompiler(fxFileName, incremental, fxoFileName, combinations, aliases, bits, platform):
    '''
    Executes shader compiler.
    '''
    if incremental:
        stdOut = tempfile.TemporaryFile('w+t')
        sp = subprocess.Popen(
            "%s\\ShaderCompiler.exe /mtime %s" % 
            (os.path.dirname(os.path.realpath(__file__)), compilerOptions), 
            stdout=stdOut, stdin=subprocess.PIPE, stderr=stdOut, universal_newlines=True)
        inputString = ''
        for opts in combinations:
            defs = ''
            allFlags = 0
            for d in defines + opts:
                defs += ' ';
                if d.find("=1") > 0:  # '=1' isn't valid, hence > 0 rather than >= 0
                    defs += d[0:d.find('=1')] + ' 1'
                    allFlags |= bits.get(d[:-2], 0)
                elif d.find("=") > 0:
                    defs += d[0:d.find('=')] + ' 0'
                else:                # assume situation flag behaviour
                    defs += d + ' 1'
                    allFlags |= bits.get(d, 0)
            try:
                sp.stdin.write(fxFileName + " " + fxoFileName + " " + str(allFlags) + " -1 PLATFORM " + str(platform) + defs + "\n")
                sp.stdin.flush()
            except:
                return
        sp.stdin.close()
        sp.wait()
        stdOut.flush()

        stdOut.seek(0)
        outputs = stdOut.read().split("\n")
        outOfDate = False
        for i in outputs:
            if i == fxoFileName:
                outOfDate = True
                break
        if not outOfDate:
            return

    try:
        os.makedirs(os.path.dirname(fxoFileName))
    except:
        pass
    p4.run_edit(fxoFileName)

    sp = subprocess.Popen(
        "%s\\ShaderCompiler.exe %s %s %s" % 
        (os.path.dirname(os.path.realpath(__file__)), compilerOptions, fxFileName, fxoFileName), 
        stdout=sys.stdout, stdin=subprocess.PIPE, stderr=sys.stdout, universal_newlines=True)
    inputString = ''
    for opts in combinations:
        defs = ''
        allFlags = 0
        for d in defines + opts:
            defs += ' ';
            if d.find("=1") > 0:  # '=1' isn't valid, hence > 0 rather than >= 0
                defs += d[0:d.find('=1')] + ' 1'
                allFlags |= bits.get(d[:-2], 0)
            elif d.find("=") > 0:
                defs += d[0:d.find('=')] + ' 0'
            else:                # assume situation flag behaviour
                defs += d + ' 1'
                allFlags |= bits.get(d, 0)
        try:
            sp.stdin.write(str(allFlags) + " -1 PLATFORM " + str(platform) + defs + "\n")
            sp.stdin.flush()
        except:
            return
    for (fromDefines, toDefines) in aliases:
        fromPermutation = GetPermuteIndex(fromDefines, bits)
        toPermutation = GetPermuteIndex(toDefines, bits)
        try:
            sp.stdin.write(str(toPermutation) + " " + str(fromPermutation) + "\n")
            sp.stdin.flush()
        except:
            return
        
    sp.stdin.close()
    sp.wait()
    sys.stdout.flush()


def mulOpts(yy, xxs):
    '''
    Takes a list of binary options expressed as sub-lists, produces all permutations 
    of said options, and has a little black heart full of poison and hate.
    '''
    if xxs is None or len(xxs) == 0:
        return yy
    elif yy is None or len(yy) == 0:
        return mulOpts([xxs[0][:1], xxs[0][1:]], xxs[1:])
    else:
        xs = xxs[0]
        zz = []
        for x in xs:
            for y in yy:
                zz += [y + [x]]
        return mulOpts(zz, xxs[1:])


def CompileFile(fxFileName, incremental):
    '''
    Compiles a single .fx file.
    '''
    
    print "Effect file: ", os.path.basename(fxFileName)

    bits = {}

    someDefs = []
    predicates = []
    unused = []
    alwaysCachePredicate = None
    redFileName = ''
    outputFileName = ''
    match = re.match(r'(.*[\\/])Shaders([\\/].*)', fxFileName)
    if match is None:
        print "Could not find Shaders folder in path %s\n" % fxFileName
        return
    redFileName = os.path.splitext(match.group(1) + "Shaders\\ShaderDescriptions" + match.group(2))[0] + '.red'
    try:
        #load the yaml file that is associated with the shader
        redFile = yaml.load(file(redFileName).read())
        #grab a bunch o' properties from said yaml file
        alwaysCachePredicate = redFile.get('alwaysCachePredicate')
        if 'permuteTags' in redFile:
            for tag in redFile['permuteTags']: 
                dfn = tag['permuteDefineString']
                prt = tag.get('precompileHint')
                prd = tag.get('predicate')            
                unu = tag.get('unused')            
                bits[dfn] = int(tag['tagBits'])
                if prd != None:
                    predicates.append((dfn,prd))            
                if unu != None:
                    unused.append((dfn,unu))            
                if prt != "-1" and prt != -1 and not dfn in defines:
                    if prt is None or prt == "0" or prt == 0:
                        someDefs.append(dfn)
                    elif prt == "1" or prt == 1:
                        defines.append(dfn)
    except:
        print "Error loading %s\n" % redFileName
        return

    #get the 'splosion of situation flag permutations
    defOpts = []
    if someDefs:
        defOpts = [[r+"=0", r+"=1"] for r in someDefs]
        defOpts = mulOpts(None, defOpts)

    #filter the list based on predicate properties
    filteredOpts = []
    aliases = []
    for opts in defOpts:
        #construct a little environment where all the defines map to True or False
        env = {}
        allOpts = opts + defines
        for o in allOpts:
            if o.find('=') >= 0:
                sp = o.split('=')
                k = sp[0]
                env[k] = (sp[1] == '1')
            else:
                env[o] = True

        #eval the predicate in the context of this little environment
        predsToRemove = []
        def tryPred(p):
            try:
                return eval(p, env)
            except:
                print "exception evaluating predicate: ", p
                predsToRemove.append(p)
                return False

        #and here we actually do the repeated filtering
        doNotAppend = False
        for (d,p) in predicates:
            for o in opts:
                if (o == d or o[:-2] == d) and o[-2:] == '=1':
                    if not tryPred(p):
                        doNotAppend = True
                if doNotAppend:
                    break;
            if doNotAppend:
                break;
        # ... remove all the predicates that threw up
        predicates = filter( lambda (d,p): not p in predsToRemove, predicates )
        if not doNotAppend:
            filteredOpts.append(opts)

    #predicate filtering may have made some entries into duplicates
    # lists aren't hashable, so just convert them to strings and back
    # (nb: this is kind of sick)
    filteredOpts = map(eval, list(set(map(lambda s: s.__str__(), filteredOpts))))
        
    finalOpts = []
    for opts in filteredOpts:
        #construct a little environment where all the defines map to True or False
        env = {}
        allOpts = opts + defines
        for o in allOpts:
            if o.find('=') >= 0:
                sp = o.split('=')
                k = sp[0]
                env[k] = (sp[1] == '1')
            else:
                env[o] = True
                
        #print env

        #eval the predicate in the context of this little environment
        predsToRemove = []
        def tryAlias(expression, environment):
            try:
                return eval(expression, environment)
            except:
                print "exception evaluating skip expression: ", p
                predsToRemove.append(expression)
                return False

        skip = False
        predsToRemove = []
        for (d,p) in unused:
            for o in opts:
                if o[:-2] == d and o[-2:] == '=1':
                    if tryAlias(p, env):
                        alias = []
                        for o2 in opts:
                            if o2 == o:
                                alias.append(o[:-2] + '=0')
                            else:
                                alias.append(o2)
                        aliases.append([opts, alias])
                        skip = True
        if not skip:
            # and finally add our filtered options to the final output
            finalOpts.append(opts)
    
    reduced = True
    while reduced:
        reduced = False
        for alias in aliases:
            for (name, expression) in unused:
                #construct a little environment where all the defines map to True or False
                env = {}
                allOpts = alias[1] + defines
                for o in allOpts:
                    if o.find('=') >= 0:
                        sp = o.split('=')
                        k = sp[0]
                        env[k] = (sp[1] == '1')
                    else:
                        env[o] = True

                #eval the predicate in the context of this little environment
                predsToRemove = []
                def tryAlias(expression, environment):
                    try:
                        return eval(expression, environment)
                    except:
                        print "exception evaluating skip expression: ", p
                        predsToRemove.append(expression)
                        return False

                for define in alias[1]:
                    if define[:-2] == name and define[-2:] == '=1':
                        if tryAlias(expression, env):
                            toDefines = []
                            for o2 in alias[1]:
                                if o2 == define:
                                    toDefines.append(define[:-2] + '=0')
                                else:
                                    toDefines.append(o2)
                            alias[1] = toDefines
                            reduced = True
                            break
        
    addFile = False

    platformName = { 1: "DX9", 2: "DX11" }
    
    if finalOpts:
        print "Situation flag combinations: %i; aliases: %i" % (len(finalOpts), len(aliases))
        
    for platform in platformName:
        platformFolder = '\\' + platformName[platform]
        outputFileName = match.group(1) + "Shaders\\Compiled" + platformFolder + match.group(2) + "t"

        print "Compiling for %s platform" % platformName[platform]
        if finalOpts:
            RunFXCompiler(fxFileName, incremental, outputFileName, finalOpts, aliases, bits, platform)
        else:
            RunFXCompiler(fxFileName, incremental, outputFileName, [defines], aliases, bits, platform)    
            pass

        p4.run_add(outputFileName)

if len(sys.argv) == 1:
    PrintUsage()
    exit(1)

compilerOptions = ''
incrementalBuild = False

path = None
defines = []
count = len(sys.argv)
i = 1
while i < count:
    arg = sys.argv[i]
    if arg.startswith('/'):
        if arg == '/no_warnings':
            compilerOptions += ' /no_warnings'
        elif arg == '/threads':
            i += 1
            compilerOptions += ' /threads ' + sys.argv[i]
        elif arg == '/incr':
            incrementalBuild = True
        else:
            uarg = arg[1:].upper()
            defines.append(uarg)
    else:
        path = arg
    i += 1
    
if not path:
    PrintUsage()
    exit(1)

files = []
if os.path.isfile(path):
    files.append(os.path.abspath(path))
else:
    files.extend(GetFXFiles(path))

shadersToCompile = set()
for f in files:
    shadersToCompile.add(FxFileToShaderName(f))

# to the meat of the problem...
for each in files:
    CompileFile(each, incrementalBuild)
    
p4.disconnect()