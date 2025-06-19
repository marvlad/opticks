/**
U4StepPoint.cc
===============

Boundary class changes need to match in all the below::

    U4OpBoundaryProcess.h
    U4Physics.cc
    U4Recorder.cc
    U4StepPoint.cc

**/


#include <sstream>

#include "G4StepPoint.hh"
#include "G4VProcess.hh"
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"

#include "SLOG.hh"
#include "SSys.hh"
#include "OpticksPhoton.h"
#include "OpticksPhoton.hh"
#include "scuda.h"
#include "squad.h"
#include "sphoton.h"
#include "SFastSimOpticalModel.hh"

#include "U4StepStatus.h"
#include "U4OpBoundaryProcess.h"
#include "U4OpBoundaryProcessStatus.h"
#include "U4Physics.hh"
#include "U4StepPoint.hh"


const plog::Severity U4StepPoint::LEVEL = SLOG::EnvLevel("U4StepPoint", "DEBUG");
const char* U4StepPoint::OpFastSim_ = SSys::getenvvar("U4StepPoint_OpFastSim", "fast_sim_man" );


/**
U4StepPoint::Update
---------------------

* cf CWriter::writeStepPoint_

**/

void U4StepPoint::Update(sphoton& photon, const G4StepPoint* point)  // static
{
    const G4ThreeVector& pos = point->GetPosition();
    const G4ThreeVector& mom = point->GetMomentumDirection();
    const G4ThreeVector& pol = point->GetPolarization();

    G4double time = point->GetGlobalTime();
    G4double energy = point->GetKineticEnergy();
    G4double wavelength = h_Planck*c_light/energy ;

    photon.pos.x = pos.x();
    photon.pos.y = pos.y();
    photon.pos.z = pos.z();
    photon.time  = time/ns ;

    photon.mom.x = mom.x();
    photon.mom.y = mom.y();
    photon.mom.z = mom.z();
    //photon.iindex = 0u ;

    photon.pol.x = pol.x();
    photon.pol.y = pol.y();
    photon.pol.z = pol.z();
    photon.wavelength = wavelength/nm ;
}

std::string U4StepPoint::DescPositionTime(const G4StepPoint* point )
{
    const G4ThreeVector& pos = point->GetPosition();
    G4double time = point->GetGlobalTime();
    std::stringstream ss ;

    ss << "U4StepPoint::DescPositionTime ("
       << " " << std::setw(10) << std::fixed << std::setw(10) << std::setprecision(3) << pos.x()
       << " " << std::setw(10) << std::fixed << std::setw(10) << std::setprecision(3) << pos.y()
       << " " << std::setw(10) << std::fixed << std::setw(10) << std::setprecision(3) << pos.z()
       << " " << std::setw(10) << std::fixed << std::setw(10) << std::setprecision(3) << time/ns
       << ")"
       ;

    std::string s = ss.str();
    return s ;
}

const char* U4StepPoint::ProcessName(const G4StepPoint* point) // static
{
    const G4VProcess* process = point->GetProcessDefinedStep() ;
    if(process == nullptr) return nullptr ;
    const G4String& processName = process->GetProcessName() ;
    return processName.c_str();
}

unsigned U4StepPoint::ProcessDefinedStepType(const G4StepPoint* point) // static
{
    const G4VProcess* process = point->GetProcessDefinedStep() ;
    if(process == nullptr) return U4StepPoint_NoProc ;
    const G4String& processName = process->GetProcessName() ;
    return ProcessDefinedStepType( processName.c_str() );
}

unsigned U4StepPoint::ProcessDefinedStepType(const char* name) // static
{
    unsigned type = U4StepPoint_Undefined ;
    if(strcmp(name, NoProc_) == 0 )         type = U4StepPoint_NoProc ;
    if(strcmp(name, Transportation_) == 0 ) type = U4StepPoint_Transportation ;
    if(strcmp(name, OpRayleigh_) == 0)      type = U4StepPoint_OpRayleigh ;
    if(strcmp(name, OpAbsorption_) == 0)    type = U4StepPoint_OpAbsorption ;
    if(strcmp(name, OpFastSim_) == 0)       type = U4StepPoint_OpFastSim ;
    if(strcmp(name, OTHER_) == 0)           type = U4StepPoint_OTHER ;
    return type ;
}

