#ifndef SHUTTLE_APPROX_EXP_EXPLORE_H
#define SHUTTLE_APPROX_EXP_EXPLORE_H
#include <stdint.h>
#if defined(__GNUC__) || defined(__clang__)
#define AL_INLINE static inline __attribute__((always_inline))
#else
#define AL_INLINE static inline
#endif

AL_INLINE int64_t al_exp_hs(__int128 a, int64_t b){ return (int64_t)((a*(__int128)b)>>64); }
AL_INLINE uint64_t al_exp_hu(uint64_t a, uint64_t b){ return (uint64_t)(((__uint128_t)a*(__uint128_t)b)>>64); }

/* t=4 squarings, degree 12, total mul 16, precision 2^-57.296 */
static const int64_t kExp_t4d12[12] = { INT64_C(-7104798205265038309), INT64_C(1368213201631587293), INT64_C(-175656630072120389), INT64_C(16913620434756998), INT64_C(-1302862549935409), INT64_C(83633327282588), INT64_C(-4601647634195), INT64_C(221541656092), INT64_C(-9480798432), INT64_C(365154736), INT64_C(-12785458), INT64_C(410362) };
AL_INLINE void shuttle_exp_t4d12_x1(const int xx[1], const int yy[1], uint64_t out[1]){
    int64_t s0=(int64_t)(((uint64_t)yy[0]*(uint64_t)(yy[0]+512*xx[0]))<<40);
    __int128 a0=kExp_t4d12[11];
    for(int k=10;k>=0;k--){ int64_t c=kExp_t4d12[k];
        a0=(__int128)c+((__int128)al_exp_hs(a0,s0)*2); }
    a0=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a0,s0)*2);
    uint64_t v0=(uint64_t)a0;
    for(int q=0;q<4;q++){
        v0=al_exp_hu(v0,v0);
    }
    out[0]=v0;
}
AL_INLINE void shuttle_exp_t4d12_x2(const int xx[2], const int yy[2], uint64_t out[2]){
    int64_t s0=(int64_t)(((uint64_t)yy[0]*(uint64_t)(yy[0]+512*xx[0]))<<40); int64_t s1=(int64_t)(((uint64_t)yy[1]*(uint64_t)(yy[1]+512*xx[1]))<<40);
    __int128 a0=kExp_t4d12[11]; __int128 a1=kExp_t4d12[11];
    for(int k=10;k>=0;k--){ int64_t c=kExp_t4d12[k];
        a0=(__int128)c+((__int128)al_exp_hs(a0,s0)*2); a1=(__int128)c+((__int128)al_exp_hs(a1,s1)*2); }
    a0=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a0,s0)*2); a1=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a1,s1)*2);
    uint64_t v0=(uint64_t)a0; uint64_t v1=(uint64_t)a1;
    for(int q=0;q<4;q++){
        v0=al_exp_hu(v0,v0); v1=al_exp_hu(v1,v1);
    }
    out[0]=v0; out[1]=v1;
}
AL_INLINE void shuttle_exp_t4d12_x3(const int xx[3], const int yy[3], uint64_t out[3]){
    int64_t s0=(int64_t)(((uint64_t)yy[0]*(uint64_t)(yy[0]+512*xx[0]))<<40); int64_t s1=(int64_t)(((uint64_t)yy[1]*(uint64_t)(yy[1]+512*xx[1]))<<40); int64_t s2=(int64_t)(((uint64_t)yy[2]*(uint64_t)(yy[2]+512*xx[2]))<<40);
    __int128 a0=kExp_t4d12[11]; __int128 a1=kExp_t4d12[11]; __int128 a2=kExp_t4d12[11];
    for(int k=10;k>=0;k--){ int64_t c=kExp_t4d12[k];
        a0=(__int128)c+((__int128)al_exp_hs(a0,s0)*2); a1=(__int128)c+((__int128)al_exp_hs(a1,s1)*2); a2=(__int128)c+((__int128)al_exp_hs(a2,s2)*2); }
    a0=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a0,s0)*2); a1=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a1,s1)*2); a2=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a2,s2)*2);
    uint64_t v0=(uint64_t)a0; uint64_t v1=(uint64_t)a1; uint64_t v2=(uint64_t)a2;
    for(int q=0;q<4;q++){
        v0=al_exp_hu(v0,v0); v1=al_exp_hu(v1,v1); v2=al_exp_hu(v2,v2);
    }
    out[0]=v0; out[1]=v1; out[2]=v2;
}
AL_INLINE void shuttle_exp_t4d12_x4(const int xx[4], const int yy[4], uint64_t out[4]){
    int64_t s0=(int64_t)(((uint64_t)yy[0]*(uint64_t)(yy[0]+512*xx[0]))<<40); int64_t s1=(int64_t)(((uint64_t)yy[1]*(uint64_t)(yy[1]+512*xx[1]))<<40); int64_t s2=(int64_t)(((uint64_t)yy[2]*(uint64_t)(yy[2]+512*xx[2]))<<40); int64_t s3=(int64_t)(((uint64_t)yy[3]*(uint64_t)(yy[3]+512*xx[3]))<<40);
    __int128 a0=kExp_t4d12[11]; __int128 a1=kExp_t4d12[11]; __int128 a2=kExp_t4d12[11]; __int128 a3=kExp_t4d12[11];
    for(int k=10;k>=0;k--){ int64_t c=kExp_t4d12[k];
        a0=(__int128)c+((__int128)al_exp_hs(a0,s0)*2); a1=(__int128)c+((__int128)al_exp_hs(a1,s1)*2); a2=(__int128)c+((__int128)al_exp_hs(a2,s2)*2); a3=(__int128)c+((__int128)al_exp_hs(a3,s3)*2); }
    a0=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a0,s0)*2); a1=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a1,s1)*2); a2=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a2,s2)*2); a3=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a3,s3)*2);
    uint64_t v0=(uint64_t)a0; uint64_t v1=(uint64_t)a1; uint64_t v2=(uint64_t)a2; uint64_t v3=(uint64_t)a3;
    for(int q=0;q<4;q++){
        v0=al_exp_hu(v0,v0); v1=al_exp_hu(v1,v1); v2=al_exp_hu(v2,v2); v3=al_exp_hu(v3,v3);
    }
    out[0]=v0; out[1]=v1; out[2]=v2; out[3]=v3;
}
AL_INLINE void shuttle_exp_t4d12_x8(const int xx[8], const int yy[8], uint64_t out[8]){
    int64_t s0=(int64_t)(((uint64_t)yy[0]*(uint64_t)(yy[0]+512*xx[0]))<<40); int64_t s1=(int64_t)(((uint64_t)yy[1]*(uint64_t)(yy[1]+512*xx[1]))<<40); int64_t s2=(int64_t)(((uint64_t)yy[2]*(uint64_t)(yy[2]+512*xx[2]))<<40); int64_t s3=(int64_t)(((uint64_t)yy[3]*(uint64_t)(yy[3]+512*xx[3]))<<40); int64_t s4=(int64_t)(((uint64_t)yy[4]*(uint64_t)(yy[4]+512*xx[4]))<<40); int64_t s5=(int64_t)(((uint64_t)yy[5]*(uint64_t)(yy[5]+512*xx[5]))<<40); int64_t s6=(int64_t)(((uint64_t)yy[6]*(uint64_t)(yy[6]+512*xx[6]))<<40); int64_t s7=(int64_t)(((uint64_t)yy[7]*(uint64_t)(yy[7]+512*xx[7]))<<40);
    __int128 a0=kExp_t4d12[11]; __int128 a1=kExp_t4d12[11]; __int128 a2=kExp_t4d12[11]; __int128 a3=kExp_t4d12[11]; __int128 a4=kExp_t4d12[11]; __int128 a5=kExp_t4d12[11]; __int128 a6=kExp_t4d12[11]; __int128 a7=kExp_t4d12[11];
    for(int k=10;k>=0;k--){ int64_t c=kExp_t4d12[k];
        a0=(__int128)c+((__int128)al_exp_hs(a0,s0)*2); a1=(__int128)c+((__int128)al_exp_hs(a1,s1)*2); a2=(__int128)c+((__int128)al_exp_hs(a2,s2)*2); a3=(__int128)c+((__int128)al_exp_hs(a3,s3)*2); a4=(__int128)c+((__int128)al_exp_hs(a4,s4)*2); a5=(__int128)c+((__int128)al_exp_hs(a5,s5)*2); a6=(__int128)c+((__int128)al_exp_hs(a6,s6)*2); a7=(__int128)c+((__int128)al_exp_hs(a7,s7)*2); }
    a0=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a0,s0)*2); a1=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a1,s1)*2); a2=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a2,s2)*2); a3=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a3,s3)*2); a4=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a4,s4)*2); a5=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a5,s5)*2); a6=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a6,s6)*2); a7=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a7,s7)*2);
    uint64_t v0=(uint64_t)a0; uint64_t v1=(uint64_t)a1; uint64_t v2=(uint64_t)a2; uint64_t v3=(uint64_t)a3; uint64_t v4=(uint64_t)a4; uint64_t v5=(uint64_t)a5; uint64_t v6=(uint64_t)a6; uint64_t v7=(uint64_t)a7;
    for(int q=0;q<4;q++){
        v0=al_exp_hu(v0,v0); v1=al_exp_hu(v1,v1); v2=al_exp_hu(v2,v2); v3=al_exp_hu(v3,v3); v4=al_exp_hu(v4,v4); v5=al_exp_hu(v5,v5); v6=al_exp_hu(v6,v6); v7=al_exp_hu(v7,v7);
    }
    out[0]=v0; out[1]=v1; out[2]=v2; out[3]=v3; out[4]=v4; out[5]=v5; out[6]=v6; out[7]=v7;
}

