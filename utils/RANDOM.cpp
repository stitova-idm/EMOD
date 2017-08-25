/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "RANDOM.h"
#include <assert.h>
#include <math.h>

#ifndef WIN32
#include <memory.h>    // memset
#include <climits>     // UINT_MAX
#include <wmmintrin.h> // AES
#include <smmintrin.h> // _mm_insert_epi64
#include <tmmintrin.h> // _mm_shuffle_epi8
#endif

double RANDOMBASE::cdf_random_num_precision = 0.01; // precision of random number obtained from a cumulative probability function
double RANDOMBASE::tan_pi_4 = 1.0;
double RANDOMBASE::pi = atan(tan_pi_4) * 4.0;

RANDOMBASE::RANDOMBASE(uint32_t iSequence, size_t nCache)
    : iSeq(iSequence)
    , random_bits(reinterpret_cast<uint32_t*>(malloc(nCache*sizeof(uint32_t))))
    , random_floats(reinterpret_cast<float*>(malloc(nCache*sizeof(float))))
    , index(UINT_MAX)   // Make sure fill_bits() is called...
    , bGauss(false)
    , cache_count(nCache)
{
}

RANDOMBASE::~RANDOMBASE()
{
#ifdef _DEBUG
    free(random_bits);
    free(random_floats);
#endif
}

uint32_t RANDOMBASE::ul()
{
    if (index >= cache_count)
    {
        fill_bits();
        bits_to_float();
        index = 0;
    }

    return random_bits[index++];
}

float RANDOMBASE::e()
{
    if (index >= cache_count)
    {
        fill_bits();
        bits_to_float();
        index = 0;
    }

    return random_floats[index++];
}

void RANDOMBASE::fill_bits()
{
    assert(false);
}

#define FLOAT_EXP   8

void RANDOMBASE::bits_to_float()
{
    __m128i m = _mm_set1_epi32(0x007FFFFF);
    __m128i o = _mm_set1_epi32(0x00000001);
    __m128 f = _mm_set1_ps(1.0f);
    __m128i fi = _mm_castps_si128(f);
    for (size_t i = 0; i < cache_count; i += 4)
    {
        __m128i x = _mm_load_si128(reinterpret_cast<__m128i const*>(random_bits+i));    // x = bits
//        x = _mm_and_si128(x, m);                                    // x &= 0x007FFFFF
        x = _mm_srli_epi32(x, (FLOAT_EXP+1));                       // x = x >> 9 (we just want the 23 mantissa bits)
        x = _mm_or_si128(x, o);                                     // x |= 0x00000001
        __m128i y = _mm_or_si128(fi, x);                            // y = fi | x
        __m128 z = _mm_castsi128_ps(y);                             // z = y interpreted as floating point
        z = _mm_sub_ps(z, f);                                       // z -= 1.0f
        _mm_store_ps(random_floats + i, z);
    }
}

/******************************Public*Routine******************************\
* eGauss()                                                                 *
*                                                                          *
* Returns a normal deviate for each call.  It generates two deviates at a  *
* time and caches one for the next call.                                   *
*                                                                          *
*  Sat 26-Mar-1994 14:10:12 -by- Charles Whitmer [chuckwh]                 *
* I actually wrote this back in 1982 for some physics stuff!               *
\**************************************************************************/

double RANDOMBASE::eGauss()
{
    if (bGauss)
    {
        bGauss = false;
        return eGauss_;
    }

    double rad, norm;
    double s, r1, r2;

    rad = -2.0 * log(ee());
    do
    {
        r1 = ee() - 0.5;
        r2 = ee() - 0.5;
        s = r1 * r1 + r2 * r2;
    }
    while (s > 0.25);
    norm = sqrt(rad / s);
    eGauss_ = r1 * norm;
    bGauss = true;
    return r2 * norm;
}

double RANDOMBASE::ee()
{
    union
    {
        double ee;
        struct
        {
            uint32_t Low;
            uint32_t High;
        };
    } ee_ul;

    uint32_t ll = ul();    // Choose a random 32 bits.

    ee_ul.ee = 1.0;
    ee_ul.High += (ll >> (DOUBLE_EXP + 1));
    ee_ul.Low = (ll << (31 - DOUBLE_EXP)) + (1 << (30 - DOUBLE_EXP));

    return ee_ul.ee - 1.0;
}

