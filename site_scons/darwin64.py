# darwin64.py
# Mac OS X platform specific configuration

def config(env,args):

        debug = args.get('debug',0)
        standalone = args.get('standalone',0)
        if int(debug):
                env.Append(CXXFLAGS = env.Split('-g -O0'))
                env.Append(CPPDEFINES = 'WITH_DEBUG')
        else:
                env.Append(CXXFLAGS = '-O')
                env.Append(CPPDEFINES = 'NDEBUG')

        if int(standalone):
                env.Append(STANDALONE = '1')

        env.Append(CXXFLAGS = env.Split('-Wall -Woverloaded-virtual'))
        env.Append(CPPDEFINES = 'MACVERS')

        cxxversion = env.subst('$CXXVERSION')

        if float(cxxversion[0:2])>=4.0:
		env.Append(CXXFLAGS = env.Split('-Wextra \
                   -Wno-missing-field-initializers'))
                if not int(debug):
                         env.Append(CXXFLAGS = '-Wno-unused-parameter')

        env.Append(SHLINKFLAGS = '-Wl,-undefined,dynamic_lookup')

#end darwin64.py