const char* U4StepPoint::ProcessDefinedStepTypeName( unsigned type ) // static
{
    const char* s = nullptr ;
    switch(type)
    {
        case U4StepPoint_Undefined:      s = Undefined_      ; break ;
        case U4StepPoint_NoProc:         s = NoProc_         ; break ;
        case U4StepPoint_Transportation: s = Transportation_ ; break ;
        case U4StepPoint_OpRayleigh:     s = OpRayleigh_     ; break ;
        case U4StepPoint_OpAbsorption:   s = OpAbsorption_   ; break ;
        case U4StepPoint_OpFastSim:      s = OpFastSim_      ; break ;
        default:                         s = OTHER_          ; break ;
    }
    return s ;
}


template <typename T>
bool U4StepPoint::IsTransportationBoundary(const G4StepPoint* point)
{
    G4StepStatus status = point->GetStepStatus()  ;
    unsigned proc = ProcessDefinedStepType(point);
    return status == fGeomBoundary && proc == U4StepPoint_Transportation ;
}



/**
U4StepPoint::Flag
------------------

Adapted from cfg4/OpStatus.cc:OpStatus::OpPointFlag

Q: Why does this never return BULK_REEMIT ?
A: As that always starts as BULK_ABSORB and then gets changed into BULK_REEMIT
   by subsequent history rewriting when another track comes along with
   the corresponding ancestry.

**/

template <typename T>
unsigned U4StepPoint::Flag(const G4StepPoint* point, bool warn, bool& tir )
{
    G4StepStatus status = point->GetStepStatus()  ;
    unsigned proc = ProcessDefinedStepType(point);
    unsigned flag = 0 ;
    tir = false ;

    if( status == fPostStepDoItProc && proc == U4StepPoint_OpAbsorption )
    {
        flag = BULK_ABSORB ;
    }
    else if( status == fPostStepDoItProc && proc == U4StepPoint_OpRayleigh )
    {
        flag = BULK_SCATTER ;
    }
    else if( status == fGeomBoundary && proc == U4StepPoint_Transportation )
    {
        unsigned bstat = U4OpBoundaryProcess::GetStatus<T>();
        T* proc = U4OpBoundaryProcess::Get<T>();

        LOG_IF( error, bstat == Undefined )
            << " U4OpBoundaryProcess::GetStatus<T>() : Undefined "
            << std::endl
            << " U4OpBoundaryProcess::Get<T>() " << ( proc ? "YES" : "NO " )
            << std::endl
            << " U4Physics::Switches() "
            << std::endl
            << U4Physics::Switches()
            ;

        tir = bstat == TotalInternalReflection ;

        flag = BoundaryFlag(bstat) ;   // BT BR NA SA SD SR DR

        LOG_IF(error, flag == 0 )   // NAN_ABORT are common from StepTooSmall
            << " UNEXPECTED BoundaryFlag ZERO  "
            << std::endl
            << " flag " << flag
            << " OpticksPhoton::Flag(flag) " << OpticksPhoton::Flag(flag)
            << std::endl
            << " bstat " << bstat
            << " U4OpBoundaryProcessStatus::Name(bstat) " << U4OpBoundaryProcessStatus::Name(bstat)
            ;
    }
    else if( status == fGeomBoundary && proc == U4StepPoint_OpFastSim )
    {
        flag = DEFER_FSTRACKINFO ; // signal that must get FastSim DoIt status from trackinfo label
    }
    else if( status == fWorldBoundary && proc == U4StepPoint_Transportation )
    {
        flag = MISS ;
    }
    else
    {
        if(warn)
        {
            const char* procName = ProcessName(point);
            LOG(error)
                << " failed to define flag for StepPoint "
                << " G4StepStatus " << U4StepStatus::Name(status)
                << " proc " << ProcessDefinedStepTypeName(proc )
                << " procName " << ( procName ? procName : "-" )
                ;
        }

    }
    return flag ;
}

/**
U4StepPoint::BoundaryFlag
--------------------------

Canonical stack::

    U4StepPoint::BoundaryFlag
    U4StepPoint::Flag
    U4Recorder::UserSteppingAction_Optical
    U4Recorder::UserSteppingAction

**/

