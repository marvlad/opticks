##
## Copyright (c) 2019 Opticks Team. All Rights Reserved.
##
## This file is part of Opticks
## (see https://bitbucket.org/simoncblyth/opticks).
##
## Licensed under the Apache License, Version 2.0 (the "License");
## you may not use this file except in compliance with the License.
## You may obtain a copy of the License at
##
##   http://www.apache.org/licenses/LICENSE-2.0
##
## Unless required by applicable law or agreed to in writing, software
## distributed under the License is distributed on an "AS IS" BASIS,
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
## See the License for the specific language governing permissions and
## limitations under the License.
##

om-source(){ echo $BASH_SOURCE ; }
om-vi(){ vi $(om-source) ; }
om-env(){  olocal- ; opticks- ;
  local msg="=== $FUNCNAME :"
  if [ "$1" == "quiet" -o "$1" == "q" -o -n "$OPTICKS_QUIET" ]; then
     #echo $msg quiet running
     oe- 2> /dev/null
  else
     #echo $msg normal running
     oe- ;
  fi
}
om-usage(){ cat << EOU

OM : Opticks Minimal Approach to Configuring and Building
===========================================================

The below subproj functions configure, build, install or test
either one, all or a range of Opticks subprojects
in the appropriate dependency order.

The commands are sensitive to the invoking directory, performing
actions for all subprojects when invoked from top level
directories. Also an argument can be used to select a
starting subproject, eg "cudarap:" starts from there
or ":" starts from the subproject of the current directory.

When building for the first time it is necessary to
use "om-install" as later subprojects cannot be configured
until earlier ones have been installed.

Usage Examples
---------------

om-subs okconf:opticksgeo
    list the subprojects in this inclusive range

om-subs :opticksgeo
    same as the above, as first is implicit

om-test :opticksgeo
    run the tests for all subprojects from the first okconf upto opticksgeo


Partial test running
----------------------

::

    TESTARG="-R U4TreeCreate" om-test  ## runs just the tests with that pattern
    TESTARG="-E U4TreeTest" om-test   ## exclude the regexp tests

The TESTARG is included on the ctest command line, see ctest --help

Real World Usage
------------------

::

   ggeo    # change directory to ggeo
   okc     # change directory to optickscore
   vi OpticksSwitches.h

   om      # build just current dir project
   om :    # build current dir project and all subsequent
   om +    # build all subsequent proj (not including current)
   ## a bug preventing "om :" and "om +" from working has been fixed

   om-subs :    #  list this project and all subsequent
   om-subs +    #  list all subsequent projects



SUBPROJ FUNCTIONS
-----------------

om-subs
   list subprojects in dependency order

om-conf
   configures using cmake

om-make
   build and install

om--
   shorter name for om-make

om
   even shorter function that does om-;om--

om-install
   configures, builds and installs by doing both om-conf and om-make

om-cleaninstall
   cleans, configures, builds and installs by doing om-clean, om-conf and om-make

om-prefix-clean
   really deep clean that deletes the dirs within opticks-prefix : lib lib64 include build
   after that it is necessary to om-install

om-test
   runs ctests

om-visit
   visits each subproj in turn

om-echo
   echos the pwd of each subproj


OTHER FUNCTIONS
-----------------

om-cd
   cd from a source tree directory to the corresponding
   directory in the build tree or vice versa


FUNCTIONS INSENSITIVE TO INVOKING DIRECTORY
-------------------------------------------------

om-visit-all
om-conf-all
om-make-all
om-test-all

om-testlog
    parse test logs from previous test runs and present summary totals



EOU
}

om-home-default(){  echo $(dirname $(om-source)) ; }
om-home(){   echo ${OPTICKS_HOME:-$(om-home-default)} ; }

#om-local(){  echo ${LOCAL_BASE:-/usr/local} ; }
#om-name(){   echo $(basename $(om-home)) ; }
#om-fold(){   echo $(om-local)/$(om-name) ; }
#om-prefix(){ echo $(om-fold) ; }
# this is the crucial prefix : need a more obvious way to set it  : OPTICKS_PREFIX envvar ?

om-prefix(){ echo ${OPTICKS_PREFIX:-/usr/local/opticks} ; }



om-pkg-config-path-reversed(){ $FUNCNAME- | python ; }
om-pkg-config-path-reversed-(){ cat << EPY
import os
dirs=os.environ["PKG_CONFIG_PATH"].split(":")
print(":".join(reversed(dirs)))
EPY
}

om-pkg-config-path(){ $FUNCNAME- | python ; }
om-pkg-config-path-(){ cat << EPY
import os
dirs=[]
for pfx in os.environ["CMAKE_PREFIX_PATH"].split(":"):
    for lib in ["lib","lib64"]:
        libdir = os.path.join(pfx,lib)
        if os.path.isdir(libdir):   # judge on libdir not pkgconfig dir as need to create initially for some
            dirs.append(os.path.join(libdir, "pkgconfig"))
            break   # second sub:lib64 ignored if there is a lib
        pass
    pass
print(":".join(dirs))
EPY
}


