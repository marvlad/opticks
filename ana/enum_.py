#!/usr/bin/env python
#
# Copyright (c) 2019 Opticks Team. All Rights Reserved.
#
# This file is part of Opticks
# (see https://bitbucket.org/simoncblyth/opticks).
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

"""
enum_.py
=================

Canonical usage is for parsing SysRap/OpticksPhoton.h and SysRap/OpticksGenstep.h
which is done from custom commands in optickscore/CMakeLists.txt

Renaming to enum_ was required due to a py3 module name clash.

Test this with::

   cd ~/opticks/sysrap ; ../ana/enum_.py OpticksGenstep.h --quiet --simple --inipath /tmp/OpticksGenstep_Enum.ini && cat /tmp/OpticksGenstep_Enum.ini
   cd ~/opticks/sysrap ; ../ana/enum_.py OpticksPhoton.h --quiet --inipath /tmp/OpticksPhoton_Enum.ini && cat /tmp/OpticksPhoton_Enum.ini

The non-simple form is for parsing enum values of form "0x1 << 4"

"""
import os, re, argparse
import logging
log = logging.getLogger(__name__)

class Enum(dict):
    lptn = re.compile(r"^\s*(\w+)\s*=\s*(.*?),*\s*?$")
    vptn = re.compile(r"^0x1 <<\s*(\d+)$")
    END = "};"

    def __init__(self, path, mskval=True, simple=False, end=True):
        """
        :param path:
        :param mskval:
        :param simple: when True means plain int enum, ie not bitshifted 0x1 << 1 etc..
        """
        dict.__init__(self)
        log.debug("parsing %s " % path )
        path = os.path.expandvars(path)
        log.debug("path expands to %s " % path )

        self.end = end

        if simple:
            self.parse_simple(path)
        else:
            self.parse(path, mskval=mskval)
        pass

    def __repr__(self):
        return "\n".join([" %-2d : %s " % (kv[1], kv[0] ) for kv in sorted(self.items(), key=lambda kv:kv[1])])

    def _get_ini(self):
        return "\n".join(["%s=%s" % (kv[0], kv[1] ) for kv in sorted(self.items(), key=lambda kv:kv[1])])
    ini = property(_get_ini)

    def parse_simple(self, path):
        """
        """
        lines = list(map(str.strip,open(path,"r").readlines()))
        for line in lines:
            if self.end and line.startswith(self.END): break
            lm = self.lptn.match(line)

            if not lm: continue
            lg = lm.groups()
            assert len(lg) == 2
            label, val = lg
            self[label] = int(val)
        pass

    def parse(self, path, mskval=True):
        """
        :param path:
        :param mskval:
        """
        lines = list(map(str.strip,open(path,"r").readlines()))
        for line in lines:
            if self.end and line.startswith(self.END): break
            lm = self.lptn.match(line)
            if not lm: continue

            lg = lm.groups()
            assert len(lg) == 2
            label, val = lg

            vm = self.vptn.match(val)
            assert vm
            vg = vm.groups()
            assert len(vg) == 1
            n = int(vg[0])

            emsk = eval(val)
            msk = 0x1 << n
            assert emsk == msk

            log.debug( "%-40s     ==> [%s]    [%s]  ==> [%d] ==> [%x]  " % (line, label, val, n, msk) )

            self[label] = msk if mskval else n + 1   ## Q: WHY +1 ?


if __name__ == '__main__':

    default_path = "$OPTICKS_PREFIX/include/SysRap/OpticksPhoton.h"
    ## NB envvar not needed when absolute path argument given

    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument(     "path",  nargs="?", help="Path to input header", default=default_path )
    parser.add_argument(     "--inipath", default=None, help="When a path is provided an ini file will be written to it." )
    parser.add_argument(     "--quiet", action="store_true", default=False, help="Skip dumping" )
    parser.add_argument(     "--mskval", action="store_true", default=False, help="Store the mask value rather than the smaller power of two int" )
    parser.add_argument(     "--simple", action="store_true", default=False, help="Simple value enum without bit shifting for masks " )
    parser.add_argument(     "--level", default="info", help="logging level" )
    parser.add_argument(     "--noend", dest="end", default=True, action="store_false", help="inhibit ending of parse at END" )
    args = parser.parse_args()

    fmt = '[%(asctime)s] p%(process)s {%(pathname)s:%(lineno)d} %(levelname)s - %(message)s'
    logging.basicConfig(level=getattr(logging,args.level.upper()), format=fmt)

    if args.path == default_path:
        pass
        log.info("using default path %s " % args.path)
    else:
        pass
        log.info("using argument input path %s " % args.path)
    pass

    d = Enum(args.path, mskval=args.mskval, simple=args.simple, end=args.end)

    if not args.quiet:
        print(d)
        print(d.ini)
    pass
    if not args.inipath is None:
        log.info("writing ini to inipath %s " % args.inipath)
        open(args.inipath, "w").write(d.ini)
    pass






