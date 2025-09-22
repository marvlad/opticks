#pragma once
/**
SPMTAccessor.h
===============

Provides access to JUNO PMT data during standalone 
optical only testing WITH_CUSTOM4 and without j/PMTSim.
**/

#include <cstdlib>
#include <array>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cassert>
#include "SPMT.h"

#ifdef WITH_CUSTOM4 
#include "C4IPMTAccessor.h"
struct SPMTAccessor : public C4IPMTAccessor
#else
struct SPMTAccessor
#endif
{
    static constexpr const char* TYPENAME = "SPMTAccessor" ;
    static SPMTAccessor* Load(const char* path);
    SPMTAccessor(const SPMT* pmt );
    const SPMT* pmt ;
    bool VERBOSE ;

    //[C4IPMTAccessor protocol methods
    int         get_num_lpmt() const ;
    double      get_qescale( int pmtid ) const ;
    int         get_pmtcat( int pmtid ) const ;
    double      get_pmtid_qe( int pmtid, double energy_MeV ) const ;
    void        get_stackspec( std::array<double, 16>& ss, int pmtcat, double energy_eV ) const ;
    const char* get_typename() const ;

#ifdef WITH_CUSTOM4
    // Implement the pure virtuals from C4IPMTAccessor
    void        get_stackspec_pmtid_theta_deg( std::array<double, 16>& ss, int pmtcat, int pmtid, double energy_eV, double theta_deg ) const override ;
    double      get_pmtid_qe_angular(int pmtid,double energy, double lposcost, double minus_cos_theta_aoi) const override ;
    int         get_implementation_version() const override ;
    void        set_implementation_version(int v) override ;
#endif
    //]

    static std::string Desc(std::array<double, 16>& ss );
};

inline SPMTAccessor* SPMTAccessor::Load(const char* path)
{
    SPMT* pmt = SPMT::Load(path);
    if(pmt == nullptr) return nullptr ;

    SPMTAccessor* accessor = new SPMTAccessor(pmt);
    assert( accessor );
    return accessor ;
}

inline SPMTAccessor::SPMTAccessor( const SPMT* _pmt )
    :
    pmt(_pmt),
    VERBOSE(getenv("SPMTAccessor__VERBOSE") != nullptr)
{
}

inline int SPMTAccessor::get_num_lpmt() const
{
    return SPMT::NUM_LPMT ;
}

inline double SPMTAccessor::get_qescale( int pmtid ) const
{
    float qs = pmt->get_qescale(pmtid);
    return qs ;
}
inline int SPMTAccessor::get_pmtcat( int pmtid ) const
{
    int cat = pmt->get_lpmtcat(pmtid) ;   // assumes pmtid is for LPMT  

    if(VERBOSE) std::cout
        << "SPMTAccessor::get_pmtcat"
        << " pmtid " << pmtid
        << " cat " << cat
        << std::endl
        ;

    return cat ;
}

inline double SPMTAccessor::get_pmtid_qe( int pmtid, double energy_MeV ) const
{
    float energy_eV = energy_MeV*1e6 ;
    float qe = pmt->get_pmtid_qe(pmtid, energy_eV) ;

    if(VERBOSE) std::cout
        << "SPMTAccessor::get_pmtid_qe"
        << " pmtid " << pmtid
        << " energy_MeV " << std::scientific << energy_MeV
        << " energy_eV " << std::scientific << energy_eV
        << " qe " << std::scientific << qe
        << std::endl
        ;

    return qe ;
}

inline void SPMTAccessor::get_stackspec( std::array<double, 16>& spec, int pmtcat, double energy_eV_ ) const
{
    float energy_eV = energy_eV_ ;
    quad4 q_spec ;
    pmt->get_stackspec(q_spec, pmtcat, energy_eV);

    const float* qq = q_spec.cdata();
    for(int i=0 ; i < 16 ; i++) spec[i] = double(qq[i]) ;
}

inline std::string SPMTAccessor::Desc(std::array<double, 16>& spec ) // static
{
    std::stringstream ss ;
    ss << "SPMTAccessor::Desc" << std::endl ;
    for(int i=0 ; i < 16 ; i++) ss
        << ( i % 4 == 0 ? "\n" : " " )
        << std::setw(10) << std::fixed  << spec[i] << " "
        << ( i == 15 ? "\n" : " " )
        ;

    std::string str = ss.str() ;
    return str ;
}

inline const char* SPMTAccessor::get_typename() const
{
    return TYPENAME ;
}

#ifdef WITH_CUSTOM4
inline void SPMTAccessor::get_stackspec_pmtid_theta_deg(
    std::array<double, 16>& ss,
    int pmtcat,
    int /*pmtid*/,
    double energy_eV,
    double /*theta_deg*/) const
{
    // Just ignore theta for now and call the simpler version
    get_stackspec(ss, pmtcat, energy_eV);
}

inline double SPMTAccessor::get_pmtid_qe_angular(
    int pmtid,
    double energy,
    double /*lposcost*/,
    double /*minus_cos_theta_aoi*/) const
{
    // Ignore angular dependence for now
    return get_pmtid_qe(pmtid, energy*1e-6); // expects MeV
}

inline int SPMTAccessor::get_implementation_version() const
{
    return 1; // placeholder
}

inline void SPMTAccessor::set_implementation_version(int v)
{
    if(VERBOSE) std::cout
        << "SPMTAccessor::set_implementation_version " << v
        << std::endl;
}
#endif
