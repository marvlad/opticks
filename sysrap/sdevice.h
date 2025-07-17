#pragma once

/**
sdevice.h
============

Simplified version of the former cudarap/CDevice.cu

To select the GPU need to use CUDA_VISIBLE_DEVICES
and metadata recording is handled with sdevice.h scontext.h

* scontext.h needs updating to handle updated sdevice.h and
  metadata from scontext needs to be included into the SEvt run metadata

* running sysrap/tests/sdevice_test.sh without CUDA_VISIBLE_DEVICES
  defined persists info on all GPUs into ~/.opticks/runcache/sdevice.bin

**/

#include <cstddef>
#include <vector>
#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
#include <cassert>
#include <csignal>
#include <sstream>
#include <cstdlib>
#include <fstream>
#include <cuda_runtime_api.h>

#include "sdirectory.h"
#include "spath.h"
#include "ssys.h"


struct sdevice
{
    static constexpr const char* CVD = "CUDA_VISIBLE_DEVICES" ;
    static constexpr const char* _DIRPATH = "sdevice__DIRPATH" ;
    static constexpr const char* _PERSIST = "sdevice__PERSIST" ;
    static constexpr const char* DIRPATH_DEFAULT = "$HOME/.opticks/sdevice" ;  // formerly scontext
    static constexpr const char* FILENAME = "sdevice.bin" ;
    static constexpr const char* _level = "sdevice__level" ;
    static int level ;
    static int PERSIST ;

    int ordinal ;
    int index ;

    char name[256] ;
    char uuid[16] ;
    int major  ;
    int minor  ;
    int compute_capability ;
    int multiProcessorCount ;
    size_t totalGlobalMem ;   // bytes


    const char* brief() const ;
    const char* desc() const ;
    bool matches(const sdevice& other) const ;
    size_t totalGlobalMem_bytes() const ;
    float totalGlobalMem_GB() const ;
    static int DeviceCount();
    static void Collect(std::vector<sdevice>& devices, bool ordinal_from_index=false );
    static int Size();
    void write( std::ostream& out ) const ;
    void read( std::istream& in );

    static void Visible(std::vector<sdevice>& visible );
    static int FindIndexOfMatchingDevice( const sdevice& d, const std::vector<sdevice>& all );

    static std::string Path(const char* dirpath);

    static const char* ResolveDirPath();
    static const char* CreateDirPath();

    static void Save(const std::vector<sdevice>& devices );
    static void Load(      std::vector<sdevice>& devices );

    static std::string Brief(const std::vector<sdevice>& devices );
    static std::string Desc( const std::vector<sdevice>& devices );
    static std::string VRAM( const std::vector<sdevice>& devices );
};


/**
With c++11::

    warning: inline variables are only available with ‘-std=c++17’ or ‘-std=gnu++17’

**/

inline int sdevice::level = ssys::getenvint(_level, 0 );
inline int sdevice::PERSIST = ssys::getenvint(_PERSIST, 0);


inline const char* sdevice::brief() const
{
    std::stringstream ss ;
    ss << "idx/ord/mpc/cc:"
       << index
       << "/"
       << ordinal
       << "/"
       << multiProcessorCount
       << "/"
       << compute_capability
       << std::setw(8) << std::fixed << std::setprecision(3) <<  totalGlobalMem_GB() << " GB "
       ;
    std::string s = ss.str();
    return strdup(s.c_str());
}

inline const char* sdevice::desc() const
{
    std::stringstream ss ;
    ss
        << std::setw(30) << brief()
        << " "
        << name
        ;
    std::string s = ss.str();
    return strdup(s.c_str());
}

/**
sdevice::matches
------------------

Returns true when both uuid and name matches.

**/

inline bool sdevice::matches(const sdevice& other) const
{
   return strncmp(other.uuid, uuid, sizeof(uuid)) == 0 && strncmp(other.name, name, sizeof(name)) == 0;
}

inline size_t sdevice::totalGlobalMem_bytes() const
{
    return totalGlobalMem ;
}
inline float sdevice::totalGlobalMem_GB() const
{
    return float(totalGlobalMem)/float(1024*1024*1024)  ;
}

inline int sdevice::DeviceCount() // static
{
    int devCount(0) ;  // TWAS A BUG TO NOT INITIALIZE THIS
    cudaGetDeviceCount(&devCount);
    return devCount ;
}

/**
sdevice::Collect
--------------------

Use CUDA API to collect a summary of the cudaDeviceProp properties
regarding all attached devices into the vector of sdevice argument.

ordinal_from_index:true
    sdevice.ordinal value is taken from the index corresponding to the ordering
    of devices returned by cudaGetDeviceProperties(&p, i) : this
    is used by sdevice::Visible when when no CUDA_VISIBLE_DEVICES envvar
    is defined

ordinal_from_index:false
    sdevice.ordinal is set to initial placeholder -1 : sdevice::Visible
    however when CUDA_VISIBLE_DEVICES envvar is defined sets the ordinal
    by matching device properties with the persisted list of all of them

**/

inline void sdevice::Collect(std::vector<sdevice>& devices, bool ordinal_from_index)
{
    int devCount = DeviceCount() ;
    if(level > 0) std::cout << "sdevice::Collect cudaGetDeviceCount : " << devCount << std::endl ;

    for (int i = 0; i < devCount; ++i)
    {
        cudaDeviceProp p;
        cudaGetDeviceProperties(&p, i);

        sdevice d ;

        assert( sizeof(p.name) == sizeof(char)*256 ) ;
        assert( sizeof(d.name) == sizeof(char)*256 ) ;
        strncpy( d.name, p.name, sizeof(d.name) );

#ifndef CUDART_VERSION
#error CUDART_VERSION Undefined!
#elif (CUDART_VERSION >= 10000)
        assert( sizeof(p.uuid) == sizeof(uuid) );
        strncpy( d.uuid, p.uuid.bytes, sizeof(p.uuid) );
#elif (CUDART_VERSION >= 9000)
#endif

        d.index = i ;
        d.ordinal = ordinal_from_index ? i : -1 ;
        d.major = p.major ;
        d.minor = p.minor ;
        d.compute_capability = p.major*10 + p.minor ;

        d.multiProcessorCount = p.multiProcessorCount ;
        d.totalGlobalMem = p.totalGlobalMem ;

        devices.push_back(d);
    }
}

inline int sdevice::Size()
{
    return
        sizeof(int) +       // ordinal
        sizeof(int) +       // index
        sizeof(char)*256 +  // name
        sizeof(char)*16 +   // uuid
        sizeof(int) +       // major
        sizeof(int) +       // minor
        sizeof(int) +       // compute_capability
        sizeof(int) +       // multiProcessorCount
        sizeof(size_t) ;    // totalGlobalMem
}
inline void sdevice::write( std::ostream& out ) const
{
    int size = Size();
    char* buffer = new char[size];
    char* p = buffer ;

    memcpy( p, &ordinal,             sizeof(ordinal) )             ; p += sizeof(ordinal) ;
    memcpy( p, &index,               sizeof(index) )               ; p += sizeof(index) ;
    memcpy( p, name,                 sizeof(name) )                ; p += sizeof(name) ;
    memcpy( p, uuid,                 sizeof(uuid) )                ; p += sizeof(uuid) ;
    memcpy( p, &major,               sizeof(major) )               ; p += sizeof(major) ;
    memcpy( p, &minor,               sizeof(minor) )               ; p += sizeof(minor) ;
    memcpy( p, &compute_capability,  sizeof(compute_capability) )  ; p += sizeof(compute_capability) ;
    memcpy( p, &multiProcessorCount, sizeof(multiProcessorCount) ) ; p += sizeof(multiProcessorCount) ;
    memcpy( p, &totalGlobalMem,      sizeof(totalGlobalMem) )      ; p += sizeof(totalGlobalMem) ;

    out.write(buffer, size);
    assert( p - buffer == size );
    delete [] buffer ;
}

inline void sdevice::read( std::istream& in )
{
    int size = Size();
    char* buffer = new char[size];
    in.read(buffer, size);
    char* p = buffer ;

    memcpy( &ordinal,  p,           sizeof(ordinal) )             ; p += sizeof(ordinal) ;
    memcpy( &index,    p,           sizeof(index) )               ; p += sizeof(index) ;
    memcpy( name,      p,           sizeof(name) )                ; p += sizeof(name) ;
    memcpy( uuid,      p,           sizeof(uuid) )                ; p += sizeof(uuid) ;
    memcpy( &major,    p,           sizeof(major) )               ; p += sizeof(major) ;
    memcpy( &minor,    p,           sizeof(minor) )               ; p += sizeof(minor) ;
    memcpy( &compute_capability, p, sizeof(compute_capability) )  ; p += sizeof(compute_capability) ;
    memcpy( &multiProcessorCount,p, sizeof(multiProcessorCount) ) ; p += sizeof(multiProcessorCount) ;
    memcpy( &totalGlobalMem,     p, sizeof(totalGlobalMem) )      ; p += sizeof(totalGlobalMem) ;

    delete [] buffer ;
}



/**
sdevice::Visible
------------------

This assumes that the ordinal is the index when all GPUs are visible
and it finds this by arranging to persist the query when
CUDA_VISIBLE_DEVICES is not defined and use that to provide something
to match against when the envvar is defined.

Initially tried to do this in one go by changing envvar
and repeating the query. But that doesnt work,
presumably as the CUDA_VISIBLE_DEVICES value only has
any effect when cuda is initialized.

Of course the disadvantage of this approach
is that need to arrange to do the persisting of all devices
at some initialization time and need to find an
appropriate place for the file.

The purpose is for reference running, especially performance
scanning : so its acceptable to require running a metadata
capturing executable prior to scanning.

Possibly NVML can provide a better solution, see nvml-
Actually maybe not : the NVML enumeration order follows nvidia-smi
not CUDA.


1. check existance of CUDA_VISIBLE_DEVICES envvar setting *no_cvd*
   when not defined, indicating all GPUs are visible

2. invoke Collect setting *ordinal_from_index* according to *no_cvd*


**/

inline void sdevice::Visible(std::vector<sdevice>& visible )
{
    if(level > 0) std::cout << "[sdevice::Visible" << std::endl ;

    char* cvd = getenv(CVD);
    bool no_cvd = cvd == NULL ;
    std::vector<sdevice> all ;

    bool ordinal_from_index = no_cvd  ;
    Collect(visible, ordinal_from_index);

    int VISIBLE_COUNT = visible.size() ;
    assert( sdevice::DeviceCount() == VISIBLE_COUNT );

    if(level > 0) std::cerr << "sdevice::Visible no_cvd:" << no_cvd << std::endl ;


    if( no_cvd )
    {
        if(VISIBLE_COUNT > 0 && PERSIST == 1)
        {
            if(level > 0) std::cerr
                << "sdevice::Visible no_cvd save"
                << " VISIBLE_COUNT " << VISIBLE_COUNT
                << " PERSIST " << PERSIST
                << " level " << level
                << "\n"
                ;
            Save( visible );
        }
    }
    else
    {
        if(level > 0) std::cerr << "sdevice::Visible with cvd " << cvd << std::endl ;
        Load(all);

        for(unsigned i=0 ; i < visible.size() ; i++)
        {
            sdevice& v = visible[i] ;
            v.ordinal = FindIndexOfMatchingDevice( v, all );
        }
    }
    if(level > 0) std::cout << "]sdevice::Visible" << std::endl ;
}


const char* sdevice::ResolveDirPath()
{
    const char* DIRPATH = ssys::getenvvar(_DIRPATH, DIRPATH_DEFAULT)  ;
    const char* dirpath = spath::Resolve(DIRPATH) ;
    return dirpath ;
}


const char* sdevice::CreateDirPath()
{
    const char* dirpath = ResolveDirPath();
    bool exists = spath::Exists(dirpath);

    if(level > 0) std::cout
        << "[sdevice::CreateDirPath"
        << " level " << level << "\n"
        << " dirpath [" << ( dirpath ? dirpath : "-" )  << "]\n"
        << " _DIRPATH [" << ( _DIRPATH ? _DIRPATH : "-" ) << "]\n"
        << " DIRPATH_DEFAULT [" << ( DIRPATH_DEFAULT ? DIRPATH_DEFAULT : "-" ) << "]\n"
        << " exists " << ( exists ? "YES" : "NO " ) << "\n"
        << "\n"
        ;

    if( !exists )
    {
        int rc = sdirectory::MakeDirs(dirpath, 0);
        if(rc!=0) std::cerr
            << "sdevice::CreateDirPath "
            << " FAILED to create dir \n"
            << " ABOUT TO RAISE SIGINT \n"
            << " level " << level << "\n"
            << " dirpath [" << ( dirpath ? dirpath : "-" )  << "]\n"
            << " _DIRPATH [" << ( _DIRPATH ? _DIRPATH : "-" ) << "]\n"
            << " DIRPATH_DEFAULT [" << ( DIRPATH_DEFAULT ? DIRPATH_DEFAULT : "-" ) << "]\n"
            ;

        if(rc!=0) std::raise(SIGINT);
        assert(rc == 0);
    }
    return dirpath ;
}



