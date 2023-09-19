# rootcint.py

import os
import re
from SCons.Script import Action, Builder

def generate(env):
    """
    RootCint(dictionary,headers[,PCMNAME=pcmfilename])
    env.RootCint(dictionary,headers[,PCMNAME=pcmfilename])

    Generate ROOT dictionary source file "dictionary" from list of class
    headers "headers". The last item in "headers" must be the LinkDef file.
    Optional PCMNAME is the name for the pcm and rootmap files, including
    the leading "lib".
    """
    bld = Builder(action = Action(rootcint_builder,rootcint_print),
                  emitter = rootcint_emitter)
    env.Append(BUILDERS = {'RootCint' : bld})
    env.Replace(PCMNAME = '')

def exists(env):
    return True

def rootcint_print(target, source, env):
    tgt = os.path.basename(target[0].abspath)
    return 'RootCint: generating '+tgt

def rootcint_emitter(target, source, env):
    """
    With ROOT >= 6, rootcling generates a <dict>_rdict.pcm mapping file
    in addition to the dictionary source file. Add this "side effect" artifact
    to the list of targets so that SCons can automatically keep track of it.
    """
    if int(env.get('ROOTVERS','0')[0]) >= 6:
        if env['PCMNAME']:
            target.append(env['PCMNAME']+'_rdict.pcm')
            target.append(env['PCMNAME']+'.rootmap')
        else:
            # Default PCM file name that rootcling generates without -s <pcmname>
            target.append(re.sub(r'\.cxx\Z','_rdict.pcm',str(target[0])))
            target.append(re.sub(r'\.cxx\Z','.rootmap',str(target[0])))
    return target, source

def rootcint_builder(target, source, env):
    """
    Call rootcint (for ROOT < 6) or rootcling (for ROOT >= 6) to
    generate a ROOT dictionary from the given headers in 'source'.
    Any relevant -D definitions and -I include directives are taken from
    $_CCCOMCOM. The ROOT version is expected to be given in $ROOTVERS;
    if undefined, ROOT 6 is assumed.
    """
    dictname = target[0]
    cpppath = env.subst('$_CCCOMCOM')
    if int(env.get('verbose',0)) > 4:
        print('rootcint_builder: cpppath = %s' % cpppath)
#    ccflags = env.subst('$CCFLAGS')
    headers = ""
    for f in source:
        headers += str(f) + " "
    if int(env.get('ROOTVERS','6')[0]) >= 6:
        command = "rootcling -f %s " % dictname
        # I know it's dumb to add the _rdict.pcm string in the emitter, only
        # to remove it again here. But in this way, we can let SCons add
        # the correct relative path in front this file name instead of
        # having to add it manually to PCMNAME in the RootCint call.
        pcmname = re.sub(r'_rdict\.pcm\Z','',str(target[1]))
        if int(env.get('verbose',0)) > 4:
            print('rootcint_builder: target = ', target)
            print('rootcint_builder: pcmname = %s' % pcmname)
        if env['PCMNAME']:
            command += "-s %s " % pcmname
        command += " -rmf " + str(target[2])
        command += " -rml %s%s " % (os.path.basename(pcmname), env.subst('$SHLIBSUFFIX'))
        command += " %s %s" % (cpppath,headers)
    else:
        command = "rootcint -f %s -c %s %s" % (dictname,cpppath,headers)
    if int(env.get('verbose',0)) > 2:
        print ('RootCint Command = %s' % command)
    ok = os.system(command)
    if not ok:
        return None
    return target, source
