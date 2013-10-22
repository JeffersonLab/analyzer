#! /usr/bin/env python
#
# Call the actual scons installed on this system

import os
import sys

# which function from http://twistedmatrix.com
# Copyright (c) 2001-2004 Twisted Matrix Laboratories
# Modified 22-Oct-2013 Ole Hansen <ole@jlab.org>

def which(name, flags=os.X_OK):
    """Search PATH for executable files with the given name.
    
    On newer versions of MS-Windows, the PATHEXT environment variable will be
    set to the list of file extensions for files considered executable. This
    will normally include things like ".EXE". This fuction will also find files
    with the given name ending with any of these extensions.

    On MS-Windows the only flag that has any meaning is os.F_OK. Any other
    flags will be ignored.
    
    @type name: C{str}
    @param name: The name for which to search.
    
    @type flags: C{int}
    @param flags: Arguments to L{os.access}.
    
    @rtype: C{str}
    @param: Full path of the first executable found
    """
    exts = filter(None, os.environ.get('PATHEXT', '').split(os.pathsep))
    path = os.environ.get('PATH', None)
    print path
    if path is None:
        print 'no path'
        return name
    for p in os.environ.get('PATH', '').split(os.pathsep):
        p = os.path.join(p, name)
        if os.access(p, flags) and os.path.isfile(p):
            print 'found ' + p
            return p
        for e in exts:
            pext = p + e
            if os.access(pext, flags) and os.path.isfile(p):
                return pext
    return ''

# Call the scons found in the PATH, along with all aruments
sconsexec = which("scons")
# Not sure this is the most elegant way...
for av in sys.argv[1:]:
    sconsexec += ' ' + av
os.system( sconsexec )

# Local Variables:
# tab-width:4
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=4 shiftwidth=4:
