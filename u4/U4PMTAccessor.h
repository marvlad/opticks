#pragma once
/**
U4PMTAccessor.h  DUMMY PLACEHOLDER
======================================

This is a mock standin for "jcv PMTAccessor" 
for usage WITH_CUSTOM4 NOT:WITH_PMTSIM

See SPMTAccessor for a standalone accessor that 
actually provides almost the same as the PMTAccessor
from the monolith.

**/

#ifdef WITH_CUSTOM4
#include "C4IPMTAccessor.h"
#include <array>

struct U4PMTAccessor : public C4IPMTAccessor
{
    static constexpr const char* TypeName = "U4PMTAccessor" ;

    // C4IPMTAccessor interface
    int    get_num_lpmt() const override ;
    double get_pmtid_qe( int pmtid, double energy ) const override ;
    double get_qescale(  int pmtid ) const override ;
    int    get_pmtcat( int pmtid  ) const override ;
    void   get_stackspec( std::array<double, 16>& ss, int pmtcat, double energy_eV ) const override ;
    const char* get_typename() const override ;

    // Missing pure virtuals from C4IPMTAccessor
    void   get_stackspec_pmtid_theta_deg( std::array<double, 16>& ss, int pmtcat, int pmtid, double energy_eV, double theta_deg ) const override ;
    double get_pmtid_qe_angular(int pmtid,double energy, double lposcost, double minus_cos_theta_aoi) const override ;
    int    get_implementation_version() const override ;
    void   set_implementation_version(int v) override ;

private:
    int impl_version = 1 ;  // store implementation version
};


inline int U4PMTAccessor::get_num_lpmt() const
{
    return 1000 ;
}
inline double U4PMTAccessor::get_pmtid_qe( int /*pmtid*/, double /*energy*/ ) const
{
    return 1. ;
}
inline double U4PMTAccessor::get_qescale( int /*pmtid*/ ) const
{
    return 1. ;
}
inline int U4PMTAccessor::get_pmtcat( int /*pmtid*/ ) const
{
    return 1 ;
}

inline void U4PMTAccessor::get_stackspec( std::array<double, 16>& ss, int /*pmtcat*/, double /*energy_eV*/ ) const
{
    ss.fill(0.);
    ss[4*0+0] = 1.482 ; // PyrexRINDEX

    ss[4*1+0] = 1.920 ; // ARC_RINDEX
    ss[4*1+1] = 0.000 ; // ARC_KINDEX 
    ss[4*1+2] = 36.49 ; // ARC_THICKNESS (nm) 

    ss[4*2+0] = 2.429 ; // PHC_RINDEX 
    ss[4*2+1] = 1.366 ; // PHC_KINDEX 
    ss[4*2+2] = 21.13 ; // PHC_THICKNESS (nm) 

    ss[4*3+0] = 1.000 ;  // VacuumRINDEX
}

inline const char* U4PMTAccessor::get_typename() const
{
    return TypeName ;
}

// New methods to satisfy abstract interface

inline void U4PMTAccessor::get_stackspec_pmtid_theta_deg(
    std::array<double,16>& ss,
    int pmtcat,
    int /*pmtid*/,
    double energy_eV,
    double /*theta_deg*/) const
{
    // just ignore pmtid, theta and delegate to simpler version
    get_stackspec(ss, pmtcat, energy_eV);
}

inline double U4PMTAccessor::get_pmtid_qe_angular(
    int pmtid,
    double energy,
    double /*lposcost*/,
    double /*minus_cos_theta_aoi*/) const
{
    // ignore angular dependence for now
    return get_pmtid_qe(pmtid, energy);
}

inline int U4PMTAccessor::get_implementation_version() const
{
    return impl_version ;
}

inline void U4PMTAccessor::set_implementation_version(int v)
{
    impl_version = v ;
}

#endif
