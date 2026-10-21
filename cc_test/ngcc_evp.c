/* Run one instance per process so an author-code crash identifies its instance. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/provider.h>
#include <openssl/core_names.h>
#include <openssl/err.h>
#include <openssl/params.h>

#define CHECK(c,stage) do { if (!(c)) { fprintf(stderr,"FAIL %s\n",stage); ERR_print_errors_fp(stderr); goto err; } } while (0)
static EVP_PKEY *import_key(OSSL_LIB_CTX *lib,const char *name,int selection,OSSL_PARAM *params)
{
    EVP_PKEY_CTX *c=EVP_PKEY_CTX_new_from_name(lib,name,"provider=ngcc");
    EVP_PKEY *k=NULL;
    if (!c || EVP_PKEY_fromdata_init(c)<=0 || EVP_PKEY_fromdata(c,&k,selection,params)<=0) {
        EVP_PKEY_free(k);k=NULL;
    }
    EVP_PKEY_CTX_free(c);return k;
}

/* Shared operation contexts must not let author APIs alter the stored key. */
static int key_unchanged(EVP_PKEY *key, const OSSL_PARAM *before, int selection)
{
    static const char *names[] = {
        OSSL_PKEY_PARAM_PUB_KEY, OSSL_PKEY_PARAM_PRIV_KEY
    };
    OSSL_PARAM *after = NULL;
    const OSSL_PARAM *a, *b;
    size_t i;
    int ok = 0;

    if (EVP_PKEY_todata(key, selection, &after) <= 0)
        return 0;
    for (i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
        if (i == 1 && (selection & OSSL_KEYMGMT_SELECT_PRIVATE_KEY) == 0)
            continue;
        a = OSSL_PARAM_locate_const(before, names[i]);
        b = OSSL_PARAM_locate_const(after, names[i]);
        if (a == NULL || b == NULL || a->data_size != b->data_size
            || CRYPTO_memcmp(a->data, b->data, a->data_size) != 0)
            goto end;
    }
    ok = 1;
end:
    OSSL_PARAM_free(after);
    return ok;
}