// Poisson() added by Philip Eckhoff, uses Gaussian approximation for ratetime>10
uint64_t RANDOMBASE::Poisson(double ratetime)
{
    if (ratetime <= 0)
    {
        return 0;
    }
    uint64_t events = 0;
    double Time = 0;
    double tempval;
    if (ratetime < 10)
    {
        while (Time < 1)
        {
            Time += -log(e()) / ratetime;
            if (Time < 1)
            {
                events++;
            }
        }
    }
    else
    {
        tempval = (eGauss() * sqrt(ratetime) + ratetime + .5);
        if (tempval < 0)
        {
            events = 0;
        }
        else
        {
            events = uint64_t(tempval);
        }
    }
    return events;
}

// Poisson_true added by Philip Eckhoff, actual Poisson, without approximation
uint32_t RANDOMBASE::Poisson_true(double ratetime)
{
    if (ratetime <= 0)
    {
        return 0;
    }
    uint32_t events = 0;
    double Time = 0;
    while (Time < 1)
    {
        Time += -log(e()) / ratetime;
        if (Time < 1)
        {
            events++;
        }
    }
    return events;
}

//  expdist() added by Philip Eckhoff
double RANDOMBASE::expdist(double rate) // rate = 1/mean
{
    if (rate == 0)
    {
        return 0;
    }
    else
    {
        return -log(e()) / rate;
    }
}

// Weibull(lambda, kappa) added by Daniel Klein
double RANDOMBASE::Weibull(double lambda, double kappa)
{
    if (lambda <= 0 || kappa <= 0)
    {
        return 0;
    }
    return lambda * pow( -log(e()), 1/kappa );
}

// NOTE: The only reason for the arguments to be floats is that
//       it kept the regression tests from changing.
//       We get a slightly different result for 1 / inv_kappa
//       if the numbers are doubles instead of floats.  This slightly
//       different number causes thing to change slightly.
double RANDOMBASE::Weibull2(float lambda, float inv_kappa)
{
    if( inv_kappa == 0.0 )
    {
        return lambda ;
    }
    else
    {
        return Weibull( lambda, 1.0f/inv_kappa );
    }
}

// LogLogistic(alpha, beta) added by Anna Bershteyn
// alpha is a scale parameter > 0, beta is a shape parameter > 0
double RANDOMBASE::LogLogistic(double alpha, double beta)
{
    if (alpha <= 0 || beta <= 0)
    {
        return 0;
    }
    double uniform_rand = e();
    return alpha * pow(uniform_rand/(1-uniform_rand), 1/beta);
}

double RANDOMBASE::time_varying_rate_dist( std::vector <float> v_rate, float timestep, float rate)
{ 
    double e_log = -log(e());
    double tempsum = 0.0f;
    int temp_break_step = 0;
    double ret= 0.0f;

    for ( auto rit = v_rate.begin(); rit!= v_rate.end()-1; ++rit )
    {
        tempsum = tempsum + double(*rit) * double(timestep) +  double(0.5*(*(rit+1)-*rit) * double(timestep))+ double(rate) * double(timestep);

        if (tempsum  > e_log)
        {
            int break_step = temp_break_step + 1;
            ret = double(break_step) * double(timestep)  - (tempsum - e_log)/( double(v_rate[break_step-1]) + 0.5*( v_rate[break_step]- v_rate[break_step - 1]) + double(rate)) ;

            return ret;
        }

        temp_break_step ++ ;
    }

    ret =  double(temp_break_step) *  double(timestep) + (e_log-tempsum)/(double(rate) + double(v_rate.back()));
    return ret;
}

// binomial_approx() added by Philip Eckhoff
uint64_t RANDOMBASE::binomial_approx(uint64_t n, double p)
{
    int64_t tempval = 0;

    if (n <= 0 || p <= 0)
    {
        tempval = 0;
    }
    else if (p >= 1)
    {
        tempval = n;
    }
    else
    {
        double mean = n * p;
        if (n < 10)
        {
            for (int i = 0; i < n; i++)
            {
                if (e() < p)
                {
                    tempval++;
                }
            }
        }
        else
        {
            tempval = int64_t(eGauss() * sqrt(mean * (1.0 - p)) + mean + 0.5);
            if (tempval < 0)
            {
                tempval = 0;
            }
        }
    }

    if (tempval < 0)
    {
        tempval = 0;
    }
    if (uint64_t(tempval) > n)
    {
        tempval = n;
    }

    return uint64_t(tempval);
}

