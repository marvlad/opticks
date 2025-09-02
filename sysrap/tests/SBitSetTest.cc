#include "SBitSet.h"

const char* SPECS = R"LITERAL(
1,10,100,-1
~1,10,100,-1
t1,10,100,-1
t
~
~0
t0
t-1
~-1
0

0,1,2,3
~0,1,2,3
)LITERAL" ; 

void test_0()
{

    unsigned num_bits = 128 ; 
    bool dump = true ;

    bool* bits = new bool[num_bits] ; 

    std::stringstream ss(SPECS) ;    
    std::string line ; 
    while (std::getline(ss, line))  
    {   
        const char* spec = line.c_str(); 
        SBitSet::ParseSpec( num_bits, bits, spec, dump ); 
        std::cout 
            << std::setw(20) << spec 
            << " : "
            << SBitSet::Desc(num_bits, bits,  false )
            << std::endl 
            ; 
    }    
}

void test_1()
{
    SBitSet* bs = new SBitSet(128); 

    std::stringstream ss(SPECS) ;    
    std::string line ; 
    while (std::getline(ss, line))  
    {   
        const char* spec = line.c_str(); 

        bs->parse_spec(spec); 

        std::cout 
            << std::setw(20) << spec 
            << " : "
            << bs->desc()
            << std::endl 
            ; 

    }  

    delete bs ; 
}


void test_2()
{
    std::stringstream ss(SPECS) ;    
    std::string line ; 
    while (std::getline(ss, line))  
    {   
        const char* spec = line.c_str(); 
        SBitSet* bs = SBitSet::Create(128, spec); 
        std::cout 
            << std::setw(20) << spec 
            << " : "
            << bs->desc()
            << std::endl 
            ; 

        delete bs ; 
    }  
}


int main(int argc, char** argv)
{
    test_0(); 
    test_1(); 
    test_2(); 

    return 0 ; 
}
