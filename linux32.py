# linux32.py
# Linux i686 platform specific configuration

def config(env,args):

	debug = args.get('debug',0)
	standalone = args.get('standalone',0)
	if int(debug):
		env.Append(CXXFLAGS = env.Split('-g -O0'))
                env.Append(CPPDEFINES= 'WITH_DEBUG')
	else:	
		env.Append(CXXFLAGS = '-O')
		env.Append(CPPDEFINES= 'NDEBUG')

	if int(standalone):
		env.Append(STANDALONE= '1')

	env.Append(CXXFLAGS = env.Split('-m32 -Wall -Woverloaded-virtual'))
	env.Append(CPPDEFINES = 'LINUXVERS')

	cxxversion = env.subst('$CXXVERSION')

        if float(cxxversion[0:2])>=4.0:
		env.Append(CXXFLAGS = env.Split('-Wextra \
                   -Wno-missing-field-initializers'))
                if not int(debug):
                         env.Append(CXXFLAGS = '-Wno-unused-parameter')
	
        env.Append(SHLINKFLAGS = '-m32')

#end linux32.py