uint64_t RANDOMBASE::binomial_true(uint64_t n, double p)
{
    int64_t tempval = 0;

    if (n <= 0 || p <= 0)
    {
        tempval = 0;
    }
    else if (p >= 1)
    {
        tempval = n;
    }
    else
    {
        for (int i = 0; i < n; i++)
        {
            if (e() < p)
            {
                tempval++;
            }
        }
    }

    if (tempval < 0)
    {
        tempval = 0;
    }

    if (uint64_t(tempval) > n)
    {
        tempval = n;
    }

    return uint64_t(tempval);
}

// binomial_approx2() added by Edward Wenger
// for small numbers, just do n random draws for true binomial
// use poisson approximation near edges (p=0 or p=1) if (n^0.31)*p < 0.47
// or less conservatively for large-n, use normal as long as +/- 3 sigma within [0,1]
// use normal approximation for intermediate probabilities around p=0.5
uint64_t RANDOMBASE::binomial_approx2(uint64_t n, double p)
{
    int64_t tempval = 0;

    if (n <= 0 || p <= 0)
    {
        tempval = 0;
    }
    else if (p >= 1)
    {
        tempval = n;
    }
    else
    {
        double mean = n * p;
        // for small numbers, just do n random draws for true binomial
        if (n < 10)
        {
            for (int i = 0; i < n; i++)
            {
                if (e() < p)
                {
                    tempval++;
                }
            }
        }
        else
        {
            bool under_50pct = p < 0.5;
            double p_tmp = under_50pct ? p : (1-p);
            //if ( ( pow(n, 0.31) * p_tmp ) < 0.47 )
            if ( n < 9*(1-p_tmp)/p_tmp ) // 3-sigma within [0,1]
            {
                // use poisson approximation for probabilities near p=0 or p=1
                double poisson_tmp = Poisson_true(n * p_tmp);
                tempval = int64_t(under_50pct ? poisson_tmp : (n-poisson_tmp));
            }
            else
            {
                // use normal approximation for intermediate probabilities
                tempval = int64_t(eGauss() * sqrt(mean * (1.0 - p)) + mean + .5);
                if (tempval < 0)
                {
                    tempval = 0;
                }
            }
        }
    }

    if (tempval < 0)
    {
        tempval = 0;
    }
    if (uint64_t(tempval) > n)
    {
        tempval = n;
    }

    return uint64_t(tempval);
}

void RANDOM::fill_bits()
{
    for (size_t i = 0; i < cache_count; ++i)
    {
        random_bits[i] = iSeq = 69069 * iSeq + 1;
    }
}

// Finds an uniformally distributed number between 0 and N
uint32_t RANDOMBASE::uniformZeroToN( uint32_t N )
{
    uint64_t ulA = uint64_t(ul());
    uint64_t ulB = uint64_t(ul());
    ulB <<= 32;
    ulA += ulB;
    uint64_t ll = (ulA & 0xFFFFFFFFL) * N;
    ll >>= 32;
    return uint32_t(ll);
}

const uint32_t c1[4] = {0xBAA96887L, 0x1E17D32CL, 0x03BCDC3CL, 0x0F33D1B2L};
const uint32_t c2[4] = {0x4B0F3B58L, 0xE874F0C3L, 0x6955C5A6L, 0x55A7CA46L};

#define HI(x) ((uint32_t) ((uint16_t*) &x)[1])
#define LO(x) ((uint32_t) ((uint16_t*) &x)[0])
#define XCHG(x) ((LO(x) << 16) | HI(x))

