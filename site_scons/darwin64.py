# darwin64.py
# Mac OS X platform specific configuration

def config(env,args):

    debug = args.get('debug',0)
    if int(debug):
        env.Append(CXXFLAGS = env.Split('-g -O0'))
    else:
        env.Append(CXXFLAGS = '-O')
        env.Append(CPPDEFINES= 'NDEBUG')

    env.Append(CPPDEFINES= 'WITH_DEBUG')

    env.Append(CXXFLAGS = env.Split('-Wall -fPIC'))
    env.Append(CPPDEFINES = 'MACVERS')

    cxxversion = env.subst('$CXXVERSION')

    if float(cxxversion[0:2])>=4.0:
        env.Append(CXXFLAGS = env.Split('-Wextra -Wno-missing-field-initializers'))
        if not int(debug):
            env.Append(CXXFLAGS = '-Wno-unused-parameter')

    env.Append(SHLINKFLAGS = '-Wl,-undefined,dynamic_lookup')
    env.Append(SHLINKFLAGS = '-Wl,-headerpad_max_install_names')
    env.Replace(RPATH_ORIGIN_TAG = '@loader_path')

#end darwin64.py
