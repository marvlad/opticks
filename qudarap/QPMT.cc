/**
QPMT.cc
==========

QPMT::init
QPMT::init_thickness
QPMT::init_lcqs
    prep hostside qpmt.h instance and upload to device at d_pmt

QPMT::lpmtcat_check
    check domain and lookup shape consistency

QPMT::lpmtcat_scan
   interface in .cc to kernel launcher QPMT_lpmtcat_scan in .cu

QPMT::mct_lpmtid_
   interface in .cc to kernel launcher QPMT_mct_lpmtid in .cu


**/

#include "SLOG.hh"

#if defined(MOCK_CURAND) || defined(MOCK_CUDA)
#else
#include <cuda_runtime.h>
#endif

#include <csignal>
#include <vector_types.h>
#include "QPMT.hh"


#if defined(MOCK_CURAND) || defined(MOCK_CUDA)
#include "QPMT_MOCK.h"

template<typename T>
const plog::Severity QPMT<T>::LEVEL = plog::info ;
#else
template<typename T>
const plog::Severity QPMT<T>::LEVEL = SLOG::EnvLevel("QPMT", "DEBUG");
#endif

template<typename T>
const QPMT<T>* QPMT<T>::INSTANCE = nullptr ;

template<typename T>
const QPMT<T>* QPMT<T>::Get(){ return INSTANCE ;  }



template<typename T>
inline std::string QPMT<T>::Desc() // static
{
    std::stringstream ss ;
    ss << "QPMT<" << ( sizeof(T) == 4 ? "float" : "double" ) << "> " ;
#ifdef WITH_CUSTOM4
    ss << "WITH_CUSTOM4 " ;
#else
    ss << "NOT:WITH_CUSTOM4 " ;
#endif
    ss << " INSTANCE:" << ( INSTANCE ? "YES" : "NO " ) << " " ;
    if(INSTANCE) ss << INSTANCE->desc() ;
    std::string str = ss.str();
    return str ;
}

/**
QPMT::init
------------

1. populate hostside qpmt.h instance with device side pointers
2. upload the hostside qpmt.h instance to GPU
3. retain d_pmt pointer to use in launches

**/

template<typename T>
inline void QPMT<T>::init()
{
    LOG(LEVEL) << "[" ;

    INSTANCE = this ;

    const int ni = s_pmt::NUM_CAT ;   // 3:NNVT/HAMA/NNVT_HiQE
    const int nj = s_pmt::NUM_LAYR ;  // 4:Pyrex/ARC/PHC/Vacuum
    const int nk = s_pmt::NUM_PROP ;  // 2:RINDEX/KINDEX
                                    // -----------------
                                    // 3*4*2 = 24

    bool src_rindex_expect = src_rindex->has_shape(ni, nj, nk, -1, 2 ) ;
    bool src_thickness_expect = src_thickness->has_shape(ni, nj, 1 ) ;

    assert( src_rindex_expect);
    assert( src_thickness_expect );

    if(!src_rindex_expect) std::raise(SIGINT);
    if(!src_thickness_expect) std::raise(SIGINT);

    init_prop();
    init_thickness();
    init_lcqs();

#if defined(MOCK_CURAND) || defined(MOCK_CUDA)
    d_pmt = pmt ;
#else
    d_pmt = QU::UploadArray<qpmt<T>>( (const qpmt<T>*)pmt, 1u, "QPMT::init/d_pmt" ) ;
    // getting above line to link required template instanciation at tail of qpmt.h
#endif
    LOG(LEVEL) << "]" ;
}

template<typename T>
inline void QPMT<T>::init_prop()
{
    pmt->rindex_prop = rindex_prop->getDevicePtr() ;
    pmt->qeshape_prop = qeshape_prop->getDevicePtr() ;
    pmt->cetheta_prop = cetheta_prop->getDevicePtr() ;
    pmt->cecosth_prop = cecosth_prop->getDevicePtr() ;
}