/* t=5 squarings, degree 10, total mul 15, precision 2^-56.036 */
static const int64_t kExp_t5d10[10] = { INT64_C(-3552399102632519154), INT64_C(342053300407896823), INT64_C(-21957078759015049), INT64_C(1057101277172312), INT64_C(-40714454685482), INT64_C(1306770738790), INT64_C(-35950372142), INT64_C(865397094), INT64_C(-18517184), INT64_C(356596) };
AL_INLINE void shuttle_exp_t5d10_x1(const int xx[1], const int yy[1], uint64_t out[1]){
    int64_t s0=(int64_t)(((uint64_t)yy[0]*(uint64_t)(yy[0]+512*xx[0]))<<40);
    __int128 a0=kExp_t5d10[9];
    for(int k=8;k>=0;k--){ int64_t c=kExp_t5d10[k];
        a0=(__int128)c+((__int128)al_exp_hs(a0,s0)*2); }
    a0=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a0,s0)*2);
    uint64_t v0=(uint64_t)a0;
    for(int q=0;q<5;q++){
        v0=al_exp_hu(v0,v0);
    }
    out[0]=v0;
}
AL_INLINE void shuttle_exp_t5d10_x2(const int xx[2], const int yy[2], uint64_t out[2]){
    int64_t s0=(int64_t)(((uint64_t)yy[0]*(uint64_t)(yy[0]+512*xx[0]))<<40); int64_t s1=(int64_t)(((uint64_t)yy[1]*(uint64_t)(yy[1]+512*xx[1]))<<40);
    __int128 a0=kExp_t5d10[9]; __int128 a1=kExp_t5d10[9];
    for(int k=8;k>=0;k--){ int64_t c=kExp_t5d10[k];
        a0=(__int128)c+((__int128)al_exp_hs(a0,s0)*2); a1=(__int128)c+((__int128)al_exp_hs(a1,s1)*2); }
    a0=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a0,s0)*2); a1=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a1,s1)*2);
    uint64_t v0=(uint64_t)a0; uint64_t v1=(uint64_t)a1;
    for(int q=0;q<5;q++){
        v0=al_exp_hu(v0,v0); v1=al_exp_hu(v1,v1);
    }
    out[0]=v0; out[1]=v1;
}
AL_INLINE void shuttle_exp_t5d10_x3(const int xx[3], const int yy[3], uint64_t out[3]){
    int64_t s0=(int64_t)(((uint64_t)yy[0]*(uint64_t)(yy[0]+512*xx[0]))<<40); int64_t s1=(int64_t)(((uint64_t)yy[1]*(uint64_t)(yy[1]+512*xx[1]))<<40); int64_t s2=(int64_t)(((uint64_t)yy[2]*(uint64_t)(yy[2]+512*xx[2]))<<40);
    __int128 a0=kExp_t5d10[9]; __int128 a1=kExp_t5d10[9]; __int128 a2=kExp_t5d10[9];
    for(int k=8;k>=0;k--){ int64_t c=kExp_t5d10[k];
        a0=(__int128)c+((__int128)al_exp_hs(a0,s0)*2); a1=(__int128)c+((__int128)al_exp_hs(a1,s1)*2); a2=(__int128)c+((__int128)al_exp_hs(a2,s2)*2); }
    a0=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a0,s0)*2); a1=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a1,s1)*2); a2=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a2,s2)*2);
    uint64_t v0=(uint64_t)a0; uint64_t v1=(uint64_t)a1; uint64_t v2=(uint64_t)a2;
    for(int q=0;q<5;q++){
        v0=al_exp_hu(v0,v0); v1=al_exp_hu(v1,v1); v2=al_exp_hu(v2,v2);
    }
    out[0]=v0; out[1]=v1; out[2]=v2;
}
AL_INLINE void shuttle_exp_t5d10_x4(const int xx[4], const int yy[4], uint64_t out[4]){
    int64_t s0=(int64_t)(((uint64_t)yy[0]*(uint64_t)(yy[0]+512*xx[0]))<<40); int64_t s1=(int64_t)(((uint64_t)yy[1]*(uint64_t)(yy[1]+512*xx[1]))<<40); int64_t s2=(int64_t)(((uint64_t)yy[2]*(uint64_t)(yy[2]+512*xx[2]))<<40); int64_t s3=(int64_t)(((uint64_t)yy[3]*(uint64_t)(yy[3]+512*xx[3]))<<40);
    __int128 a0=kExp_t5d10[9]; __int128 a1=kExp_t5d10[9]; __int128 a2=kExp_t5d10[9]; __int128 a3=kExp_t5d10[9];
    for(int k=8;k>=0;k--){ int64_t c=kExp_t5d10[k];
        a0=(__int128)c+((__int128)al_exp_hs(a0,s0)*2); a1=(__int128)c+((__int128)al_exp_hs(a1,s1)*2); a2=(__int128)c+((__int128)al_exp_hs(a2,s2)*2); a3=(__int128)c+((__int128)al_exp_hs(a3,s3)*2); }
    a0=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a0,s0)*2); a1=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a1,s1)*2); a2=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a2,s2)*2); a3=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a3,s3)*2);
    uint64_t v0=(uint64_t)a0; uint64_t v1=(uint64_t)a1; uint64_t v2=(uint64_t)a2; uint64_t v3=(uint64_t)a3;
    for(int q=0;q<5;q++){
        v0=al_exp_hu(v0,v0); v1=al_exp_hu(v1,v1); v2=al_exp_hu(v2,v2); v3=al_exp_hu(v3,v3);
    }
    out[0]=v0; out[1]=v1; out[2]=v2; out[3]=v3;
}
AL_INLINE void shuttle_exp_t5d10_x8(const int xx[8], const int yy[8], uint64_t out[8]){
    int64_t s0=(int64_t)(((uint64_t)yy[0]*(uint64_t)(yy[0]+512*xx[0]))<<40); int64_t s1=(int64_t)(((uint64_t)yy[1]*(uint64_t)(yy[1]+512*xx[1]))<<40); int64_t s2=(int64_t)(((uint64_t)yy[2]*(uint64_t)(yy[2]+512*xx[2]))<<40); int64_t s3=(int64_t)(((uint64_t)yy[3]*(uint64_t)(yy[3]+512*xx[3]))<<40); int64_t s4=(int64_t)(((uint64_t)yy[4]*(uint64_t)(yy[4]+512*xx[4]))<<40); int64_t s5=(int64_t)(((uint64_t)yy[5]*(uint64_t)(yy[5]+512*xx[5]))<<40); int64_t s6=(int64_t)(((uint64_t)yy[6]*(uint64_t)(yy[6]+512*xx[6]))<<40); int64_t s7=(int64_t)(((uint64_t)yy[7]*(uint64_t)(yy[7]+512*xx[7]))<<40);
    __int128 a0=kExp_t5d10[9]; __int128 a1=kExp_t5d10[9]; __int128 a2=kExp_t5d10[9]; __int128 a3=kExp_t5d10[9]; __int128 a4=kExp_t5d10[9]; __int128 a5=kExp_t5d10[9]; __int128 a6=kExp_t5d10[9]; __int128 a7=kExp_t5d10[9];
    for(int k=8;k>=0;k--){ int64_t c=kExp_t5d10[k];
        a0=(__int128)c+((__int128)al_exp_hs(a0,s0)*2); a1=(__int128)c+((__int128)al_exp_hs(a1,s1)*2); a2=(__int128)c+((__int128)al_exp_hs(a2,s2)*2); a3=(__int128)c+((__int128)al_exp_hs(a3,s3)*2); a4=(__int128)c+((__int128)al_exp_hs(a4,s4)*2); a5=(__int128)c+((__int128)al_exp_hs(a5,s5)*2); a6=(__int128)c+((__int128)al_exp_hs(a6,s6)*2); a7=(__int128)c+((__int128)al_exp_hs(a7,s7)*2); }
    a0=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a0,s0)*2); a1=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a1,s1)*2); a2=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a2,s2)*2); a3=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a3,s3)*2); a4=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a4,s4)*2); a5=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a5,s5)*2); a6=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a6,s6)*2); a7=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a7,s7)*2);
    uint64_t v0=(uint64_t)a0; uint64_t v1=(uint64_t)a1; uint64_t v2=(uint64_t)a2; uint64_t v3=(uint64_t)a3; uint64_t v4=(uint64_t)a4; uint64_t v5=(uint64_t)a5; uint64_t v6=(uint64_t)a6; uint64_t v7=(uint64_t)a7;
    for(int q=0;q<5;q++){
        v0=al_exp_hu(v0,v0); v1=al_exp_hu(v1,v1); v2=al_exp_hu(v2,v2); v3=al_exp_hu(v3,v3); v4=al_exp_hu(v4,v4); v5=al_exp_hu(v5,v5); v6=al_exp_hu(v6,v6); v7=al_exp_hu(v7,v7);
    }
    out[0]=v0; out[1]=v1; out[2]=v2; out[3]=v3; out[4]=v4; out[5]=v5; out[6]=v6; out[7]=v7;
}