om-prefix-all-dirs-(){ sed 's/#.*//' << EOD
#bashrc
bin
build
cmake
#el9_amd64_gcc11
externals
gl
include
lib
lib64
metadata
optix
py
tests
EOD
}


om-prefix-few-dirs-(){ cat << EOD
lib
lib64
build
include
EOD
}
om-prefix-clean-notes(){ cat << EON

See notes/issues/CMake_dependency_include_dirs_issue_reported_by_Hans.rst

If users report CMake dependency issues, the cause might not be due to CMake
but rather due to failures to include stale headers.  In order to find
these bad inclusions a deeper clean than om-cleaninstall is needed.
In that case use::

    om-prefix-clean

EON
}
om-prefix-clean(){
   local msg="=== $FUNCNAME :"
   cd $(om-prefix)
   pwd
   local dirs=$(om-prefix-all-dirs-)
   echo $msg om-prefix : $(om-prefix)
   echo $msg pwd       : $(pwd)
   echo $msg om-prefix-all-dirs- : $dirs
   echo $msg CAUTION - AFTER THIS YOU WILL NEED TO USE opticks-full FOR REINSTALLATION
   local ans
   read -p "$msg enter YES to proceed with deletion of prefix dirs : " ans

   if [ "$ans" == "YES" ]; then
       local cmd
       for dir in $dirs ; do
           cmd="rm -rf $dir"
           echo $cmd
           eval $cmd
       done
   else
       echo $msg SKIPPED
   fi
}



#om-cmake-generator(){ echo $(opticks-cmake-generator) ; }
om-cmake-generator(){ echo "Unix Makefiles" ; }


om-bdir-trim-tests()
{
    local iwd=$(pwd)
    local name=$(basename ${iwd/tests})   # trim tests to get name of subproj from tests folder or subproj folder
    local bdir=$(om-bdir $name)
    echo $bdir
}


om-bdir(){
   : TODO separate bdir depending on Release/Debug so its faster to switch
   local gen=$(om-cmake-generator)
   case $gen in
      "Unix Makefiles") echo $(om-prefix)/build/$1 ;;
               "Xcode") echo $(om-prefix)/build_xcode/$1 ;;
   esac
}
om-sdir(){
   : TODO generalize current approach of just special casing pkg names, could use a naming convention jPMTSim
   case $1 in
      PMTSim) echo $HOME/j/$1 ;;
      PMTFastSim) echo $HOME/j/$1 ;;
           *) echo $(om-home)/$1 ;;
   esac
}
om-bdir-real(){ echo $(realpath $(om-bdir)) ; }
om-sdir-real(){ echo $(realpath $(om-sdir)) ; }


