/* Exercise shared candidate DRNG state through concurrent provider contexts. */
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

int ngcc_evp_check(int argc,char **argv);
static const char *module_dir;
static const char *test_kind = "both";
static void *worker(void *arg)
{
    const char *kind = strcmp(test_kind, "both") == 0
                       ? ((uintptr_t)arg < 4 ? "kem" : "sig") : test_kind;
    const char *name = strcmp(kind, "kem") == 0
                       ? "kem-08-bw-kem-128" : "sig-01-aigis-sig1";
    char *argv[]={(char*)"ngcc_threads",(char*)module_dir,(char*)kind,(char*)name};
    for(int i=0;i<4;i++) if(ngcc_evp_check(4,argv)) return (void*)1;
    return NULL;
}
int main(int argc,char **argv)
{
    pthread_t threads[8];
    int failed=0;
    if (argc != 2 && argc != 3)
        return 2;
    if (argc == 3)
        test_kind = argv[2];
    if (strcmp(test_kind, "kem") != 0 && strcmp(test_kind, "sig") != 0
        && strcmp(test_kind, "both") != 0)
        return 2;
    module_dir=argv[1];setbuf(stdout,NULL);
    for(uintptr_t i=0;i<8;i++)if(pthread_create(&threads[i],NULL,worker,(void*)i))return 2;
    for(int i=0;i<8;i++) { void *result;pthread_join(threads[i],&result);if(result)failed=1; }
    printf("THREADS %s: 8 threads, 32 complete EVP round trips\n",failed?"FAIL":"PASS");
    return failed;
}