/* t=6 squarings, degree 9, total mul 15, precision 2^-55.251 */
static const int64_t kExp_t6d9[9] = { INT64_C(-1776199551316259577), INT64_C(85513325101974206), INT64_C(-2744634844876881), INT64_C(66068829823270), INT64_C(-1272326708921), INT64_C(20418292794), INT64_C(-280862282), INT64_C(3380457), INT64_C(-36166) };
AL_INLINE void shuttle_exp_t6d9_x1(const int xx[1], const int yy[1], uint64_t out[1]){
    int64_t s0=(int64_t)(((uint64_t)yy[0]*(uint64_t)(yy[0]+512*xx[0]))<<40);
    __int128 a0=kExp_t6d9[8];
    for(int k=7;k>=0;k--){ int64_t c=kExp_t6d9[k];
        a0=(__int128)c+((__int128)al_exp_hs(a0,s0)*2); }
    a0=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a0,s0)*2);
    uint64_t v0=(uint64_t)a0;
    for(int q=0;q<6;q++){
        v0=al_exp_hu(v0,v0);
    }
    out[0]=v0;
}
AL_INLINE void shuttle_exp_t6d9_x2(const int xx[2], const int yy[2], uint64_t out[2]){
    int64_t s0=(int64_t)(((uint64_t)yy[0]*(uint64_t)(yy[0]+512*xx[0]))<<40); int64_t s1=(int64_t)(((uint64_t)yy[1]*(uint64_t)(yy[1]+512*xx[1]))<<40);
    __int128 a0=kExp_t6d9[8]; __int128 a1=kExp_t6d9[8];
    for(int k=7;k>=0;k--){ int64_t c=kExp_t6d9[k];
        a0=(__int128)c+((__int128)al_exp_hs(a0,s0)*2); a1=(__int128)c+((__int128)al_exp_hs(a1,s1)*2); }
    a0=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a0,s0)*2); a1=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a1,s1)*2);
    uint64_t v0=(uint64_t)a0; uint64_t v1=(uint64_t)a1;
    for(int q=0;q<6;q++){
        v0=al_exp_hu(v0,v0); v1=al_exp_hu(v1,v1);
    }
    out[0]=v0; out[1]=v1;
}
AL_INLINE void shuttle_exp_t6d9_x3(const int xx[3], const int yy[3], uint64_t out[3]){
    int64_t s0=(int64_t)(((uint64_t)yy[0]*(uint64_t)(yy[0]+512*xx[0]))<<40); int64_t s1=(int64_t)(((uint64_t)yy[1]*(uint64_t)(yy[1]+512*xx[1]))<<40); int64_t s2=(int64_t)(((uint64_t)yy[2]*(uint64_t)(yy[2]+512*xx[2]))<<40);
    __int128 a0=kExp_t6d9[8]; __int128 a1=kExp_t6d9[8]; __int128 a2=kExp_t6d9[8];
    for(int k=7;k>=0;k--){ int64_t c=kExp_t6d9[k];
        a0=(__int128)c+((__int128)al_exp_hs(a0,s0)*2); a1=(__int128)c+((__int128)al_exp_hs(a1,s1)*2); a2=(__int128)c+((__int128)al_exp_hs(a2,s2)*2); }
    a0=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a0,s0)*2); a1=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a1,s1)*2); a2=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a2,s2)*2);
    uint64_t v0=(uint64_t)a0; uint64_t v1=(uint64_t)a1; uint64_t v2=(uint64_t)a2;
    for(int q=0;q<6;q++){
        v0=al_exp_hu(v0,v0); v1=al_exp_hu(v1,v1); v2=al_exp_hu(v2,v2);
    }
    out[0]=v0; out[1]=v1; out[2]=v2;
}
AL_INLINE void shuttle_exp_t6d9_x4(const int xx[4], const int yy[4], uint64_t out[4]){
    int64_t s0=(int64_t)(((uint64_t)yy[0]*(uint64_t)(yy[0]+512*xx[0]))<<40); int64_t s1=(int64_t)(((uint64_t)yy[1]*(uint64_t)(yy[1]+512*xx[1]))<<40); int64_t s2=(int64_t)(((uint64_t)yy[2]*(uint64_t)(yy[2]+512*xx[2]))<<40); int64_t s3=(int64_t)(((uint64_t)yy[3]*(uint64_t)(yy[3]+512*xx[3]))<<40);
    __int128 a0=kExp_t6d9[8]; __int128 a1=kExp_t6d9[8]; __int128 a2=kExp_t6d9[8]; __int128 a3=kExp_t6d9[8];
    for(int k=7;k>=0;k--){ int64_t c=kExp_t6d9[k];
        a0=(__int128)c+((__int128)al_exp_hs(a0,s0)*2); a1=(__int128)c+((__int128)al_exp_hs(a1,s1)*2); a2=(__int128)c+((__int128)al_exp_hs(a2,s2)*2); a3=(__int128)c+((__int128)al_exp_hs(a3,s3)*2); }
    a0=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a0,s0)*2); a1=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a1,s1)*2); a2=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a2,s2)*2); a3=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a3,s3)*2);
    uint64_t v0=(uint64_t)a0; uint64_t v1=(uint64_t)a1; uint64_t v2=(uint64_t)a2; uint64_t v3=(uint64_t)a3;
    for(int q=0;q<6;q++){
        v0=al_exp_hu(v0,v0); v1=al_exp_hu(v1,v1); v2=al_exp_hu(v2,v2); v3=al_exp_hu(v3,v3);
    }
    out[0]=v0; out[1]=v1; out[2]=v2; out[3]=v3;
}
AL_INLINE void shuttle_exp_t6d9_x8(const int xx[8], const int yy[8], uint64_t out[8]){
    int64_t s0=(int64_t)(((uint64_t)yy[0]*(uint64_t)(yy[0]+512*xx[0]))<<40); int64_t s1=(int64_t)(((uint64_t)yy[1]*(uint64_t)(yy[1]+512*xx[1]))<<40); int64_t s2=(int64_t)(((uint64_t)yy[2]*(uint64_t)(yy[2]+512*xx[2]))<<40); int64_t s3=(int64_t)(((uint64_t)yy[3]*(uint64_t)(yy[3]+512*xx[3]))<<40); int64_t s4=(int64_t)(((uint64_t)yy[4]*(uint64_t)(yy[4]+512*xx[4]))<<40); int64_t s5=(int64_t)(((uint64_t)yy[5]*(uint64_t)(yy[5]+512*xx[5]))<<40); int64_t s6=(int64_t)(((uint64_t)yy[6]*(uint64_t)(yy[6]+512*xx[6]))<<40); int64_t s7=(int64_t)(((uint64_t)yy[7]*(uint64_t)(yy[7]+512*xx[7]))<<40);
    __int128 a0=kExp_t6d9[8]; __int128 a1=kExp_t6d9[8]; __int128 a2=kExp_t6d9[8]; __int128 a3=kExp_t6d9[8]; __int128 a4=kExp_t6d9[8]; __int128 a5=kExp_t6d9[8]; __int128 a6=kExp_t6d9[8]; __int128 a7=kExp_t6d9[8];
    for(int k=7;k>=0;k--){ int64_t c=kExp_t6d9[k];
        a0=(__int128)c+((__int128)al_exp_hs(a0,s0)*2); a1=(__int128)c+((__int128)al_exp_hs(a1,s1)*2); a2=(__int128)c+((__int128)al_exp_hs(a2,s2)*2); a3=(__int128)c+((__int128)al_exp_hs(a3,s3)*2); a4=(__int128)c+((__int128)al_exp_hs(a4,s4)*2); a5=(__int128)c+((__int128)al_exp_hs(a5,s5)*2); a6=(__int128)c+((__int128)al_exp_hs(a6,s6)*2); a7=(__int128)c+((__int128)al_exp_hs(a7,s7)*2); }
    a0=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a0,s0)*2); a1=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a1,s1)*2); a2=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a2,s2)*2); a3=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a3,s3)*2); a4=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a4,s4)*2); a5=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a5,s5)*2); a6=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a6,s6)*2); a7=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a7,s7)*2);
    uint64_t v0=(uint64_t)a0; uint64_t v1=(uint64_t)a1; uint64_t v2=(uint64_t)a2; uint64_t v3=(uint64_t)a3; uint64_t v4=(uint64_t)a4; uint64_t v5=(uint64_t)a5; uint64_t v6=(uint64_t)a6; uint64_t v7=(uint64_t)a7;
    for(int q=0;q<6;q++){
        v0=al_exp_hu(v0,v0); v1=al_exp_hu(v1,v1); v2=al_exp_hu(v2,v2); v3=al_exp_hu(v3,v3); v4=al_exp_hu(v4,v4); v5=al_exp_hu(v5,v5); v6=al_exp_hu(v6,v6); v7=al_exp_hu(v7,v7);
    }
    out[0]=v0; out[1]=v1; out[2]=v2; out[3]=v3; out[4]=v4; out[5]=v5; out[6]=v6; out[7]=v7;
}