om-url(){ echo http://bitbucket.org/simoncblyth/$(om-name)/src/$(om-rdir) ; }
om-open(){ open $(om-url) ; }

om-info(){ cat << EOI

   om-home-default    : $(om-home-default)
   om-home            : $(om-home)  from OPTICKS_HOME envvar if defined
   om-name            : $(om-name)
   om-local           : $(om-local)  from LOCAL_BASE envvar if defined
   om-fold            : $(om-fold)
   om-prefix          : $(om-prefix)
   om-cmake-generator : $(om-cmake-generator)
   om-bdir            : $(om-bdir)
   om-sdir            : $(om-sdir)


   CMAKE_PREFIX_PATH  : $CMAKE_PREFIX_PATH
   PKG_CONFIG_PATH    : $PKG_CONFIG_PATH


EOI
}



om-subs--old(){ cat << EOS
#yoctoglrap
#assimprap
#openmeshrap
#opticksgeo
#cudarap
#thrustrap
#optixrap
#okop
#oglrap
#opticksgl
#ok
#cfg4
#okg4
#g4ok
#integration

EOS
}


om-subs--all(){ cat << EOS
# to update this list : opticks-deps --subdirs
# the names must be the case correct directory names
okconf
sysrap
#boostrap
#npy
#optickscore
#ggeo
#extg4
ana
analytic
bin
CSG
#CSG_GGeo
#GeoChain
qudarap
gdxml
u4
CSGOptiX
g4cx
EOS
}

om-subs-minimal-notes(){ cat << EON
om-subs-minimal
================

Minimimal package build of Opticks intended to facilitate
loading from separately created geocache and OptiX 7 rendering.
This is convenient for use on nodes which are difficult to work with.

HMM: it would not be too difficuly move some optickscore
functionality down to sysrap, which would allow
the following three pkgs to be skipped

boostrap
npy
optickscore

Started this in sysrap/SOpticks

Also could skip building most of the tests to speed up the build alot

EON
}


om-subs--minimal(){ cat << EOS
okconf
sysrap
boostrap
npy
optickscore
CSG
qudarap
CSGOptiX
EOS
}

om-subs-thoughts(){ cat << EOS
okconf      # not enough functionality, TODO: merge into sysrap
sysrap
boostrap      # REMOVE
npy           # REMOVE
optickscore   # REMOVE
ggeo          # REMOVE
extg4         # REMOVE
ana
analytic
bin
CSG
CSG_GGeo      # REMOVE
GeoChain      # REMOVE
qudarap
gdxml
u4
CSGOptiX
g4cx
EOS
}


om-subs-alt-notes(){ cat << EON

TODO:

* need to generalize om/opticks-deps machinery to work with new OptiX7 chain of sub pkgs,
  so far have been manually building them and relying on the old chain too using
  new pkg shortcuts : c, cg, qu, cx

* subs are pkg directory names, some new pkgs dirs not using lowercase convention

* have been avoiding boostrap and npy dependency in new developments
  moving instead to sysrap/NP and SPath but have been using optickscore and ggeo

  * BUT, it will take considerable effort to migrate away from
    boostrap and npy usage in ggeo and elsewhere, not worthwhile currently

* using an envvar OM_ALT to signal that the alt pkgs should be
  appended to the usual list seems like a good approach ..
  because of minimal disruption as needs no new base functions.

  * actually could just add to standard pkgs all but CSGOptiX
    which needs special handling for OptiX7 b7 building ?

  * could make building of CSGOptiX dependent on OPTICKS_OPTIX7_PREFIX being defined ?
    hmm : not quite so simple still building it with Six swap without 7


When working on the new pkgs only starting from CSG, build just those with::

    c
    om :     # colon means build this pkg and subsequent
    cx
    ./b7


EON
}

om-alt(){ om-subs--alt ; }

om-subs--alt(){  cat << EOS
CSG
CSG_GGeo
GeoChain
qudarap
CSGOptiX
EOS
}

om-subs--()
{
   case ${OM_SUBS:-all} in
     all)     om-subs--all ;;
     minimal) om-subs--minimal ;;
     alt)     om-subs--alt ;;
   esac
}

