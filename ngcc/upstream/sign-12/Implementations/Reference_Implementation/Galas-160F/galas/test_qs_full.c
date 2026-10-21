/* test_qs_full.c — verify the full degree-3 constraint system.
 * Generates a valid (k,x,y) via keygen, computes sigma witnesses, checks all
 * constraints pass. Then corrupts k and checks they fail. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gf2n.h"
#include "owf.h"
#include "bf.h"
#include "qs_full_constraints.h"

static int fails = 0;

static void run(unsigned lambda) {
    const galas_owf_params* P = galas_owf_params_for(lambda);
    const gf_ctx* fc = (lambda==160)?bf_ctx_160():(lambda==256)?bf_ctx_256():(lambda==384)?bf_ctx_384():bf_ctx_512();
    unsigned lb = lambda/8, m = lambda/2, mb = m/8;
    char lbl[80];

    /* deterministic k,x with k[0]&0x03==0x03 */
    uint8_t k[64]={0}, x[64]={0}, y[64]={0};
    for (unsigned i=0;i<lb;i++){k[i]=(uint8_t)(i*7+0x43);x[i]=(uint8_t)(i*3+1);}
    k[0] |= 0x03;
    /* find a valid (k,x) via rejection */
    int ok=0;
    for(unsigned t=0;t<2000 && !ok;t++){
        x[0]=(uint8_t)(t+1);
        if(galas_owf_eval(P,fc,y,x,k)==0) ok=1;
    }
    if(!ok){snprintf(lbl,sizeof(lbl),"n=%u could not find valid witness",lambda);printf("FAIL [%s]\n",lbl);fails++;return;}

    /* compute sigma witnesses from a0,a1,a2 */
    uint8_t a0[64],a1[64],a2[64],rho0[64],rho1[64],rho2[64];
    for(unsigned i=0;i<lb;i++){rho0[i]=x[i]^k[i]^P->c0[i];rho1[i]=k[i]^P->c1[i];rho2[i]=k[i]^P->c2[i];}
    galas_apply_lin_map(fc,(gf_limb_t*)a0,(gf_limb_t*)rho0,P->M0);
    galas_apply_lin_map(fc,(gf_limb_t*)a1,(gf_limb_t*)rho1,P->M1);
    galas_apply_lin_map(fc,(gf_limb_t*)a2,(gf_limb_t*)rho2,P->M2);

    uint8_t s0[32]={0},s1[32]={0},s2[32]={0};
    galas_compute_sigma(s0,a0,fc,P);
    galas_compute_sigma(s1,a1,fc,P);
    galas_compute_sigma(s2,a2,fc,P);

    /* check: valid witness passes all constraints */
    int c_ok = galas_constraints_eval(fc,P,k,s0,s1,s2,x,y);
    snprintf(lbl,sizeof(lbl),"n=%u valid witness constraints PASS",lambda);
    if(c_ok){printf("ok   [%s]\n",lbl);}else{printf("FAIL [%s]\n",lbl);fails++;}

    /* check: corrupted k fails */
    uint8_t k_bad[64]; memcpy(k_bad,k,lb); k_bad[lb-1]^=0x01;
    int c_bad = galas_constraints_eval(fc,P,k_bad,s0,s1,s2,x,y);
    snprintf(lbl,sizeof(lbl),"n=%u corrupted k constraints FAIL",lambda);
    if(!c_bad){printf("ok   [%s]\n",lbl);}else{printf("FAIL [%s]\n",lbl);fails++;}

    /* check: corrupted sigma fails */
    uint8_t s0_bad[32]; memcpy(s0_bad,s0,mb); s0_bad[0]^=0x01;
    int c_sbad = galas_constraints_eval(fc,P,k,s0_bad,s1,s2,x,y);
    snprintf(lbl,sizeof(lbl),"n=%u corrupted sigma constraints FAIL",lambda);
    if(!c_sbad){printf("ok   [%s]\n",lbl);}else{printf("FAIL [%s]\n",lbl);fails++;}
}

int main(void){
    unsigned lambdas[]={160,256,384,512};
    for(unsigned i=0;i<4;i++) run(lambdas[i]);
    printf("\nfull constraints: %d failures\n",fails);
    return fails?1:0;
}