template<typename T>
inline void QPMT<T>::init_thickness()
{
    const char* label = "QPMT::init_thickness/d_thickness" ;

#if defined(MOCK_CURAND) || defined(MOCK_CUDA)
    T* d_thickness = const_cast<T*>(thickness->cvalues<T>()) ;
#else
    T* d_thickness = QU::UploadArray<T>(thickness->cvalues<T>(), thickness->num_values(), label ); ;
#endif

    pmt->thickness = d_thickness ;
}

template<typename T>
inline void QPMT<T>::init_lcqs()
{
    LOG(LEVEL)
       << " src_lcqs " << ( src_lcqs ? src_lcqs->sstr() : "-" )
       << " lcqs " << ( lcqs ? lcqs->sstr() : "-" )
       ;

    const char* label = "QPMT::init_lcqs/d_lcqs" ;

#if defined(MOCK_CURAND) || defined(MOCK_CUDA)
    T* d_lcqs = lcqs ? const_cast<T*>(lcqs->cvalues<T>()) : nullptr ;
#else
    T* d_lcqs = lcqs ? QU::UploadArray<T>(lcqs->cvalues<T>(), lcqs->num_values(), label) : nullptr ;
#endif

    pmt->lcqs = d_lcqs ;
    pmt->i_lcqs = (int*)d_lcqs ;   // HMM: would cause issues with T=double
}



/**
NB these decls cannot be extern "C" as need C++ name mangling for template types
**/

#if defined(MOCK_CURAND) || defined(MOCK_CUDA)

template <typename F>
extern void QPMT_lpmtcat_MOCK(
    qpmt<F>* pmt,
    int etype,
    F* lookup,
    const F* domain,
    unsigned domain_width
);

template <typename T>
extern void QPMT_mct_lpmtid_MOCK(
    qpmt<T>* pmt,
    int etype,
    T* lookup,
    const T* domain,
    unsigned domain_width,
    const int* lpmtid,
    unsigned num_lpmtid
);


#else
template <typename T>
extern void QPMT_lpmtcat_scan(
    dim3 numBlocks,
    dim3 threadsPerBlock,
    qpmt<T>* pmt,
    int etype,
    T* lookup,
    const T* domain,
    unsigned domain_width
);

template <typename T>
extern void QPMT_mct_lpmtid_scan(
    dim3 numBlocks,
    dim3 threadsPerBlock,
    qpmt<T>* pmt,
    int etype,
    T* lookup,
    const T* domain,
    unsigned domain_width,
    const int* lpmtid,
    unsigned num_lpmtid
);
#endif




template<typename T>
void QPMT<T>::lpmtcat_check_domain_lookup_shape( int etype, const NP* domain, const NP* lookup) const
{
    const char* elabel = qpmt_enum::Label(etype) ;
    bool domain_expect = domain->shape.size() == 1 && domain->shape[0] > 0 ;

    LOG_IF(fatal, !domain_expect)
        << " etype " << etype
        << " elabel " << elabel
        << " FATAL UNEXPECTED DOMAIN SHAPE "
        << " domain.sstr " << ( domain ? domain->sstr() : "-" )
        ;

    assert( domain_expect );
    if(!domain_expect) std::raise(SIGINT);

    unsigned num_domain = domain->shape[0] ;
    unsigned num_domain_1 = 0 ;

    switch(etype)
    {
        case qpmt_RINDEX : num_domain_1 = lookup->shape[lookup->shape.size()-1] ; break ;
        case qpmt_QESHAPE: num_domain_1 = lookup->shape[lookup->shape.size()-1] ; break ;
        case qpmt_CETHETA: num_domain_1 = lookup->shape[lookup->shape.size()-1] ; break ;
        case qpmt_CECOSTH: num_domain_1 = lookup->shape[lookup->shape.size()-1] ; break ;
        case qpmt_CATSPEC: num_domain_1 = lookup->shape[lookup->shape.size()-3] ; break ; // (4,4) payload
    }

    bool num_domain_expect = num_domain == num_domain_1 ;

    LOG_IF(fatal, !num_domain_expect)
        << " etype " << etype
        << " elabel " << elabel
        << " FATAL DOMAIN MISMATCH "
        << " num_domain " << num_domain
        << " num_domain_1 " << num_domain_1
        ;

    assert( num_domain_expect );
    if(!num_domain_expect) std::raise(SIGINT) ;

}

