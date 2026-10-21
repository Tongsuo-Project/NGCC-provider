#ifndef SHUTTLE_APPROX_LOG_EXPLORE_H
#define SHUTTLE_APPROX_LOG_EXPLORE_H
#include <stdint.h>
#if defined(__GNUC__) || defined(__clang__)
#define AL_INLINE static inline __attribute__((always_inline))
#else
#define AL_INLINE static inline
#endif

/* rounded high-half of a signed 128-bit product: ((a*x)+2^63)>>64 */
AL_INLINE int64_t al_mulhi(__int128 a, uint64_t x){
    return (int64_t)(((a*(__int128)(__uint128_t)x)+((__int128)1<<63))>>64);
}
/* constant-time equality mask: all-ones iff a==b */
AL_INLINE uint64_t al_eqmask(uint32_t a, uint32_t b){
    uint64_t z=(uint64_t)(a^b); uint64_t nz=(z|(~z+1))>>63; return nz-1;
}

/* baseline: deployed single-segment degree 21, Q62, u=b-1 as Q62 */
static const int64_t kLogBaseline[22] = {
    INT64_C(12), INT64_C(6653256548922149505), INT64_C(-3326628274459181085), INT64_C(2217752182851402654), INT64_C(-1663314133014378403), INT64_C(1330651220520872915), INT64_C(-1108874821083789136), INT64_C(950452344052204895), INT64_C(-831560255485224283), INT64_C(738694605269400153), INT64_C(-662829495809222924), INT64_C(595932539625226929), INT64_C(-528794318160472007), INT64_C(451527267200338167), INT64_C(-358255448212785553), INT64_C(253529021404104913), INT64_C(-153211958011988634), INT64_C(75505384931375114), INT64_C(-28766651603269161), INT64_C(7878760815328986), INT64_C(-1372175949002259), INT64_C(113674624297114)
};
AL_INLINE int64_t log2_frac_baseline(uint64_t u_q62){
    __int128 acc = kLogBaseline[21];
    for (int k=20; k>=0; k--)
        acc = (__int128)kLogBaseline[k] + (((acc*(__int128)(__uint128_t)u_q62)+((__int128)1<<61))>>62);
    return (int64_t)acc;
}

/* g=1: 2 segments, degree 16, Q62, err 2^-58.347 */
static const int64_t kLogPoly_g1[2][17] = {
    {INT64_C(0), INT64_C(3326628274461076927), INT64_C(-831657068614827972), INT64_C(277219022853148032), INT64_C(-103957133178363997), INT64_C(41582848318547257), INT64_C(-17326145889455872), INT64_C(7425257656606283), INT64_C(-3247596031877027), INT64_C(1440516991930163), INT64_C(-641870705388984), INT64_C(281198814226473), INT64_C(-115860798737125), INT64_C(41719728660608), INT64_C(-11828093671363), INT64_C(2263998159649), INT64_C(-213629956287)},
    {INT64_C(2697663385880076776), INT64_C(2217752182974053732), INT64_C(-369625363829007342), INT64_C(82138969739717246), INT64_C(-20534742433670584), INT64_C(5475931300202179), INT64_C(-1521091903038660), INT64_C(434596986293940), INT64_C(-126754629711570), INT64_C(37548555834050), INT64_C(-11246108228910), INT64_C(3377510039121), INT64_C(-994783322638), INT64_C(272677464043), INT64_C(-63077215428), INT64_C(10462335887), INT64_C(-894433938)},
};
AL_INLINE int64_t log2_frac_g1(uint32_t sel, uint64_t x_q64){
    int64_t c[17]; for (int k=0;k<=16;k++) c[k]=0;
    for (uint32_t j=0;j<2;j++){ uint64_t m=al_eqmask(j,sel);
        for (int k=0;k<=16;k++) c[k]|=(int64_t)(m&(uint64_t)kLogPoly_g1[j][k]); }
    __int128 acc=c[16]; for (int k=15;k>=0;k--) acc=(__int128)c[k]+(__int128)al_mulhi(acc,x_q64);
    return (int64_t)acc;
}
AL_INLINE void log2_frac_g1_x2(const uint32_t sel[2], const uint64_t xx[2], int64_t out[2]){
    uint64_t M[2][2];
    for(uint32_t j=0;j<2;j++){ M[0][j]=al_eqmask(j,sel[0]); M[1][j]=al_eqmask(j,sel[1]); }
    int64_t h0=0; int64_t h1=0;
    for(uint32_t j=0;j<2;j++){ int64_t v=kLogPoly_g1[j][16];
        h0|=(int64_t)(M[0][j]&(uint64_t)v); h1|=(int64_t)(M[1][j]&(uint64_t)v); }
    __int128 a0=h0; __int128 a1=h1;
    for(int k=15;k>=0;k--){
        int64_t c0=0; int64_t c1=0;
        for(uint32_t j=0;j<2;j++){ int64_t v=kLogPoly_g1[j][k];
            c0|=(int64_t)(M[0][j]&(uint64_t)v); c1|=(int64_t)(M[1][j]&(uint64_t)v); }
        a0=(__int128)c0+(__int128)al_mulhi(a0,xx[0]); a1=(__int128)c1+(__int128)al_mulhi(a1,xx[1]);
    }
    out[0]=(int64_t)a0; out[1]=(int64_t)a1;
}
AL_INLINE void log2_frac_g1_x3(const uint32_t sel[3], const uint64_t xx[3], int64_t out[3]){
    uint64_t M[3][2];
    for(uint32_t j=0;j<2;j++){ M[0][j]=al_eqmask(j,sel[0]); M[1][j]=al_eqmask(j,sel[1]); M[2][j]=al_eqmask(j,sel[2]); }
    int64_t h0=0; int64_t h1=0; int64_t h2=0;
    for(uint32_t j=0;j<2;j++){ int64_t v=kLogPoly_g1[j][16];
        h0|=(int64_t)(M[0][j]&(uint64_t)v); h1|=(int64_t)(M[1][j]&(uint64_t)v); h2|=(int64_t)(M[2][j]&(uint64_t)v); }
    __int128 a0=h0; __int128 a1=h1; __int128 a2=h2;
    for(int k=15;k>=0;k--){
        int64_t c0=0; int64_t c1=0; int64_t c2=0;
        for(uint32_t j=0;j<2;j++){ int64_t v=kLogPoly_g1[j][k];
            c0|=(int64_t)(M[0][j]&(uint64_t)v); c1|=(int64_t)(M[1][j]&(uint64_t)v); c2|=(int64_t)(M[2][j]&(uint64_t)v); }
        a0=(__int128)c0+(__int128)al_mulhi(a0,xx[0]); a1=(__int128)c1+(__int128)al_mulhi(a1,xx[1]); a2=(__int128)c2+(__int128)al_mulhi(a2,xx[2]);
    }
    out[0]=(int64_t)a0; out[1]=(int64_t)a1; out[2]=(int64_t)a2;
}
AL_INLINE void log2_frac_g1_x4(const uint32_t sel[4], const uint64_t xx[4], int64_t out[4]){
    uint64_t M[4][2];
    for(uint32_t j=0;j<2;j++){ M[0][j]=al_eqmask(j,sel[0]); M[1][j]=al_eqmask(j,sel[1]); M[2][j]=al_eqmask(j,sel[2]); M[3][j]=al_eqmask(j,sel[3]); }
    int64_t h0=0; int64_t h1=0; int64_t h2=0; int64_t h3=0;
    for(uint32_t j=0;j<2;j++){ int64_t v=kLogPoly_g1[j][16];
        h0|=(int64_t)(M[0][j]&(uint64_t)v); h1|=(int64_t)(M[1][j]&(uint64_t)v); h2|=(int64_t)(M[2][j]&(uint64_t)v); h3|=(int64_t)(M[3][j]&(uint64_t)v); }
    __int128 a0=h0; __int128 a1=h1; __int128 a2=h2; __int128 a3=h3;
    for(int k=15;k>=0;k--){
        int64_t c0=0; int64_t c1=0; int64_t c2=0; int64_t c3=0;
        for(uint32_t j=0;j<2;j++){ int64_t v=kLogPoly_g1[j][k];
            c0|=(int64_t)(M[0][j]&(uint64_t)v); c1|=(int64_t)(M[1][j]&(uint64_t)v); c2|=(int64_t)(M[2][j]&(uint64_t)v); c3|=(int64_t)(M[3][j]&(uint64_t)v); }
        a0=(__int128)c0+(__int128)al_mulhi(a0,xx[0]); a1=(__int128)c1+(__int128)al_mulhi(a1,xx[1]); a2=(__int128)c2+(__int128)al_mulhi(a2,xx[2]); a3=(__int128)c3+(__int128)al_mulhi(a3,xx[3]);
    }
    out[0]=(int64_t)a0; out[1]=(int64_t)a1; out[2]=(int64_t)a2; out[3]=(int64_t)a3;
}
AL_INLINE void log2_frac_g1_x8(const uint32_t sel[8], const uint64_t xx[8], int64_t out[8]){
    uint64_t M[8][2];
    for(uint32_t j=0;j<2;j++){ M[0][j]=al_eqmask(j,sel[0]); M[1][j]=al_eqmask(j,sel[1]); M[2][j]=al_eqmask(j,sel[2]); M[3][j]=al_eqmask(j,sel[3]); M[4][j]=al_eqmask(j,sel[4]); M[5][j]=al_eqmask(j,sel[5]); M[6][j]=al_eqmask(j,sel[6]); M[7][j]=al_eqmask(j,sel[7]); }
    int64_t h0=0; int64_t h1=0; int64_t h2=0; int64_t h3=0; int64_t h4=0; int64_t h5=0; int64_t h6=0; int64_t h7=0;
    for(uint32_t j=0;j<2;j++){ int64_t v=kLogPoly_g1[j][16];
        h0|=(int64_t)(M[0][j]&(uint64_t)v); h1|=(int64_t)(M[1][j]&(uint64_t)v); h2|=(int64_t)(M[2][j]&(uint64_t)v); h3|=(int64_t)(M[3][j]&(uint64_t)v); h4|=(int64_t)(M[4][j]&(uint64_t)v); h5|=(int64_t)(M[5][j]&(uint64_t)v); h6|=(int64_t)(M[6][j]&(uint64_t)v); h7|=(int64_t)(M[7][j]&(uint64_t)v); }
    __int128 a0=h0; __int128 a1=h1; __int128 a2=h2; __int128 a3=h3; __int128 a4=h4; __int128 a5=h5; __int128 a6=h6; __int128 a7=h7;
    for(int k=15;k>=0;k--){
        int64_t c0=0; int64_t c1=0; int64_t c2=0; int64_t c3=0; int64_t c4=0; int64_t c5=0; int64_t c6=0; int64_t c7=0;
        for(uint32_t j=0;j<2;j++){ int64_t v=kLogPoly_g1[j][k];
            c0|=(int64_t)(M[0][j]&(uint64_t)v); c1|=(int64_t)(M[1][j]&(uint64_t)v); c2|=(int64_t)(M[2][j]&(uint64_t)v); c3|=(int64_t)(M[3][j]&(uint64_t)v); c4|=(int64_t)(M[4][j]&(uint64_t)v); c5|=(int64_t)(M[5][j]&(uint64_t)v); c6|=(int64_t)(M[6][j]&(uint64_t)v); c7|=(int64_t)(M[7][j]&(uint64_t)v); }
        a0=(__int128)c0+(__int128)al_mulhi(a0,xx[0]); a1=(__int128)c1+(__int128)al_mulhi(a1,xx[1]); a2=(__int128)c2+(__int128)al_mulhi(a2,xx[2]); a3=(__int128)c3+(__int128)al_mulhi(a3,xx[3]); a4=(__int128)c4+(__int128)al_mulhi(a4,xx[4]); a5=(__int128)c5+(__int128)al_mulhi(a5,xx[5]); a6=(__int128)c6+(__int128)al_mulhi(a6,xx[6]); a7=(__int128)c7+(__int128)al_mulhi(a7,xx[7]);
    }
    out[0]=(int64_t)a0; out[1]=(int64_t)a1; out[2]=(int64_t)a2; out[3]=(int64_t)a3; out[4]=(int64_t)a4; out[5]=(int64_t)a5; out[6]=(int64_t)a6; out[7]=(int64_t)a7;
}

