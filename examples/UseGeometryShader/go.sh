#!/bin/bash
usage(){ cat << EOU
go.sh
=====

See also build.sh which doesnt use cmake

TODO: time scrubbing control

EOU
}

cd $(dirname $(realpath $BASH_SOURCE))

arg=${1:-build_run}
sdir=$(pwd)
name=$(basename $sdir)

bdir=/tmp/$USER/opticks/$name/build
idir=/tmp/$USER/opticks/$name/install

PREFIX=$idir
bin=$PREFIX/lib/$name

source ~/.opticks_config

export SHADER_FOLD=$sdir/rec_flying_point

#export RECORD_FOLD=/tmp/$USER/opticks/GeoChain/BoxedSphere/CXRaindropTest
#export RECORD_FOLD=/tmp/blyth/opticks/GeoChain/BoxedSphere/CXRaindropTest/SCVD0/70000
#export RECORD_FOLD=/tmp/$USER/opticks/QSimTest/mock_propagate
#export RECORD_FOLD=/tmp/blyth/opticks/U4RecorderTest/ShimG4OpAbsorption_FLOAT_ShimG4OpRayleigh_FLOAT/hama_body_log
#export RECORD_FOLD=/tmp/blyth/opticks/GEOM/V1J011/ntds3/ALL1/p001
#export RECORD_FOLD=$TMP/GEOM/RaindropRockAirWater/G4CXTest/ALL0/B000
export RECORD_FOLD=/tmp/sphoton_test


if [ "${arg/build}" != "$arg" ] ; then

    rm -rf $bdir && mkdir -p $bdir && cd $bdir && pwd

    cmake $sdir \
         -DCMAKE_BUILD_TYPE=Debug \
         -DOPTICKS_PREFIX=$OPTICKS_PREFIX \
         -DCMAKE_INSTALL_PREFIX=$PREFIX \
         -DCMAKE_MODULE_PATH=$OPTICKS_HOME/cmake/Modules

    make
    [ $? -ne 0 ] && exit 1

    make install
fi

if [ "${arg/run}" != "$arg" ] ; then
    echo $BASH_SOURCE : run $bin
    EYE=0,-3,0,1 $bin
    [ $? -ne 0 ] && echo $BASH_SOURCE : run error && exit 2
fi

if [ "${arg/dbg}" != "$arg" ] ; then
    echo $BASH_SOURCE : dbg $bin
    dbg__ $bin
    [ $? -ne 0 ] && echo $BASH_SOURCE : dbg error && exit 3
fi

exit 0