template<typename T>
const T* QPMT<T>::Upload(const NP* arr, const char* label)
{
    unsigned num_arr = arr->shape[0] ;
#if defined(MOCK_CURAND) || defined(MOCK_CUDA)
    const T* d_arr = arr->cvalues<T>() ;
#else
    const T* d_arr = QU::UploadArray<T>( arr->cvalues<T>(), num_arr, label ) ;
#endif
    return d_arr ;
}


template<typename T>
T* QPMT<T>::Alloc(NP* out, const char* label)
{
    unsigned num_values = out->num_values() ;
#if defined(MOCK_CURAND) || defined(MOCK_CUDA)
    T* d_arr = out->values<T> ;
#else
    T* d_arr = QU::device_alloc<T>(num_values,label) ;
#endif
    return d_arr ;
}



/**
QPMT::lpmtcat_scan
--------------------

Canonical usage from QPMTTest.h make_qscan

1. create hostside lookup array for the output
2. upload domain array to d_domain
3. allocate lookup array at d_lookup
4. invoke QPMT_lpmtcat launch using d_pmt pointer argument
5. copy d_lookup to h_lookup

The shape of the lookup output depends on the etype.
For some etype the lookup contains energy_eV scans for all pmt cat (3),
layers (4) and props (2) (RINDEX, KINDEX) resulting in
lookup shape of (3,4,2, domain_width ).

The lookup are populated with nested loops (eg 3*4*2=24 props) in the kernel
with the energy domain passed in as input. Parallelism is over the energy.

**/

template<typename T>
NP* QPMT<T>::lpmtcat_scan(int etype, const NP* domain ) const
{
    const char* elabel = qpmt_enum::Label(etype) ;

    unsigned num_domain = domain->shape[0] ;
    NP* lookup = MakeArray_lpmtcat(etype, num_domain );
    lpmtcat_check_domain_lookup_shape(etype, domain, lookup) ;
    unsigned lookup_num_values = lookup->num_values() ;

    const T* d_domain = Upload(domain, "QPMT::lpmtcat_scan/d_domain" );

    LOG(LEVEL)
        << " etype " << etype
        << " elabel " << elabel
        << " domain " << domain->sstr()
        << " num_domain " << num_domain
        << " lookup " << lookup->sstr()
        << " lookup_num_values " << lookup_num_values
        ;

    T* h_lookup = lookup->values<T>() ;

    T* d_lookup = Alloc(lookup, "QPMT<T>::lpmtcat_scan/d_lookup" );


#if defined(MOCK_CURAND) || defined(MOCK_CUDA)
    QPMT_lpmtcat_MOCK( d_pmt, etype, d_lookup, d_domain, num_domain );
#else
    dim3 numBlocks ;
    dim3 threadsPerBlock ;
    QU::ConfigureLaunch1D( numBlocks, threadsPerBlock, num_domain, 512u );

    QPMT_lpmtcat_scan(numBlocks, threadsPerBlock, d_pmt, etype, d_lookup, d_domain, num_domain );

    QU::copy_device_to_host_and_free<T>( h_lookup, d_lookup, lookup_num_values, "QPMT::lpmtcat_scan/cd2haf" );
    cudaDeviceSynchronize();
#endif

    return lookup ;
}


/**
QPMT::mct_lpmtid_scan
----------------------

Canonical usage from QPMTTest.h make_qscan


mct means minus_cos_theta for an AOI scan

1. create lookup output array with shape depending on etype
2. allocate d_lookup on device
3. upload domain to d_domain
4. upload lpmtid to d_lpmtid
5. invoke launch QPMT_mct_lpmtid
6. download d_lookup to h_lookup


Q: Can dependency on CUSTOM4 be avoided ?
A: NO, as using multilayer stack calc so need the
   header with the TMM calc from CUSTOM4.
   Dont want to duplicate that header.

**/