void PSEUDO_DES::fill_bits()
{
    uint32_t kk[3];
    uint32_t iA;
    uint32_t iB;
#ifdef _DEBUG
    uint32_t ul;
#endif

    for (size_t i = 0; i < cache_count; ++i)
    {
        iA = iNum ^ c1[0];
        iB = LO(iA) * LO(iA) + ~(HI(iA) * HI(iA));
        kk[0] = iSeq ^ ((XCHG(iB) ^ c2[0]) + LO(iA) * HI(iA));

        iA = kk[0] ^ c1[1];
        iB = LO(iA) * LO(iA) + ~(HI(iA) * HI(iA));
        kk[1] = iNum ^ ((XCHG(iB) ^ c2[1]) + LO(iA) * HI(iA));

        iNum++;
        if (iNum == 0)
            iSeq++;

        iA = kk[1] ^ c1[2];
        iB = LO(iA) * LO(iA) + ~(HI(iA) * HI(iA));
        kk[2] = kk[0] ^ ((XCHG(iB) ^ c2[2]) + LO(iA) * HI(iA));

        iA = kk[2] ^ c1[3];
        iB = LO(iA) * LO(iA) + ~(HI(iA) * HI(iA));

        random_bits[i] =
#ifdef _DEBUG
            ul =
#endif
            kk[1] ^ ((XCHG(iB) ^ c2[3]) + LO(iA) * HI(iA));
    }
}

// M Behrend
// gamma-distributed random number
// shape constant k=2

double RANDOMBASE::rand_gamma(double mean)
{

    if (mean <= 0)
    {
        return 0;
    }
    double q = e(); // get a uniform random number

    double p1 = 0; // guess random variable interval
    double p2 = mean;
    double p_left; // temp variable
    double slope;
    double delta_p;
    double delta_cdf;

    do
    {
        delta_cdf = q - gamma_cdf(p2, mean);
        slope = (gamma_cdf(p2, mean) - gamma_cdf(p1, mean)) / (p2 - p1);
        delta_p = delta_cdf / slope;

        if (delta_p > 0)
        {
            p1 = p2;
            p2 = p2 + delta_p;
        }
        else
        {
            p2 = p2 + delta_p;
            if (p2 < p1)
            {
                p_left = p2;
                p2 = p1;
                p1 = p_left;
            }
        }
    }
    while (fabs(delta_cdf) > cdf_random_num_precision);

    return p2;
}

double RANDOMBASE::gamma_cdf(double x, double mean)
{
    double theta = mean / 2;
    double cdf = 1 - (x / theta + 1) * exp(-x / theta);

    if (x < 0)
    {
        cdf = 0;
    }
    return cdf;
}

double RANDOMBASE::get_cdf_random_num_precision()
{
    return cdf_random_num_precision;
}

double RANDOMBASE::get_pi()
{
    return pi;
}

/* AES Counter PRNG ***********************************************************/

inline __m128i AES_128_ASSIST(__m128i temp1, __m128i temp2)
{
    __m128i   temp3;

    temp2 = _mm_shuffle_epi32(temp2, 0xFF);
    temp3 = _mm_slli_si128(temp1, 0x4);
    temp1 = _mm_xor_si128(temp1, temp3);
    temp3 = _mm_slli_si128(temp3, 0x4);
    temp1 = _mm_xor_si128(temp1, temp3);
    temp3 = _mm_slli_si128(temp3, 0x4);
    temp1 = _mm_xor_si128(temp1, temp3);
    temp1 = _mm_xor_si128(temp1, temp2);

    return temp1;
}

void AES_128_Key_Expansion(const unsigned char *userkey, AES_KEY *key)
{
    __m128i temp1, temp2;
    __m128i *Key_Schedule = (__m128i*)key;

    temp1            = _mm_loadu_si128((__m128i*)userkey);
    Key_Schedule[0]  = temp1;
    temp2            = _mm_aeskeygenassist_si128 (temp1 ,0x1);
    temp1            = AES_128_ASSIST(temp1, temp2);
    Key_Schedule[1]  = temp1;
    temp2            = _mm_aeskeygenassist_si128 (temp1,0x2);
    temp1            = AES_128_ASSIST(temp1, temp2);
    Key_Schedule[2]  = temp1;
    temp2            = _mm_aeskeygenassist_si128 (temp1,0x4);
    temp1            = AES_128_ASSIST(temp1, temp2);
    Key_Schedule[3]  = temp1;
    temp2            = _mm_aeskeygenassist_si128 (temp1,0x8);
    temp1            = AES_128_ASSIST(temp1, temp2);
    Key_Schedule[4]  = temp1;
    temp2            = _mm_aeskeygenassist_si128 (temp1,0x10);
    temp1            = AES_128_ASSIST(temp1, temp2);
    Key_Schedule[5]  = temp1;
    temp2            = _mm_aeskeygenassist_si128 (temp1,0x20);
    temp1            = AES_128_ASSIST(temp1, temp2);
    Key_Schedule[6]  = temp1;
    temp2            = _mm_aeskeygenassist_si128 (temp1,0x40);
    temp1            = AES_128_ASSIST(temp1, temp2);
    Key_Schedule[7]  = temp1;
    temp2            = _mm_aeskeygenassist_si128 (temp1,0x80);
    temp1            = AES_128_ASSIST(temp1, temp2);
    Key_Schedule[8]  = temp1;
    temp2            = _mm_aeskeygenassist_si128 (temp1,0x1b);
    temp1            = AES_128_ASSIST(temp1, temp2);
    Key_Schedule[9]  = temp1;
    temp2            = _mm_aeskeygenassist_si128 (temp1,0x36);
    temp1            = AES_128_ASSIST(temp1, temp2);
    Key_Schedule[10] = temp1;
}