/* g=2: 4 segments, degree 13, Q62, err 2^-59.453 */
static const int64_t kLogPoly_g2[4][14] = {
    {INT64_C(0), INT64_C(1663314137230540005), INT64_C(-207914267153782834), INT64_C(34652377857794422), INT64_C(-6497320829889967), INT64_C(1299464001399016), INT64_C(-270720747985221), INT64_C(58008183734025), INT64_C(-12680634058615), INT64_C(2802591328231), INT64_C(-611697204761), INT64_C(123141961189), INT64_C(-19530076391), INT64_C(1717255301)},
    {INT64_C(1484631294131014398), INT64_C(1330651309784432187), INT64_C(-133065130978439107), INT64_C(17742017463685572), INT64_C(-2661302618120493), INT64_C(425808407352253), INT64_C(-70968007646487), INT64_C(12165731010280), INT64_C(-2128477788302), INT64_C(377485364884), INT64_C(-66839897685), INT64_C(11228353229), INT64_C(-1554229086), INT64_C(124985134)},
    {INT64_C(2697663385880076776), INT64_C(1108876091487026868), INT64_C(-92406340957251847), INT64_C(10267371217462318), INT64_C(-1283421402046772), INT64_C(171122852511038), INT64_C(-23767057140815), INT64_C(3395273701197), INT64_C(-495094418506), INT64_C(73261264270), INT64_C(-10884612082), INT64_C(1561954161), INT64_C(-191070343), INT64_C(14130118)},
    {INT64_C(3723267405961586381), INT64_C(950465221274594463), INT64_C(-67890372948185266), INT64_C(6465749804587722), INT64_C(-692758907616243), INT64_C(79172446438655), INT64_C(-9425290482070), INT64_C(1154114515161), INT64_C(-144257705268), INT64_C(18306992888), INT64_C(-2339854304), INT64_C(292286931), INT64_C(-31958889), INT64_C(2187745)},
};
AL_INLINE int64_t log2_frac_g2(uint32_t sel, uint64_t x_q64){
    int64_t c[14]; for (int k=0;k<=13;k++) c[k]=0;
    for (uint32_t j=0;j<4;j++){ uint64_t m=al_eqmask(j,sel);
        for (int k=0;k<=13;k++) c[k]|=(int64_t)(m&(uint64_t)kLogPoly_g2[j][k]); }
    __int128 acc=c[13]; for (int k=12;k>=0;k--) acc=(__int128)c[k]+(__int128)al_mulhi(acc,x_q64);
    return (int64_t)acc;
}
AL_INLINE void log2_frac_g2_x2(const uint32_t sel[2], const uint64_t xx[2], int64_t out[2]){
    uint64_t M[2][4];
    for(uint32_t j=0;j<4;j++){ M[0][j]=al_eqmask(j,sel[0]); M[1][j]=al_eqmask(j,sel[1]); }
    int64_t h0=0; int64_t h1=0;
    for(uint32_t j=0;j<4;j++){ int64_t v=kLogPoly_g2[j][13];
        h0|=(int64_t)(M[0][j]&(uint64_t)v); h1|=(int64_t)(M[1][j]&(uint64_t)v); }
    __int128 a0=h0; __int128 a1=h1;
    for(int k=12;k>=0;k--){
        int64_t c0=0; int64_t c1=0;
        for(uint32_t j=0;j<4;j++){ int64_t v=kLogPoly_g2[j][k];
            c0|=(int64_t)(M[0][j]&(uint64_t)v); c1|=(int64_t)(M[1][j]&(uint64_t)v); }
        a0=(__int128)c0+(__int128)al_mulhi(a0,xx[0]); a1=(__int128)c1+(__int128)al_mulhi(a1,xx[1]);
    }
    out[0]=(int64_t)a0; out[1]=(int64_t)a1;
}
AL_INLINE void log2_frac_g2_x3(const uint32_t sel[3], const uint64_t xx[3], int64_t out[3]){
    uint64_t M[3][4];
    for(uint32_t j=0;j<4;j++){ M[0][j]=al_eqmask(j,sel[0]); M[1][j]=al_eqmask(j,sel[1]); M[2][j]=al_eqmask(j,sel[2]); }
    int64_t h0=0; int64_t h1=0; int64_t h2=0;
    for(uint32_t j=0;j<4;j++){ int64_t v=kLogPoly_g2[j][13];
        h0|=(int64_t)(M[0][j]&(uint64_t)v); h1|=(int64_t)(M[1][j]&(uint64_t)v); h2|=(int64_t)(M[2][j]&(uint64_t)v); }
    __int128 a0=h0; __int128 a1=h1; __int128 a2=h2;
    for(int k=12;k>=0;k--){
        int64_t c0=0; int64_t c1=0; int64_t c2=0;
        for(uint32_t j=0;j<4;j++){ int64_t v=kLogPoly_g2[j][k];
            c0|=(int64_t)(M[0][j]&(uint64_t)v); c1|=(int64_t)(M[1][j]&(uint64_t)v); c2|=(int64_t)(M[2][j]&(uint64_t)v); }
        a0=(__int128)c0+(__int128)al_mulhi(a0,xx[0]); a1=(__int128)c1+(__int128)al_mulhi(a1,xx[1]); a2=(__int128)c2+(__int128)al_mulhi(a2,xx[2]);
    }
    out[0]=(int64_t)a0; out[1]=(int64_t)a1; out[2]=(int64_t)a2;
}
AL_INLINE void log2_frac_g2_x4(const uint32_t sel[4], const uint64_t xx[4], int64_t out[4]){
    uint64_t M[4][4];
    for(uint32_t j=0;j<4;j++){ M[0][j]=al_eqmask(j,sel[0]); M[1][j]=al_eqmask(j,sel[1]); M[2][j]=al_eqmask(j,sel[2]); M[3][j]=al_eqmask(j,sel[3]); }
    int64_t h0=0; int64_t h1=0; int64_t h2=0; int64_t h3=0;
    for(uint32_t j=0;j<4;j++){ int64_t v=kLogPoly_g2[j][13];
        h0|=(int64_t)(M[0][j]&(uint64_t)v); h1|=(int64_t)(M[1][j]&(uint64_t)v); h2|=(int64_t)(M[2][j]&(uint64_t)v); h3|=(int64_t)(M[3][j]&(uint64_t)v); }
    __int128 a0=h0; __int128 a1=h1; __int128 a2=h2; __int128 a3=h3;
    for(int k=12;k>=0;k--){
        int64_t c0=0; int64_t c1=0; int64_t c2=0; int64_t c3=0;
        for(uint32_t j=0;j<4;j++){ int64_t v=kLogPoly_g2[j][k];
            c0|=(int64_t)(M[0][j]&(uint64_t)v); c1|=(int64_t)(M[1][j]&(uint64_t)v); c2|=(int64_t)(M[2][j]&(uint64_t)v); c3|=(int64_t)(M[3][j]&(uint64_t)v); }
        a0=(__int128)c0+(__int128)al_mulhi(a0,xx[0]); a1=(__int128)c1+(__int128)al_mulhi(a1,xx[1]); a2=(__int128)c2+(__int128)al_mulhi(a2,xx[2]); a3=(__int128)c3+(__int128)al_mulhi(a3,xx[3]);
    }
    out[0]=(int64_t)a0; out[1]=(int64_t)a1; out[2]=(int64_t)a2; out[3]=(int64_t)a3;
}
AL_INLINE void log2_frac_g2_x8(const uint32_t sel[8], const uint64_t xx[8], int64_t out[8]){
    uint64_t M[8][4];
    for(uint32_t j=0;j<4;j++){ M[0][j]=al_eqmask(j,sel[0]); M[1][j]=al_eqmask(j,sel[1]); M[2][j]=al_eqmask(j,sel[2]); M[3][j]=al_eqmask(j,sel[3]); M[4][j]=al_eqmask(j,sel[4]); M[5][j]=al_eqmask(j,sel[5]); M[6][j]=al_eqmask(j,sel[6]); M[7][j]=al_eqmask(j,sel[7]); }
    int64_t h0=0; int64_t h1=0; int64_t h2=0; int64_t h3=0; int64_t h4=0; int64_t h5=0; int64_t h6=0; int64_t h7=0;
    for(uint32_t j=0;j<4;j++){ int64_t v=kLogPoly_g2[j][13];
        h0|=(int64_t)(M[0][j]&(uint64_t)v); h1|=(int64_t)(M[1][j]&(uint64_t)v); h2|=(int64_t)(M[2][j]&(uint64_t)v); h3|=(int64_t)(M[3][j]&(uint64_t)v); h4|=(int64_t)(M[4][j]&(uint64_t)v); h5|=(int64_t)(M[5][j]&(uint64_t)v); h6|=(int64_t)(M[6][j]&(uint64_t)v); h7|=(int64_t)(M[7][j]&(uint64_t)v); }
    __int128 a0=h0; __int128 a1=h1; __int128 a2=h2; __int128 a3=h3; __int128 a4=h4; __int128 a5=h5; __int128 a6=h6; __int128 a7=h7;
    for(int k=12;k>=0;k--){
        int64_t c0=0; int64_t c1=0; int64_t c2=0; int64_t c3=0; int64_t c4=0; int64_t c5=0; int64_t c6=0; int64_t c7=0;
        for(uint32_t j=0;j<4;j++){ int64_t v=kLogPoly_g2[j][k];
            c0|=(int64_t)(M[0][j]&(uint64_t)v); c1|=(int64_t)(M[1][j]&(uint64_t)v); c2|=(int64_t)(M[2][j]&(uint64_t)v); c3|=(int64_t)(M[3][j]&(uint64_t)v); c4|=(int64_t)(M[4][j]&(uint64_t)v); c5|=(int64_t)(M[5][j]&(uint64_t)v); c6|=(int64_t)(M[6][j]&(uint64_t)v); c7|=(int64_t)(M[7][j]&(uint64_t)v); }
        a0=(__int128)c0+(__int128)al_mulhi(a0,xx[0]); a1=(__int128)c1+(__int128)al_mulhi(a1,xx[1]); a2=(__int128)c2+(__int128)al_mulhi(a2,xx[2]); a3=(__int128)c3+(__int128)al_mulhi(a3,xx[3]); a4=(__int128)c4+(__int128)al_mulhi(a4,xx[4]); a5=(__int128)c5+(__int128)al_mulhi(a5,xx[5]); a6=(__int128)c6+(__int128)al_mulhi(a6,xx[6]); a7=(__int128)c7+(__int128)al_mulhi(a7,xx[7]);
    }
    out[0]=(int64_t)a0; out[1]=(int64_t)a1; out[2]=(int64_t)a2; out[3]=(int64_t)a3; out[4]=(int64_t)a4; out[5]=(int64_t)a5; out[6]=(int64_t)a6; out[7]=(int64_t)a7;
}

/* g=3: 8 segments, degree 11, Q62, err 2^-60.047 */
static const int64_t kLogPoly_g3[8][12] = {
    {INT64_C(0), INT64_C(831657068615270112), INT64_C(-51978566788450843), INT64_C(4331547232285571), INT64_C(-406082552071460), INT64_C(40608249235662), INT64_C(-4230003058932), INT64_C(453157986150), INT64_C(-49472070931), INT64_C(5398974645), INT64_C(-541824113), INT64_C(36489788)},
    {INT64_C(783640753332765648), INT64_C(739250727658017880), INT64_C(-41069484869888161), INT64_C(3042184064403836), INT64_C(-253515338386937), INT64_C(22534694954287), INT64_C(-2086539343001), INT64_C(198702580306), INT64_C(-19293759583), INT64_C(1879810627), INT64_C(-170861826), INT64_C(10721324)},
    {INT64_C(1484631294131014398), INT64_C(665325654892216114), INT64_C(-33266282744610290), INT64_C(2217752182964330), INT64_C(-166331413628431), INT64_C(13306512553847), INT64_C(-1108874104497), INT64_C(95041727681), INT64_C(-8308807826), INT64_C(730860828), INT64_C(-60658900), INT64_C(3560231)},
    {INT64_C(2118754372093087486), INT64_C(604841504447469201), INT64_C(-27492795656702973), INT64_C(1666230039796937), INT64_C(-113606593591109), INT64_C(8262297536337), INT64_C(-625930982471), INT64_C(48772297455), INT64_C(-3877161142), INT64_C(310734010), INT64_C(-23714325), INT64_C(1307369)},
    {INT64_C(2697663385880076776), INT64_C(554438045743513436), INT64_C(-23101585239312996), INT64_C(1283421402182866), INT64_C(-80213837624913), INT64_C(5347589109245), INT64_C(-371360116965), INT64_C(26525156510), INT64_C(-1933227383), INT64_C(142260193), INT64_C(-10043662), INT64_C(522024)},
    {INT64_C(3230208054902495130), INT64_C(511788965301704711), INT64_C(-19684190973142464), INT64_C(1009445690929911), INT64_C(-58237251395240), INT64_C(3583830829048), INT64_C(-229732651263), INT64_C(15146983629), INT64_C(-1019152795), INT64_C(69312895), INT64_C(-4550918), INT64_C(223736)},
    {INT64_C(3723267405961586381), INT64_C(475232610637297232), INT64_C(-16972593237046319), INT64_C(808218725573435), INT64_C(-43297431725216), INT64_C(2474138944702), INT64_C(-147270135417), INT64_C(9016444057), INT64_C(-563377257), INT64_C(35612298), INT64_C(-2184608), INT64_C(101888)},
    {INT64_C(4182294680011091174), INT64_C(443550436594810750), INT64_C(-14785014553160354), INT64_C(657111757918148), INT64_C(-32855587895041), INT64_C(1752298016126), INT64_C(-97349871915), INT64_C(5562807311), INT64_C(-324429647), INT64_C(19154801), INT64_C(-1102353), INT64_C(48903)},
};
AL_INLINE int64_t log2_frac_g3(uint32_t sel, uint64_t x_q64){
    int64_t c[12]; for (int k=0;k<=11;k++) c[k]=0;
    for (uint32_t j=0;j<8;j++){ uint64_t m=al_eqmask(j,sel);
        for (int k=0;k<=11;k++) c[k]|=(int64_t)(m&(uint64_t)kLogPoly_g3[j][k]); }
    __int128 acc=c[11]; for (int k=10;k>=0;k--) acc=(__int128)c[k]+(__int128)al_mulhi(acc,x_q64);
    return (int64_t)acc;
}
AL_INLINE void log2_frac_g3_x2(const uint32_t sel[2], const uint64_t xx[2], int64_t out[2]){
    uint64_t M[2][8];
    for(uint32_t j=0;j<8;j++){ M[0][j]=al_eqmask(j,sel[0]); M[1][j]=al_eqmask(j,sel[1]); }
    int64_t h0=0; int64_t h1=0;
    for(uint32_t j=0;j<8;j++){ int64_t v=kLogPoly_g3[j][11];
        h0|=(int64_t)(M[0][j]&(uint64_t)v); h1|=(int64_t)(M[1][j]&(uint64_t)v); }
    __int128 a0=h0; __int128 a1=h1;
    for(int k=10;k>=0;k--){
        int64_t c0=0; int64_t c1=0;
        for(uint32_t j=0;j<8;j++){ int64_t v=kLogPoly_g3[j][k];
            c0|=(int64_t)(M[0][j]&(uint64_t)v); c1|=(int64_t)(M[1][j]&(uint64_t)v); }
        a0=(__int128)c0+(__int128)al_mulhi(a0,xx[0]); a1=(__int128)c1+(__int128)al_mulhi(a1,xx[1]);
    }
    out[0]=(int64_t)a0; out[1]=(int64_t)a1;
}
AL_INLINE void log2_frac_g3_x3(const uint32_t sel[3], const uint64_t xx[3], int64_t out[3]){
    uint64_t M[3][8];
    for(uint32_t j=0;j<8;j++){ M[0][j]=al_eqmask(j,sel[0]); M[1][j]=al_eqmask(j,sel[1]); M[2][j]=al_eqmask(j,sel[2]); }
    int64_t h0=0; int64_t h1=0; int64_t h2=0;
    for(uint32_t j=0;j<8;j++){ int64_t v=kLogPoly_g3[j][11];
        h0|=(int64_t)(M[0][j]&(uint64_t)v); h1|=(int64_t)(M[1][j]&(uint64_t)v); h2|=(int64_t)(M[2][j]&(uint64_t)v); }
    __int128 a0=h0; __int128 a1=h1; __int128 a2=h2;
    for(int k=10;k>=0;k--){
        int64_t c0=0; int64_t c1=0; int64_t c2=0;
        for(uint32_t j=0;j<8;j++){ int64_t v=kLogPoly_g3[j][k];
            c0|=(int64_t)(M[0][j]&(uint64_t)v); c1|=(int64_t)(M[1][j]&(uint64_t)v); c2|=(int64_t)(M[2][j]&(uint64_t)v); }
        a0=(__int128)c0+(__int128)al_mulhi(a0,xx[0]); a1=(__int128)c1+(__int128)al_mulhi(a1,xx[1]); a2=(__int128)c2+(__int128)al_mulhi(a2,xx[2]);
    }
    out[0]=(int64_t)a0; out[1]=(int64_t)a1; out[2]=(int64_t)a2;
}
AL_INLINE void log2_frac_g3_x4(const uint32_t sel[4], const uint64_t xx[4], int64_t out[4]){
    uint64_t M[4][8];
    for(uint32_t j=0;j<8;j++){ M[0][j]=al_eqmask(j,sel[0]); M[1][j]=al_eqmask(j,sel[1]); M[2][j]=al_eqmask(j,sel[2]); M[3][j]=al_eqmask(j,sel[3]); }
    int64_t h0=0; int64_t h1=0; int64_t h2=0; int64_t h3=0;
    for(uint32_t j=0;j<8;j++){ int64_t v=kLogPoly_g3[j][11];
        h0|=(int64_t)(M[0][j]&(uint64_t)v); h1|=(int64_t)(M[1][j]&(uint64_t)v); h2|=(int64_t)(M[2][j]&(uint64_t)v); h3|=(int64_t)(M[3][j]&(uint64_t)v); }
    __int128 a0=h0; __int128 a1=h1; __int128 a2=h2; __int128 a3=h3;
    for(int k=10;k>=0;k--){
        int64_t c0=0; int64_t c1=0; int64_t c2=0; int64_t c3=0;
        for(uint32_t j=0;j<8;j++){ int64_t v=kLogPoly_g3[j][k];
            c0|=(int64_t)(M[0][j]&(uint64_t)v); c1|=(int64_t)(M[1][j]&(uint64_t)v); c2|=(int64_t)(M[2][j]&(uint64_t)v); c3|=(int64_t)(M[3][j]&(uint64_t)v); }
        a0=(__int128)c0+(__int128)al_mulhi(a0,xx[0]); a1=(__int128)c1+(__int128)al_mulhi(a1,xx[1]); a2=(__int128)c2+(__int128)al_mulhi(a2,xx[2]); a3=(__int128)c3+(__int128)al_mulhi(a3,xx[3]);
    }
    out[0]=(int64_t)a0; out[1]=(int64_t)a1; out[2]=(int64_t)a2; out[3]=(int64_t)a3;
}
AL_INLINE void log2_frac_g3_x8(const uint32_t sel[8], const uint64_t xx[8], int64_t out[8]){
    uint64_t M[8][8];
    for(uint32_t j=0;j<8;j++){ M[0][j]=al_eqmask(j,sel[0]); M[1][j]=al_eqmask(j,sel[1]); M[2][j]=al_eqmask(j,sel[2]); M[3][j]=al_eqmask(j,sel[3]); M[4][j]=al_eqmask(j,sel[4]); M[5][j]=al_eqmask(j,sel[5]); M[6][j]=al_eqmask(j,sel[6]); M[7][j]=al_eqmask(j,sel[7]); }
    int64_t h0=0; int64_t h1=0; int64_t h2=0; int64_t h3=0; int64_t h4=0; int64_t h5=0; int64_t h6=0; int64_t h7=0;
    for(uint32_t j=0;j<8;j++){ int64_t v=kLogPoly_g3[j][11];
        h0|=(int64_t)(M[0][j]&(uint64_t)v); h1|=(int64_t)(M[1][j]&(uint64_t)v); h2|=(int64_t)(M[2][j]&(uint64_t)v); h3|=(int64_t)(M[3][j]&(uint64_t)v); h4|=(int64_t)(M[4][j]&(uint64_t)v); h5|=(int64_t)(M[5][j]&(uint64_t)v); h6|=(int64_t)(M[6][j]&(uint64_t)v); h7|=(int64_t)(M[7][j]&(uint64_t)v); }
    __int128 a0=h0; __int128 a1=h1; __int128 a2=h2; __int128 a3=h3; __int128 a4=h4; __int128 a5=h5; __int128 a6=h6; __int128 a7=h7;
    for(int k=10;k>=0;k--){
        int64_t c0=0; int64_t c1=0; int64_t c2=0; int64_t c3=0; int64_t c4=0; int64_t c5=0; int64_t c6=0; int64_t c7=0;
        for(uint32_t j=0;j<8;j++){ int64_t v=kLogPoly_g3[j][k];
            c0|=(int64_t)(M[0][j]&(uint64_t)v); c1|=(int64_t)(M[1][j]&(uint64_t)v); c2|=(int64_t)(M[2][j]&(uint64_t)v); c3|=(int64_t)(M[3][j]&(uint64_t)v); c4|=(int64_t)(M[4][j]&(uint64_t)v); c5|=(int64_t)(M[5][j]&(uint64_t)v); c6|=(int64_t)(M[6][j]&(uint64_t)v); c7|=(int64_t)(M[7][j]&(uint64_t)v); }
        a0=(__int128)c0+(__int128)al_mulhi(a0,xx[0]); a1=(__int128)c1+(__int128)al_mulhi(a1,xx[1]); a2=(__int128)c2+(__int128)al_mulhi(a2,xx[2]); a3=(__int128)c3+(__int128)al_mulhi(a3,xx[3]); a4=(__int128)c4+(__int128)al_mulhi(a4,xx[4]); a5=(__int128)c5+(__int128)al_mulhi(a5,xx[5]); a6=(__int128)c6+(__int128)al_mulhi(a6,xx[6]); a7=(__int128)c7+(__int128)al_mulhi(a7,xx[7]);
    }
    out[0]=(int64_t)a0; out[1]=(int64_t)a1; out[2]=(int64_t)a2; out[3]=(int64_t)a3; out[4]=(int64_t)a4; out[5]=(int64_t)a5; out[6]=(int64_t)a6; out[7]=(int64_t)a7;
}

