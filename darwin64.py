import platform
import os

def config(env,args):

        debug = args.get('debug',0)
        standalone = args.get('standalone',0)
        if int(debug):
                env.Append(CXXFLAGS = '-g -O0')
                env.Append(CPPDEFINES= '-DWITH_DEBUG')
        else:
                env.Append(CXXFLAGS = '-O')
                env.Append(CPPDEFINES= '-DNDEBUG')

        if int(standalone):
                env.Append(STANDALONE= '1')

        cxx = env.subst('$CXX')
        if cxx == 'g++':
                env.Append(CXXFLAGS = '-Wall -Woverloaded-virtual')
        elif cxx == 'clang++':
                env.Append(CXXFLAGS = '-Wall')
                env.Replace(LINKFLAGS = '')

        env.Append(CPPDEFINES = '-DMACVERS')

        cxxversion = env.subst('$CXXVERSION')

	if float(cxxversion[0:2])>=4.0:
			env.Append(CXXFLAGS = '-Wextra -Wno-missing-field-initializers')
			env.Append(CXXFLAGS = '-Wno-unused-parameter')

        if float(cxxversion[0:2])>=3.0:
                        env.Append(CPPDEFINES = '-DHAS_SSTREAM')

        env['SHLINKFLAGS'] = '$LINKFLAGS -shared -Wl,-undefined,dynamic_lookup'
        env['SHLIBSUFFIX'] = '.so'


#end darwin64.py