int AES_set_encrypt_key(const unsigned char *userKey, const int bits, AES_KEY *key)
{
    if (!userKey || !key) return -1;

    AES_128_Key_Expansion(userKey, key);
    key->nr = 10;
    return 0;
}

void AES_Init_Ex(AES_KEY *pExpanded)
{
    __m128i key;
    memset(&key, 0, sizeof(key));
    AES_set_encrypt_key((const unsigned char *)&key, 128, pExpanded);
}

AES_COUNTER::AES_COUNTER(uint32_t iSequence, uint32_t rank, size_t nCache)
    : RANDOMBASE(iSequence, nCache)
    , m_nonce(uint64_t(rank) << 32 | uint64_t(iSequence))
    , m_iteration(0)
{
    AES_Init_Ex(&m_keySchedule);
}

AES_COUNTER::~AES_COUNTER()
{

}

void AES_Get_Bits_Ex(void *buffer, size_t bytes, uint64_t nonce, uint32_t count, AES_KEY *pKey)
{
    __m128i *in = (__m128i *)buffer;
    __m128i *out = in;
    __m128i *key = pKey->KEY;
    size_t length;
    unsigned int nr = pKey->nr;

    __m128i ctr_block, tmp, ONE, BSWAP_EPI64;
    unsigned int i,j;

    if (bytes%16) {
        length = bytes / 16 + 1;
    }
    else {
        length = bytes / 16;
    }

    memset(&ctr_block, 0xCC, sizeof(ctr_block));

    ONE         = _mm_set_epi32(0,1,0,0);
    BSWAP_EPI64 = _mm_setr_epi8(7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8);
    ctr_block   = _mm_insert_epi64(ctr_block, nonce, 1);
    ctr_block   = _mm_insert_epi32(ctr_block, count, 1);
    ctr_block   = _mm_srli_si128(ctr_block, 4);
    ctr_block   = _mm_shuffle_epi8(ctr_block, BSWAP_EPI64);
    ctr_block   = _mm_add_epi64(ctr_block, ONE);

    {
        memset(in, 0, sizeof(__m128i));
        tmp       = _mm_shuffle_epi8(ctr_block, BSWAP_EPI64);
        ctr_block = _mm_add_epi64(ctr_block, ONE);
        tmp       = _mm_xor_si128(tmp, key[0]);

        for (j = 1; j < nr; j++) {
            tmp = _mm_aesenc_si128 (tmp, key[j]);
        }

        tmp = _mm_aesenclast_si128(tmp, key[j]);
        tmp = _mm_xor_si128(tmp,_mm_loadu_si128(in));
        _mm_storeu_si128(out,tmp);
    }

    for (i = 1; i < length; i++) {
        tmp       = _mm_shuffle_epi8(ctr_block, BSWAP_EPI64);
        ctr_block = _mm_add_epi64(ctr_block, ONE);
        tmp       = _mm_xor_si128(tmp, key[0]);

        for (j = 1; j < nr; j++) {
            tmp = _mm_aesenc_si128(tmp, key[j]);
        }

        tmp = _mm_aesenclast_si128(tmp, key[j]);
        tmp = _mm_xor_si128(tmp,_mm_loadu_si128(in+i-1));
        _mm_storeu_si128(out+i,tmp);
    }
}

void AES_COUNTER::fill_bits()
{
    memset(random_bits, 0, sizeof(__m128i));
    AES_Get_Bits_Ex(random_bits, cache_count * sizeof(uint32_t), m_nonce, m_iteration++, &m_keySchedule);
}