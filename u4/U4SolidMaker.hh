#pragma once

#include <string>
#include <vector>
#include "G4ThreeVector.hh"
class G4VSolid ;
class G4MultiUnion ;
class G4Box ;

#include "U4_API_EXPORT.hh"
#include "plog/Severity.h"

struct U4_API U4SolidMaker
{
    static const plog::Severity LEVEL ;
    static const char* NAMES ;

    static const char* Name( const char* prefix, unsigned idx );
    static G4VSolid* PrimitiveClone( const G4VSolid* src, const char* prefix, unsigned idx);

    static bool  CanMake(const char* name);
    static const G4VSolid* Make(const char* name);
    static const G4VSolid* Make(const char* name, std::string& meta );

    static G4double Sagitta( G4double radius, G4double chord );
    static G4double SagittaMax( G4double radius, G4double sy, G4double sz );
    static G4double SagittaMax( G4double InnerRadius, G4double OuterRadius,  G4double sy, G4double sz );


    static const G4VSolid* JustOrb(const char* name);
    static const G4VSolid* LHCbRichSphMirr(const char* name);
    static const G4VSolid* LHCbRichFlatMirr(const char* name);

    static const G4VSolid* JustOrbOrbUnion(       const char* name);
    static const G4VSolid* JustOrbOrbIntersection(const char* name);
    static const G4VSolid* JustOrbOrbDifference(  const char* name);
    static const G4VSolid* JustOrbOrb_(const char* name, char op);
    static const G4VSolid* ThreeOrbUnion(const char* name );

    static const G4VSolid* SphereWithPhiCutDEV(const char* name);


    static const G4VSolid* GeneralSphereDEV(const char* name, std::string& meta );
    static const G4VSolid* SphereWithPhiSegment(const char* name);
    static const G4VSolid* SphereWithThetaSegment(const char* name);
    static const G4VSolid* AdditionAcrylicConstruction(const char* name);

    static const int XJfixtureConstruction_debug_mode ;
    static const G4VSolid* XJfixtureConstruction(const char* name);
    static const G4VSolid* AltXJfixtureConstruction(const char* name);
    static const G4VSolid* AltXJfixtureConstructionU(const char* name);
    static const G4VSolid* AltXJfixtureConstruction_(const char* name, const char* opt);
    static const G4VSolid* AnnulusBoxUnion(const char* name) ;
    static const G4VSolid* AnnulusTwoBoxUnion(const char* name) ;
    static const G4VSolid* AnnulusOtherTwoBoxUnion(const char* name) ;
    static const G4VSolid* AnnulusCrossTwoBoxUnion(const char* name) ;
    static const G4VSolid* AnnulusFourBoxUnion_(const char* name, G4double inner_radius) ;
    static const G4VSolid* AnnulusFourBoxUnion(const char* name) ;
    static const G4VSolid* CylinderFourBoxUnion(const char* name) ;

    static const G4VSolid* BoxFourBoxUnion_(const char* name, const char* opt );
    static const G4VSolid* BoxFourBoxUnion(const char* name);
    static const G4VSolid* BoxCrossTwoBoxUnion(const char* name );
    static const G4VSolid* BoxThreeBoxUnion(const char* name);

    static const G4VSolid* OrbOrbMultiUnion(const char* name);

    static const G4VSolid* GridMultiUnion_(const char* name, G4VSolid* item, double gridspace, int nx, int ny, int nz );
    static const G4VSolid* OrbGridMultiUnion(const char* name);
    static const G4VSolid* BoxGridMultiUnion(const char* name);

    static const G4VSolid* BoxFourBoxContiguous_(const char* name, const char* opt );
    static const G4VSolid* BoxFourBoxContiguous(const char* name);


    static G4VSolid* Uncoincide_Box_Box_Union( const G4VSolid* bbu  );
    static std::string Desc( const G4Box* box );
    static std::string Desc( const G4ThreeVector* v );

    enum { X, Y, Z, ERR } ;
    static int OneAxis( const G4ThreeVector* v );
    static double HalfLength( const G4Box* box, int axis );
    static void ChangeBoxHalfLength( G4Box* box, int axis, double delta );
    static void ChangeThreeVector( G4ThreeVector* v, int axis, double delta );


    static const int XJanchorConstruction_debug_mode ;
    static const G4VSolid* XJanchorConstruction(const char* name);
    static const G4VSolid* SJReceiverConstruction(const char* name);

    static const G4VSolid* BoxMinusTubs0(const char* name);
    static const G4VSolid* BoxMinusTubs1(const char* name);
    static const G4VSolid* BoxMinusOrb(const char* name);
    static const G4VSolid* UnionOfHemiEllipsoids(const char* name);
    static const G4VSolid* PolyconeWithMultipleRmin(const char* name);

    static void Extract( std::vector<long>& vals, const char* s );
    static bool StartsWith( const char* n, const char* q );

    static const G4VSolid* SphereIntersectBox(const char* name);
    static const G4VSolid* LocalFastenerAcrylicConstruction(const char* name);
    static const G4VSolid* AltLocalFastenerAcrylicConstruction(const char* name);
    static const G4VSolid* BltLocalFastenerAcrylicConstruction(const char* name);

    static const G4VSolid* WaterDistributer(const char* name);
    static const G4VSolid* AltWaterDistributer(const char* name);
    static       G4VSolid* WaterDistributerHelper(const char* name, double distance, double TubeR);
    static void            AltWaterDistributerHelper(G4MultiUnion* multiUnion, double distance, double TubeR, double yoffset );

};