unsigned U4StepPoint::BoundaryFlag(unsigned status) // BT BR NA SA SD SR DR
{
    unsigned flag = 0 ;
    switch(status)
    {
        case FresnelRefraction:
        case SameMaterial:
        case Transmission:
                               flag=BOUNDARY_TRANSMIT;
                               break;
        case TotalInternalReflection:
        case       FresnelReflection:
                               flag=BOUNDARY_REFLECT;
                               break;
        case StepTooSmall:
                               flag=NAN_ABORT;
                               break;
        case Absorption:
                               flag=SURFACE_ABSORB ;
                               break;
        case Detection:
                               flag=SURFACE_DETECT ;
                               break;
        case SpikeReflection:
                               flag=SURFACE_SREFLECT ;
                               break;
        case LobeReflection:
        case LambertianReflection:
                               flag=SURFACE_DREFLECT ;
                               break;
        case NoRINDEX:
                               flag=SURFACE_ABSORB ;
                               //flag=NAN_ABORT;
                               break;
        case Undefined:
        case BackScattering:
        case NotAtBoundary:
        case PolishedLumirrorAirReflection:
        case PolishedLumirrorGlueReflection:
        case PolishedAirReflection:
        case PolishedTeflonAirReflection:
        case PolishedTiOAirReflection:
        case PolishedTyvekAirReflection:
        case PolishedVM2000AirReflection:
        case PolishedVM2000GlueReflection:
        case EtchedLumirrorAirReflection:
        case EtchedLumirrorGlueReflection:
        case EtchedAirReflection:
        case EtchedTeflonAirReflection:
        case EtchedTiOAirReflection:
        case EtchedTyvekAirReflection:
        case EtchedVM2000AirReflection:
        case EtchedVM2000GlueReflection:
        case GroundLumirrorAirReflection:
        case GroundLumirrorGlueReflection:
        case GroundAirReflection:
        case GroundTeflonAirReflection:
        case GroundTiOAirReflection:
        case GroundTyvekAirReflection:
        case GroundVM2000AirReflection:
        case GroundVM2000GlueReflection:
        case Dichroic:
                               flag=0;   // leads to bad flag asserts
                               break;
    }
    return flag ;
}


template <typename T>
std::string U4StepPoint::Desc(const G4StepPoint* point)
{
    G4StepStatus status = point->GetStepStatus()  ;
    const char* statusName = U4StepStatus::Name(status);

    unsigned proc = ProcessDefinedStepType(point);
    const char* procName = ProcessDefinedStepTypeName(proc);
    const char* procNameRaw = ProcessName(point);

    unsigned bstat = U4OpBoundaryProcess::GetStatus<T>();
    const char* bstatName = U4OpBoundaryProcessStatus::Name(bstat);

    bool warn = false ;
    bool is_tir = false ;
    unsigned flag = Flag<T>(point, warn, is_tir);
    const char* flagName = OpticksPhoton::Flag(flag);

    std::stringstream ss ;
    ss << "U4StepPoint::Desc"
       << std::endl
       << " proc " << proc
       << " procName " << procName
       << " procNameRaw " << ( procNameRaw ? procNameRaw : "-" )
       << std::endl
       << " status " << status
       << " statusName " << statusName
       << std::endl
       << " bstat " << bstat
       << " bstatName " << bstatName
       << " is_tir " << is_tir
       << std::endl
       << " flag " << flag
       << " flagName " << flagName
       ;
    std::string s = ss.str();
    return s ;
}

#if defined(WITH_CUSTOM4)
template unsigned U4StepPoint::Flag<C4OpBoundaryProcess>(const G4StepPoint*, bool, bool& );
template std::string U4StepPoint::Desc<C4OpBoundaryProcess>(const G4StepPoint* point);
#elif defined(WITH_PMTSIM)
template unsigned U4StepPoint::Flag<CustomG4OpBoundaryProcess>(const G4StepPoint*, bool, bool& );
template std::string U4StepPoint::Desc<CustomG4OpBoundaryProcess>(const G4StepPoint* point);
#elif defined(WITH_INSTRUMENTED_DEBUG)
template unsigned U4StepPoint::Flag<InstrumentedG4OpBoundaryProcess>(const G4StepPoint*, bool, bool& );
template std::string U4StepPoint::Desc<InstrumentedG4OpBoundaryProcess>(const G4StepPoint* point);
#else
template unsigned U4StepPoint::Flag<G4OpBoundaryProcess>(const G4StepPoint*, bool, bool& );
template std::string U4StepPoint::Desc<G4OpBoundaryProcess>(const G4StepPoint* point);
#endif


