#!/bin/bash
usage(){ cat << EOU
rsync.sh
==========

Update copies a remote directory to local node.
The argument directory is expected not to have a trailing slash.
When the argument begins with "." the local destination
directory is prefixed with $HOME

Only files with the below filetype extensions are copied::

   .gdml
   .txt
   .log
   .tlog
   .npy
   .jpg
   .mp4
   .json

Configure the remote node with REMOTE envvar.
Default REMOTE is "P" which requires a "host P"
entry in ~/.ssh/config

Usage::

   source ~/opticks/bin/rsync.sh /tmp/GEOM/V1J009/CSGOptiX


EOU
}


arg=$1
REMOTE=${REMOTE:-A}

if [ -z "$arg" ]; then
    echo == $BASH_SOURCE no argument provided
    return 1
fi

if [ "${arg/.npy}" != "$arg" ]; then

   opath=$arg
   odir=$(dirname $arg)
   echo $BASH_SOURCE rsync single path $opath

   mkdir -p $odir
   rsync -zv --progress $REMOTE:$opath $opath

else

    odir=$arg
    if [ -d "$odir" ]; then
        echo == $BASH_SOURCE odir $odir exists
    else
        echo == $BASH_SOURCE creating rsync destination directory $odir
    fi


    xdir=$odir/  ## require trailing slash to avoid rsync duplicating path element
    from=$REMOTE:$xdir

    if [ "${odir:0:1}" == "." ]; then
        to=$HOME/$xdir
    else
        to=$xdir
    fi

    vars="BASH_SOURCE xdir REMOTE from to"
    for var in $vars ; do printf "%-30s : %s \n" $var "${!var}" ; done ;

    mkdir -p "$to"
    rsync -zarv --progress --include="*/" \
                           --include '*.gdml' \
                           --include="*.txt" \
                           --include="*.log" \
                           --include="*.tlog" \
                           --include="*.npy" \
                           --include="*.jpg" \
                           --include="*.mp4" \
                           --include "*.json" \
                           --exclude="*" \
                           "$from" "$to"

    [ $? -ne 0 ] && echo $BASH_SOURCE rsync fail && return 1

    tto=${to%/}  # trim the trailing slash

    find $tto -name '*.json' -o -name '*.txt' -o -name '*.log' -o -name '*.gdml' -print0 | xargs -0 ls -1rt
    echo == $BASH_SOURCE tto $tto jpg mp4 npy
    find $tto -name '*.jpg' -o -name '*.mp4' -o -name '*.npy' -print0 | xargs -0 ls -1rt

fi

return 0

