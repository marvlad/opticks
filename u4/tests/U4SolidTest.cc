#include <csignal>
#include "G4Orb.hh"
#include "G4Box.hh"
#include "G4UnionSolid.hh"
#include "G4RotationMatrix.hh"

#include "U4Solid.h"
#include "U4SolidMaker.hh"

#include "ssys.h"
#include "sn.h"
#include "s_csg.h"


struct U4SolidTest
{
    static void Setup();

    static constexpr const char* _Convert_level = "U4SolidTest__Convert_level" ;
    static int Convert(const G4VSolid* solid);
    static int Orb();
    static int Box();
    static int Uni();

    static constexpr const char* _MAKE = "U4SolidTest__MAKE" ;
    static int MAKE();
    static int ALL();

    static int Main();
};


void U4SolidTest::Setup()
{
    s_csg* csg = new s_csg ;
    assert(csg);
    if(!csg) std::raise(SIGINT);
}

int U4SolidTest::Convert(const G4VSolid* solid )
{
    int lvid = 0 ;
    int depth = 0 ;
    int level = ssys::getenvint(_Convert_level,1) ;
    sn* nd = U4Solid::Convert(solid, lvid, depth, level);
    std::cout
        << "[U4SolidTest::Convert nd.desc\n"
        <<   nd->desc()
        << "]U4SolidTest::Convert nd.desc\n"
        ;

    if(level > 2 ) std::cout
        << "\nU4SolidTest::Convert nd->render() \n\n"
        << nd->render()
        << "\n"
        ;

    if(level > 3 ) std::cout
        << "\nU4SolidTest::Convert nd->detail_r() \n\n"
        << nd->detail_r()
        << "\n"
        ;

    if(level > 3 ) std::cout
        << "\nU4SolidTest::Convert nd->desc_prim_all() \n\n"
        << nd->desc_prim_all(false)
        << "\n"
        ;

    std::cout << "sn::Desc.0.before-delete-expect-some-nodes\n"  << sn::Desc() << "\n" ;

    delete nd ;
    std::cout << "sn::Desc.1.after-delete-expect-no-nodes\n"  << sn::Desc() << "\n" ;


    return 0 ;
}

int U4SolidTest::Orb()
{
    G4Orb* orb = new G4Orb("orb", 100.) ;
    return Convert(orb);
}
int U4SolidTest::Box()
{
    G4Box* box = new G4Box("box", 100., 200., 300. ) ;
    return Convert(box);
}
int U4SolidTest::Uni()
{
    G4Orb* orb = new G4Orb("orb", 100.) ;
    G4Box* box = new G4Box("box", 100., 200., 300. ) ;

    G4RotationMatrix* rot = new G4RotationMatrix(
            G4ThreeVector(0.707107,-0.707107,0.000000),
            G4ThreeVector(0.707107, 0.707107,0.000000),
            G4ThreeVector(0.000000, 0.000000,1.000000)
            );
    G4ThreeVector tla(50.,60.,70.);
    G4UnionSolid* orb_box = new G4UnionSolid( "orb_box", orb, box, rot, tla );

    return Convert(orb_box);
}


int U4SolidTest::MAKE()
{
    const char* make = ssys::getenvvar(_MAKE, "LocalFastenerAcrylicConstruction4" );
    const G4VSolid* solid = U4SolidMaker::Make(make) ;
    return Convert(solid);
}

int U4SolidTest::ALL()
{
    int rc = 0 ;
    rc += Orb();
    rc += Box();
    rc += Uni();
    rc += MAKE();
    return rc ;
}

int U4SolidTest::Main()
{
    Setup();
    const char* TEST = ssys::getenvvar("TEST", "Orb");
    int rc = 0 ;
    if(     strcmp(TEST, "Orb") == 0 )  rc = Orb();
    else if(strcmp(TEST, "Box") == 0 )  rc = Box();
    else if(strcmp(TEST, "Uni") == 0 )  rc = Uni();
    else if(strcmp(TEST, "ALL") == 0 )  rc = ALL();
    else if(strcmp(TEST, "MAKE") == 0 )  rc = MAKE();
    return rc ;
}

int main(int argc, char** argv)
{
    return U4SolidTest::Main();
}