/* g=4: 16 segments, degree 9, Q62, err 2^-60.334 */
static const int64_t kLogPoly_g4[16][10] = {
    {INT64_C(0), INT64_C(415828534307635015), INT64_C(-12994641697110172), INT64_C(541443403991144), INT64_C(-25380159155295), INT64_C(1269006320187), INT64_C(-66090184089), INT64_C(3534917731), INT64_C(-188462178), INT64_C(8172104)},
    {INT64_C(403351162126124448), INT64_C(391368032289538802), INT64_C(-11510824479100936), INT64_C(451404881492942), INT64_C(-19914920978712), INT64_C(937171765056), INT64_C(-45937544185), INT64_C(2313009833), INT64_C(-116353535), INT64_C(4811936)},
    {INT64_C(783640753332765648), INT64_C(369625363829008904), INT64_C(-10267371217470666), INT64_C(380273008031299), INT64_C(-15844708516911), INT64_C(704208702004), INT64_C(-32600966817), INT64_C(1550611892), INT64_C(-73831475), INT64_C(2917596)},
    {INT64_C(1143363847331251474), INT64_C(350171397311692665), INT64_C(-9215036771359269), INT64_C(323334623542791), INT64_C(-12763208734916), INT64_C(537397928913), INT64_C(-23569324790), INT64_C(1062207084), INT64_C(-48005827), INT64_C(1816274)},
    {INT64_C(1484631294131014398), INT64_C(332662827446108043), INT64_C(-8316570686152056), INT64_C(277219022863448), INT64_C(-10395713303367), INT64_C(415828330019), INT64_C(-17325720138), INT64_C(741882296), INT64_C(-31905212), INT64_C(1157821)},
    {INT64_C(1809244773414275253), INT64_C(316821740424864809), INT64_C(-7543374772020190), INT64_C(239472214979625), INT64_C(-8552579072896), INT64_C(325812410695), INT64_C(-12928778333), INT64_C(527302863), INT64_C(-21628427), INT64_C(754089)},
    {INT64_C(2118754372093087486), INT64_C(302420752223734594), INT64_C(-6873198914175532), INT64_C(208278754971754), INT64_C(-7100412080051), INT64_C(258196723378), INT64_C(-9779997827), INT64_C(380783749), INT64_C(-14927643), INT64_C(500813)},
    {INT64_C(2414503352528620721), INT64_C(289272023866180919), INT64_C(-6288522257960290), INT64_C(182276007474999), INT64_C(-5943782838693), INT64_C(206740221203), INT64_C(-7490470593), INT64_C(278982962), INT64_C(-10473034), INT64_C(338582)},
    {INT64_C(2697663385880076776), INT64_C(277219022871756715), INT64_C(-5775396309828157), INT64_C(160427675271614), INT64_C(-5013364843178), INT64_C(167112127548), INT64_C(-5802427274), INT64_C(207120861), INT64_C(-7458779), INT64_C(232670)},
    {INT64_C(2969262588262028797), INT64_C(266130261956886448), INT64_C(-5322605239137656), INT64_C(141936139709406), INT64_C(-4258084185213), INT64_C(136258671225), INT64_C(-4541904028), INT64_C(155649629), INT64_C(-5385784), INT64_C(162308)},
    {INT64_C(3230208054902495130), INT64_C(255894482650852354), INT64_C(-4921047743285573), INT64_C(126180711365663), INT64_C(-3639828208342), INT64_C(111994698659), INT64_C(-3589538521), INT64_C(118286920), INT64_C(-3938671), INT64_C(114804)},
    {INT64_C(3481304139212842424), INT64_C(246416909219339304), INT64_C(-4563276096654397), INT64_C(112673483867572), INT64_C(-3129818993472), INT64_C(92735366927), INT64_C(-2862178425), INT64_C(90828641), INT64_C(-2914446), INT64_C(82253)},
    {INT64_C(3723267405961586381), INT64_C(237616305318648615), INT64_C(-4243148309261559), INT64_C(101027340696398), INT64_C(-2706089480946), INT64_C(77316834860), INT64_C(-2301079313), INT64_C(70417314), INT64_C(-2180212), INT64_C(59637)},
    {INT64_C(3956738959306241175), INT64_C(229422639618005560), INT64_C(-3955562752034562), INT64_C(90932477058049), INT64_C(-2351701991470), INT64_C(64874532419), INT64_C(-1864198693), INT64_C(55082557), INT64_C(-1647583), INT64_C(43722)},
    {INT64_C(4182294680011091174), INT64_C(221775218297405374), INT64_C(-3696253638290077), INT64_C(82138969739624), INT64_C(-2053474242480), INT64_C(54759309354), INT64_C(-1521083326), INT64_C(43447412), INT64_C(-1256911), INT64_C(32388)},
    {INT64_C(4400453783446152532), INT64_C(214621178997489072), INT64_C(-3461631919314331), INT64_C(74443697189443), INT64_C(-1801057189335), INT64_C(46478892473), INT64_C(-1249426367), INT64_C(34537584), INT64_C(-967391), INT64_C(24225)},
};
AL_INLINE int64_t log2_frac_g4(uint32_t sel, uint64_t x_q64){
    int64_t c[10]; for (int k=0;k<=9;k++) c[k]=0;
    for (uint32_t j=0;j<16;j++){ uint64_t m=al_eqmask(j,sel);
        for (int k=0;k<=9;k++) c[k]|=(int64_t)(m&(uint64_t)kLogPoly_g4[j][k]); }
    __int128 acc=c[9]; for (int k=8;k>=0;k--) acc=(__int128)c[k]+(__int128)al_mulhi(acc,x_q64);
    return (int64_t)acc;
}
AL_INLINE void log2_frac_g4_x2(const uint32_t sel[2], const uint64_t xx[2], int64_t out[2]){
    uint64_t M[2][16];
    for(uint32_t j=0;j<16;j++){ M[0][j]=al_eqmask(j,sel[0]); M[1][j]=al_eqmask(j,sel[1]); }
    int64_t h0=0; int64_t h1=0;
    for(uint32_t j=0;j<16;j++){ int64_t v=kLogPoly_g4[j][9];
        h0|=(int64_t)(M[0][j]&(uint64_t)v); h1|=(int64_t)(M[1][j]&(uint64_t)v); }
    __int128 a0=h0; __int128 a1=h1;
    for(int k=8;k>=0;k--){
        int64_t c0=0; int64_t c1=0;
        for(uint32_t j=0;j<16;j++){ int64_t v=kLogPoly_g4[j][k];
            c0|=(int64_t)(M[0][j]&(uint64_t)v); c1|=(int64_t)(M[1][j]&(uint64_t)v); }
        a0=(__int128)c0+(__int128)al_mulhi(a0,xx[0]); a1=(__int128)c1+(__int128)al_mulhi(a1,xx[1]);
    }
    out[0]=(int64_t)a0; out[1]=(int64_t)a1;
}
AL_INLINE void log2_frac_g4_x3(const uint32_t sel[3], const uint64_t xx[3], int64_t out[3]){
    uint64_t M[3][16];
    for(uint32_t j=0;j<16;j++){ M[0][j]=al_eqmask(j,sel[0]); M[1][j]=al_eqmask(j,sel[1]); M[2][j]=al_eqmask(j,sel[2]); }
    int64_t h0=0; int64_t h1=0; int64_t h2=0;
    for(uint32_t j=0;j<16;j++){ int64_t v=kLogPoly_g4[j][9];
        h0|=(int64_t)(M[0][j]&(uint64_t)v); h1|=(int64_t)(M[1][j]&(uint64_t)v); h2|=(int64_t)(M[2][j]&(uint64_t)v); }
    __int128 a0=h0; __int128 a1=h1; __int128 a2=h2;
    for(int k=8;k>=0;k--){
        int64_t c0=0; int64_t c1=0; int64_t c2=0;
        for(uint32_t j=0;j<16;j++){ int64_t v=kLogPoly_g4[j][k];
            c0|=(int64_t)(M[0][j]&(uint64_t)v); c1|=(int64_t)(M[1][j]&(uint64_t)v); c2|=(int64_t)(M[2][j]&(uint64_t)v); }
        a0=(__int128)c0+(__int128)al_mulhi(a0,xx[0]); a1=(__int128)c1+(__int128)al_mulhi(a1,xx[1]); a2=(__int128)c2+(__int128)al_mulhi(a2,xx[2]);
    }
    out[0]=(int64_t)a0; out[1]=(int64_t)a1; out[2]=(int64_t)a2;
}
AL_INLINE void log2_frac_g4_x4(const uint32_t sel[4], const uint64_t xx[4], int64_t out[4]){
    uint64_t M[4][16];
    for(uint32_t j=0;j<16;j++){ M[0][j]=al_eqmask(j,sel[0]); M[1][j]=al_eqmask(j,sel[1]); M[2][j]=al_eqmask(j,sel[2]); M[3][j]=al_eqmask(j,sel[3]); }
    int64_t h0=0; int64_t h1=0; int64_t h2=0; int64_t h3=0;
    for(uint32_t j=0;j<16;j++){ int64_t v=kLogPoly_g4[j][9];
        h0|=(int64_t)(M[0][j]&(uint64_t)v); h1|=(int64_t)(M[1][j]&(uint64_t)v); h2|=(int64_t)(M[2][j]&(uint64_t)v); h3|=(int64_t)(M[3][j]&(uint64_t)v); }
    __int128 a0=h0; __int128 a1=h1; __int128 a2=h2; __int128 a3=h3;
    for(int k=8;k>=0;k--){
        int64_t c0=0; int64_t c1=0; int64_t c2=0; int64_t c3=0;
        for(uint32_t j=0;j<16;j++){ int64_t v=kLogPoly_g4[j][k];
            c0|=(int64_t)(M[0][j]&(uint64_t)v); c1|=(int64_t)(M[1][j]&(uint64_t)v); c2|=(int64_t)(M[2][j]&(uint64_t)v); c3|=(int64_t)(M[3][j]&(uint64_t)v); }
        a0=(__int128)c0+(__int128)al_mulhi(a0,xx[0]); a1=(__int128)c1+(__int128)al_mulhi(a1,xx[1]); a2=(__int128)c2+(__int128)al_mulhi(a2,xx[2]); a3=(__int128)c3+(__int128)al_mulhi(a3,xx[3]);
    }
    out[0]=(int64_t)a0; out[1]=(int64_t)a1; out[2]=(int64_t)a2; out[3]=(int64_t)a3;
}
AL_INLINE void log2_frac_g4_x8(const uint32_t sel[8], const uint64_t xx[8], int64_t out[8]){
    uint64_t M[8][16];
    for(uint32_t j=0;j<16;j++){ M[0][j]=al_eqmask(j,sel[0]); M[1][j]=al_eqmask(j,sel[1]); M[2][j]=al_eqmask(j,sel[2]); M[3][j]=al_eqmask(j,sel[3]); M[4][j]=al_eqmask(j,sel[4]); M[5][j]=al_eqmask(j,sel[5]); M[6][j]=al_eqmask(j,sel[6]); M[7][j]=al_eqmask(j,sel[7]); }
    int64_t h0=0; int64_t h1=0; int64_t h2=0; int64_t h3=0; int64_t h4=0; int64_t h5=0; int64_t h6=0; int64_t h7=0;
    for(uint32_t j=0;j<16;j++){ int64_t v=kLogPoly_g4[j][9];
        h0|=(int64_t)(M[0][j]&(uint64_t)v); h1|=(int64_t)(M[1][j]&(uint64_t)v); h2|=(int64_t)(M[2][j]&(uint64_t)v); h3|=(int64_t)(M[3][j]&(uint64_t)v); h4|=(int64_t)(M[4][j]&(uint64_t)v); h5|=(int64_t)(M[5][j]&(uint64_t)v); h6|=(int64_t)(M[6][j]&(uint64_t)v); h7|=(int64_t)(M[7][j]&(uint64_t)v); }
    __int128 a0=h0; __int128 a1=h1; __int128 a2=h2; __int128 a3=h3; __int128 a4=h4; __int128 a5=h5; __int128 a6=h6; __int128 a7=h7;
    for(int k=8;k>=0;k--){
        int64_t c0=0; int64_t c1=0; int64_t c2=0; int64_t c3=0; int64_t c4=0; int64_t c5=0; int64_t c6=0; int64_t c7=0;
        for(uint32_t j=0;j<16;j++){ int64_t v=kLogPoly_g4[j][k];
            c0|=(int64_t)(M[0][j]&(uint64_t)v); c1|=(int64_t)(M[1][j]&(uint64_t)v); c2|=(int64_t)(M[2][j]&(uint64_t)v); c3|=(int64_t)(M[3][j]&(uint64_t)v); c4|=(int64_t)(M[4][j]&(uint64_t)v); c5|=(int64_t)(M[5][j]&(uint64_t)v); c6|=(int64_t)(M[6][j]&(uint64_t)v); c7|=(int64_t)(M[7][j]&(uint64_t)v); }
        a0=(__int128)c0+(__int128)al_mulhi(a0,xx[0]); a1=(__int128)c1+(__int128)al_mulhi(a1,xx[1]); a2=(__int128)c2+(__int128)al_mulhi(a2,xx[2]); a3=(__int128)c3+(__int128)al_mulhi(a3,xx[3]); a4=(__int128)c4+(__int128)al_mulhi(a4,xx[4]); a5=(__int128)c5+(__int128)al_mulhi(a5,xx[5]); a6=(__int128)c6+(__int128)al_mulhi(a6,xx[6]); a7=(__int128)c7+(__int128)al_mulhi(a7,xx[7]);
    }
    out[0]=(int64_t)a0; out[1]=(int64_t)a1; out[2]=(int64_t)a2; out[3]=(int64_t)a3; out[4]=(int64_t)a4; out[5]=(int64_t)a5; out[6]=(int64_t)a6; out[7]=(int64_t)a7;
}