om-subs-(){ om-subs-- | grep -v ^\# ; }



om-subs-notes(){ cat << EON
om-subs
--------

Default with no argument lists all subproject directory names
in build dependency order. Arguments can be used to select
ranges of names.

Shorthand forms  ":" and "+" only work from within a subproj
which is taken as the first, all other forms work from anywhere.

:
    subproj from current onwards
+
    subproj from next onwards

:last
    subprojects from the first "okconf" to the last specified inclusive,
    eg ":ok" lists all up to and including "ok"

first:last
    subprojects from the "first" to "last" inclusive

EON
}

om-subs(){

   local arg=$1
   [ -z "$arg" ] && om-subs- && return
   ## without argument just return the hardcoded list, with argument returns selection from the list

   local iwd=$(pwd)
   local name=$(basename $iwd)

   local mode=":"
   local first
   local last

   ## expand shorthand argument into full
   [ "$arg" == ":" ] && arg="${name}:" && mode=":"
   [ "$arg" == "+" ] && arg="${name}:" && mode="+"

   ## form ":last" means from okconf (1st project)
   [ "${arg:0:1}" == ":" ] && first="okconf" || first=${arg/:*}
   last=${arg/*:}

   #echo $FUNCNAME arg $arg name $name first $first last $last

   local sel=0
   local sub
   om-subs- | while read sub ; do
       [ "${sub}" == "$first" -a "${mode}" == ":" ] && sel=1
       [ "${sub}" == "$first" -a "${mode}" == "+" ] && sel=2
       [ "$sel" == "1" ] && echo $sub
       [ "${sub}" == "$last" -a "${mode}" == ":" ] && sel=0
       [ "$sel" == "2" ] && sel=1
   done
}


om-subs0(){
  local arg=$1

  local iwd=$(pwd)
  local name=$(basename $iwd)

  if [ -z "$arg" ]; then
      om-subs-
  else
      [ "$arg" == ":" ] && arg="${name}:"
      [ "$arg" == "+" ] && arg="${name}+"
      local sel=0
      local sub
      om-subs- | while read sub ; do
         [ "${sub}:" == "$arg" ] && sel=1
         [ "${sub}+" == "$arg" ] && sel=2
         [ "$sel" == "1" ] && echo $sub
         [ "$sel" == "2" ] && sel=1
      done
  fi
}





om-reldir()
{
   local cwd=$(pwd)
   local sdir=$(om-sdir)
   local bdir=$(om-bdir)
   case $cwd in
      $sdir*) echo ${cwd/$sdir} ;;
      $bdir*) echo ${cwd/$bdir} ;;
   esac
}
om-bcd(){ cd $(om-bdir $(om-reldir)) ; pwd ;  }
om-scd(){ cd $(om-sdir $(om-reldir)) ; pwd ;  }


om-visit-all(){     om-all ${FUNCNAME/-all} $* ; return $? ; }
om-conf-all(){      om-all ${FUNCNAME/-all} $* ; return $? ; }
om-make-all(){      om-all ${FUNCNAME/-all} $* ; return $? ; }
om-install-all(){   om-all ${FUNCNAME/-all} $* ; return $? ; }
om-cleaninstall-all(){   om-all ${FUNCNAME/-all} $* ; return $? ; }
om-test-all(){      om-all ${FUNCNAME/-all} $* ; om-testlog ; return $? ; }
om-echo-all(){      om-all ${FUNCNAME/-all} $* ; return $? ; }
om-clean-all(){     om-all ${FUNCNAME/-all} $* ; return $? ; }
om-find-all(){      om-all ${FUNCNAME/-all} $* ; return $? ; }


om-geom(){
   : get GEOM from subshell to avoid exporting into this shell
   : HMM - this could be misleading with runners that dont follow the convention
   local geomscript=$HOME/.opticks/GEOM/GEOM.sh
   local geom=$([ -s $geomscript ] && source $geomscript > /dev/null && echo $GEOM)
   echo $geom
}

om-testlog(){ GEOM=$(om-geom) CTestLog.py $(om-bdir) $* ; }


om-conf-xcode(){ OPTICKS_CMAKE_GENERATOR=Xcode om-conf ; }

om-conf(){    om-one-or-all conf $* ; return $? ; }
om-make(){    om-one-or-all make $* ; return $? ; }
om-install(){ om-one-or-all install $* ; return $? ; }
om-cleaninstall(){ om-one-or-all cleaninstall $* ; return $? ; }
om-visit(){   om-one-or-all visit $* ; return $? ; }

om-test-help(){ cat << EOH
om-test-help
-------------

NB : are now running each test via test runners such as::

    STestRunner.sh
    CSGTestRunner.sh

To rerun selected tests use CTESTARG to change the ctest commandline,
for example using the "-R" regexp option eg::

    CTESTARG="-R U4TreeCreateTest" om-test
    CTESTARG="-R U4TreeCreate"     om-test

Or directly use ctest for more control::

    st     # cd ~/opticks/sysrap/tests
    ct     # cd ~/opticks/CSG/tests
    qt     # cd ~/opticks/qudarap/tests
    u4t    # cd ~/opticks/u4/tests
    cxt    # cd ~/opticks/CSGOptiX/tests
    gxt    # cd ~/opticks/g4cx/tests

    om-cd  # cd to build directory (eg /usr/local/opticks/build/CSG/tests)

    ctest -N                                      # list tests
    ctest -R U4TreeCreateTest --output-on-failure # run tests matching a pattern
    ctest --help                                  # to see the possibilities

To rebuild a test and rerun it use::

    TEST=SBndTest om-t  # NB no environment setup : so only for very simple tests

To run a test with the debugger within the test runner environment,
pick the appropriate runner for the package of the test and use a command
such as::

    CSGTestRunner.sh "gdb CSGFoundry_CreateFromSimTest"

NB this and ctest running uses the installed test runners, so
must install before changes to runners take effect.


EOH
}

om-test(){
    local rc=0
    #om-testenv-push
    om-one-or-all test $*
    rc=$?
    #om-testenv-pop
    #om-test-help   # call this from opticks-t- to avoid repetition
    return $rc
}
om-echo(){    om-one-or-all echo $* ; return $? ; }
om-clean(){   om-one-or-all clean $* ; return $? ; }
om-find(){    om-one-or-all find $* ; return $? ; }

om--(){       om-make $* ; }     ## just this proj
om---(){      om-make-all : ; }  ## all projs from this one onwards
om----(){     om-make-all + ; }  ## all projs following this one


om-testenv-notes(){ cat << EON
om-testenv-notes
------------------

TRYING TO USE BASH RUNNERS INSTEAD OF THIS, AS THAT PLAYS
BETTER WITH CTEST INSTALLED TESTS

EON
}

om-testenv-vars(){ echo BASH_SOURCE FUNCNAME OM_KEEP_GEOM GEOM OPTICKS_T_GEOM ; }

om-testenv-dump(){
    local pfx="$1"
    if [ -z "$QUIET" ]; then
        local fmt="$pfx %20s : %s \n"
        local vars="$(om-testenv-vars)"
        local var ; for var in $vars ; do printf "$fmt" "$var" "${!var}" ; done
    fi
}

om-testenv-push()
{
    om-testenv-dump "[push"

    export OM_KEEP_GEOM=$GEOM

    local geom_script=$HOME/.opticks/GEOM/GEOM.sh

    if [ -s $geom_script ]; then
        source $geom_script
        # script is expected to always set GEOM
        # and perhaps set OPTICKS_T_GEOM
    fi
    export GEOM=${OPTICKS_T_GEOM:-$GEOM}

    om-testenv-dump "]push"
}
om-testenv-pop()
{
    om-testenv-dump "[pop "

    if [ -n "$OM_KEEP_GEOM" ] ; then
        export GEOM=$OM_KEEP_GEOM
        unset OM_KEEP_GEOM
    else
        unset GEOM
    fi

    om-testenv-dump "]pop "
}



om-check()
{
    local msg="=== $FUNCNAME :"
    local bdir=$(om-bdir)
    local rc=0
    if [ ! -d "$bdir" ]; then
        echo $msg top level bdir $bdir does not exist : creating it
        mkdir -p $bdir
    fi
    if [ ! -d "$bdir" ]; then
        echo $msg top level bdir $bdir does not exist : create failed
        rc=1
    fi
    return $rc
}


om-all-notes(){ cat << EON
om-all-notes
==============

om-all runs the func passed in the argument
in each of the sub-package bdir


EON
}


om-all()
{
    local rc
    local iwd=$(pwd)
    local func=$1
    local msg="=== $FUNCNAME $func :"
    shift

    om-check
    rc=$?
    [ "$rc" != "0" ] && echo $msg ERROR om-check failed && return $rc

    local subs=$(om-subs $*)    # need to accept the argument to handle :/+ for building from here onwards
    local name
    : switched to for loop is easier for error propagation than piping
    for name in $subs
    do
        local sdir=$(om-sdir $name)
        local bdir=$(om-bdir $name)
        mkdir -p $bdir
        local udir
        : only om-find needs sdir the others need bdir
        case $func in
          om-find) udir=$sdir ;;
                *) udir=$bdir ;;
        esac

        cd $udir

        $func
        rc=$?
        [ "$rc" != "0" ] && echo $msg ERROR bdir $bdir : non-zero rc $rc && return $rc
    done
    cd $iwd
    return $rc
}

om-one-or-all()
{
    local rc=0
    local func=$1
    local arg=$2  # not normally used
    local iwd=$(pwd)
    local msg="=== $FUNCNAME $func :"

    om-check
    rc=$?
    [ "$rc" != "0" ] && echo $msg ERROR om-check failed && om-info && return $rc

    if [ "${iwd}/" == "$(om-sdir)" -o "${iwd}/" == "$(om-bdir)" -o  "${arg/:}" != "$arg" -o  "${arg/+}" != "$arg"    ]; then
        om-$func-all $arg
        rc=$?
    else
        om-$func-one $arg
        rc=$?
    fi

    if [ "$rc" != "0" ]; then
       echo $msg non-zero rc $rc
    fi
    cd $iwd
    return $rc
}

om-echo-one(){ echo $(pwd) ; }

om-install-one()
{
    om-visit-one $*
    om-conf-one $*
    om-make-one $*
}

om-cleaninstall-one()
{
    om-clean-one $*
    om-visit-one $*
    om-conf-one $*
    om-make-one $*
}




om-visit-one()
{
    local msg="=== $FUNCNAME :"
    local iwd=$(pwd)
    local name=$(basename $iwd)
    local sdir=$(om-sdir $name)
    local bdir=$(om-bdir $name)

    if [ ! -d "$bdir" ]; then
         echo $msg bdir $bdir does not exist : creating it
         mkdir -p $bdir
    fi
    cd $bdir
    printf "%s %-15s %-60s %-60s \n"  "$msg" $name $sdir $bdir
}

om-conf-one()
{
    local arg=$1
    local iwd=$(pwd)

    local name=$(basename ${iwd/tests})   # trim tests to get name of subproj from tests folder or subproj folder
    local sdir=$(om-sdir $name)
    local bdir=$(om-bdir $name)   # TODO: different bdir for each build type

    if [ "$arg" == "clean" ]; then
         echo $msg removed bdir $bdir as directed by clean argument
         rm -rf $bdir
    fi

    if [ ! -d "$bdir" ]; then
         echo $msg bdir $bdir does not exist : creating it
         mkdir -p $bdir
    fi

    cd $bdir
    printf "%s %-15s %-60s %-60s \n"  "$msg" $name $sdir $bdir

    local rc=0
    if [ "$name" == "okconf" ]; then
        om-cmake-okconf $sdir
        rc=$?
    else
        om-cmake $sdir
        rc=$?
    fi
    return $rc
}


om-cmake-okconf()
{
    local sdir=$1
    local bdir=$PWD
    [ "$sdir" == "$bdir" ] && echo ERROR sdir and bdir are the same $sdir && return 1000

    local rc
    cmake $sdir \
       -G "$(om-cmake-generator)" \
       -DCMAKE_BUILD_TYPE=$(opticks-buildtype) \
       -DOPTICKS_PREFIX=$(om-prefix) \
       -DCMAKE_INSTALL_PREFIX=$(om-prefix) \
       -DCMAKE_MODULE_PATH=$(om-home)/cmake/Modules \
       -DOptiX_INSTALL_DIR=$(opticks-optix-prefix) \
       -DCOMPUTE_CAPABILITY=$(opticks-compute-capability) \
       -DCOMPUTE_ARCHITECTURES=$(opticks-compute-architectures)

    # TODO: arrange for this and om-cmake to merge
    # NB not pinning CMAKE_PREFIX_PATH so can find foreigners, see oe-

    rc=$?
    return $rc
}


om-cmake()
{
    local sdir=$1
    local bdir=$PWD
    [ "$sdir" == "$bdir" ] && echo ERROR sdir and bdir are the same $sdir && return 1000

    local rc
    cmake $sdir \
       -G "$(om-cmake-generator)" \
       -DCMAKE_BUILD_TYPE=$(opticks-buildtype) \
       -DOPTICKS_PREFIX=$(om-prefix) \
       -DCMAKE_INSTALL_PREFIX=$(om-prefix) \
       -DCMAKE_MODULE_PATH=$(om-home)/cmake/Modules

    #  -DCMAKE_FIND_DEBUG_MODE=1 \
    # NB not pinning CMAKE_PREFIX_PATH so can find foreigners, see oe-

    rc=$?
    return $rc
}

om-cmake-dump(){ local sdir=${1:-sdir} ; cat << EOD

    cmake $sdir \\
       -G "$(om-cmake-generator)" \\
       -DCMAKE_BUILD_TYPE=$(opticks-buildtype) \\
       -DOPTICKS_PREFIX=$(om-prefix) \\
       -DCMAKE_INSTALL_PREFIX=$(om-prefix) \\
       -DCMAKE_MODULE_PATH=$(om-home)/cmake/Modules

    om-cmake-generator : $(om-cmake-generator)
    opticks-buildtype  : $(opticks-buildtype)
    om-prefix          : $(om-prefix)
    om-home            : $(om-home)

EOD
}



om-cmake-info(){ cat << EOI

$FUNCNAME
===============

   om-cmake-generator         : $(om-cmake-generator)
   opticks-buildtype          : $(opticks-buildtype)
   om-prefix                  : $(om-prefix)


   OPTICKS_OPTIX5_PREFIX      : $OPTICKS_OPTIX5_PREFIX
   OPTICKS_OPTIX6_PREFIX      : $OPTICKS_OPTIX6_PREFIX
   OPTICKS_OPTIX7_PREFIX      : $OPTICKS_OPTIX7_PREFIX
   OPTICKS_OPTIX_PREFIX       : $OPTICKS_OPTIX_PREFIX
   opticks-optix-prefix       : $(opticks-optix-prefix)


   opticks-compute-capability : $(opticks-compute-capability)
   OPTICKS_COMPUTE_CAPABILITY : $OPTICKS_COMPUTE_CAPABILITY

   NODE_TAG                   : $NODE_TAG

EOI
}


om-nproc(){
   case $(uname) in
      Linux) echo ${OM_NPROC:-$(nproc)} ;;
      Darwin) echo 2 ;;
   esac
}


om-make-one()
{
    local rc=0
    local iwd=$(pwd)
    local msg="=== $FUNCNAME :"
    local name=$(basename ${iwd/tests})   # trim tests to get name of subproj from tests folder or subproj folder
    local sdir=$(om-sdir $name)
    local bdir=$(om-bdir $name)
    printf "%s %-15s %-60s %-60s \n"  "$msg" $name $sdir $bdir

    if [ ! -d $bdir ]; then
       echo $msg ERROR bdir $bdir does not exist : you need to om-install OR om-conf once before using om-make or the om-- shortcut
       rc=1
       return $rc
    fi

    if [ ! -f $bdir/Makefile ]; then
       echo $msg ERROR bdir $bdir exists but does not contain a Makefile : you need to om-install OR om-conf once before using om-make or the om-- shortcut
       rc=2
       return $rc
    fi


    cd $bdir
    local t0=$(date +"%s")

    local pr=$(om-nproc)

    if [ $pr -gt 1 ]; then
        make all -j$pr
    else
        cmake --build .  --target all
    fi

    rc=$?
    local t1=$(date +"%s")
    local d1=$(( t1 - t0 ))

    [ "$rc" != "0" ] && cd $iwd && return $rc

    #echo d1 $d1
    [ "$(uname)" == "Darwin" -a $d1 -lt 1 ] && echo $msg kludge sleep 2s : make time $d1 && sleep 2

    if [ $pr -gt 1 ]; then
        make install -j$pr
    else
        cmake --build .  --target install
    fi


    rc=$?
    return $rc
    [ "$rc" != "0" ] && cd $iwd && return $rc
}


om-test-one-notes(){ cat << EON
$FUNCNAME
====================

To debug ctest running::

    cd ~/opticks/u4
    om-cd  # change to bdir

    ctest -N    # list tests
    ctest -V --interactive-debug-mode 0
        ## verbose output, but with the non-interactive mode used by test running

NB see SSys::GetInteractivityLevel the --interactive-debug-mode results in ctest
setting envvar CTEST_INTERACTIVE_DEBUG_MODE


EON
}

om-test-one()
{
    local iwd=$(pwd)
    local name=$(basename $iwd)

    local msg="=== $FUNCNAME :"
    local name=$(basename ${iwd/tests})   # trim tests to get name of subproj from tests folder or subproj folder
    local sdir=$(om-sdir $name)
    local bdir=$(om-bdir $name)
    printf "%s %-15s %-60s %-60s \n"  "$msg" $name $sdir $bdir
    cd $bdir
    local log=ctest.log
    date          | tee $log

    echo ctest $CTESTARG --interactive-debug-mode 0 --output-on-failure
    ctest $* $CTESTARG --interactive-debug-mode 0 --output-on-failure  2>&1 | tee -a $log

    date          | tee -a $log

    local geom=$(om-geom)
    echo GEOM $geom | tee -a $log

    cd $iwd
}

om-clean-one()
{
    local iwd=$(pwd)
    local name=$(basename $iwd)

    local msg="=== $FUNCNAME :"
    local name=$(basename ${iwd/tests})   # trim tests to get name of subproj from tests folder or subproj folder
    local sdir=$(om-sdir $name)
    local bdir=$(om-bdir $name)
    cd $sdir
    local cmd="rm -rf $bdir && mkdir -p $bdir"
    echo $cmd
    eval $cmd

    cd $iwd
}





om-find-one(){
   local str="${1:-ENV_HOME}"
   local opt=${2:--H}
   local iwd=$(pwd)
   find . \
        \( \
       -name '*.sh' -or \
       -name '*.bash' -or \
       -name '*.cu' -or \
       -name '*.cc' -or \
       -name '*.hh' -or \
       -name '*.cpp' -or \
       -name '*.hpp' -or \
       -name '*.h' -or \
       -name '*.txt' -or \
       -name '*.rst' -or \
       -name '*.py' \
        \) \
       -exec grep $opt "$str" {} \;

    cd $iwd
}









om-test-log(){ echo $(om-bdir-trim-tests)/ctest.log ; }
om-tl(){ cat $(om-test-log) ; }




om-divider(){ cat << EOD
//////////////////////////////////////////////////////////////////////////////////

My bash functions tend to grow, in order to avoid confusing users
trying to install Opticks should try to distinguish between

1. essentials for building
2. useful additions for developers

This divider attempts to make that split.  Possibly relocate the below into
an omu- to make that distinction stronger.

//////////////////////////////////////////////////////////////////////////////////
EOD
}


om-mk-notes(){ cat << EON
om-mk
=======

om-mk allows running commands from the parallel build tree,
particularly useful for quickly building and running/debugging
single test executables. This is especially useful with projects
like NPY which is quite slow to build fully, as it allows just the
needed code to be built for the test executable.

Note this works so long as the source changes are all within the
current subproj. If there are changes in other projs this will not
detect that::

   npy-c
   cd tests

   om-mk "make help | grep Test"

   om-mk "make NTreeBuilderTest && ./NTreeBuilderTest"

   om-mk "make NConvexPolyhedronSegmentTest  && ./NConvexPolyhedronSegmentTest"

       ## build and run a single test

   TEST=NSceneLoadTest om-t


Note that this does not install ... and
there seems no way to install just the one test ?

EON
}


#om-tt(){ echo ${TEST:-NTreePositiveTest} ; }
#om-tt(){ echo ${TEST:-NTreeBalanceTest} ; }
#om-tt(){ echo ${TEST:-NCSGSaveTest} ; }
om-tt(){ echo ${TEST:-X4PolyconeTest} ; }

om-t(){  om-t- $(om-tt) ; }
om-t-(){ om-mk "make $1  && ./$1" ; }

om-d(){  om-d- $(om-tt) ; }
om-d-(){ om-mk "make $1  && lldb ./$1" ; }

om-mk()
{
    local msg="=== $FUNCNAME :"
    local iwd=$(pwd)
    local rdir=$(om-reldir)   #  relative dir, when invoked from within source or build trees
    [ "${rdir/tests}" == "${rdir}" ] && rdir="$rdir/tests"
    local bdir=$(om-bdir $rdir)
    cd $bdir
    echo $msg bdir $bdir rdir $rdir : $1
    eval $1
    cd $iwd
}

om-pdir()
{
    local here=$(realpath $(pwd -P));
    local stop=$(om-sdir-real);
    local btop=$(om-bdir-real);
    stop=${stop%/}
    btop=${btop%/}
    :  remove trailing slash

    [ -n "$DEBUG" ] && echo $FUNCNAME here $here stop $stop btop $btop

    : symbolic links can trick this
    case $here in
        $stop)  echo $btop ;;
        $btop)  echo $stop ;;
        $stop*) echo $btop/${here/$stop\/} ;;
        $btop*) echo $stop/${here/$btop\/} ;;
             *) echo "" ;;
    esac
    return 0
}

om-rdir()
{
    local here=$(pwd -P);
    local stop=$(om-sdir);
    local btop=$(om-bdir);
    stop=${stop%/}  # remove trailing slash
    btop=${btop%/}
    case $here in
        $stop)  echo "" ;;
        $btop)  echo "" ;;
        $stop*) echo ${here/$stop\/} ;;
        $btop*) echo ${here/$btop\/} ;;
             *) echo "" ;;
    esac
    return 0
}

om-cd()
{
    local msg="=== $FUNCNAME :"
    local iwd=$(pwd -P)
    local pdir=$(om-pdir);
    [ -z "$pdir" ] && echo pwd $iwd is not inside Opticks source tree or its counterpart build tree && return 1;
    [ -n "$pdir" ] && cd $pdir
    #printf "%s %-60s to %-60s \n"  "$msg" $name $iwd $pdir
    pwd
}



om-gen()
{
   cd $(opticks-home)

   local rel=$1
   rel=${rel/.bash}

   local nam=$2
   [ -z "$nam" ] && nam=$(basename $rel)

   local dir=$(dirname $rel)
   mkdir -p $dir

   om-gen- $nam $rel > $rel.bash

   . $rel.bash

   $nam-vi
}


om-gen-(){ cat << EOT

$1-source(){ echo \$BASH_SOURCE ; }
$1-vi(){ vi \$($1-source) om.bash opticks.bash externals/externals.bash ; }
$1-env(){  olocal- ; opticks- ; }
$1-usage(){ cat << EOU

$1 Usage
===================

Generate a file for bash precursor functions or notes using om-gen like this::

   om-gen yoctoglrap/yog         # makes precursor yog-
   om-gen notes/geant4/opnovice

Hook up a line like the below to opticks.bash or externals/externals.bash::

   $1-(){ . \$(opticks-home)/$2.bash      && $1-env \$* ; }


EOU
}

$1-dir(){ echo \$(dirname \$($1-source)) ; }
$1-cd(){  cd \$($1-dir) ; }
$1-c(){   cd \$($1-dir) ; }
$1--(){   opticks-- \$($1-dir) ; }

EOT
}





om-tst-(){ local cls=$1 ; cat << EOT

// TEST=${cls}Test om-test

#include "${cls}.hh"
#include "OPTICKS_LOG.hh"

int main(int argc, char** argv)
{
    OPTICKS_LOG(argc, argv);


    return 0 ;
}

EOT

}


om-libpath(){
   #local libprefix=$LOCAL_BASE/opticks_$(opticks-buildtype)
   local libprefix=$OPTICKS_PREFIX
   case $(uname) in
      Darwin) echo $libprefix/lib:$libprefix/externals/lib ;;
      Linux) echo $libprefix/lib64:$libprefix/externals/lib:/lib64 ;;
   esac
}

om-run()
{
   case $(uname) in
     Linux)  LD_LIBRARY_PATH=$(om-libpath) $*   ;;
     Darwin) DYLD_LIBRARY_PATH=$(om-libpath) $* ;;
   esac
}

om-run-notes(){ cat << EON
om-run
   workaround a failure to setup RPATH for some executables.
   Of order 375 Opticks executables do not need this...
   but some executables made in examples do for unkown reasons.
   This temorarily sets the library path to enable them to find
   the Opticks libs.

Usage::

    om-run OneTriangleTest
    om-run UseInstanceTest

EON
}

om-c(){  om-cls ${1:-Opticks} ; }
om-cls()
{
    local base=$(om-home)
    local cls=${1:-NTreeJUNO}
    local hdr=$(find $base \( -name ${cls}.hh -or -name ${cls}.hpp -or -name ${cls}.h  \) )
    local imp=$(find $base \( -name ${cls}.cc -or -name ${cls}.cpp \) )
    local tst=$(find $base \( -name ${cls}Test.cc -or -name ${cls}.txt -or -name ${cls}.py \) )
    local cmd="vi $hdr $imp $tst"
    echo $cmd
    eval $cmd
}