/* t=7 squarings, degree 8, total mul 15, precision 2^-54.486 */
static const int64_t kExp_t7d8[8] = { INT64_C(-888099775658129789), INT64_C(21378331275493551), INT64_C(-343079355609610), INT64_C(4129301863954), INT64_C(-39760209654), INT64_C(319035825), INT64_C(-2194237), INT64_C(13205) };
AL_INLINE void shuttle_exp_t7d8_x1(const int xx[1], const int yy[1], uint64_t out[1]){
    int64_t s0=(int64_t)(((uint64_t)yy[0]*(uint64_t)(yy[0]+512*xx[0]))<<40);
    __int128 a0=kExp_t7d8[7];
    for(int k=6;k>=0;k--){ int64_t c=kExp_t7d8[k];
        a0=(__int128)c+((__int128)al_exp_hs(a0,s0)*2); }
    a0=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a0,s0)*2);
    uint64_t v0=(uint64_t)a0;
    for(int q=0;q<7;q++){
        v0=al_exp_hu(v0,v0);
    }
    out[0]=v0;
}
AL_INLINE void shuttle_exp_t7d8_x2(const int xx[2], const int yy[2], uint64_t out[2]){
    int64_t s0=(int64_t)(((uint64_t)yy[0]*(uint64_t)(yy[0]+512*xx[0]))<<40); int64_t s1=(int64_t)(((uint64_t)yy[1]*(uint64_t)(yy[1]+512*xx[1]))<<40);
    __int128 a0=kExp_t7d8[7]; __int128 a1=kExp_t7d8[7];
    for(int k=6;k>=0;k--){ int64_t c=kExp_t7d8[k];
        a0=(__int128)c+((__int128)al_exp_hs(a0,s0)*2); a1=(__int128)c+((__int128)al_exp_hs(a1,s1)*2); }
    a0=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a0,s0)*2); a1=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a1,s1)*2);
    uint64_t v0=(uint64_t)a0; uint64_t v1=(uint64_t)a1;
    for(int q=0;q<7;q++){
        v0=al_exp_hu(v0,v0); v1=al_exp_hu(v1,v1);
    }
    out[0]=v0; out[1]=v1;
}
AL_INLINE void shuttle_exp_t7d8_x3(const int xx[3], const int yy[3], uint64_t out[3]){
    int64_t s0=(int64_t)(((uint64_t)yy[0]*(uint64_t)(yy[0]+512*xx[0]))<<40); int64_t s1=(int64_t)(((uint64_t)yy[1]*(uint64_t)(yy[1]+512*xx[1]))<<40); int64_t s2=(int64_t)(((uint64_t)yy[2]*(uint64_t)(yy[2]+512*xx[2]))<<40);
    __int128 a0=kExp_t7d8[7]; __int128 a1=kExp_t7d8[7]; __int128 a2=kExp_t7d8[7];
    for(int k=6;k>=0;k--){ int64_t c=kExp_t7d8[k];
        a0=(__int128)c+((__int128)al_exp_hs(a0,s0)*2); a1=(__int128)c+((__int128)al_exp_hs(a1,s1)*2); a2=(__int128)c+((__int128)al_exp_hs(a2,s2)*2); }
    a0=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a0,s0)*2); a1=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a1,s1)*2); a2=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a2,s2)*2);
    uint64_t v0=(uint64_t)a0; uint64_t v1=(uint64_t)a1; uint64_t v2=(uint64_t)a2;
    for(int q=0;q<7;q++){
        v0=al_exp_hu(v0,v0); v1=al_exp_hu(v1,v1); v2=al_exp_hu(v2,v2);
    }
    out[0]=v0; out[1]=v1; out[2]=v2;
}
AL_INLINE void shuttle_exp_t7d8_x4(const int xx[4], const int yy[4], uint64_t out[4]){
    int64_t s0=(int64_t)(((uint64_t)yy[0]*(uint64_t)(yy[0]+512*xx[0]))<<40); int64_t s1=(int64_t)(((uint64_t)yy[1]*(uint64_t)(yy[1]+512*xx[1]))<<40); int64_t s2=(int64_t)(((uint64_t)yy[2]*(uint64_t)(yy[2]+512*xx[2]))<<40); int64_t s3=(int64_t)(((uint64_t)yy[3]*(uint64_t)(yy[3]+512*xx[3]))<<40);
    __int128 a0=kExp_t7d8[7]; __int128 a1=kExp_t7d8[7]; __int128 a2=kExp_t7d8[7]; __int128 a3=kExp_t7d8[7];
    for(int k=6;k>=0;k--){ int64_t c=kExp_t7d8[k];
        a0=(__int128)c+((__int128)al_exp_hs(a0,s0)*2); a1=(__int128)c+((__int128)al_exp_hs(a1,s1)*2); a2=(__int128)c+((__int128)al_exp_hs(a2,s2)*2); a3=(__int128)c+((__int128)al_exp_hs(a3,s3)*2); }
    a0=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a0,s0)*2); a1=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a1,s1)*2); a2=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a2,s2)*2); a3=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a3,s3)*2);
    uint64_t v0=(uint64_t)a0; uint64_t v1=(uint64_t)a1; uint64_t v2=(uint64_t)a2; uint64_t v3=(uint64_t)a3;
    for(int q=0;q<7;q++){
        v0=al_exp_hu(v0,v0); v1=al_exp_hu(v1,v1); v2=al_exp_hu(v2,v2); v3=al_exp_hu(v3,v3);
    }
    out[0]=v0; out[1]=v1; out[2]=v2; out[3]=v3;
}
AL_INLINE void shuttle_exp_t7d8_x8(const int xx[8], const int yy[8], uint64_t out[8]){
    int64_t s0=(int64_t)(((uint64_t)yy[0]*(uint64_t)(yy[0]+512*xx[0]))<<40); int64_t s1=(int64_t)(((uint64_t)yy[1]*(uint64_t)(yy[1]+512*xx[1]))<<40); int64_t s2=(int64_t)(((uint64_t)yy[2]*(uint64_t)(yy[2]+512*xx[2]))<<40); int64_t s3=(int64_t)(((uint64_t)yy[3]*(uint64_t)(yy[3]+512*xx[3]))<<40); int64_t s4=(int64_t)(((uint64_t)yy[4]*(uint64_t)(yy[4]+512*xx[4]))<<40); int64_t s5=(int64_t)(((uint64_t)yy[5]*(uint64_t)(yy[5]+512*xx[5]))<<40); int64_t s6=(int64_t)(((uint64_t)yy[6]*(uint64_t)(yy[6]+512*xx[6]))<<40); int64_t s7=(int64_t)(((uint64_t)yy[7]*(uint64_t)(yy[7]+512*xx[7]))<<40);
    __int128 a0=kExp_t7d8[7]; __int128 a1=kExp_t7d8[7]; __int128 a2=kExp_t7d8[7]; __int128 a3=kExp_t7d8[7]; __int128 a4=kExp_t7d8[7]; __int128 a5=kExp_t7d8[7]; __int128 a6=kExp_t7d8[7]; __int128 a7=kExp_t7d8[7];
    for(int k=6;k>=0;k--){ int64_t c=kExp_t7d8[k];
        a0=(__int128)c+((__int128)al_exp_hs(a0,s0)*2); a1=(__int128)c+((__int128)al_exp_hs(a1,s1)*2); a2=(__int128)c+((__int128)al_exp_hs(a2,s2)*2); a3=(__int128)c+((__int128)al_exp_hs(a3,s3)*2); a4=(__int128)c+((__int128)al_exp_hs(a4,s4)*2); a5=(__int128)c+((__int128)al_exp_hs(a5,s5)*2); a6=(__int128)c+((__int128)al_exp_hs(a6,s6)*2); a7=(__int128)c+((__int128)al_exp_hs(a7,s7)*2); }
    a0=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a0,s0)*2); a1=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a1,s1)*2); a2=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a2,s2)*2); a3=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a3,s3)*2); a4=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a4,s4)*2); a5=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a5,s5)*2); a6=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a6,s6)*2); a7=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a7,s7)*2);
    uint64_t v0=(uint64_t)a0; uint64_t v1=(uint64_t)a1; uint64_t v2=(uint64_t)a2; uint64_t v3=(uint64_t)a3; uint64_t v4=(uint64_t)a4; uint64_t v5=(uint64_t)a5; uint64_t v6=(uint64_t)a6; uint64_t v7=(uint64_t)a7;
    for(int q=0;q<7;q++){
        v0=al_exp_hu(v0,v0); v1=al_exp_hu(v1,v1); v2=al_exp_hu(v2,v2); v3=al_exp_hu(v3,v3); v4=al_exp_hu(v4,v4); v5=al_exp_hu(v5,v5); v6=al_exp_hu(v6,v6); v7=al_exp_hu(v7,v7);
    }
    out[0]=v0; out[1]=v1; out[2]=v2; out[3]=v3; out[4]=v4; out[5]=v5; out[6]=v6; out[7]=v7;
}