/* g=5: 32 segments, degree 8, Q62, err 2^-60.002 */
static const int64_t kLogPoly_g5[32][9] = {
    {INT64_C(0), INT64_C(207914267153817524), INT64_C(-3248660424277897), INT64_C(67680425500222), INT64_C(-1586259942778), INT64_C(39656409866), INT64_C(-1032565615), INT64_C(27504174), INT64_C(-669137)},
    {INT64_C(204731739545776358), INT64_C(201613834815823051), INT64_C(-3054755072966555), INT64_C(61712223691548), INT64_C(-1402550513993), INT64_C(34001153840), INT64_C(-858494799), INT64_C(22179560), INT64_C(-524564)},
    {INT64_C(403351162126124447), INT64_C(195684016144769435), INT64_C(-2877706119775667), INT64_C(56425610188038), INT64_C(-1244682558918), INT64_C(29286594176), INT64_C(-717716475), INT64_C(18002403), INT64_C(-414565)},
    {INT64_C(596212681665212875), INT64_C(190093044254918883), INT64_C(-2715614917927139), INT64_C(51725998433892), INT64_C(-1108414237654), INT64_C(25335140625), INT64_C(-603146362), INT64_C(14700370), INT64_C(-329843)},
    {INT64_C(783640753332765648), INT64_C(184812681914504471), INT64_C(-2566842804367905), INT64_C(47534126004621), INT64_C(-990294280473), INT64_C(22006506906), INT64_C(-509354395), INT64_C(12072554), INT64_C(-264107)},
    {INT64_C(965933157783587320), INT64_C(179817744565463811), INT64_C(-2429969521154749), INT64_C(43783234613683), INT64_C(-887497990076), INT64_C(19189120131), INT64_C(-432143899), INT64_C(9968000), INT64_C(-212747)},
    {INT64_C(1143363847331251474), INT64_C(175085698655846344), INT64_C(-2303759192839952), INT64_C(40416827943205), INT64_C(-797700544529), INT64_C(16793675479), INT64_C(-368248048), INT64_C(8272383), INT64_C(-172353)},
    {INT64_C(1316185422355184002), INT64_C(170596321767234900), INT64_C(-2187132330349061), INT64_C(37386877440788), INT64_C(-718978406781), INT64_C(14748258981), INT64_C(-315106508), INT64_C(6898462), INT64_C(-140385)},
    {INT64_C(1484631294131014398), INT64_C(166331413723054028), INT64_C(-2079142671538092), INT64_C(34652377858109), INT64_C(-649732080415), INT64_C(12994628812), INT64_C(-270699696), INT64_C(5779189), INT64_C(-114934)},
    {INT64_C(1648917580557901400), INT64_C(162274549973711247), INT64_C(-1978957926508607), INT64_C(32178177666117), INT64_C(-588625197658), INT64_C(11485359438), INT64_C(-233424816), INT64_C(4862689), INT64_C(-94558)},
    {INT64_C(1809244773414275253), INT64_C(158410870212432409), INT64_C(-1885843693005094), INT64_C(29934026872540), INT64_C(-534536191287), INT64_C(10181633450), INT64_C(-202002457), INT64_C(4108562), INT64_C(-78156)},
    {INT64_C(1965799209408045220), INT64_C(154726896486561888), INT64_C(-1799149959146025), INT64_C(27893797815766), INT64_C(-486519727021), INT64_C(9051523083), INT64_C(-175405696), INT64_C(3485157), INT64_C(-64887)},
    {INT64_C(2118754372093087486), INT64_C(151210376111867300), INT64_C(-1718299728543911), INT64_C(26034844371510), INT64_C(-443775754439), INT64_C(8068644603), INT64_C(-152805935), INT64_C(2967530), INT64_C(-54098)},
    {INT64_C(2268272047463780046), INT64_C(147850145531603582), INT64_C(-1642779394795566), INT64_C(24337472515189), INT64_C(-405624540369), INT64_C(7211098456), INT64_C(-133531245), INT64_C(2535914), INT64_C(-45286)},
    {INT64_C(2414503352528620721), INT64_C(144636011933090461), INT64_C(-1572130564490090), INT64_C(22784500934391), INT64_C(-371486427004), INT64_C(6460629827), INT64_C(-117034135), INT64_C(2174563), INT64_C(-38057)},
    {INT64_C(2557589653257460679), INT64_C(141558649977067260), INT64_C(-1505943084862398), INT64_C(21360894820539), INT64_C(-340865341829), INT64_C(5801960222), INT64_C(-102866466), INT64_C(1870868), INT64_C(-32100)},
    {INT64_C(2697663385880076776), INT64_C(138609511435878359), INT64_C(-1443849077457050), INT64_C(20053459408956), INT64_C(-313335302393), INT64_C(5222252517), INT64_C(-90659822), INT64_C(1614685), INT64_C(-27171)},
    {INT64_C(2834848793495784858), INT64_C(135780745896370637), INT64_C(-1385517815269074), INT64_C(18850582520527), INT64_C(-288529323568), INT64_C(4710680735), INT64_C(-80110053), INT64_C(1397814), INT64_C(-23078)},
    {INT64_C(2969262588262028797), INT64_C(133065130978443224), INT64_C(-1330651309784421), INT64_C(17742017463675), INT64_C(-266130261349), INT64_C(4258082429), INT64_C(-70965064), INT64_C(1213598), INT64_C(-19666)},
    {INT64_C(3101014548006201223), INT64_C(130456010763179632), INT64_C(-1278980497678222), INT64_C(16718699315957), INT64_C(-245863224727), INT64_C(3856676566), INT64_C(-63015096), INT64_C(1056608), INT64_C(-16810)},
    {INT64_C(3230208054902495130), INT64_C(127947241325426178), INT64_C(-1230261935821397), INT64_C(15772588920704), INT64_C(-227489262852), INT64_C(3499833578), INT64_C(-56084979), INT64_C(922401), INT64_C(-14413)},
    {INT64_C(3356940582836414350), INT64_C(125533142432493608), INT64_C(-1184274928608423), INT64_C(14896539982426), INT64_C(-210800093730), INT64_C(3181887165), INT64_C(-50027902), INT64_C(807325), INT64_C(-12394)},
    {INT64_C(3481304139212842424), INT64_C(123208454609669652), INT64_C(-1140819024163602), INT64_C(14084185483442), INT64_C(-195613686965), INT64_C(2897979665), INT64_C(-44720410), INT64_C(708365), INT64_C(-10687)},
    {INT64_C(3603385666224101885), INT64_C(120968300889493841), INT64_C(-1099711826268121), INT64_C(13329840318351), INT64_C(-181770549537), INT64_C(2643934517), INT64_C(-40058342), INT64_C(623027), INT64_C(-9240)},
    {INT64_C(3723267405961586381), INT64_C(118808152659324308), INT64_C(-1060787077315391), INT64_C(12628417587045), INT64_C(-169130592463), INT64_C(2416150683), INT64_C(-35953540), INT64_C(549238), INT64_C(-8010)},
    {INT64_C(3841027233211328250), INT64_C(116723799103897566), INT64_C(-1023892974595589), INT64_C(11975356427982), INT64_C(-157570479127), INT64_C(2211514952), INT64_C(-32331165), INT64_C(485268), INT64_C(-6961)},
    {INT64_C(3956738959306241175), INT64_C(114711319809002780), INT64_C(-988890688008642), INT64_C(11366559632252), INT64_C(-146981374394), INT64_C(2027328836), INT64_C(-29127496), INT64_C(429672), INT64_C(-6064)},
    {INT64_C(4070472610004118119), INT64_C(112767060151223072), INT64_C(-955653052129006), INT64_C(10798339572052), INT64_C(-137267028320), INT64_C(1861247441), INT64_C(-26288126), INT64_C(381237), INT64_C(-5295)},
    {INT64_C(4182294680011091174), INT64_C(110887609148702687), INT64_C(-924063409572520), INT64_C(10267371217449), INT64_C(-128342140099), INT64_C(1711228190), INT64_C(-23766473), INT64_C(338942), INT64_C(-4634)},
    {INT64_C(4292268366467094717), INT64_C(109069779490527233), INT64_C(-894014585987926), INT64_C(9770651212963), INT64_C(-120130957434), INT64_C(1575487669), INT64_C(-21522551), INT64_C(301925), INT64_C(-4064)},
    {INT64_C(4400453783446152532), INT64_C(107310589498744536), INT64_C(-865407979828583), INT64_C(9305462148677), INT64_C(-112566074290), INT64_C(1452465218), INT64_C(-19521946), INT64_C(269457), INT64_C(-3572)},
    {INT64_C(4506908159294352029), INT64_C(105607246808288274), INT64_C(-838152752446731), INT64_C(8869341295718), INT64_C(-105587396301), INT64_C(1340792111), INT64_C(-17734967), INT64_C(240918), INT64_C(-3146)},
};
AL_INLINE int64_t log2_frac_g5(uint32_t sel, uint64_t x_q64){
    int64_t c[9]; for (int k=0;k<=8;k++) c[k]=0;
    for (uint32_t j=0;j<32;j++){ uint64_t m=al_eqmask(j,sel);
        for (int k=0;k<=8;k++) c[k]|=(int64_t)(m&(uint64_t)kLogPoly_g5[j][k]); }
    __int128 acc=c[8]; for (int k=7;k>=0;k--) acc=(__int128)c[k]+(__int128)al_mulhi(acc,x_q64);
    return (int64_t)acc;
}
AL_INLINE void log2_frac_g5_x2(const uint32_t sel[2], const uint64_t xx[2], int64_t out[2]){
    uint64_t M[2][32];
    for(uint32_t j=0;j<32;j++){ M[0][j]=al_eqmask(j,sel[0]); M[1][j]=al_eqmask(j,sel[1]); }
    int64_t h0=0; int64_t h1=0;
    for(uint32_t j=0;j<32;j++){ int64_t v=kLogPoly_g5[j][8];
        h0|=(int64_t)(M[0][j]&(uint64_t)v); h1|=(int64_t)(M[1][j]&(uint64_t)v); }
    __int128 a0=h0; __int128 a1=h1;
    for(int k=7;k>=0;k--){
        int64_t c0=0; int64_t c1=0;
        for(uint32_t j=0;j<32;j++){ int64_t v=kLogPoly_g5[j][k];
            c0|=(int64_t)(M[0][j]&(uint64_t)v); c1|=(int64_t)(M[1][j]&(uint64_t)v); }
        a0=(__int128)c0+(__int128)al_mulhi(a0,xx[0]); a1=(__int128)c1+(__int128)al_mulhi(a1,xx[1]);
    }
    out[0]=(int64_t)a0; out[1]=(int64_t)a1;
}
AL_INLINE void log2_frac_g5_x3(const uint32_t sel[3], const uint64_t xx[3], int64_t out[3]){
    uint64_t M[3][32];
    for(uint32_t j=0;j<32;j++){ M[0][j]=al_eqmask(j,sel[0]); M[1][j]=al_eqmask(j,sel[1]); M[2][j]=al_eqmask(j,sel[2]); }
    int64_t h0=0; int64_t h1=0; int64_t h2=0;
    for(uint32_t j=0;j<32;j++){ int64_t v=kLogPoly_g5[j][8];
        h0|=(int64_t)(M[0][j]&(uint64_t)v); h1|=(int64_t)(M[1][j]&(uint64_t)v); h2|=(int64_t)(M[2][j]&(uint64_t)v); }
    __int128 a0=h0; __int128 a1=h1; __int128 a2=h2;
    for(int k=7;k>=0;k--){
        int64_t c0=0; int64_t c1=0; int64_t c2=0;
        for(uint32_t j=0;j<32;j++){ int64_t v=kLogPoly_g5[j][k];
            c0|=(int64_t)(M[0][j]&(uint64_t)v); c1|=(int64_t)(M[1][j]&(uint64_t)v); c2|=(int64_t)(M[2][j]&(uint64_t)v); }
        a0=(__int128)c0+(__int128)al_mulhi(a0,xx[0]); a1=(__int128)c1+(__int128)al_mulhi(a1,xx[1]); a2=(__int128)c2+(__int128)al_mulhi(a2,xx[2]);
    }
    out[0]=(int64_t)a0; out[1]=(int64_t)a1; out[2]=(int64_t)a2;
}
AL_INLINE void log2_frac_g5_x4(const uint32_t sel[4], const uint64_t xx[4], int64_t out[4]){
    uint64_t M[4][32];
    for(uint32_t j=0;j<32;j++){ M[0][j]=al_eqmask(j,sel[0]); M[1][j]=al_eqmask(j,sel[1]); M[2][j]=al_eqmask(j,sel[2]); M[3][j]=al_eqmask(j,sel[3]); }
    int64_t h0=0; int64_t h1=0; int64_t h2=0; int64_t h3=0;
    for(uint32_t j=0;j<32;j++){ int64_t v=kLogPoly_g5[j][8];
        h0|=(int64_t)(M[0][j]&(uint64_t)v); h1|=(int64_t)(M[1][j]&(uint64_t)v); h2|=(int64_t)(M[2][j]&(uint64_t)v); h3|=(int64_t)(M[3][j]&(uint64_t)v); }
    __int128 a0=h0; __int128 a1=h1; __int128 a2=h2; __int128 a3=h3;
    for(int k=7;k>=0;k--){
        int64_t c0=0; int64_t c1=0; int64_t c2=0; int64_t c3=0;
        for(uint32_t j=0;j<32;j++){ int64_t v=kLogPoly_g5[j][k];
            c0|=(int64_t)(M[0][j]&(uint64_t)v); c1|=(int64_t)(M[1][j]&(uint64_t)v); c2|=(int64_t)(M[2][j]&(uint64_t)v); c3|=(int64_t)(M[3][j]&(uint64_t)v); }
        a0=(__int128)c0+(__int128)al_mulhi(a0,xx[0]); a1=(__int128)c1+(__int128)al_mulhi(a1,xx[1]); a2=(__int128)c2+(__int128)al_mulhi(a2,xx[2]); a3=(__int128)c3+(__int128)al_mulhi(a3,xx[3]);
    }
    out[0]=(int64_t)a0; out[1]=(int64_t)a1; out[2]=(int64_t)a2; out[3]=(int64_t)a3;
}
AL_INLINE void log2_frac_g5_x8(const uint32_t sel[8], const uint64_t xx[8], int64_t out[8]){
    uint64_t M[8][32];
    for(uint32_t j=0;j<32;j++){ M[0][j]=al_eqmask(j,sel[0]); M[1][j]=al_eqmask(j,sel[1]); M[2][j]=al_eqmask(j,sel[2]); M[3][j]=al_eqmask(j,sel[3]); M[4][j]=al_eqmask(j,sel[4]); M[5][j]=al_eqmask(j,sel[5]); M[6][j]=al_eqmask(j,sel[6]); M[7][j]=al_eqmask(j,sel[7]); }
    int64_t h0=0; int64_t h1=0; int64_t h2=0; int64_t h3=0; int64_t h4=0; int64_t h5=0; int64_t h6=0; int64_t h7=0;
    for(uint32_t j=0;j<32;j++){ int64_t v=kLogPoly_g5[j][8];
        h0|=(int64_t)(M[0][j]&(uint64_t)v); h1|=(int64_t)(M[1][j]&(uint64_t)v); h2|=(int64_t)(M[2][j]&(uint64_t)v); h3|=(int64_t)(M[3][j]&(uint64_t)v); h4|=(int64_t)(M[4][j]&(uint64_t)v); h5|=(int64_t)(M[5][j]&(uint64_t)v); h6|=(int64_t)(M[6][j]&(uint64_t)v); h7|=(int64_t)(M[7][j]&(uint64_t)v); }
    __int128 a0=h0; __int128 a1=h1; __int128 a2=h2; __int128 a3=h3; __int128 a4=h4; __int128 a5=h5; __int128 a6=h6; __int128 a7=h7;
    for(int k=7;k>=0;k--){
        int64_t c0=0; int64_t c1=0; int64_t c2=0; int64_t c3=0; int64_t c4=0; int64_t c5=0; int64_t c6=0; int64_t c7=0;
        for(uint32_t j=0;j<32;j++){ int64_t v=kLogPoly_g5[j][k];
            c0|=(int64_t)(M[0][j]&(uint64_t)v); c1|=(int64_t)(M[1][j]&(uint64_t)v); c2|=(int64_t)(M[2][j]&(uint64_t)v); c3|=(int64_t)(M[3][j]&(uint64_t)v); c4|=(int64_t)(M[4][j]&(uint64_t)v); c5|=(int64_t)(M[5][j]&(uint64_t)v); c6|=(int64_t)(M[6][j]&(uint64_t)v); c7|=(int64_t)(M[7][j]&(uint64_t)v); }
        a0=(__int128)c0+(__int128)al_mulhi(a0,xx[0]); a1=(__int128)c1+(__int128)al_mulhi(a1,xx[1]); a2=(__int128)c2+(__int128)al_mulhi(a2,xx[2]); a3=(__int128)c3+(__int128)al_mulhi(a3,xx[3]); a4=(__int128)c4+(__int128)al_mulhi(a4,xx[4]); a5=(__int128)c5+(__int128)al_mulhi(a5,xx[5]); a6=(__int128)c6+(__int128)al_mulhi(a6,xx[6]); a7=(__int128)c7+(__int128)al_mulhi(a7,xx[7]);
    }
    out[0]=(int64_t)a0; out[1]=(int64_t)a1; out[2]=(int64_t)a2; out[3]=(int64_t)a3; out[4]=(int64_t)a4; out[5]=(int64_t)a5; out[6]=(int64_t)a6; out[7]=(int64_t)a7;
}