/**
sdevice::FindIndexOfMatchingDevice
------------------------------------

**/

inline int sdevice::FindIndexOfMatchingDevice( const sdevice& d, const std::vector<sdevice>& all )
{
    int index = -1 ;
    if(level > 0) std::cout
         << "sdevice::FindIndexOfMatchingDevice"
         << " d " << d.desc()
         << " all.size " << all.size()
         << std::endl
         ;

    for(unsigned i=0 ; i < all.size() ; i++)
    {
        const sdevice& a = all[i] ;
        bool m = a.matches(d) ;
        if(level > 0) std::cout
            << "sdevice::FindIndexOfMatchingDevice"
            << " a " << a.desc()
            << " m " << m
            << std::endl
            ;

        if(m)
        {
           index = a.index ;
           break ;
        }
    }
    if(level > 0) std::cout << "sdevice::FindIndexOfMatchingDevice  index : " << index << std::endl ;
    return index ;
}




inline std::string sdevice::Path(const char* dirpath)
{
    std::stringstream ss ;
    if( dirpath ) ss << dirpath << "/" ;
    ss << FILENAME ;
    return ss.str();
}

/**
sdevice::Save
--------------

All sdevice struct from the vector are written into a single file

**/

inline void sdevice::Save( const std::vector<sdevice>& devices )
{
    const char* dirpath = CreateDirPath();
    std::string _path = Path(dirpath);
    const char* path = _path.c_str();

    if(level > 0) std::cout
        << "sdevice::Save "
        << " dirpath [" << ( dirpath ? dirpath : "-" ) << "]"
        << " path [" << ( path ? path : "-" ) << "]"
        << std::endl
        ;

    std::ofstream out(path, std::ofstream::binary);
    if(out.fail())
    {
        std::cerr << "sdevice::Save failed open for [" << ( path ? path : "-" ) << "]\n" ;
        return ;
    }

    for(unsigned i = 0 ; i < devices.size() ; ++i )
    {
        const sdevice& d = devices[i] ;
        d.write(out);
    }
}

/**
sdevice::Load
---------------

The sdevice struct vector is populated by reading
from the single file until reaching EOF.

**/


inline void sdevice::Load( std::vector<sdevice>& devices )
{
    const char* dirpath = ResolveDirPath();
    std::string _path = Path(dirpath);
    const char* path = _path.c_str();

    if(level > 0) std::cout
        << "sdevice::Load"
        << " dirpath [" << ( dirpath ? dirpath : "-" ) << "]"
        << " path [" << ( path ? path : "-" ) << "]"
        << std::endl
        ;
    std::ifstream in(path, std::ofstream::binary);

    sdevice d ;

    while(true)
    {
        d.read(in);
        if(in.eof()) return ;
        if(in.fail())
        {
            if( level > 0 || PERSIST == 1 ) std::cerr
                << "sdevice::Load"
                << " failed read from "
                << " dirpath [" << ( dirpath ? dirpath : "-" ) << "]"
                << " path [" << ( path ? path : "-" ) << "]"
                << " PERSIST " << PERSIST
                << std::endl
                ;
            return ;
        }
        devices.push_back(d);
    }
}

inline std::string sdevice::Brief( const std::vector<sdevice>& devices )
{
    std::stringstream ss ;
    for(unsigned i=0 ; i < devices.size() ; i++)
    {
        const sdevice& d = devices[i] ;
        ss << d.ordinal << ':' ;
        for(unsigned j=0 ; j < strlen(d.name) ; j++)
        {
            char c = *(d.name+j) ;
            ss << ( c == ' ' ? '_' : c ) ;
        }
        if( i < devices.size() - 1 ) ss << ' ' ;
    }
    return ss.str();
}

inline std::string sdevice::Desc( const std::vector<sdevice>& devices )
{
    std::stringstream ss ;
    ss << "[" << Brief(devices) << "]" << std::endl  ;
    for(unsigned i=0 ; i < devices.size() ; i++)
    {
        const sdevice& d = devices[i] ;
        ss << d.desc() << std::endl ;
    }
    std::string str = ss.str();
    return str ;
}

inline std::string sdevice::VRAM( const std::vector<sdevice>& devices )
{
    std::stringstream ss ;
    for(unsigned i=0 ; i < devices.size() ; i++)
    {
        const sdevice& d = devices[i] ;
        ss << d.totalGlobalMem_bytes() << "\n" ;
    }
    std::string str = ss.str();
    return str ;
}

