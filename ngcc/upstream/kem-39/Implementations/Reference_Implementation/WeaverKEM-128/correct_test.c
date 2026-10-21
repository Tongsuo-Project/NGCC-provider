
//
//  PQCgenKAT_kem.c
//
//  Created by Bassham, Lawrence E (Fed) on 8/29/17.
//  Copyright © 2017 Bassham, Lawrence E (Fed). All rights reserved.
//
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
//#include "rand.h"
#include "rng.h"
#include "params.h"
#include "api.h"
#include "kem.h"

#define	MAX_MARKER_LEN		50
#define KAT_SUCCESS          0
#define KAT_FILE_OPEN_ERROR -1
#define KAT_DATA_ERROR      -3
#define KAT_CRYPTO_FAILURE  -4

int		FindMarker(FILE *infile, const char *marker);
int		ReadHex(FILE *infile, unsigned char *A, int Length, char *str);
void	fprintBstr(FILE *fp, char *S, unsigned char *A, unsigned long long L);

#define MAX_TEST_ROUND   100000
#define MAX_FAILURE_STOP 10

//#define LOG_FOR_DEBUG

static int str_to_hex(uint8_t *dst_buf, uint32_t len, const char *src_str, uint32_t len_str)
{
    uint8_t high, low;
    uint32_t idx, ii = 0;
    uint32_t outlen = len_str / 2;

    if (len < outlen)
    {
        printf("Buflen too short! Should be larger or equal to: %u", outlen);
        return 0;
    }
    for (idx = 0; idx < len_str; idx += 2)
    {
        high = src_str[idx];
        low = src_str[idx + 1];

        if (high >= '0' && high <= '9')
            high = high - '0';
        else if (high >= 'A' && high <= 'F')
            high = high - 'A' + 10;
        else if (high >= 'a' && high <= 'f')
            high = high - 'a' + 10;
        else
            return 0;

        if (low >= '0' && low <= '9')
            low = low - '0';
        else if (low >= 'A' && low <= 'F')
            low = low - 'A' + 10;
        else if (low >= 'a' && low <= 'f')
            low = low - 'a' + 10;
        else
            return 0;

        dst_buf[ii++] = high << 4 | low;
    }
    return 1;
}

int
main()
{
    char *chk_seed[] = {
        "CFD8F5EAD732B5E316D03A38CC869BBC71A8B55873E13AE3B9E7300531C9B97A699B763F2C87D22419D45FBC641B103D",
        "9ADFED97E47C06EB0280D23726DEED9525B38A433D8DB726065864B552F6795F70E95ACD1AB9D46F2A85DF1D38A3BEFD",
        "DC190FDE305958B67703EDEA6E52DAF3E933B552AE1608B2A9E2E976BF4DC91B14D7D66AABA71BDF41B79E6EE00AB735",
        "1510754DA54DE33E6107F6BA1A8DCF20B16880D9C955384839E95E32410294E90394E83482D3C0643780DD5B98086704",
        "72F2734FD47762EC8ED792BA749E4311815829933CF90B061DFAA7EC0728947BA61EB879DA3FAFC34581F9B9B0633ED9",
        "4180EA73E69382C9CA1F78EEC409566144273E9AC7D5960B822AC1B1B30767C44044D0B7F1AF16B029EB54EAE2E260AC",
    };
    
    
    char                fn_rsp[32];
    FILE                *fp_rsp;
    unsigned char       seed[96] = { 0 };
    unsigned char       entropy_input[96];
    unsigned char       ct[CRYPTO_CIPHERTEXTBYTES], ss[CRYPTO_BYTES], ss1[CRYPTO_BYTES];
    uint64_t            fail, success;
    uint64_t            ctr;
    unsigned char       pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES];
    int                 ret_val;
    
    // Create the REQUEST file
#ifdef LOG_FOR_DEBUG
    sprintf(fn_rsp, "PQCkemKAT_%d.rsp", WEAVER_SECRETKEYBYTES);
    if ( (fp_rsp = fopen(fn_rsp, "w")) == NULL ) {
        printf("Couldn't open <%s> for write\n", fn_rsp);
        return KAT_FILE_OPEN_ERROR;
    }
    fprintf(fp_rsp, "# %s\n\n", CRYPTO_ALGNAME);