/* g=6: 64 segments, degree 7, Q62, err 2^-60.228 */
static const int64_t kLogPoly_g6[64][8] = {
    {INT64_C(0), INT64_C(103957133576908765), INT64_C(-812165106069442), INT64_C(8460053186694), INT64_C(-99141241660), INT64_C(1239250659), INT64_C(-16118065), INT64_C(204675)},
    {INT64_C(103153330606121625), INT64_C(102357793060340933), INT64_C(-787367638925497), INT64_C(8075565525823), INT64_C(-93179595859), INT64_C(1146812212), INT64_C(-14686657), INT64_C(183779)},
    {INT64_C(204731739545776358), INT64_C(100806917407911526), INT64_C(-763688768241575), INT64_C(7714027960603), INT64_C(-87659403008), INT64_C(1062526186), INT64_C(-13401506), INT64_C(165285)},
    {INT64_C(304782595603293869), INT64_C(99302336551077026), INT64_C(-741062213067580), INT64_C(7373753362585), INT64_C(-82542010252), INT64_C(985565565), INT64_C(-12245652), INT64_C(148887)},
    {INT64_C(403351162126124447), INT64_C(97842008072384717), INT64_C(-719426529943864), INT64_C(7053201272829), INT64_C(-77792656652), INT64_C(915198234), INT64_C(-11204441), INT64_C(134322)},
    {INT64_C(500480719981309593), INT64_C(96424007955393635), INT64_C(-698724695328814), INT64_C(6750963238885), INT64_C(-73380031247), INT64_C(850774520), INT64_C(-10265061), INT64_C(121363)},
    {INT64_C(596212681665212875), INT64_C(95046522127459441), INT64_C(-678903729481741), INT64_C(6465749803690), INT64_C(-69275887223), INT64_C(791716888), INT64_C(-9416287), INT64_C(109813)},
    {INT64_C(690586697319517456), INT64_C(93707838717213534), INT64_C(-659914357163375), INT64_C(6196378939699), INT64_C(-65454703955), INT64_C(737511057), INT64_C(-8648270), INT64_C(99503)},
    {INT64_C(783640753332765648), INT64_C(92406340957252235), INT64_C(-641710701091940), INT64_C(5941765750133), INT64_C(-61893390408), INT64_C(687698313), INT64_C(-7952346), INT64_C(90284)},
    {INT64_C(875411264141121919), INT64_C(91140500670166589), INT64_C(-624250004590102), INT64_C(5700913283285), INT64_C(-58571024354), INT64_C(641868840), INT64_C(-7320885), INT64_C(82028)},
    {INT64_C(965933157783587320), INT64_C(89908872282731905), INT64_C(-607492380288657), INT64_C(5472904326348), INT64_C(-55468622659), INT64_C(599655940), INT64_C(-6747152), INT64_C(74624)},
    {INT64_C(1055239955714717669), INT64_C(88710087318962147), INT64_C(-591400582126350), INT64_C(5256894062826), INT64_C(-52568938588), INT64_C(560730990), INT64_C(-6225194), INT64_C(67974)},
    {INT64_C(1143363847331251474), INT64_C(87542849327923172), INT64_C(-575939798209963), INT64_C(5052103492603), INT64_C(-49856282630), INT64_C(524799055), INT64_C(-5749740), INT64_C(61993)},
    {INT64_C(1230335759627285963), INT64_C(86405929206781312), INT64_C(-561077462381644), INT64_C(4857813526693), INT64_C(-47316363865), INT64_C(491595053), INT64_C(-5316115), INT64_C(56606)},
    {INT64_C(1316185422355184002), INT64_C(85298160883617450), INT64_C(-546783082587244), INT64_C(4673359679853), INT64_C(-44936149274), INT64_C(460880394), INT64_C(-4920166), INT64_C(51747)},
    {INT64_C(1400941429035756762), INT64_C(84218437328128621), INT64_C(-533028084355202), INT64_C(4498127293793), INT64_C(-42703738781), INT64_C(432440043), INT64_C(-4558197), INT64_C(47359)},
    {INT64_C(1484631294131014398), INT64_C(83165706861527014), INT64_C(-519785667884505), INT64_C(4331547232060), INT64_C(-40608254079), INT64_C(406079934), INT64_C(-4226917), INT64_C(43391)},
    {INT64_C(1567281506665531296), INT64_C(82138969739779767), INT64_C(-507030677406013), INT64_C(4173091994830), INT64_C(-38639739587), INT64_C(381624696), INT64_C(-3923388), INT64_C(39798)},
    {INT64_C(1648917580557901400), INT64_C(81137274986855623), INT64_C(-494739481627137), INT64_C(4022272208096), INT64_C(-36789074071), INT64_C(358915654), INT64_C(-3644984), INT64_C(36542)},
    {INT64_C(1729564101901571123), INT64_C(80159717456893508), INT64_C(-482889864198125), INT64_C(3878633447143), INT64_C(-35047891684), INT64_C(337809059), INT64_C(-3389358), INT64_C(33586)},
    {INT64_C(1809244773414275253), INT64_C(79205435106216204), INT64_C(-471460923251261), INT64_C(3741753358926), INT64_C(-33408511305), INT64_C(318174530), INT64_C(-3154403), INT64_C(30901)},
    {INT64_C(1887982456257138845), INT64_C(78273606457907778), INT64_C(-460432979164139), INT64_C(3611239052076), INT64_C(-31863873235), INT64_C(299893668), INT64_C(-2938231), INT64_C(28458)},
    {INT64_C(1965799209408045220), INT64_C(77363448243280944), INT64_C(-449787489786495), INT64_C(3486724726852), INT64_C(-30407482396), INT64_C(282858835), INT64_C(-2739147), INT64_C(26233)},
    {INT64_C(2042716326758930047), INT64_C(76474213206001852), INT64_C(-439506972448267), INT64_C(3367869520517), INT64_C(-29033357309), INT64_C(266972063), INT64_C(-2555623), INT64_C(24205)},
    {INT64_C(2118754372093087486), INT64_C(75605188055933650), INT64_C(-429574932135968), INT64_C(3254355546339), INT64_C(-27735984198), INT64_C(252144090), INT64_C(-2386284), INT64_C(22354)},
    {INT64_C(2193933212086227468), INT64_C(74755691560923159), INT64_C(-419975795286069), INT64_C(3145886106879), INT64_C(-26510275659), INT64_C(238293497), INT64_C(-2229893), INT64_C(20663)},
    {INT64_C(2268272047463780046), INT64_C(73925072765801791), INT64_C(-410694848698884), INT64_C(3042184064315), INT64_C(-25351533391), INT64_C(225345942), INT64_C(-2085329), INT64_C(19117)},
    {INT64_C(2341789442436693607), INT64_C(73112709328814958), INT64_C(-401718183125343), INT64_C(2942990352455), INT64_C(-24255414555), INT64_C(213233479), INT64_C(-1951582), INT64_C(17702)},
    {INT64_C(2414503352528620721), INT64_C(72318005966545230), INT64_C(-393032641122516), INT64_C(2848062616728), INT64_C(-23217901365), INT64_C(201893937), INT64_C(-1827736), INT64_C(16405)},
    {INT64_C(2486431150898841404), INT64_C(71540392999163024), INT64_C(-384625768812693), INT64_C(2757173969890), INT64_C(-22235273582), INT64_C(191270384), INT64_C(-1712963), INT64_C(15215)},
    {INT64_C(2557589653257460679), INT64_C(70779324988533630), INT64_C(-376485771215594), INT64_C(2670111852507), INT64_C(-21304083591), INT64_C(181310627), INT64_C(-1606510), INT64_C(14123)},
    {INT64_C(2627995141462265872), INT64_C(70034279462338539), INT64_C(-368601470854403), INT64_C(2586676988373), INT64_C(-20421133808), INT64_C(171966779), INT64_C(-1507696), INT64_C(13120)},
    {INT64_C(2697663385880076776), INT64_C(69304755717939179), INT64_C(-360962269364257), INT64_C(2506682426068), INT64_C(-19583456168), INT64_C(163194857), INT64_C(-1415900), INT64_C(12197)},
    {INT64_C(2766609666589412753), INT64_C(68590273700228466), INT64_C(-353558111856839), INT64_C(2429952658743), INT64_C(-18788293490), INT64_C(154954435), INT64_C(-1330559), INT64_C(11348)},
    {INT64_C(2834848793495784858), INT64_C(67890372948185318), INT64_C(-346379453817264), INT64_C(2356322815022), INT64_C(-18033082525), INT64_C(147208319), INT64_C(-1251159), INT64_C(10566)},
    {INT64_C(2902395125425853134), INT64_C(67204611605274356), INT64_C(-339417230329661), INT64_C(2285637914621), INT64_C(-17315438523), INT64_C(139922258), INT64_C(-1177232), INT64_C(9845)},
    {INT64_C(2969262588262028797), INT64_C(66532565489221612), INT64_C(-332662827446101), INT64_C(2217752182921), INT64_C(-16633141165), INT64_C(133064688), INT64_C(-1108351), INT64_C(9179)},
    {INT64_C(3035464692174811580), INT64_C(65873827217051101), INT64_C(-326108055529950), INT64_C(2152528419291), INT64_C(-15984121734), INT64_C(126606497), INT64_C(-1044126), INT64_C(8564)},
    {INT64_C(3101014548006201223), INT64_C(65228005381589816), INT64_C(-319745124419552), INT64_C(2089837414462), INT64_C(-15366451400), INT64_C(120520811), INT64_C(-984202), INT64_C(7996)},
    {INT64_C(3165924882853879154), INT64_C(64594723775943313), INT64_C(-313566620271564), INT64_C(2029557412719), INT64_C(-14778330512), INT64_C(114782801), INT64_C(-928251), INT64_C(7471)},
    {INT64_C(3230208054902495130), INT64_C(63973620662713089), INT64_C(-307565483955347), INT64_C(1971573615060), INT64_C(-14218078803), INT64_C(109369514), INT64_C(-875977), INT64_C(6985)},
    {INT64_C(3293876067545289651), INT64_C(63364348084972964), INT64_C(-301734990880819), INT64_C(1915777719843), INT64_C(-13684126430), INT64_C(104259712), INT64_C(-827104), INT64_C(6534)},
    {INT64_C(3356940582836414350), INT64_C(62766571216246804), INT64_C(-296068732152103), INT64_C(1862067497779), INT64_C(-13175005751), INT64_C(99433728), INT64_C(-781384), INT64_C(6117)},
    {INT64_C(3419412934311659540), INT64_C(62179967746936086), INT64_C(-290560596948296), INT64_C(1810346398401), INT64_C(-12689343793), INT64_C(94873341), INT64_C(-738585), INT64_C(5729)},
    {INT64_C(3481304139212842424), INT64_C(61604227304834826), INT64_C(-285204756040898), INT64_C(1760523185409), INT64_C(-12225855342), INT64_C(90561652), INT64_C(-698496), INT64_C(5370)},
    {INT64_C(3542624910148834945), INT64_C(61039050907542764), INT64_C(-279995646364872), INT64_C(1712511598535), INT64_C(-11783336583), INT64_C(86482982), INT64_C(-660922), INT64_C(5036)},
    {INT64_C(3603385666224101885), INT64_C(60484150444746920), INT64_C(-274927956567028), INT64_C(1666230039776), INT64_C(-11360659265), INT64_C(82622770), INT64_C(-625686), INT64_C(4725)},
    {INT64_C(3663596543663664096), INT64_C(59939248188487939), INT64_C(-269996613461655), INT64_C(1621601282029), INT64_C(-10956765329), INT64_C(78967486), INT64_C(-592622), INT64_C(4436)},
    {INT64_C(3723267405961586381), INT64_C(59404076329662154), INT64_C(-265196769328846), INT64_C(1578552198365), INT64_C(-10570661959), INT64_C(75504549), INT64_C(-561578), INT64_C(4168)},
    {INT64_C(3782407853578403233), INT64_C(58878376539134170), INT64_C(-260523789996166), INT64_C(1537013510282), INT64_C(-10201417026), INT64_C(72222255), INT64_C(-532415), INT64_C(3917)},
    {INT64_C(3841027233211328250), INT64_C(58361899551948783), INT64_C(-255973243648896), INT64_C(1496919553484), INT64_C(-9848154884), INT64_C(69109703), INT64_C(-505004), INT64_C(3684)},
    {INT64_C(3899134646659635119), INT64_C(57854404773236185), INT64_C(-251540890318416), INT64_C(1458208059800), INT64_C(-9510052496), INT64_C(66156742), INT64_C(-479225), INT64_C(3466)},
    {INT64_C(3956738959306241175), INT64_C(57355659904501390), INT64_C(-247222672002159), INT64_C(1420819954019), INT64_C(-9186335846), INT64_C(63353905), INT64_C(-454969), INT64_C(3263)},
    {INT64_C(4013848808235260778), INT64_C(56865440589078301), INT64_C(-243014703372128), INT64_C(1384699164498), INT64_C(-8876276636), INT64_C(60692364), INT64_C(-432132), INT64_C(3074)},
    {INT64_C(4070472610004118119), INT64_C(56383530075611536), INT64_C(-238913263032251), INT64_C(1349792446496), INT64_C(-8579189223), INT64_C(58163877), INT64_C(-410622), INT64_C(2897)},
    {INT64_C(4126618568087710828), INT64_C(55909718898505557), INT64_C(-234914785287837), INT64_C(1316049217286), INT64_C(-8294427788), INT64_C(55760749), INT64_C(-390351), INT64_C(2731)},
    {INT64_C(4182294680011091174), INT64_C(55443804574351344), INT64_C(-231015852393129), INT64_C(1283421402172), INT64_C(-8021383715), INT64_C(53475788), INT64_C(-371238), INT64_C(2577)},
    {INT64_C(4237508744186174972), INT64_C(54985591313406291), INT64_C(-227213187245479), INT64_C(1251863290597), INT64_C(-7759483161), INT64_C(51302271), INT64_C(-353207), INT64_C(2432)},
    {INT64_C(4292268366467094717), INT64_C(54534889745263617), INT64_C(-223503646496981), INT64_C(1221331401612), INT64_C(-7508184804), INT64_C(49233908), INT64_C(-336190), INT64_C(2296)},
    {INT64_C(4346580966437978175), INT64_C(54091516657903750), INT64_C(-219884214056518), INT64_C(1191784358020), INT64_C(-7266977753), INT64_C(47264811), INT64_C(-320122), INT64_C(2169)},
    {INT64_C(4400453783446152532), INT64_C(53655294749372268), INT64_C(-216351994957145), INT64_C(1163182768577), INT64_C(-7035379611), INT64_C(45389466), INT64_C(-304943), INT64_C(2050)},
    {INT64_C(4453893882393043195), INT64_C(53226052391377290), INT64_C(-212904209565508), INT64_C(1135489117674), INT64_C(-6812934671), INT64_C(43602707), INT64_C(-290597), INT64_C(1938)},
    {INT64_C(4506908159294352029), INT64_C(52803623404144137), INT64_C(-209538188111682), INT64_C(1108667661958), INT64_C(-6599212241), INT64_C(41899690), INT64_C(-277032), INT64_C(1834)},
    {INT64_C(4559503346620458693), INT64_C(52387846841906781), INT64_C(-206251365519317), INT64_C(1082684333427), INT64_C(-6393805088), INT64_C(40275872), INT64_C(-264200), INT64_C(1735)},
};
AL_INLINE int64_t log2_frac_g6(uint32_t sel, uint64_t x_q64){
    int64_t c[8]; for (int k=0;k<=7;k++) c[k]=0;
    for (uint32_t j=0;j<64;j++){ uint64_t m=al_eqmask(j,sel);
        for (int k=0;k<=7;k++) c[k]|=(int64_t)(m&(uint64_t)kLogPoly_g6[j][k]); }
    __int128 acc=c[7]; for (int k=6;k>=0;k--) acc=(__int128)c[k]+(__int128)al_mulhi(acc,x_q64);
    return (int64_t)acc;
}
AL_INLINE void log2_frac_g6_x2(const uint32_t sel[2], const uint64_t xx[2], int64_t out[2]){
    uint64_t M[2][64];
    for(uint32_t j=0;j<64;j++){ M[0][j]=al_eqmask(j,sel[0]); M[1][j]=al_eqmask(j,sel[1]); }
    int64_t h0=0; int64_t h1=0;
    for(uint32_t j=0;j<64;j++){ int64_t v=kLogPoly_g6[j][7];
        h0|=(int64_t)(M[0][j]&(uint64_t)v); h1|=(int64_t)(M[1][j]&(uint64_t)v); }
    __int128 a0=h0; __int128 a1=h1;
    for(int k=6;k>=0;k--){
        int64_t c0=0; int64_t c1=0;
        for(uint32_t j=0;j<64;j++){ int64_t v=kLogPoly_g6[j][k];
            c0|=(int64_t)(M[0][j]&(uint64_t)v); c1|=(int64_t)(M[1][j]&(uint64_t)v); }
        a0=(__int128)c0+(__int128)al_mulhi(a0,xx[0]); a1=(__int128)c1+(__int128)al_mulhi(a1,xx[1]);
    }
    out[0]=(int64_t)a0; out[1]=(int64_t)a1;
}
AL_INLINE void log2_frac_g6_x3(const uint32_t sel[3], const uint64_t xx[3], int64_t out[3]){
    uint64_t M[3][64];
    for(uint32_t j=0;j<64;j++){ M[0][j]=al_eqmask(j,sel[0]); M[1][j]=al_eqmask(j,sel[1]); M[2][j]=al_eqmask(j,sel[2]); }
    int64_t h0=0; int64_t h1=0; int64_t h2=0;
    for(uint32_t j=0;j<64;j++){ int64_t v=kLogPoly_g6[j][7];
        h0|=(int64_t)(M[0][j]&(uint64_t)v); h1|=(int64_t)(M[1][j]&(uint64_t)v); h2|=(int64_t)(M[2][j]&(uint64_t)v); }
    __int128 a0=h0; __int128 a1=h1; __int128 a2=h2;
    for(int k=6;k>=0;k--){
        int64_t c0=0; int64_t c1=0; int64_t c2=0;
        for(uint32_t j=0;j<64;j++){ int64_t v=kLogPoly_g6[j][k];
            c0|=(int64_t)(M[0][j]&(uint64_t)v); c1|=(int64_t)(M[1][j]&(uint64_t)v); c2|=(int64_t)(M[2][j]&(uint64_t)v); }
        a0=(__int128)c0+(__int128)al_mulhi(a0,xx[0]); a1=(__int128)c1+(__int128)al_mulhi(a1,xx[1]); a2=(__int128)c2+(__int128)al_mulhi(a2,xx[2]);
    }
    out[0]=(int64_t)a0; out[1]=(int64_t)a1; out[2]=(int64_t)a2;
}
AL_INLINE void log2_frac_g6_x4(const uint32_t sel[4], const uint64_t xx[4], int64_t out[4]){
    uint64_t M[4][64];
    for(uint32_t j=0;j<64;j++){ M[0][j]=al_eqmask(j,sel[0]); M[1][j]=al_eqmask(j,sel[1]); M[2][j]=al_eqmask(j,sel[2]); M[3][j]=al_eqmask(j,sel[3]); }
    int64_t h0=0; int64_t h1=0; int64_t h2=0; int64_t h3=0;
    for(uint32_t j=0;j<64;j++){ int64_t v=kLogPoly_g6[j][7];
        h0|=(int64_t)(M[0][j]&(uint64_t)v); h1|=(int64_t)(M[1][j]&(uint64_t)v); h2|=(int64_t)(M[2][j]&(uint64_t)v); h3|=(int64_t)(M[3][j]&(uint64_t)v); }
    __int128 a0=h0; __int128 a1=h1; __int128 a2=h2; __int128 a3=h3;
    for(int k=6;k>=0;k--){
        int64_t c0=0; int64_t c1=0; int64_t c2=0; int64_t c3=0;
        for(uint32_t j=0;j<64;j++){ int64_t v=kLogPoly_g6[j][k];
            c0|=(int64_t)(M[0][j]&(uint64_t)v); c1|=(int64_t)(M[1][j]&(uint64_t)v); c2|=(int64_t)(M[2][j]&(uint64_t)v); c3|=(int64_t)(M[3][j]&(uint64_t)v); }
        a0=(__int128)c0+(__int128)al_mulhi(a0,xx[0]); a1=(__int128)c1+(__int128)al_mulhi(a1,xx[1]); a2=(__int128)c2+(__int128)al_mulhi(a2,xx[2]); a3=(__int128)c3+(__int128)al_mulhi(a3,xx[3]);
    }
    out[0]=(int64_t)a0; out[1]=(int64_t)a1; out[2]=(int64_t)a2; out[3]=(int64_t)a3;
}
AL_INLINE void log2_frac_g6_x8(const uint32_t sel[8], const uint64_t xx[8], int64_t out[8]){
    uint64_t M[8][64];
    for(uint32_t j=0;j<64;j++){ M[0][j]=al_eqmask(j,sel[0]); M[1][j]=al_eqmask(j,sel[1]); M[2][j]=al_eqmask(j,sel[2]); M[3][j]=al_eqmask(j,sel[3]); M[4][j]=al_eqmask(j,sel[4]); M[5][j]=al_eqmask(j,sel[5]); M[6][j]=al_eqmask(j,sel[6]); M[7][j]=al_eqmask(j,sel[7]); }
    int64_t h0=0; int64_t h1=0; int64_t h2=0; int64_t h3=0; int64_t h4=0; int64_t h5=0; int64_t h6=0; int64_t h7=0;
    for(uint32_t j=0;j<64;j++){ int64_t v=kLogPoly_g6[j][7];
        h0|=(int64_t)(M[0][j]&(uint64_t)v); h1|=(int64_t)(M[1][j]&(uint64_t)v); h2|=(int64_t)(M[2][j]&(uint64_t)v); h3|=(int64_t)(M[3][j]&(uint64_t)v); h4|=(int64_t)(M[4][j]&(uint64_t)v); h5|=(int64_t)(M[5][j]&(uint64_t)v); h6|=(int64_t)(M[6][j]&(uint64_t)v); h7|=(int64_t)(M[7][j]&(uint64_t)v); }
    __int128 a0=h0; __int128 a1=h1; __int128 a2=h2; __int128 a3=h3; __int128 a4=h4; __int128 a5=h5; __int128 a6=h6; __int128 a7=h7;
    for(int k=6;k>=0;k--){
        int64_t c0=0; int64_t c1=0; int64_t c2=0; int64_t c3=0; int64_t c4=0; int64_t c5=0; int64_t c6=0; int64_t c7=0;
        for(uint32_t j=0;j<64;j++){ int64_t v=kLogPoly_g6[j][k];
            c0|=(int64_t)(M[0][j]&(uint64_t)v); c1|=(int64_t)(M[1][j]&(uint64_t)v); c2|=(int64_t)(M[2][j]&(uint64_t)v); c3|=(int64_t)(M[3][j]&(uint64_t)v); c4|=(int64_t)(M[4][j]&(uint64_t)v); c5|=(int64_t)(M[5][j]&(uint64_t)v); c6|=(int64_t)(M[6][j]&(uint64_t)v); c7|=(int64_t)(M[7][j]&(uint64_t)v); }
        a0=(__int128)c0+(__int128)al_mulhi(a0,xx[0]); a1=(__int128)c1+(__int128)al_mulhi(a1,xx[1]); a2=(__int128)c2+(__int128)al_mulhi(a2,xx[2]); a3=(__int128)c3+(__int128)al_mulhi(a3,xx[3]); a4=(__int128)c4+(__int128)al_mulhi(a4,xx[4]); a5=(__int128)c5+(__int128)al_mulhi(a5,xx[5]); a6=(__int128)c6+(__int128)al_mulhi(a6,xx[6]); a7=(__int128)c7+(__int128)al_mulhi(a7,xx[7]);
    }
    out[0]=(int64_t)a0; out[1]=(int64_t)a1; out[2]=(int64_t)a2; out[3]=(int64_t)a3; out[4]=(int64_t)a4; out[5]=(int64_t)a5; out[6]=(int64_t)a6; out[7]=(int64_t)a7;
}