/* t=8 squarings, degree 7, total mul 15, precision 2^-53.522 */
static const int64_t kExp_t8d7[7] = { INT64_C(-444049887829064894), INT64_C(5344582818873388), INT64_C(-42884919451201), INT64_C(258081366497), INT64_C(-1242506552), INT64_C(4984935), INT64_C(-17142) };
AL_INLINE void shuttle_exp_t8d7_x1(const int xx[1], const int yy[1], uint64_t out[1]){
    int64_t s0=(int64_t)(((uint64_t)yy[0]*(uint64_t)(yy[0]+512*xx[0]))<<40);
    __int128 a0=kExp_t8d7[6];
    for(int k=5;k>=0;k--){ int64_t c=kExp_t8d7[k];
        a0=(__int128)c+((__int128)al_exp_hs(a0,s0)*2); }
    a0=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a0,s0)*2);
    uint64_t v0=(uint64_t)a0;
    for(int q=0;q<8;q++){
        v0=al_exp_hu(v0,v0);
    }
    out[0]=v0;
}
AL_INLINE void shuttle_exp_t8d7_x2(const int xx[2], const int yy[2], uint64_t out[2]){
    int64_t s0=(int64_t)(((uint64_t)yy[0]*(uint64_t)(yy[0]+512*xx[0]))<<40); int64_t s1=(int64_t)(((uint64_t)yy[1]*(uint64_t)(yy[1]+512*xx[1]))<<40);
    __int128 a0=kExp_t8d7[6]; __int128 a1=kExp_t8d7[6];
    for(int k=5;k>=0;k--){ int64_t c=kExp_t8d7[k];
        a0=(__int128)c+((__int128)al_exp_hs(a0,s0)*2); a1=(__int128)c+((__int128)al_exp_hs(a1,s1)*2); }
    a0=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a0,s0)*2); a1=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a1,s1)*2);
    uint64_t v0=(uint64_t)a0; uint64_t v1=(uint64_t)a1;
    for(int q=0;q<8;q++){
        v0=al_exp_hu(v0,v0); v1=al_exp_hu(v1,v1);
    }
    out[0]=v0; out[1]=v1;
}
AL_INLINE void shuttle_exp_t8d7_x3(const int xx[3], const int yy[3], uint64_t out[3]){
    int64_t s0=(int64_t)(((uint64_t)yy[0]*(uint64_t)(yy[0]+512*xx[0]))<<40); int64_t s1=(int64_t)(((uint64_t)yy[1]*(uint64_t)(yy[1]+512*xx[1]))<<40); int64_t s2=(int64_t)(((uint64_t)yy[2]*(uint64_t)(yy[2]+512*xx[2]))<<40);
    __int128 a0=kExp_t8d7[6]; __int128 a1=kExp_t8d7[6]; __int128 a2=kExp_t8d7[6];
    for(int k=5;k>=0;k--){ int64_t c=kExp_t8d7[k];
        a0=(__int128)c+((__int128)al_exp_hs(a0,s0)*2); a1=(__int128)c+((__int128)al_exp_hs(a1,s1)*2); a2=(__int128)c+((__int128)al_exp_hs(a2,s2)*2); }
    a0=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a0,s0)*2); a1=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a1,s1)*2); a2=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a2,s2)*2);
    uint64_t v0=(uint64_t)a0; uint64_t v1=(uint64_t)a1; uint64_t v2=(uint64_t)a2;
    for(int q=0;q<8;q++){
        v0=al_exp_hu(v0,v0); v1=al_exp_hu(v1,v1); v2=al_exp_hu(v2,v2);
    }
    out[0]=v0; out[1]=v1; out[2]=v2;
}
AL_INLINE void shuttle_exp_t8d7_x4(const int xx[4], const int yy[4], uint64_t out[4]){
    int64_t s0=(int64_t)(((uint64_t)yy[0]*(uint64_t)(yy[0]+512*xx[0]))<<40); int64_t s1=(int64_t)(((uint64_t)yy[1]*(uint64_t)(yy[1]+512*xx[1]))<<40); int64_t s2=(int64_t)(((uint64_t)yy[2]*(uint64_t)(yy[2]+512*xx[2]))<<40); int64_t s3=(int64_t)(((uint64_t)yy[3]*(uint64_t)(yy[3]+512*xx[3]))<<40);
    __int128 a0=kExp_t8d7[6]; __int128 a1=kExp_t8d7[6]; __int128 a2=kExp_t8d7[6]; __int128 a3=kExp_t8d7[6];
    for(int k=5;k>=0;k--){ int64_t c=kExp_t8d7[k];
        a0=(__int128)c+((__int128)al_exp_hs(a0,s0)*2); a1=(__int128)c+((__int128)al_exp_hs(a1,s1)*2); a2=(__int128)c+((__int128)al_exp_hs(a2,s2)*2); a3=(__int128)c+((__int128)al_exp_hs(a3,s3)*2); }
    a0=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a0,s0)*2); a1=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a1,s1)*2); a2=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a2,s2)*2); a3=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a3,s3)*2);
    uint64_t v0=(uint64_t)a0; uint64_t v1=(uint64_t)a1; uint64_t v2=(uint64_t)a2; uint64_t v3=(uint64_t)a3;
    for(int q=0;q<8;q++){
        v0=al_exp_hu(v0,v0); v1=al_exp_hu(v1,v1); v2=al_exp_hu(v2,v2); v3=al_exp_hu(v3,v3);
    }
    out[0]=v0; out[1]=v1; out[2]=v2; out[3]=v3;
}
AL_INLINE void shuttle_exp_t8d7_x8(const int xx[8], const int yy[8], uint64_t out[8]){
    int64_t s0=(int64_t)(((uint64_t)yy[0]*(uint64_t)(yy[0]+512*xx[0]))<<40); int64_t s1=(int64_t)(((uint64_t)yy[1]*(uint64_t)(yy[1]+512*xx[1]))<<40); int64_t s2=(int64_t)(((uint64_t)yy[2]*(uint64_t)(yy[2]+512*xx[2]))<<40); int64_t s3=(int64_t)(((uint64_t)yy[3]*(uint64_t)(yy[3]+512*xx[3]))<<40); int64_t s4=(int64_t)(((uint64_t)yy[4]*(uint64_t)(yy[4]+512*xx[4]))<<40); int64_t s5=(int64_t)(((uint64_t)yy[5]*(uint64_t)(yy[5]+512*xx[5]))<<40); int64_t s6=(int64_t)(((uint64_t)yy[6]*(uint64_t)(yy[6]+512*xx[6]))<<40); int64_t s7=(int64_t)(((uint64_t)yy[7]*(uint64_t)(yy[7]+512*xx[7]))<<40);
    __int128 a0=kExp_t8d7[6]; __int128 a1=kExp_t8d7[6]; __int128 a2=kExp_t8d7[6]; __int128 a3=kExp_t8d7[6]; __int128 a4=kExp_t8d7[6]; __int128 a5=kExp_t8d7[6]; __int128 a6=kExp_t8d7[6]; __int128 a7=kExp_t8d7[6];
    for(int k=5;k>=0;k--){ int64_t c=kExp_t8d7[k];
        a0=(__int128)c+((__int128)al_exp_hs(a0,s0)*2); a1=(__int128)c+((__int128)al_exp_hs(a1,s1)*2); a2=(__int128)c+((__int128)al_exp_hs(a2,s2)*2); a3=(__int128)c+((__int128)al_exp_hs(a3,s3)*2); a4=(__int128)c+((__int128)al_exp_hs(a4,s4)*2); a5=(__int128)c+((__int128)al_exp_hs(a5,s5)*2); a6=(__int128)c+((__int128)al_exp_hs(a6,s6)*2); a7=(__int128)c+((__int128)al_exp_hs(a7,s7)*2); }
    a0=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a0,s0)*2); a1=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a1,s1)*2); a2=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a2,s2)*2); a3=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a3,s3)*2); a4=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a4,s4)*2); a5=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a5,s5)*2); a6=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a6,s6)*2); a7=((__int128)UINT64_MAX)+((__int128)al_exp_hs(a7,s7)*2);
    uint64_t v0=(uint64_t)a0; uint64_t v1=(uint64_t)a1; uint64_t v2=(uint64_t)a2; uint64_t v3=(uint64_t)a3; uint64_t v4=(uint64_t)a4; uint64_t v5=(uint64_t)a5; uint64_t v6=(uint64_t)a6; uint64_t v7=(uint64_t)a7;
    for(int q=0;q<8;q++){
        v0=al_exp_hu(v0,v0); v1=al_exp_hu(v1,v1); v2=al_exp_hu(v2,v2); v3=al_exp_hu(v3,v3); v4=al_exp_hu(v4,v4); v5=al_exp_hu(v5,v5); v6=al_exp_hu(v6,v6); v7=al_exp_hu(v7,v7);
    }
    out[0]=v0; out[1]=v1; out[2]=v2; out[3]=v3; out[4]=v4; out[5]=v5; out[6]=v6; out[7]=v7;
}

typedef struct { int t, degree, total_mul; } exp_scheme_t;
static const exp_scheme_t EXP_SCHEMES[] = {
    {4, 12, 16},
    {5, 10, 15},
    {6, 9, 15},
    {7, 8, 15},
    {8, 7, 15},
};
#define EXP_NUM_SCHEMES 5
#undef AL_INLINE
#endif