int ngcc_evp_check(int argc,char **argv)
{
    OSSL_LIB_CTX *lib=NULL;
    OSSL_PROVIDER *def=NULL,*ngcc=NULL;
    EVP_PKEY_CTX *kg=NULL,*enc=NULL,*dec=NULL,*sign=NULL,*verify=NULL;
    EVP_PKEY *key=NULL,*pub=NULL,*copy=NULL,*key2=NULL,*bad=NULL;
    EVP_MD_CTX *md=NULL,*mv=NULL;
    OSSL_PARAM *params=NULL,*params2=NULL;
    OSSL_PARAM malformed[2];
    const OSSL_PARAM *pk,*pk2;
    unsigned char *ct=NULL,*ss=NULL,*ss2=NULL,*sig=NULL;
    size_t cl=0,sl=0,s2l=0,sn=0,cap;
    unsigned char msg[33];
    const char *name,*kind;
    int ret=1,bits;
    if(argc!=4) { fprintf(stderr,"usage: ngcc_evp module-directory kem|sig algorithm\n");return 2; }
    name=argv[3];kind=argv[2];
    for(size_t i=0;i<sizeof(msg);i++)msg[i]=(unsigned char)(i*7+3);
    CHECK((lib=OSSL_LIB_CTX_new()),"libctx");
    CHECK(OSSL_PROVIDER_set_default_search_path(lib,argv[1]),"module-path");
    CHECK((def=OSSL_PROVIDER_load(lib,"default")),"default-provider");
    CHECK((ngcc=OSSL_PROVIDER_load(lib,"ngcc")),"ngcc-provider");
    CHECK((kg=EVP_PKEY_CTX_new_from_name(lib,name,"provider=ngcc")),"key-context");
    CHECK(EVP_PKEY_keygen_init(kg)>0,"keygen-init");
    printf("STEP keygen\n");
    CHECK(EVP_PKEY_generate(kg,&key)>0,"keygen");
    CHECK(EVP_PKEY_get_int_param(key,OSSL_PKEY_PARAM_SECURITY_BITS,&bits)>0 && bits<384,"security-tier");
    CHECK(EVP_PKEY_todata(key,EVP_PKEY_KEYPAIR,&params)>0,"export");
    pk=OSSL_PARAM_locate_const(params,OSSL_PKEY_PARAM_PUB_KEY);
    CHECK(pk && pk->data_size,"export-public");
    CHECK((pub=import_key(lib,name,EVP_PKEY_PUBLIC_KEY,params)),"public-import");
    CHECK((copy=import_key(lib,name,EVP_PKEY_KEYPAIR,params)),"keypair-import");
    CHECK(EVP_PKEY_eq(key,copy)==1,"key-equality");
    malformed[0]=OSSL_PARAM_construct_octet_string(OSSL_PKEY_PARAM_PUB_KEY,pk->data,pk->data_size-1);
    malformed[1]=OSSL_PARAM_construct_end();
    bad=import_key(lib,name,EVP_PKEY_PUBLIC_KEY,malformed);
    CHECK(!bad,"reject-short-public-key");
    ERR_clear_error();
    printf("STEP second-keygen\n");
    CHECK(EVP_PKEY_generate(kg,&key2)>0,"second-keygen");
    CHECK(EVP_PKEY_todata(key2,EVP_PKEY_PUBLIC_KEY,&params2)>0,"second-export");
    pk2=OSSL_PARAM_locate_const(params2,OSSL_PKEY_PARAM_PUB_KEY);
    CHECK(pk2,"second-export-public");
    if(pk->data_size==pk2->data_size && !CRYPTO_memcmp(pk->data,pk2->data,pk->data_size))
        printf("WARNING deterministic-public-key: author implementation ignores or resets supplied DRNG state\n");
    if(!strcmp(kind,"kem")) {
        CHECK((enc=EVP_PKEY_CTX_new_from_pkey(lib,pub,"provider=ngcc")),"enc-context");
        CHECK((dec=EVP_PKEY_CTX_new_from_pkey(lib,copy,"provider=ngcc")),"dec-context");
        CHECK(EVP_PKEY_encapsulate_init(enc,NULL)>0,"enc-init");
        CHECK(EVP_PKEY_decapsulate_init(dec,NULL)>0,"dec-init");
        CHECK(EVP_PKEY_encapsulate(enc,NULL,&cl,NULL,&sl)>0 && cl && sl,"kem-lengths");
        CHECK((ct=OPENSSL_malloc(cl)) && (ss=OPENSSL_malloc(sl)) && (ss2=OPENSSL_malloc(sl)),"kem-alloc");
        for(int i=0;i<3;i++) {
            printf("STEP encapsulate-%d\n",i);
            CHECK(EVP_PKEY_encapsulate(enc,ct,&cl,ss,&sl)>0,"encapsulate");
            s2l=sl;
            CHECK(EVP_PKEY_decapsulate(dec,ss2,&s2l,ct,cl)>0,"decapsulate");
            CHECK(s2l==sl && !CRYPTO_memcmp(ss,ss2,sl),"shared-secret-match");
        }
        s2l=sl;
        CHECK(EVP_PKEY_decapsulate(dec,ss2,&s2l,ct,cl-1)<=0,"reject-short-ciphertext");
        printf("SIZES pk=%zu ct=%zu ss=%zu\n",pk->data_size,cl,sl);
    } else {
        CHECK((sign=EVP_PKEY_CTX_new_from_pkey(lib,copy,"provider=ngcc")),"sign-context");
        CHECK((verify=EVP_PKEY_CTX_new_from_pkey(lib,pub,"provider=ngcc")),"verify-context");
        CHECK(EVP_PKEY_sign_init(sign)>0,"sign-init");
        CHECK(EVP_PKEY_verify_init(verify)>0,"verify-init");
        CHECK(EVP_PKEY_sign(sign,NULL,&sn,msg,sizeof(msg))>0 && sn,"sig-capacity");
        cap=sn;
        CHECK((sig=OPENSSL_malloc(cap+1)),"sig-alloc");
        for(int i=0;i<2;i++) {
            printf("STEP sign-%d\n",i);
            sn=cap;
            CHECK(EVP_PKEY_sign(sign,sig,&sn,i?NULL:msg,i?0:sizeof(msg))>0,"sign");
            CHECK(sn<=cap && EVP_PKEY_verify(verify,sig,sn,i?NULL:msg,i?0:sizeof(msg))==1,"verify");
        }
        CHECK(EVP_PKEY_verify(verify,sig,cap+1,msg,sizeof(msg))<=0,"reject-oversized-sig");
        ERR_clear_error();
        CHECK((md=EVP_MD_CTX_new()) && (mv=EVP_MD_CTX_new()),"digest-context");
        CHECK(EVP_DigestSignInit_ex(md,NULL,NULL,lib,"provider=ngcc",copy,NULL)>0,"digest-sign-init");
        CHECK(EVP_DigestVerifyInit_ex(mv,NULL,NULL,lib,"provider=ngcc",pub,NULL)>0,"digest-verify-init");
        CHECK(EVP_DigestSignUpdate(md,msg,13)>0 && EVP_DigestSignUpdate(md,msg+13,20)>0,"digest-sign-update");
        sn=cap;
        CHECK(EVP_DigestSignFinal(md,sig,&sn)>0,"digest-sign-final");
        CHECK(EVP_DigestVerifyUpdate(mv,msg,33)>0 && EVP_DigestVerifyFinal(mv,sig,sn)==1,"digest-verify-final");
        if (!strcmp(name,"sig-01-aigis-sig1")) {
            CHECK(EVP_DigestSignInit_ex(md,NULL,NULL,lib,"provider=ngcc",copy,NULL)>0,"empty-stream-sign-init");
            CHECK(EVP_DigestVerifyInit_ex(mv,NULL,NULL,lib,"provider=ngcc",pub,NULL)>0,"empty-stream-verify-init");
            sn=cap;
            CHECK(EVP_DigestSignFinal(md,sig,&sn)>0,"empty-stream-sign-final");
            CHECK(EVP_DigestVerifyFinal(mv,sig,sn)==1,"empty-stream-verify-final");
        }
        printf("SIZES pk=%zu sig=%zu capacity=%zu\n",pk->data_size,sn,cap);
    }
    CHECK(key_unchanged(copy,params,EVP_PKEY_KEYPAIR),"stored-keypair-unchanged");
    CHECK(key_unchanged(pub,params,EVP_PKEY_PUBLIC_KEY),"stored-public-key-unchanged");
    printf("PASS key-material-unchanged\n");
    printf("PASS %s\n",name);ret=0;
err:
    OPENSSL_free(ct);OPENSSL_free(ss);OPENSSL_free(ss2);OPENSSL_free(sig);
    OSSL_PARAM_free(params);OSSL_PARAM_free(params2);
    EVP_PKEY_CTX_free(kg);EVP_PKEY_CTX_free(enc);EVP_PKEY_CTX_free(dec);EVP_PKEY_CTX_free(sign);EVP_PKEY_CTX_free(verify);
    EVP_MD_CTX_free(md);EVP_MD_CTX_free(mv);
    EVP_PKEY_free(key);EVP_PKEY_free(pub);EVP_PKEY_free(copy);EVP_PKEY_free(key2);EVP_PKEY_free(bad);
    OSSL_PROVIDER_unload(ngcc);OSSL_PROVIDER_unload(def);OSSL_LIB_CTX_free(lib);
    return ret;
}
#ifndef NGCC_TEST_NO_MAIN
int main(int argc,char **argv)
{
    setbuf(stdout,NULL);
    return ngcc_evp_check(argc,argv);
}
#endif