/* g=7: 128 segments, degree 6, Q62, err 2^-59.973 */
static const int64_t kLogPoly_g7[128][7] = {
    {INT64_C(0), INT64_C(51978566788454371), INT64_C(-203041276517135), INT64_C(1057506646809), INT64_C(-6196322894), INT64_C(38719320), INT64_C(-246379)},
    {INT64_C(51776576860734092), INT64_C(51575632162187278), INT64_C(-199905551015931), INT64_C(1033103621030), INT64_C(-6006411281), INT64_C(37241549), INT64_C(-235112)},
    {INT64_C(103153330606121625), INT64_C(51178896530170453), INT64_C(-196841909731142), INT64_C(1009445689231), INT64_C(-5823720278), INT64_C(35831159), INT64_C(-224507)},
    {INT64_C(154136388884136542), INT64_C(50788217930703504), INT64_C(-193848160040585), INT64_C(986504629641), INT64_C(-5647922669), INT64_C(34484382), INT64_C(-214456)},
    {INT64_C(204731739545776358), INT64_C(50403458703955751), INT64_C(-190922192060184), INT64_C(964253493727), INT64_C(-5478708671), INT64_C(33197895), INT64_C(-204927)},
    {INT64_C(254945234865449951), INT64_C(50024485330241799), INT64_C(-188061974925480), INT64_C(942666539532), INT64_C(-5315784618), INT64_C(31968573), INT64_C(-195887)},
    {INT64_C(304782595603293869), INT64_C(49651168275538502), INT64_C(-185265553266706), INT64_C(921719169107), INT64_C(-5158872019), INT64_C(30793473), INT64_C(-187309)},
    {INT64_C(354249414916468918), INT64_C(49283381843867848), INT64_C(-182531043865960), INT64_C(901387869638), INT64_C(-5007706658), INT64_C(29669827), INT64_C(-179167)},
    {INT64_C(403351162126124447), INT64_C(48921004036192349), INT64_C(-179856632485795), INT64_C(881650158005), INT64_C(-4862037771), INT64_C(28595025), INT64_C(-171434)},
    {INT64_C(452093186346374828), INT64_C(48563916415490216), INT64_C(-177240570859257), INT64_C(862484528551), INT64_C(-4721627265), INT64_C(27566608), INT64_C(-164088)},
    {INT64_C(500480719981309593), INT64_C(48212003977696809), INT64_C(-174681173832048), INT64_C(843870403867), INT64_C(-4586248996), INT64_C(26582257), INT64_C(-157106)},
    {INT64_C(548518882095754375), INT64_C(47865155028216976), INT64_C(-172176816648085), INT64_C(825788088375), INT64_C(-4455688083), INT64_C(25639785), INT64_C(-150469)},
    {INT64_C(596212681665212875), INT64_C(47523261063729713), INT64_C(-169725932370295), INT64_C(808218724561), INT64_C(-4329740274), INT64_C(24737128), INT64_C(-144157)},
    {INT64_C(643567020710149551), INT64_C(47186216659022410), INT64_C(-167327009428997), INT64_C(791144251656), INT64_C(-4208211351), INT64_C(23872337), INT64_C(-138151)},
    {INT64_C(690586697319517456), INT64_C(46853919358606760), INT64_C(-164978589290716), INT64_C(774547366645), INT64_C(-4090916569), INT64_C(23043572), INT64_C(-132435)},
    {INT64_C(737276408568194713), INT64_C(46526269572882237), INT64_C(-162679264240702), INT64_C(758411487429), INT64_C(-3977680131), INT64_C(22249095), INT64_C(-126993)},
    {INT64_C(783640753332765648), INT64_C(46203170478626111), INT64_C(-160427675272869), INT64_C(742720718024), INT64_C(-3868334696), INT64_C(21487261), INT64_C(-121810)},
    {INT64_C(829684235009867669), INT64_C(45884527923601104), INT64_C(-158222510081251), INT64_C(727459815673), INT64_C(-3762720920), INT64_C(20756517), INT64_C(-116873)},
    {INT64_C(875411264141121919), INT64_C(45570250335083288), INT64_C(-156062501147420), INT64_C(712614159735), INT64_C(-3660687018), INT64_C(20055392), INT64_C(-112167)},
    {INT64_C(920826160948473730), INT64_C(45260248632123538), INT64_C(-153946423918668), INT64_C(698169722267), INT64_C(-3562088361), INT64_C(19382495), INT64_C(-107681)},
    {INT64_C(965933157783587320), INT64_C(44954436141365947), INT64_C(-151873095072068), INT64_C(684113040178), INT64_C(-3466787092), INT64_C(18736509), INT64_C(-103403)},
    {INT64_C(1010736401494767393), INT64_C(44652728516256109), INT64_C(-149841370859811), INT64_C(670431188874), INT64_C(-3374651762), INT64_C(18116187), INT64_C(-99322)},
    {INT64_C(1055239955714717669), INT64_C(44355043659481068), INT64_C(-147850145531499), INT64_C(657111757292), INT64_C(-3285556999), INT64_C(17520348), INT64_C(-95427)},
    {INT64_C(1099447803072292452), INT64_C(44061301648491128), INT64_C(-145898349829341), INT64_C(644142824257), INT64_C(-3199383182), INT64_C(16947870), INT64_C(-91710)},
    {INT64_C(1143363847331251474), INT64_C(43771424663961581), INT64_C(-143984949552410), INT64_C(631512936063), INT64_C(-3116016147), INT64_C(16397694), INT64_C(-88160)},
    {INT64_C(1186991915458890095), INT64_C(43485336921059872), INT64_C(-142108944186379), INT64_C(619211085235), INT64_C(-3035346900), INT64_C(15868811), INT64_C(-84770)},
    {INT64_C(1230335759627285963), INT64_C(43202964603390652), INT64_C(-140269365595337), INT64_C(607226690368), INT64_C(-2957271355), INT64_C(15360266), INT64_C(-81530)},
    {INT64_C(1273399059149779027), INT64_C(42924235799497809), INT64_C(-138465276772491), INT64_C(595549577019), INT64_C(-2881690080), INT64_C(14871153), INT64_C(-78435)},
    {INT64_C(1316185422355184002), INT64_C(42649080441808721), INT64_C(-136695770646744), INT64_C(584169959553), INT64_C(-2808508061), INT64_C(14400611), INT64_C(-75475)},
    {INT64_C(1358698388402122607), INT64_C(42377430247911850), INT64_C(-134959968942319), INT64_C(573078423929), INT64_C(-2737634483), INT64_C(13947822), INT64_C(-72645)},
    {INT64_C(1400941429035756762), INT64_C(42109218664064307), INT64_C(-133257021088739), INT64_C(562265911332), INT64_C(-2668982512), INT64_C(13512011), INT64_C(-69938)},
    {INT64_C(1442917950289103222), INT64_C(41844380810831198), INT64_C(-131586103178645), INT64_C(551723702639), INT64_C(-2602469105), INT64_C(13092438), INT64_C(-67348)},
    {INT64_C(1484631294131014398), INT64_C(41582853430763504), INT64_C(-129946416971070), INT64_C(541443403648), INT64_C(-2538014815), INT64_C(12688402), INT64_C(-64869)},
    {INT64_C(1526084740062819198), INT64_C(41324574838025842), INT64_C(-128337188937905), INT64_C(531416931038), INT64_C(-2475543619), INT64_C(12299237), INT64_C(-62496)},
    {INT64_C(1567281506665531296), INT64_C(41069484869889880), INT64_C(-126757669351451), INT64_C(521636499023), INT64_C(-2414982747), INT64_C(11924308), INT64_C(-60224)},
    {INT64_C(1608224753099450085), INT64_C(40817524840013256), INT64_C(-125207131411025), INT64_C(512094606648), INT64_C(-2356262527), INT64_C(11563010), INT64_C(-58047)},
    {INT64_C(1648917580557901400), INT64_C(40568637493427809), INT64_C(-123684870406736), INT64_C(502784025708), INT64_C(-2299316232), INT64_C(11214770), INT64_C(-55962)},
    {INT64_C(1689363033676790756), INT64_C(40322766963164610), INT64_C(-122190202918627), INT64_C(493697789249), INT64_C(-2244079941), INT64_C(10879039), INT64_C(-53964)},
    {INT64_C(1729564101901571123), INT64_C(40079858728446751), INT64_C(-120722466049487), INT64_C(484829180613), INT64_C(-2190492405), INT64_C(10555297), INT64_C(-52048)},
    {INT64_C(1769523720813159845), INT64_C(39839859574384196), INT64_C(-119281016689724), INT64_C(476171723017), INT64_C(-2138494917), INT64_C(10243047), INT64_C(-50211)},
    {INT64_C(1809244773414275253), INT64_C(39602717553108100), INT64_C(-117865230812774), INT64_C(467719169608), INT64_C(-2088031197), INT64_C(9941817), INT64_C(-48450)},
    {INT64_C(1848730091377602357), INT64_C(39368381946284975), INT64_C(-116474502799614), INT64_C(459465494007), INT64_C(-2039047273), INT64_C(9651154), INT64_C(-46760)},
    {INT64_C(1887982456257138846), INT64_C(39136803228953887), INT64_C(-115108244790997), INT64_C(451404881272), INT64_C(-1991491377), INT64_C(9370629), INT64_C(-45138)},
    {INT64_C(1927004600664017122), INT64_C(38907933034632519), INT64_C(-113765886066135), INT64_C(443531719305), INT64_C(-1945313842), INT64_C(9099833), INT64_C(-43582)},
    {INT64_C(1965799209408045220), INT64_C(38681724121640470), INT64_C(-112446872446589), INT64_C(435840590638), INT64_C(-1900467004), INT64_C(8838373), INT64_C(-42088)},
    {INT64_C(2004368920606159019), INT64_C(38458130340590525), INT64_C(-111150665724212), INT64_C(428326264602), INT64_C(-1856905111), INT64_C(8585876), INT64_C(-40653)},
    {INT64_C(2042716326758930047), INT64_C(38237106603000924), INT64_C(-109876743112035), INT64_C(420983689862), INT64_C(-1814584236), INT64_C(8341985), INT64_C(-39275)},
    {INT64_C(2080843975796227273), INT64_C(38018608850983776), INT64_C(-108624596717061), INT64_C(413807987280), INT64_C(-1773462193), INT64_C(8106362), INT64_C(-37951)},
    {INT64_C(2118754372093087486), INT64_C(37802594027966823), INT64_C(-107393733033963), INT64_C(406794443106), INT64_C(-1733498462), INT64_C(7878679), INT64_C(-36679)},
    {INT64_C(2156449977456806991), INT64_C(37589020050407689), INT64_C(-106183672458746), INT64_C(399938502473), INT64_C(-1694654109), INT64_C(7658628), INT64_C(-35457)},
    {INT64_C(2193933212086227468), INT64_C(37377845780461578), INT64_C(-104993948821490), INT64_C(393235763187), INT64_C(-1656891719), INT64_C(7445910), INT64_C(-34281)},
    {INT64_C(2231206455504150653), INT64_C(37169030999565145), INT64_C(-103824108937302), INT64_C(386681969789), INT64_C(-1620175331), INT64_C(7240243), INT64_C(-33151)},
    {INT64_C(2268272047463780046), INT64_C(36962536382900894), INT64_C(-102673712174696), INT64_C(380273007879), INT64_C(-1584470366), INT64_C(7041356), INT64_C(-32065)},
    {INT64_C(2305132288830053050), INT64_C(36758323474708071), INT64_C(-101542330040602), INT64_C(374004898692), INT64_C(-1549743574), INT64_C(6848989), INT64_C(-31019)},
    {INT64_C(2341789442436693607), INT64_C(36556354664407478), INT64_C(-100429545781312), INT64_C(367873793909), INT64_C(-1515962973), INT64_C(6662894), INT64_C(-30013)},
    {INT64_C(2378245733919783589), INT64_C(36356593163509076), INT64_C(-99334953998633), INT64_C(361875970695), INT64_C(-1483097794), INT64_C(6482833), INT64_C(-29045)},
    {INT64_C(2414503352528620721), INT64_C(36159002983272614), INT64_C(-98258160280607), INT64_C(356007826953), INT64_C(-1451118430), INT64_C(6308581), INT64_C(-28113)},
    {INT64_C(2450564451914601718), INT64_C(35963548913092762), INT64_C(-97198780846173), INT64_C(350265876778), INT64_C(-1419996384), INT64_C(6139918), INT64_C(-27216)},
    {INT64_C(2486431150898841404), INT64_C(35770196499581511), INT64_C(-96156442203153), INT64_C(344646746109), INT64_C(-1389704223), INT64_C(5976638), INT64_C(-26352)},
    {INT64_C(2522105534219211933), INT64_C(35578912026321716), INT64_C(-95130780819020), INT64_C(339147168561), INT64_C(-1360215533), INT64_C(5818541), INT64_C(-25520)},
    {INT64_C(2557589653257460679), INT64_C(35389662494266814), INT64_C(-94121442803880), INT64_C(333763981445), INT64_C(-1331504876), INT64_C(5665436), INT64_C(-24718)},
    {INT64_C(2592885526747040901), INT64_C(35202415602762757), INT64_C(-93128083605172), INT64_C(328494121940), INT64_C(-1303547747), INT64_C(5517140), INT64_C(-23946)},
    {INT64_C(2627995141462265872), INT64_C(35017139731169268), INT64_C(-92150367713583), INT64_C(323334623436), INT64_C(-1276320539), INT64_C(5373478), INT64_C(-23202)},
    {INT64_C(2662920452889374731), INT64_C(34833803921058435), INT64_C(-91187968379715), INT64_C(318282612030), INT64_C(-1249800502), INT64_C(5234281), INT64_C(-22484)},
    {INT64_C(2697663385880076776), INT64_C(34652377858969589), INT64_C(-90240567341048), INT64_C(313335303156), INT64_C(-1223965709), INT64_C(5099389), INT64_C(-21792)},
    {INT64_C(2732225835288120360), INT64_C(34472831859700316), INT64_C(-89307854558791), INT64_C(308489998368), INT64_C(-1198795023), INT64_C(4968647), INT64_C(-21125)},
    {INT64_C(2766609666589412753), INT64_C(34295136850114232), INT64_C(-88389527964195), INT64_C(303744082247), INT64_C(-1174268062), INT64_C(4841907), INT64_C(-20482)},
    {INT64_C(2800816716486198401), INT64_C(34119264353446980), INT64_C(-87485293213950), INT64_C(299095019435), INT64_C(-1150365173), INT64_C(4719027), INT64_C(-19861)},
    {INT64_C(2834848793495784858), INT64_C(33945186474092658), INT64_C(-86594863454302), INT64_C(294540351789), INT64_C(-1127067396), INT64_C(4599870), INT64_C(-19262)},
    {INT64_C(2868707678524288214), INT64_C(33772875882853609), INT64_C(-85717959093522), INT64_C(290077695655), INT64_C(-1104356443), INT64_C(4484305), INT64_C(-18685)},
    {INT64_C(2902395125425853134), INT64_C(33602305802637177), INT64_C(-84854307582402), INT64_C(285704739245), INT64_C(-1082214664), INT64_C(4372207), INT64_C(-18127)},
    {INT64_C(2935912861547786570), INT64_C(33433449994583724), INT64_C(-84003643202457), INT64_C(281419240122), INT64_C(-1060625028), INT64_C(4263455), INT64_C(-17589)},
    {INT64_C(2969262588262028797), INT64_C(33266282744610805), INT64_C(-83165706861513), INT64_C(277219022788), INT64_C(-1039571096), INT64_C(4157932), INT64_C(-17069)},
    {INT64_C(3002445981483370645), INT64_C(33100778850359010), INT64_C(-82340245896402), INT64_C(273101976358), INT64_C(-1019036994), INT64_C(4055529), INT64_C(-16567)},
    {INT64_C(3035464692174811580), INT64_C(32936913608525550), INT64_C(-81527013882476), INT64_C(269066052339), INT64_C(-999007396), INT64_C(3956137), INT64_C(-16082)},
    {INT64_C(3068320346840439651), INT64_C(32774662802572222), INT64_C(-80725770449673), INT64_C(265109262485), INT64_C(-979467502), INT64_C(3859653), INT64_C(-15614)},
    {INT64_C(3101014548006201223), INT64_C(32614002690794907), INT64_C(-79936281104877), INT64_C(261229676740), INT64_C(-960403014), INT64_C(3765980), INT64_C(-15161)},
    {INT64_C(3133548874688915798), INT64_C(32454909994742249), INT64_C(-79158317060335), INT64_C(257425421264), INT64_C(-941800120), INT64_C(3675021), INT64_C(-14724)},
    {INT64_C(3165924882853879154), INT64_C(32297361887971656), INT64_C(-78391655067881), INT64_C(253694676527), INT64_C(-923645472), INT64_C(3586686), INT64_C(-14301)},
    {INT64_C(3198144105861386369), INT64_C(32141335985131213), INT64_C(-77636077258760), INT64_C(250035675486), INT64_C(-905926172), INT64_C(3500887), INT64_C(-13892)},
    {INT64_C(3230208054902495130), INT64_C(31986810331356544), INT64_C(-76891370988827), INT64_C(246446701823), INT64_C(-888629752), INT64_C(3417539), INT64_C(-13497)},
    {INT64_C(3262118219424338960), INT64_C(31833763391972063), INT64_C(-76157328688918), INT64_C(242926088260), INT64_C(-871744159), INT64_C(3336561), INT64_C(-13115)},
    {INT64_C(3293876067545289651), INT64_C(31682174042486482), INT64_C(-75433747720196), INT64_C(239472214925), INT64_C(-855257740), INT64_C(3257874), INT64_C(-12746)},
    {INT64_C(3325483046460258250), INT64_C(31532021558872802), INT64_C(-74720430234286), INT64_C(236083507792), INT64_C(-839159223), INT64_C(3181404), INT64_C(-12389)},
    {INT64_C(3356940582836414350), INT64_C(31383285608123402), INT64_C(-74017183038018), INT64_C(232758437171), INT64_C(-823437708), INT64_C(3107078), INT64_C(-12043)},
    {INT64_C(3388250083199594232), INT64_C(31235946239071179), INT64_C(-73323817462599), INT64_C(229495516261), INT64_C(-808082649), INT64_C(3034826), INT64_C(-11708)},
    {INT64_C(3419412934311659540), INT64_C(31089983873468043), INT64_C(-72640149237066), INT64_C(226293299752), INT64_C(-793083845), INT64_C(2964580), INT64_C(-11385)},
    {INT64_C(3450430503539059618), INT64_C(30945379297312377), INT64_C(-71965998365834), INT64_C(223150382479), INT64_C(-778431422), INT64_C(2896277), INT64_C(-11071)},
    {INT64_C(3481304139212842424), INT64_C(30802113652417413), INT64_C(-71301189010217), INT64_C(220065398131), INT64_C(-764115826), INT64_C(2829853), INT64_C(-10768)},
    {INT64_C(3512035170980351009), INT64_C(30660168428212724), INT64_C(-70645549373754), INT64_C(217037017998), INT64_C(-750127807), INT64_C(2765249), INT64_C(-10474)},
    {INT64_C(3542624910148834945), INT64_C(30519525453771381), INT64_C(-69998911591211), INT64_C(214063949774), INT64_C(-736458411), INT64_C(2702407), INT64_C(-10190)},
    {INT64_C(3573074650021198695), INT64_C(30380166890055530), INT64_C(-69361111621124), INT64_C(211144936397), INT64_C(-723098970), INT64_C(2641271), INT64_C(-9915)},
    {INT64_C(3603385666224101885), INT64_C(30242075222373460), INT64_C(-68731989141751), INT64_C(208278754932), INT64_C(-710041087), INT64_C(2581787), INT64_C(-9648)},
    {INT64_C(3633559217028619578), INT64_C(30105233253041453), INT64_C(-68111387450313), INT64_C(205464215495), INT64_C(-697276630), INT64_C(2523903), INT64_C(-9390)},
    {INT64_C(3663596543663664096), INT64_C(29969624094243969), INT64_C(-67499153365408), INT64_C(202700160216), INT64_C(-684797723), INT64_C(2467570), INT64_C(-9139)},
    {INT64_C(3693498870622363581), INT64_C(29835231161085924), INT64_C(-66895137132473), INT64_C(199985462240), INT64_C(-672596734), INT64_C(2412739), INT64_C(-8897)},
    {INT64_C(3723267405961586381), INT64_C(29702038164831077), INT64_C(-66299192332206), INT64_C(197319024760), INT64_C(-660666269), INT64_C(2359363), INT64_C(-8661)},
    {INT64_C(3752903341594794444), INT64_C(29570029106320716), INT64_C(-65711175791818), INT64_C(194699780087), INT64_C(-648999162), INT64_C(2307398), INT64_C(-8433)},
    {INT64_C(3782407853578403233), INT64_C(29439188269567085), INT64_C(-65130947499036), INT64_C(192126688752), INT64_C(-637588467), INT64_C(2256800), INT64_C(-8213)},
    {INT64_C(3811782102391820155), INT64_C(29309500215516128), INT64_C(-64558370518752), INT64_C(189598738640), INT64_C(-626427452), INT64_C(2207529), INT64_C(-7998)},
    {INT64_C(3841027233211328250), INT64_C(29180949775974391), INT64_C(-63993310912219), INT64_C(187114944154), INT64_C(-615509589), INT64_C(2159542), INT64_C(-7791)},
    {INT64_C(3870144376177976739), INT64_C(29053522047695027), INT64_C(-63435637658717), INT64_C(184674345408), INT64_C(-604828549), INT64_C(2112802), INT64_C(-7589)},
    {INT64_C(3899134646659635119), INT64_C(28927202386618092), INT64_C(-62885222579599), INT64_C(182276007445), INT64_C(-594378195), INT64_C(2067270), INT64_C(-7394)},
    {INT64_C(3927999145507362739), INT64_C(28801976402260438), INT64_C(-62341940264628), INT64_C(179919019492), INT64_C(-584152573), INT64_C(2022911), INT64_C(-7204)},
    {INT64_C(3956738959306241175), INT64_C(28677829952250695), INT64_C(-61805668000535), INT64_C(177602494225), INT64_C(-574145909), INT64_C(1979689), INT64_C(-7020)},
    {INT64_C(3985355160620812318), INT64_C(28554749137004984), INT64_C(-61276285701723), INT64_C(175325567072), INT64_C(-564352601), INT64_C(1937570), INT64_C(-6842)},
    {INT64_C(4013848808235260778), INT64_C(28432720294539150), INT64_C(-60753675843028), INT64_C(173087395536), INT64_C(-554767213), INT64_C(1896523), INT64_C(-6669)},
    {INT64_C(4042220947388475078), INT64_C(28311729995413452), INT64_C(-60237723394492), INT64_C(170887158539), INT64_C(-545384471), INT64_C(1856514), INT64_C(-6500)},
    {INT64_C(4070472610004118119), INT64_C(28191765037805768), INT64_C(-59728315758059), INT64_C(168724055787), INT64_C(-536199254), INT64_C(1817514), INT64_C(-6337)},
    {INT64_C(4098604814915833537), INT64_C(28072812442709541), INT64_C(-59225342706134), INT64_C(166597307165), INT64_C(-527206595), INT64_C(1779493), INT64_C(-6179)},
    {INT64_C(4126618568087710828), INT64_C(27954859449252778), INT64_C(-58728696321955), INT64_C(164506152137), INT64_C(-518401669), INT64_C(1742422), INT64_C(-6025)},
    {INT64_C(4154514862830128517), INT64_C(27837893510134566), INT64_C(-58238270941700), INT64_C(162449849186), INT64_C(-509779792), INT64_C(1706274), INT64_C(-5876)},
    {INT64_C(4182294680011091174), INT64_C(27721902287175672), INT64_C(-57753963098279), INT64_C(160427675250), INT64_C(-501336418), INT64_C(1671023), INT64_C(-5731)},
    {INT64_C(4209958988263172691), INT64_C(27606873646979922), INT64_C(-57275671466760), INT64_C(158438925196), INT64_C(-493067129), INT64_C(1636642), INT64_C(-5590)},
    {INT64_C(4237508744186174972), INT64_C(27492795656703145), INT64_C(-56803296811366), INT64_C(156482911304), INT64_C(-484967637), INT64_C(1603106), INT64_C(-5453)},
    {INT64_C(4264944892545608072), INT64_C(27379656579926589), INT64_C(-56336741934002), INT64_C(154558962760), INT64_C(-477033774), INT64_C(1570391), INT64_C(-5320)},
    {INT64_C(4292268366467094717), INT64_C(27267444872631808), INT64_C(-55875911624242), INT64_C(152666425182), INT64_C(-469261493), INT64_C(1538475), INT64_C(-5191)},
    {INT64_C(4319480087626799256), INT64_C(27156149179274127), INT64_C(-55420712610760), INT64_C(150804660145), INT64_C(-461646861), INT64_C(1507333), INT64_C(-5065)},
    {INT64_C(4346580966437978175), INT64_C(27045758328951875), INT64_C(-54971053514127), INT64_C(148973044734), INT64_C(-454186056), INT64_C(1476945), INT64_C(-4943)},
    {INT64_C(4373571902233746604), INT64_C(26936261331668669), INT64_C(-54526844800946), INT64_C(147170971103), INT64_C(-446875363), INT64_C(1447289), INT64_C(-4824)},
    {INT64_C(4400453783446152532), INT64_C(26827647374686134), INT64_C(-54087998739283), INT64_C(145397846055), INT64_C(-439711175), INT64_C(1418345), INT64_C(-4709)},
    {INT64_C(4427227487781647899), INT64_C(26719905818964503), INT64_C(-53654429355347), INT64_C(143653090625), INT64_C(-432689980), INT64_C(1390092), INT64_C(-4597)},
    {INT64_C(4453893882393043195), INT64_C(26613026195688645), INT64_C(-53226052391374), INT64_C(141936139693), INT64_C(-425808369), INT64_C(1362513), INT64_C(-4488)},
    {INT64_C(4480453824048029814), INT64_C(26506998202877136), INT64_C(-52802785264693), INT64_C(140246441589), INT64_C(-419063023), INT64_C(1335587), INT64_C(-4382)},
    {INT64_C(4506908159294352029), INT64_C(26401811702072068), INT64_C(-52384547027918), INT64_C(138583457729), INT64_C(-412450719), INT64_C(1309297), INT64_C(-4279)},
    {INT64_C(4533257724621708207), INT64_C(26297456715107357), INT64_C(-51971258330249), INT64_C(136946662251), INT64_C(-405968320), INT64_C(1283626), INT64_C(-4178)},
    {INT64_C(4559503346620458693), INT64_C(26193923420953391), INT64_C(-51562841379827), INT64_C(135335541664), INT64_C(-399612775), INT64_C(1258557), INT64_C(-4081)},
    {INT64_C(4585645842137215621), INT64_C(26091202152635926), INT64_C(-51159219907127), INT64_C(133749594513), INT64_C(-393381116), INT64_C(1234072), INT64_C(-3986)},
};
AL_INLINE int64_t log2_frac_g7(uint32_t sel, uint64_t x_q64){
    int64_t c[7]; for (int k=0;k<=6;k++) c[k]=0;
    for (uint32_t j=0;j<128;j++){ uint64_t m=al_eqmask(j,sel);
        for (int k=0;k<=6;k++) c[k]|=(int64_t)(m&(uint64_t)kLogPoly_g7[j][k]); }
    __int128 acc=c[6]; for (int k=5;k>=0;k--) acc=(__int128)c[k]+(__int128)al_mulhi(acc,x_q64);
    return (int64_t)acc;
}
AL_INLINE void log2_frac_g7_x2(const uint32_t sel[2], const uint64_t xx[2], int64_t out[2]){
    uint64_t M[2][128];
    for(uint32_t j=0;j<128;j++){ M[0][j]=al_eqmask(j,sel[0]); M[1][j]=al_eqmask(j,sel[1]); }
    int64_t h0=0; int64_t h1=0;
    for(uint32_t j=0;j<128;j++){ int64_t v=kLogPoly_g7[j][6];
        h0|=(int64_t)(M[0][j]&(uint64_t)v); h1|=(int64_t)(M[1][j]&(uint64_t)v); }
    __int128 a0=h0; __int128 a1=h1;
    for(int k=5;k>=0;k--){
        int64_t c0=0; int64_t c1=0;
        for(uint32_t j=0;j<128;j++){ int64_t v=kLogPoly_g7[j][k];
            c0|=(int64_t)(M[0][j]&(uint64_t)v); c1|=(int64_t)(M[1][j]&(uint64_t)v); }
        a0=(__int128)c0+(__int128)al_mulhi(a0,xx[0]); a1=(__int128)c1+(__int128)al_mulhi(a1,xx[1]);
    }
    out[0]=(int64_t)a0; out[1]=(int64_t)a1;
}
AL_INLINE void log2_frac_g7_x3(const uint32_t sel[3], const uint64_t xx[3], int64_t out[3]){
    uint64_t M[3][128];
    for(uint32_t j=0;j<128;j++){ M[0][j]=al_eqmask(j,sel[0]); M[1][j]=al_eqmask(j,sel[1]); M[2][j]=al_eqmask(j,sel[2]); }
    int64_t h0=0; int64_t h1=0; int64_t h2=0;
    for(uint32_t j=0;j<128;j++){ int64_t v=kLogPoly_g7[j][6];
        h0|=(int64_t)(M[0][j]&(uint64_t)v); h1|=(int64_t)(M[1][j]&(uint64_t)v); h2|=(int64_t)(M[2][j]&(uint64_t)v); }
    __int128 a0=h0; __int128 a1=h1; __int128 a2=h2;
    for(int k=5;k>=0;k--){
        int64_t c0=0; int64_t c1=0; int64_t c2=0;
        for(uint32_t j=0;j<128;j++){ int64_t v=kLogPoly_g7[j][k];
            c0|=(int64_t)(M[0][j]&(uint64_t)v); c1|=(int64_t)(M[1][j]&(uint64_t)v); c2|=(int64_t)(M[2][j]&(uint64_t)v); }
        a0=(__int128)c0+(__int128)al_mulhi(a0,xx[0]); a1=(__int128)c1+(__int128)al_mulhi(a1,xx[1]); a2=(__int128)c2+(__int128)al_mulhi(a2,xx[2]);
    }
    out[0]=(int64_t)a0; out[1]=(int64_t)a1; out[2]=(int64_t)a2;
}
AL_INLINE void log2_frac_g7_x4(const uint32_t sel[4], const uint64_t xx[4], int64_t out[4]){
    uint64_t M[4][128];
    for(uint32_t j=0;j<128;j++){ M[0][j]=al_eqmask(j,sel[0]); M[1][j]=al_eqmask(j,sel[1]); M[2][j]=al_eqmask(j,sel[2]); M[3][j]=al_eqmask(j,sel[3]); }
    int64_t h0=0; int64_t h1=0; int64_t h2=0; int64_t h3=0;
    for(uint32_t j=0;j<128;j++){ int64_t v=kLogPoly_g7[j][6];
        h0|=(int64_t)(M[0][j]&(uint64_t)v); h1|=(int64_t)(M[1][j]&(uint64_t)v); h2|=(int64_t)(M[2][j]&(uint64_t)v); h3|=(int64_t)(M[3][j]&(uint64_t)v); }
    __int128 a0=h0; __int128 a1=h1; __int128 a2=h2; __int128 a3=h3;
    for(int k=5;k>=0;k--){
        int64_t c0=0; int64_t c1=0; int64_t c2=0; int64_t c3=0;
        for(uint32_t j=0;j<128;j++){ int64_t v=kLogPoly_g7[j][k];
            c0|=(int64_t)(M[0][j]&(uint64_t)v); c1|=(int64_t)(M[1][j]&(uint64_t)v); c2|=(int64_t)(M[2][j]&(uint64_t)v); c3|=(int64_t)(M[3][j]&(uint64_t)v); }
        a0=(__int128)c0+(__int128)al_mulhi(a0,xx[0]); a1=(__int128)c1+(__int128)al_mulhi(a1,xx[1]); a2=(__int128)c2+(__int128)al_mulhi(a2,xx[2]); a3=(__int128)c3+(__int128)al_mulhi(a3,xx[3]);
    }
    out[0]=(int64_t)a0; out[1]=(int64_t)a1; out[2]=(int64_t)a2; out[3]=(int64_t)a3;
}
AL_INLINE void log2_frac_g7_x8(const uint32_t sel[8], const uint64_t xx[8], int64_t out[8]){
    uint64_t M[8][128];
    for(uint32_t j=0;j<128;j++){ M[0][j]=al_eqmask(j,sel[0]); M[1][j]=al_eqmask(j,sel[1]); M[2][j]=al_eqmask(j,sel[2]); M[3][j]=al_eqmask(j,sel[3]); M[4][j]=al_eqmask(j,sel[4]); M[5][j]=al_eqmask(j,sel[5]); M[6][j]=al_eqmask(j,sel[6]); M[7][j]=al_eqmask(j,sel[7]); }
    int64_t h0=0; int64_t h1=0; int64_t h2=0; int64_t h3=0; int64_t h4=0; int64_t h5=0; int64_t h6=0; int64_t h7=0;
    for(uint32_t j=0;j<128;j++){ int64_t v=kLogPoly_g7[j][6];
        h0|=(int64_t)(M[0][j]&(uint64_t)v); h1|=(int64_t)(M[1][j]&(uint64_t)v); h2|=(int64_t)(M[2][j]&(uint64_t)v); h3|=(int64_t)(M[3][j]&(uint64_t)v); h4|=(int64_t)(M[4][j]&(uint64_t)v); h5|=(int64_t)(M[5][j]&(uint64_t)v); h6|=(int64_t)(M[6][j]&(uint64_t)v); h7|=(int64_t)(M[7][j]&(uint64_t)v); }
    __int128 a0=h0; __int128 a1=h1; __int128 a2=h2; __int128 a3=h3; __int128 a4=h4; __int128 a5=h5; __int128 a6=h6; __int128 a7=h7;
    for(int k=5;k>=0;k--){
        int64_t c0=0; int64_t c1=0; int64_t c2=0; int64_t c3=0; int64_t c4=0; int64_t c5=0; int64_t c6=0; int64_t c7=0;
        for(uint32_t j=0;j<128;j++){ int64_t v=kLogPoly_g7[j][k];
            c0|=(int64_t)(M[0][j]&(uint64_t)v); c1|=(int64_t)(M[1][j]&(uint64_t)v); c2|=(int64_t)(M[2][j]&(uint64_t)v); c3|=(int64_t)(M[3][j]&(uint64_t)v); c4|=(int64_t)(M[4][j]&(uint64_t)v); c5|=(int64_t)(M[5][j]&(uint64_t)v); c6|=(int64_t)(M[6][j]&(uint64_t)v); c7|=(int64_t)(M[7][j]&(uint64_t)v); }
        a0=(__int128)c0+(__int128)al_mulhi(a0,xx[0]); a1=(__int128)c1+(__int128)al_mulhi(a1,xx[1]); a2=(__int128)c2+(__int128)al_mulhi(a2,xx[2]); a3=(__int128)c3+(__int128)al_mulhi(a3,xx[3]); a4=(__int128)c4+(__int128)al_mulhi(a4,xx[4]); a5=(__int128)c5+(__int128)al_mulhi(a5,xx[5]); a6=(__int128)c6+(__int128)al_mulhi(a6,xx[6]); a7=(__int128)c7+(__int128)al_mulhi(a7,xx[7]);
    }
    out[0]=(int64_t)a0; out[1]=(int64_t)a1; out[2]=(int64_t)a2; out[3]=(int64_t)a3; out[4]=(int64_t)a4; out[5]=(int64_t)a5; out[6]=(int64_t)a6; out[7]=(int64_t)a7;
}

/* scheme descriptors: {name, g, degree, segments, scan_ops} */
typedef struct { const char *name; int g; int degree; int segments; int scan_ops; } al_scheme_t;
static const al_scheme_t AL_SCHEMES[] = {
    {"baseline", 0, 21, 1, 0},
    {"g1", 1, 16, 2, 34},
    {"g2", 2, 13, 4, 56},
    {"g3", 3, 11, 8, 96},
    {"g4", 4, 9, 16, 160},
    {"g5", 5, 8, 32, 288},
    {"g6", 6, 7, 64, 512},
    {"g7", 7, 6, 128, 896},
};
#define AL_NUM_SCHEMES 8
#undef AL_INLINE
#endif
