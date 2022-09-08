# linux64.py
# Linux x86_64 platform specific configuration

def config(env,args):

    debug = args.get('debug',0)
    if int(debug):
        env.Append(CXXFLAGS = env.Split('-g -O0'))
    else:
        env.Append(CXXFLAGS = '-O')
        env.Append(CPPDEFINES= 'NDEBUG')

    env.Append(CPPDEFINES= 'WITH_DEBUG')

    env.Append(CXXFLAGS = env.Split('-Wall -fPIC'))
    env.Append(CPPDEFINES = 'LINUXVERS')

    cxxversion = env.subst('$CXXVERSION')

    if float(cxxversion[0:2])>=4.0:
        env.Append(CXXFLAGS = env.Split('-Wextra -Wno-missing-field-initializers -Wno-maybe-uninitialized'))
        if not int(debug):
            env.Append(CXXFLAGS = '-Wno-unused-parameter')

    env.Append(LIBSUBDIR = '64')

#end linux64.py
