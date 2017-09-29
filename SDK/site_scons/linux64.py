# linux64.py
# Linux x86_64 platform specific configuration

def config(env,args):

        debug = args.get('debug',0)
        standalone = args.get('standalone',0)
        cppcheck = args.get('cppcheck',0)
        checkheaders = args.get('checkheaders',0)
        srcdist = args.get('srcdist',0)

        if int(debug):
                env.Append(CXXFLAGS = env.Split('-g -O0'))
                env.Append(CPPDEFINES= 'WITH_DEBUG')
        else:
                env.Append(CXXFLAGS = '-O')
                env.Append(CPPDEFINES= 'NDEBUG')

        if int(standalone):
                env.Append(STANDALONE= '1')

        if int(cppcheck):
                env.Append(CPPCHECK= '1')

        if int(checkheaders):
                env.Append(CHECKHEADERS= '1')

        if int(srcdist):
                env.Append(SRCDIST= '1')

        env.Append(CXXFLAGS = env.Split('-Wall'))
        env.Append(CPPDEFINES = 'LINUXVERS')

        cxxversion = env.subst('$CXXVERSION')

        if float(cxxversion[0:2])>=4.0:
                env.Append(CXXFLAGS = env.Split('-Wextra -Wno-missing-field-initializers'))
                if not int(debug):
                        env.Append(CXXFLAGS = '-Wno-unused-parameter')

#end linux64.py
