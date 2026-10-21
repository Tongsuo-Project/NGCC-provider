/* Reproduce candidate API failures without going through provider dispatch. */
#include "ngcc_ref.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern NGCC_SCHEME *NGCC_GETTER(void);
int main(int argc, char **argv)
{
    NGCC_SCHEME *s=NGCC_GETTER();
    unsigned char seed[64], msg[33];
    unsigned char *pk=calloc(1,s->pk_len), *sk=calloc(1,s->sk_len);
    unsigned char *sig=calloc(1,s->output_len+sizeof(msg));
    unsigned long long pl=s->pk_len, sl=s->sk_len, n;
    int count=argc>1?atoi(argv[1]):40, failures=0, rc;
    int first=argc>2?atoi(argv[2]):0;
    setbuf(stdout,NULL);
    if (!pk || !sk || !sig) return 2;
    for (size_t i=0;i<sizeof(seed);i++) seed[i]=(unsigned char)(i*3+7);
    for (size_t i=0;i<sizeof(msg);i++) msg[i]=(unsigned char)(i*7+3);
    if (s->seed(seed,sizeof(seed)) || s->keygen(pk,&pl,sk,&sl)) return 2;
    for (int trial=first;trial<count;trial++) {
        for (size_t i=0;i<sizeof(seed);i++) seed[i]=(unsigned char)(i*3+trial+11);
        seed[0]=(unsigned char)trial;seed[1]=(unsigned char)(trial>>8);
        if (s->seed(seed,sizeof(seed))) return 2;
        n=s->output_len;
        printf("BEGIN trial=%d\n",trial);
        rc=s->sign(sk,sl,msg,sizeof(msg),sig,&n);
        printf("trial=%d sign_rc=%d length=%llu capacity=%zu",trial,rc,n,s->output_len);
        if (rc || n>s->output_len) { failures++;puts(" FAIL");continue; }
        rc=s->verify(pk,pl,sig,n,msg,sizeof(msg));
        printf(" verify_rc=%d %s\n",rc,rc?"FAIL":"PASS");
        if (rc) failures++;
    }
    printf("RESULT %s: %d failures / %d trials\n",s->instance,failures,count-first);
    free(pk);free(sk);free(sig);
    return failures?1:0;
}
