#pragma once
/**
SEvent.hh
===========

ALL THESE ARE STATIC GENSTEP UTILITIES

BUT CANNOT MOVE THEM ALL TO HEADER ONLY SGenstep.h
AND DELETE SEvent.{hh,cc} BECAUSE THIS USES SEventConfig
COMPLICATING THINGS

**/

struct NP ;
struct quad4 ;
struct quad6 ;
struct storch ;
struct uint4 ;
struct sframe ;

template <typename T> struct Tran ;

#include <vector>
#include "plog/Severity.h"
#include "SYSRAP_API_EXPORT.hh"

#include "sframe.h"

struct SYSRAP_API SEvent
{
    static const plog::Severity LEVEL ;

    static NP* GENSTEP ;
    static NP*  GetGENSTEP();
    static void SetGENSTEP(NP* gs);
    static bool HasGENSTEP();

    static NP* HIT ;
    static NP*  GetHIT();
    static void SetHIT(NP* gs);
    static bool HasHIT();



    static NP*   MakeInputPhotonGenstep( const NP* input_photon, int gentype, const sframe* fr );
    static quad6 MakeInputPhotonGenstep_(const NP* input_photon, int gentype, const sframe* fr );

    static NP* MakeDemoGenstep(const char* config=nullptr, int idx_arg=-1);
    static NP* MakeTorchGenstep(int idx_arg=-1);
    static NP* MakeCerenkovGenstep(int idx_arg=-1);
    static NP* MakeScintGenstep(int idx_arg=-1);
    static NP* MakeCarrierGenstep(int idx_arg=-1);
    static NP* MakeGenstep(int gentype, int idx_arg=-1);

    template <typename T>
    static void FillGenstep( NP* gs, int gs_start, int gs_stop, int numphoton_per_genstep, bool dump ) ;


    static NP* MakeSeed( const NP* gs );


    static NP* MakeCountGenstep(const char* config=nullptr, int* total=nullptr);
    static unsigned SumCounts(const std::vector<int>& counts);

    static void ExpectedSeeds(std::vector<int>& seeds, const std::vector<int>& counts );
    static int  CompareSeeds( const int* seeds, const int* xseeds, int num_seed );
    static std::string DescSeed( const int* seed, int num_seed, int edgeitems );


    static NP* MakeCountGenstep(const std::vector<int>& photon_counts_per_genstep, int* total );
};