template<typename T>
NP* QPMT<T>::mct_lpmtid_scan(int etype, const NP* domain, const NP* lpmtid ) const
{
    const char* elabel = qpmt_enum::Label(etype) ;
    unsigned num_domain = domain->shape[0] ;
    unsigned num_lpmtid = lpmtid->shape[0] ;

    NP* lookup = MakeArray_lpmtid(etype, num_domain, num_lpmtid );
    LOG_IF(fatal, lookup == nullptr)
          << " etype " << etype
          << " elabel " << elabel
          << " FATAL no lookup "
          ;

    assert(lookup);
    if(!lookup) std::raise(SIGINT);

    unsigned num_lookup = lookup->num_values() ;


    if( etype == qpmt_ART )
    {
        std::vector<std::pair<std::string, std::string>> kvs =
        {
            { "title", "QPMT.title" },
            { "brief", "QPMT.brief" },
            { "name",  "QPMT.name"  },
            { "label", "QPMT.label" },
            { "ExecutableName", ExecutableName }
        };
        lookup->set_meta_kv<std::string>(kvs);
    }


    LOG(LEVEL)
        << " etype " << etype
        << " elabel " << elabel
        << " domain " << domain->sstr()
        << " lpmtid " << lpmtid->sstr()
        << " num_domain " << num_domain
        << " num_lpmtid " << num_lpmtid
        << " lookup " << lookup->sstr()
        << " num_lookup " << num_lookup
        ;



#ifdef WITH_CUSTOM4
    T* h_lookup = lookup->values<T>() ;

#if defined(MOCK_CURAND) || defined(MOCK_CUDA)
    T* d_lookup = h_lookup ;
#else
    T* d_lookup = QU::device_alloc<T>(num_lookup,"QPMT::mct_lpmtid_scan/d_lookup") ;
#endif

    assert( lpmtid->uifc == 'i' && lpmtid->ebyte == 4 );

#if defined(MOCK_CURAND) || defined(MOCK_CUDA)
    const T*   d_domain = domain->cvalues<T>() ;
    const int* d_lpmtid = lpmtid->cvalues<int>() ;
#else
    const char* label_0 = "QPMT::mct_lpmtid_scan/d_domain" ;
    const char* label_1 = "QPMT::mct_lpmtid_scan/d_lpmtid" ;

    const T*   d_domain = QU::UploadArray<T>(   domain->cvalues<T>(),   num_domain, label_0) ;
    const int* d_lpmtid = QU::UploadArray<int>( lpmtid->cvalues<int>(), num_lpmtid, label_1) ;
#endif



#if defined(MOCK_CURAND) || defined(MOCK_CUDA)
    QPMT_mct_lpmtid_MOCK( d_pmt, etype, d_lookup, d_domain, num_domain, d_lpmtid, num_lpmtid );
#else

    dim3 numBlocks ;
    dim3 threadsPerBlock ;
    QU::ConfigureLaunch1D( numBlocks, threadsPerBlock, num_domain, 512u );

    QPMT_mct_lpmtid_scan(
        numBlocks,
        threadsPerBlock,
        d_pmt,
        etype,
        d_lookup,
        d_domain,
        num_domain,
        d_lpmtid,
        num_lpmtid );

    cudaDeviceSynchronize();

    const char* label = "QPMT::mct_lpmtid_scan" ;
    QU::copy_device_to_host_and_free<T>( h_lookup, d_lookup, num_lookup, label );
    cudaDeviceSynchronize();
#endif

#else
    LOG(fatal) << " QPMT::mct_lpmtid_scan requires compilation WITH_CUSTOM4 " ;
    assert(0) ;
#endif

    return lookup ;
}

// found the below can live in header, when headeronly
//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wattributes"
// quell warning: type attributes ignored after type is already defined [-Wattributes]
template struct QUDARAP_API QPMT<float>;
//template struct QUDARAP_API QPMT<double>;
//#pragma GCC diagnostic pop

