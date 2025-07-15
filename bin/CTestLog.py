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
CTestLog.py
=============

Collective reporting from a bunch of separate ctest.log files::

    CTestLog.py /usr/local/opticks-cmake-overhaul/build

Or from a single logfile::

    CTestLog.py /tmp/2091440.out


Canonical usage from opticks-t/om-test/om-testlog::

    om-testlog ()
    {
        CTestLog.py $(om-bdir) $*
    }


"""
import sys, re, os, logging, argparse, datetime
log = logging.getLogger(__name__)


class Test(dict):

    tmpl = "  %(num)-3s/%(den)-3s Test #%(num2)-3s: %(name)-55s %(result)-30s %(time)-6s "

    def __init__(self, *args, **kwa):
        dict.__init__(self, *args, **kwa)
    def __repr__(self):
        return self.tmpl % self


class CTestLog(object):
    """
    47/49 Test #47: GGeoTest.RecordsNPYTest ..........   Passed    0.03 sec

    """
    NAME = "ctest.log"
    TPATN = re.compile(r"\s*(?P<num>\d*)/(?P<den>\d*)\s*Test\s*#(?P<num2>\d*):\s*(?P<name>\S*)\s*(?P<div>\.*)\s*(?P<result>.*)\s+(?P<time>\d+\.\d+) sec$")
    SKIPS = "yoctoglrap openmeshrap".split()

    @classmethod
    def examine_logs(cls, args):
        logs = []
        root = str(args.base)
        for dirpath, dirs, names in os.walk(root):
            if cls.NAME in names:
                log.debug(dirpath)
                reldir = dirpath[len(root):]
                log.debug("reldir:[%s] dirpath:[%s] root:[%s] %d " % (reldir, dirpath, root, len(root)) )
                if reldir == "" and not args.withtop:
                    log.debug("skipping toplevel tests, reldir [%s]" % reldir)
                    continue
                pass
                if reldir in cls.SKIPS:
                    log.info("skipping reldir [%s]" % reldir)
                    continue
                pass
                path = os.path.join(dirpath, cls.NAME)
                lg = cls.Parse(path, reldir)
                logs.append(lg)
            pass
        pass
        cls.calc_totals(logs)

    @classmethod
    def calc_totals(cls, logs):
        tot = {}
        tot["tests"] = 0
        tot["fails"] = 0
        for lg in logs:
            tot["tests"] += len(lg.tests)
            tot["fails"] += len(lg.fails)
        pass
        cls.logs = logs
        cls.tot = tot
        cls.dt = max(map(lambda lg:lg.dt, cls.logs ))


    @classmethod
    def examine_single_logfile(cls, args):
        lg = cls.Parse(args.base, None)
        lgs = [lg]
        cls.calc_totals(lgs)

    @classmethod
    def desc_totals(cls):
        return "%(fails)-3s / %(tests)-3s " % cls.tot

    num_tests = property(lambda self:len(self.tests))
    num_fails = property(lambda self:len(self.fails))

    @classmethod
    def Parse(cls, path, reldir):
        log.debug("reading %s " % path)
        lines = list(map(str.rstrip, open(path,"r").readlines() ))
        lg = cls(lines, path=path, reldir=reldir)
        return lg

    def __init__(self, lines, path=None, reldir=None):
        self.lines = lines
        self.reldir = reldir
        self.name = os.path.basename(reldir) if not reldir is None else None
        self.path = path
        self.tests = []
        self.fails = []
        dt = datetime.datetime.fromtimestamp(os.stat(path).st_ctime) if path is not None else None
        self.dt = dt

        for line in lines:
            m = self.TPATN.match(line)
            if m:
                tst = Test(m.groupdict())
                self.tests.append(tst)
                if not tst["result"].strip() == "Passed":
                    self.fails.append(tst)
                pass
                #print line
                #print tst
            pass
        pass


    def __repr__(self):
        return "CTestLog : %20s : %6d/%6d : %s : %s " % ( self.reldir, self.num_fails, self.num_tests, self.dt, self.path  )

    def __str__(self):
        return "\n".join([repr(self)] + self.lines )


if __name__ == '__main__':

    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument( "base", nargs="*",  help="Directory in which to look for ctest.log files OR filepath" )
    parser.add_argument( "--withtop", action="store_true", help="Switch on handling of the usually skipped top level test results" )
    parser.add_argument( "--slow", default="15", help="Slow test time cut in seconds, comma delimited list of values accepted" )
    parser.add_argument( "--level", default="info", help="log level" )
    args = parser.parse_args()

    level = getattr( logging, args.level.upper() )
    fmt = '[%(asctime)s] p%(process)s {%(pathname)s:%(lineno)d} %(levelname)s - %(message)s'
    logging.basicConfig(level=level, format=fmt)

    if len(args.base) == 0:
        args.base = os.getcwd()
    else:
        args.base = args.base[0]
    pass

    if os.path.isdir(args.base):
        CTestLog.examine_logs(args)
    else:
        CTestLog.examine_single_logfile(args)
    pass
    lgs = sorted(CTestLog.logs, key=lambda lg:lg.dt)

    print("\n\nTESTS:")
    for lg in lgs:
        print("")
        print(repr(lg))
        for tst in lg.tests:
            print(tst)
        pass
    pass

    print("\n\nLOGS:")
    for lg in lgs:
        print(repr(lg))
    pass

    tt=list(map(float, args.slow.split(",")))
    for t in tt:
        print("\n\nSLOW: tests taking longer that %s seconds" % str(t) )
        for lg in lgs:
            for tst in filter(lambda tst:float(tst['time'])>float(t),lg.tests):
                print(tst)
            pass
        pass
    pass

    print("\n\nFAILS:  %s  :  %s  :  GEOM %s  " % ( CTestLog.desc_totals(), CTestLog.dt.strftime("%c"), os.environ.get("GEOM", "no-geom")))
    for lg in lgs:
        for tst in lg.fails:
            print(tst)
        pass
    pass
    print("\n\n")