#endif 
    for (int i=0; i<48; i++)
        entropy_input[i] = i;

    randombytes_init(entropy_input, NULL, 256);
    
    success = fail = 0;
    ctr = 0;
	
    printf("%s start..\n", CRYPTO_ALGNAME);
    printf("sk length: %d\n", CRYPTO_SECRETKEYBYTES);
    printf("pk length: %d\n", CRYPTO_PUBLICKEYBYTES);
    printf("ct length: %d\n", CRYPTO_CIPHERTEXTBYTES);
    
    while(ctr < MAX_TEST_ROUND) {

#if 1
        randombytes(seed, 48);
#else
        if (!str_to_hex(seed, 48, chk_seed[ctr], strlen(chk_seed[ctr]))) {
            printf("ERROR: unable to read 'seed' from <%s>\n", chk_seed[ctr]);
            return -1;
        }
#endif
        randombytes_init(seed, NULL, 256);
        


        // Generate the public/private keypair
        if ( (ret_val = crypto_kem_keypair(pk, sk)) != 0) {
            printf("crypto_kem_keypair returned <%d>\n", ret_val);
            return KAT_CRYPTO_FAILURE;
        }
        if ( (ret_val = crypto_kem_enc(ct, ss, pk)) != 0) {
            printf("crypto_kem_enc returned <%d>\n", ret_val);
            return KAT_CRYPTO_FAILURE;
        }
        if ( (ret_val = crypto_kem_dec(ss1, ct, sk)) != 0) {
            fail++;
            printf("crypto_kem_dec returned <%d> at %ld-th round.\n", ret_val, ctr);
            //return KAT_CRYPTO_FAILURE;
            continue;
        }
        if ( memcmp(ss, ss1, CRYPTO_BYTES) ) {
            fail++;
#ifdef LOG_FOR_DEBUG
			//fprintf(fp_rsp, "crypto_kem_dec returned bad 'ss' value\n");
            fprintf(fp_rsp, "count = %lu\n", ctr);
            fprintBstr(fp_rsp, "seed = ", seed, 48);
            fprintBstr(fp_rsp, "pk = ", pk, CRYPTO_PUBLICKEYBYTES);
            fprintBstr(fp_rsp, "sk = ", sk, WEAVER_SECRETKEYBYTES);
            fprintBstr(fp_rsp, "ct = ", ct, CRYPTO_CIPHERTEXTBYTES);
            fprintBstr(fp_rsp, "ss = ", ss, CRYPTO_BYTES);
            fprintBstr(fp_rsp, "ss1 = ", ss1, CRYPTO_BYTES);
            printf("crypto_kem_dec returned bad 'ss' value at %lu-th round.\n", ctr);
#endif
#ifdef MAX_FAILURE_STOP
            if (fail >= MAX_FAILURE_STOP) {
                ctr++;
                break;
            }
#endif
        }
        ctr++;
    }
    printf("Failure times: %ld / in total %ld rounds of Test\n", fail, ctr);
#ifdef LOG_FOR_DEBUG
    fclose(fp_rsp);
#endif
    return KAT_SUCCESS;
}

//
// ALLOW TO READ HEXADECIMAL ENTRY (KEYS, DATA, TEXT, etc.)
//
//
// ALLOW TO READ HEXADECIMAL ENTRY (KEYS, DATA, TEXT, etc.)
//
int
FindMarker(FILE *infile, const char *marker)
{
	char	line[MAX_MARKER_LEN];
	int		i, len;
	int curr_line;

	len = (int)strlen(marker);
	if ( len > MAX_MARKER_LEN-1 )
		len = MAX_MARKER_LEN-1;

	for ( i=0; i<len; i++ )
	  {
	    curr_line = fgetc(infile);
	    line[i] = curr_line;
	    if (curr_line == EOF )
	      return 0;
	  }
	line[len] = '\0';

	while ( 1 ) {
		if ( !strncmp(line, marker, len) )
			return 1;

		for ( i=0; i<len-1; i++ )
			line[i] = line[i+1];
		curr_line = fgetc(infile);
		line[len-1] = curr_line;
		if (curr_line == EOF )
		    return 0;
		line[len] = '\0';
	}

	// shouldn't get here
	return 0;
}

//
// ALLOW TO READ HEXADECIMAL ENTRY (KEYS, DATA, TEXT, etc.)
//
int
ReadHex(FILE *infile, unsigned char *A, int Length, char *str)
{
	int			i, ch, started;
	unsigned char	ich;

	if ( Length == 0 ) {
		A[0] = 0x00;
		return 1;
	}
	memset(A, 0x00, Length);
	started = 0;
	if ( FindMarker(infile, str) )
		while ( (ch = fgetc(infile)) != EOF ) {
			if ( !isxdigit(ch) ) {
				if ( !started ) {
					if ( ch == '\n' )
						break;
					else
						continue;
				}
				else
					break;
			}
			started = 1;
			if ( (ch >= '0') && (ch <= '9') )
				ich = ch - '0';
			else if ( (ch >= 'A') && (ch <= 'F') )
				ich = ch - 'A' + 10;
			else if ( (ch >= 'a') && (ch <= 'f') )
				ich = ch - 'a' + 10;
            else // shouldn't ever get here
                ich = 0;
			
			for ( i=0; i<Length-1; i++ )
				A[i] = (A[i] << 4) | (A[i+1] >> 4);
			A[Length-1] = (A[Length-1] << 4) | ich;
		}
	else
		return 0;

	return 1;
}

void
fprintBstr(FILE *fp, char *S, unsigned char *A, unsigned long long L)
{
	unsigned long long  i;

	fprintf(fp, "%s", S);

	for ( i=0; i<L; i++ )
		fprintf(fp, "%02X", A[i]);

	if ( L == 0 )
		fprintf(fp, "00");

	fprintf(fp, "\n");
}

