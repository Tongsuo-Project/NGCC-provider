/* Restore the original key codecs and exercise certificate/TLS authentication. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/provider.h>
#include <openssl/encoder.h>
#include <openssl/decoder.h>
#include <openssl/core_names.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509v3.h>

#define CHECK(condition, stage)                                               \
    do {                                                                      \
        if (!(condition)) {                                                   \
            fprintf(stderr, "FAIL %s\n", stage);                              \
            goto end;                                                         \
        }                                                                     \
    } while (0)

/* Reimport encoded keys through the registered provider decoder chain. */
static EVP_PKEY *roundtrip(OSSL_LIB_CTX *libctx, EVP_PKEY *key,
                           const char *format, int private_key, const char *oid)
{
    OSSL_ENCODER_CTX *encoder = NULL;
    OSSL_DECODER_CTX *decoder = NULL;
    PKCS8_PRIV_KEY_INFO *p8 = NULL;
    X509_PUBKEY *spki = NULL;
    ASN1_OBJECT *public_oid = NULL;
    const ASN1_OBJECT *key_oid = NULL;
    EVP_PKEY *copy = NULL;
    unsigned char *data = NULL;
    const unsigned char *cursor;
    size_t length = 0, remaining;
    int selection = private_key ? EVP_PKEY_KEYPAIR : EVP_PKEY_PUBLIC_KEY;
    const char *structure = private_key ? "PrivateKeyInfo" : "SubjectPublicKeyInfo";

    encoder = OSSL_ENCODER_CTX_new_for_pkey(key, selection, format, structure, NULL);
    if (encoder == NULL || !OSSL_ENCODER_to_data(encoder, &data, &length)) {
        fprintf(stderr, "encode %s %s failed\n", format, structure);
        goto end;
    }
    if (strcmp(format, "DER") == 0) {
        cursor = data;
        if (private_key) {
            p8 = d2i_PKCS8_PRIV_KEY_INFO(NULL, &cursor, (long)length);
            if (p8 == NULL || !PKCS8_pkey_get0(&key_oid, NULL, NULL, NULL, p8))
                goto end;
        } else {
            spki = d2i_X509_PUBKEY(NULL, &cursor, (long)length);
            if (spki == NULL
                || !X509_PUBKEY_get0_param(&public_oid, NULL, NULL, NULL, spki))
                goto end;
            key_oid = public_oid;
        }
        if (cursor != data + length
            || OBJ_cmp(key_oid, OBJ_nid2obj(OBJ_txt2nid(oid))) != 0) {
            fprintf(stderr, "wrong %s AlgorithmIdentifier OID\n", structure);
            goto end;
        }
    }
    cursor = data;
    remaining = length;
    decoder = OSSL_DECODER_CTX_new_for_pkey(&copy, format, structure, NULL,
                                           selection, libctx, NULL);
    if (decoder == NULL || !OSSL_DECODER_from_data(decoder, &cursor, &remaining)) {
        fprintf(stderr, "decode %s %s failed (%zu bytes)\n", format, structure, length);
        EVP_PKEY_free(copy);
        copy = NULL;
    } else if (remaining != 0 || EVP_PKEY_eq(key, copy) != 1) {
        fprintf(stderr, "compare %s %s failed, remaining=%zu eq=%d\n",
                format, structure, remaining, EVP_PKEY_eq(key, copy));
        EVP_PKEY_free(copy);
        copy = NULL;
    }
end:
    OSSL_ENCODER_CTX_free(encoder);
    OSSL_DECODER_CTX_free(decoder);
    PKCS8_PRIV_KEY_INFO_free(p8);
    X509_PUBKEY_free(spki);
    OPENSSL_clear_free(data, length);
    return copy;
}

/* The old PKCS#8 representation stored only the raw secret key. */
static int check_legacy_pkcs8(OSSL_LIB_CTX *libctx, EVP_PKEY *key, const char *oid)
{
    PKCS8_PRIV_KEY_INFO *p8 = NULL;
    OSSL_DECODER_CTX *decoder = NULL;
    EVP_PKEY *decoded = NULL;
    EVP_PKEY_CTX *sign = NULL, *verify = NULL;
    unsigned char *secret = NULL, *der = NULL, *sig = NULL;
    const unsigned char *cursor;
    size_t secret_len = 0, remaining, sig_len;
    int der_len = 0, iteration, ret = 0;
    static const unsigned char message[] = "legacy PKCS8 reload";

    if (!EVP_PKEY_get_raw_private_key(key, NULL, &secret_len))
        goto end;
    for (iteration = 0; iteration < 2; ++iteration) {
        secret = OPENSSL_malloc(secret_len);
        if (secret == NULL || !EVP_PKEY_get_raw_private_key(key, secret, &secret_len)
            || (p8 = PKCS8_PRIV_KEY_INFO_new()) == NULL)
            goto end;
        if (!PKCS8_pkey_set0(p8, OBJ_txt2obj(oid, 1), 0, V_ASN1_UNDEF, NULL,
                             secret, (int)(secret_len - iteration)))
            goto end;
        secret = NULL;
        der_len = i2d_PKCS8_PRIV_KEY_INFO(p8, &der);
        if (der_len <= 0)
            goto end;
        cursor = der;
        remaining = (size_t)der_len;
        decoder = OSSL_DECODER_CTX_new_for_pkey(&decoded, "DER", "PrivateKeyInfo",
                                               NULL, EVP_PKEY_KEYPAIR, libctx, NULL);
        if (decoder == NULL)
            goto end;
        if (iteration == 0) {
            if (!OSSL_DECODER_from_data(decoder, &cursor, &remaining) || remaining != 0)
                goto end;
            sign = EVP_PKEY_CTX_new_from_pkey(libctx, decoded, NULL);
            verify = EVP_PKEY_CTX_new_from_pkey(libctx, key, NULL);
            sig_len = (size_t)EVP_PKEY_get_size(key);
            sig = OPENSSL_malloc(sig_len);
            if (sign == NULL || verify == NULL || sig == NULL
                || EVP_PKEY_sign_init(sign) <= 0 || EVP_PKEY_verify_init(verify) <= 0
                || EVP_PKEY_sign(sign, sig, &sig_len, message, sizeof(message)) <= 0
                || EVP_PKEY_verify(verify, sig, sig_len, message, sizeof(message)) != 1)
                goto end;
        } else {
            if (OSSL_DECODER_from_data(decoder, &cursor, &remaining) != 0 || decoded != NULL)
                goto end;
            ERR_clear_error();
        }
        OSSL_DECODER_CTX_free(decoder);
        decoder = NULL;
        EVP_PKEY_free(decoded);
        decoded = NULL;
        PKCS8_PRIV_KEY_INFO_free(p8);
        p8 = NULL;
        OPENSSL_clear_free(der, (size_t)der_len);
        der = NULL;
    }
    printf("PASS legacy-PKCS8 sign-verify; malformed-private-key-length rejected\n");
    ret = 1;
end:
    OSSL_DECODER_CTX_free(decoder);
    EVP_PKEY_CTX_free(sign);
    EVP_PKEY_CTX_free(verify);
    EVP_PKEY_free(decoded);
    PKCS8_PRIV_KEY_INFO_free(p8);
    OPENSSL_clear_free(secret, secret_len);
    OPENSSL_clear_free(der, der_len > 0 ? (size_t)der_len : 0);
    OPENSSL_free(sig);
    return ret;
}

static int extension(X509 *cert, X509 *issuer, int nid, const char *value)
{
    X509V3_CTX ctx;
    X509_EXTENSION *ext;
    int ok;

    X509V3_set_ctx(&ctx, issuer, cert, NULL, NULL, 0);
    ext = X509V3_EXT_conf_nid(NULL, &ctx, nid, value);
    if (ext == NULL)
        return 0;
    ok = X509_add_ext(cert, ext, -1);
    X509_EXTENSION_free(ext);
    return ok;
}

/* The same proposal signs the root and issues the end-entity certificate. */
static X509 *issue(OSSL_LIB_CTX *libctx, EVP_PKEY *subject_key,
                   X509 *issuer, EVP_PKEY *issuer_key)
{
    X509 *cert = X509_new_ex(libctx, NULL);
    X509_NAME *subject;
    EVP_MD_CTX *md = NULL;
    int ca = issuer == NULL, ok = 0;

    if (cert == NULL)
        goto end;
    subject = X509_get_subject_name(cert);
    if (!X509_set_version(cert, 2)
        || !ASN1_INTEGER_set(X509_get_serialNumber(cert), ca ? 1 : 2)
        || X509_gmtime_adj(X509_getm_notBefore(cert), -60) == NULL
        || X509_gmtime_adj(X509_getm_notAfter(cert), 86400) == NULL
        || !X509_NAME_add_entry_by_txt(subject, "CN", MBSTRING_ASC,
                                       (const unsigned char *)(ca ? "NGCC Test CA" : "localhost"),
                                       -1, -1, 0)
        || !X509_set_issuer_name(cert, ca ? subject : X509_get_subject_name(issuer))
        || !X509_set_pubkey(cert, subject_key)
        || !extension(cert, ca ? cert : issuer, NID_basic_constraints,
                       ca ? "critical,CA:TRUE" : "critical,CA:FALSE")
        || !extension(cert, ca ? cert : issuer, NID_key_usage,
                       ca ? "critical,keyCertSign,cRLSign" : "critical,digitalSignature"))
        goto end;
    if (!ca && (!extension(cert, issuer, NID_ext_key_usage, "serverAuth")
                || !extension(cert, issuer, NID_subject_alt_name, "DNS:localhost")))
        goto end;
    md = EVP_MD_CTX_new();
    if (md == NULL
        || EVP_DigestSignInit_ex(md, NULL, NULL, libctx, NULL, issuer_key, NULL) <= 0
        || X509_sign_ctx(cert, md) <= 0)
        goto end;
    ok = 1;
end:
    EVP_MD_CTX_free(md);
    if (!ok) {
        X509_free(cert);
        cert = NULL;
    }
    return cert;
}

/* Parse DER into a fresh X509 so verification must invoke the SPKI decoder. */
static X509 *reload_cert(OSSL_LIB_CTX *libctx, X509 *cert)
{
    unsigned char *der = NULL;
    const unsigned char *cursor;
    X509 *copy = X509_new_ex(libctx, NULL);
    int length = i2d_X509(cert, &der);

    cursor = der;
    if (length <= 0 || copy == NULL || d2i_X509(&copy, &cursor, length) == NULL
        || cursor != der + length) {
        X509_free(copy);
        copy = NULL;
    }
    OPENSSL_free(der);
    return copy;
}

static int verify_chain(OSSL_LIB_CTX *libctx, X509 *root, X509 *cert)
{
    X509_STORE *store = X509_STORE_new();
    X509_STORE_CTX *ctx = X509_STORE_CTX_new_ex(libctx, NULL);
    X509_VERIFY_PARAM *param;
    int ok = 0;

    if (store == NULL || ctx == NULL || !X509_STORE_add_cert(store, root)
        || !X509_STORE_CTX_init(ctx, store, cert, NULL))
        goto end;
    param = X509_STORE_CTX_get0_param(ctx);
    X509_VERIFY_PARAM_set_auth_level(param, 2);
    X509_VERIFY_PARAM_set_flags(param, X509_V_FLAG_CHECK_SS_SIGNATURE);
    if (!X509_VERIFY_PARAM_set1_host(param, "localhost", 0)
        || !X509_STORE_CTX_set_purpose(ctx, X509_PURPOSE_SSL_SERVER))
        goto end;
    ok = X509_verify_cert(ctx) == 1;
    if (!ok)
        fprintf(stderr, "verify: %s\n", X509_verify_cert_error_string(X509_STORE_CTX_get_error(ctx)));
end:
    X509_STORE_CTX_free(ctx);
    X509_STORE_free(store);
    return ok;
}

typedef struct {
    unsigned int expected;
    int certificate_verify, bad;
} SIGNATURE_TRACE;

static void trace_sig(int write_p, int version, int type, const void *data,
                            size_t length, SSL *ssl, void *arg)
{
    SIGNATURE_TRACE *trace = arg;
    const unsigned char *p = data;
    unsigned int id, bytes;

    (void)version;
    (void)ssl;
    if (!write_p || type != SSL3_RT_HANDSHAKE || length == 0
        || p[0] != SSL3_MT_CERTIFICATE_VERIFY)
        return;
    if (length < 8) {
        trace->bad = 1;
        return;
    }
    id = ((unsigned int)p[4] << 8) | p[5];
    bytes = ((unsigned int)p[6] << 8) | p[7];
    if (id != trace->expected || bytes != length - 8 || bytes == 0)
        trace->bad = 1;
    ++trace->certificate_verify;
}

static int handshake_step(SSL *ssl)
{
    int ret, error;

    if (SSL_is_init_finished(ssl))
        return 1;
    ret = SSL_do_handshake(ssl);
    if (ret == 1)
        return 1;
    error = SSL_get_error(ssl, ret);
    return error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE ? 0 : -1;
}

static int handshake(OSSL_LIB_CTX *libctx, EVP_PKEY *key, X509 *cert, X509 *root,
                      const char *sigalg, unsigned int code, const char *group)
{
    SSL_CTX *cc = NULL, *sc = NULL;
    SSL *client = NULL, *server = NULL;
    BIO *a = NULL, *b = NULL;
    SIGNATURE_TRACE trace = { code, 0, 0 };
    unsigned char first[32], second[32], received[32];
    size_t length;
    const char *negotiated;
    int ret = 0, cr = 0, sr = 0, i;

    CHECK((cc = SSL_CTX_new_ex(libctx, NULL, TLS_client_method())) != NULL, "client-context");
    CHECK((sc = SSL_CTX_new_ex(libctx, NULL, TLS_server_method())) != NULL, "server-context");
    CHECK(SSL_CTX_set_min_proto_version(cc, TLS1_3_VERSION)
          && SSL_CTX_set_max_proto_version(cc, TLS1_3_VERSION)
          && SSL_CTX_set_min_proto_version(sc, TLS1_3_VERSION)
          && SSL_CTX_set_max_proto_version(sc, TLS1_3_VERSION), "TLS13-only");
    CHECK(SSL_CTX_set1_groups_list(cc, group) && SSL_CTX_set1_groups_list(sc, group), "KEM-group");
    CHECK(SSL_CTX_set1_sigalgs_list(cc, sigalg)
          && SSL_CTX_set1_sigalgs_list(sc, sigalg), "TLS-sig-selection");
    SSL_CTX_set_max_cert_list(cc, 16 * 1024 * 1024);
    SSL_CTX_set_max_cert_list(sc, 16 * 1024 * 1024);
    SSL_CTX_set_verify(cc, SSL_VERIFY_PEER, NULL);
    CHECK(X509_STORE_add_cert(SSL_CTX_get_cert_store(cc), root), "TLS-trust-root");
    CHECK(SSL_CTX_use_certificate(sc, cert) && SSL_CTX_use_PrivateKey(sc, key)
          && SSL_CTX_check_private_key(sc), "TLS-certificate-and-reloaded-key");
    SSL_CTX_set_msg_callback(sc, trace_sig);
    SSL_CTX_set_msg_callback_arg(sc, &trace);
    CHECK((client = SSL_new(cc)) != NULL && (server = SSL_new(sc)) != NULL, "TLS-session");
    CHECK(SSL_set1_host(client, "localhost"), "TLS-hostname");
    CHECK(BIO_new_bio_pair(&a, 0, &b, 0), "TLS-BIO-pair");
    SSL_set_bio(client, a, a);
    a = NULL;
    SSL_set_bio(server, b, b);
    b = NULL;
    SSL_set_connect_state(client);
    SSL_set_accept_state(server);
    for (i = 0; i < 10000 && !(cr == 1 && sr == 1); ++i) {
        cr = handshake_step(client);
        sr = handshake_step(server);
        if (cr < 0 || sr < 0)
            break;
    }
    CHECK(cr == 1 && sr == 1 && SSL_version(client) == TLS1_3_VERSION, "TLS-handshake");
    CHECK(SSL_get_verify_result(client) == X509_V_OK, "TLS-certificate-chain");
    negotiated = SSL_group_to_name(client, SSL_get_negotiated_group(client));
    CHECK(negotiated != NULL && OPENSSL_strcasecmp(negotiated, group) == 0, "TLS-negotiated-group");
    CHECK(trace.certificate_verify == 1 && !trace.bad, "CertificateVerify-wire-id");
    CHECK(SSL_export_keying_material(client, first, 32, "ngcc-test", 8, NULL, 0, 0)
          && SSL_export_keying_material(server, second, 32, "ngcc-test", 8, NULL, 0, 0)
          && CRYPTO_memcmp(first, second, 32) == 0, "TLS-exporter");
    CHECK(SSL_write_ex(client, first, 32, &length) && length == 32
          && SSL_read_ex(server, received, 32, &length) && length == 32
          && CRYPTO_memcmp(first, received, 32) == 0, "TLS-client-data");
    CHECK(SSL_write_ex(server, second, 32, &length) && length == 32
          && SSL_read_ex(client, received, 32, &length) && length == 32
          && CRYPTO_memcmp(second, received, 32) == 0, "TLS-server-data");
    printf("PASS TLS1.3 sigalg=%s code=0x%04X group=%s chain=verified hostname=verified\n",
           sigalg, code, group);
    ret = 1;
end:
    BIO_free(a);
    BIO_free(b);
    SSL_free(client);
    SSL_free(server);
    SSL_CTX_free(cc);
    SSL_CTX_free(sc);
    return ret;
}

static int list_sigalg(const OSSL_PARAM params[], void *arg)
{
    OSSL_LIB_CTX *libctx = arg;
    SSL_CTX *ctx = NULL;
    EVP_KEYMGMT *keymgmt = NULL;
    EVP_SIGNATURE *sig = NULL;
    const char *name, *algorithm, *oid;
    unsigned int code, bits;
    int min_tls, max_tls, min_dtls, max_dtls, ok;
    const OSSL_PARAM *p;

#define GET(field, fn, target)                                                \
    do {                                                                      \
        p = OSSL_PARAM_locate_const(params, field);                           \
        if (p == NULL || !fn(p, &target))                                     \
            return 0;                                                         \
    } while (0)
    GET(OSSL_CAPABILITY_TLS_SIGALG_IANA_NAME, OSSL_PARAM_get_utf8_string_ptr, name);
    GET(OSSL_CAPABILITY_TLS_SIGALG_NAME, OSSL_PARAM_get_utf8_string_ptr, algorithm);
    GET(OSSL_CAPABILITY_TLS_SIGALG_OID, OSSL_PARAM_get_utf8_string_ptr, oid);
    GET(OSSL_CAPABILITY_TLS_SIGALG_CODE_POINT, OSSL_PARAM_get_uint, code);
    GET(OSSL_CAPABILITY_TLS_SIGALG_SECURITY_BITS, OSSL_PARAM_get_uint, bits);
    GET(OSSL_CAPABILITY_TLS_SIGALG_MIN_TLS, OSSL_PARAM_get_int, min_tls);
    GET(OSSL_CAPABILITY_TLS_SIGALG_MAX_TLS, OSSL_PARAM_get_int, max_tls);
    GET(OSSL_CAPABILITY_TLS_SIGALG_MIN_DTLS, OSSL_PARAM_get_int, min_dtls);
    GET(OSSL_CAPABILITY_TLS_SIGALG_MAX_DTLS, OSSL_PARAM_get_int, max_dtls);
#undef GET
    if (min_tls != TLS1_3_VERSION || max_tls != TLS1_3_VERSION
        || min_dtls != -1 || max_dtls != -1)
        return 0;
    keymgmt = EVP_KEYMGMT_fetch(libctx, name, "provider=ngcc");
    sig = EVP_SIGNATURE_fetch(libctx, name, "provider=ngcc");
    ctx = SSL_CTX_new_ex(libctx, NULL, TLS_method());
    ok = strcmp(name, algorithm) == 0 && keymgmt != NULL && sig != NULL
        && ctx != NULL && SSL_CTX_set1_sigalgs_list(ctx, name);
    EVP_KEYMGMT_free(keymgmt);
    EVP_SIGNATURE_free(sig);
    SSL_CTX_free(ctx);
    if (!ok)
        return 0;
    printf("SIGNATURE %s %s %s %u %u\n", name, algorithm, oid, code, bits);
    return 1;
}

int main(int argc, char **argv)
{
    OSSL_LIB_CTX *libctx = NULL;
    OSSL_PROVIDER *def = NULL, *ngcc = NULL;
    EVP_PKEY *ca_key = NULL, *key = NULL, *copy = NULL, *pub = NULL;
    X509 *root = NULL, *cert = NULL, *decoded = NULL;
    X509_REQ *req = NULL;
    EVP_MD_CTX *md = NULL;
    SSL_CTX *sslctx = NULL;
    const char *algorithm, *oid;
    unsigned int code;
    int ret = 1, i;

    setbuf(stdout, NULL);
    if (argc != 3 && argc != 9)
        return 2;
    CHECK((libctx = OSSL_LIB_CTX_new()) != NULL, "libctx");
    CHECK(OSSL_PROVIDER_set_default_search_path(libctx, argv[1]), "provider-path");
    CHECK((def = OSSL_PROVIDER_load(libctx, "default")) != NULL, "default-provider");
    CHECK((ngcc = OSSL_PROVIDER_load(libctx, "ngcc")) != NULL, "ngcc-provider");
    if (argc == 3) {
        CHECK(strcmp(argv[2], "--list") == 0, "list-argument");
        CHECK(OSSL_PROVIDER_get_capabilities(ngcc, "TLS-SIGALG", list_sigalg, libctx), "TLS-SIGALG-list");
        ret = 0;
        goto end;
    }
    algorithm = argv[2];
    oid = argv[3];
    code = (unsigned int)strtoul(argv[4], NULL, 0);
    printf("STEP keygen\n");
    CHECK((key = EVP_PKEY_Q_keygen(libctx, NULL, algorithm)) != NULL, "subject-keygen");
    CHECK(EVP_PKEY_is_a(key, oid), "keymgmt-OID-alias");
    CHECK((ca_key = EVP_PKEY_Q_keygen(libctx, NULL, algorithm)) != NULL, "CA-keygen");
    for (i = 0; i < 2; ++i) {
        const char *format = i == 0 ? "DER" : "PEM";

        CHECK((copy = roundtrip(libctx, key, format, 1, oid)) != NULL, "PKCS8-roundtrip");
        CHECK((pub = roundtrip(libctx, key, format, 0, oid)) != NULL, "SPKI-roundtrip");
        EVP_PKEY_free(key);
        key = copy;
        copy = NULL;
        EVP_PKEY_free(pub);
        pub = NULL;
    }
    if (strcmp(algorithm, "sig-01-aigis-sig1") == 0)
        CHECK(check_legacy_pkcs8(libctx, key, oid), "legacy-and-malformed-PKCS8");
    printf("STEP CSR\n");
    CHECK((req = X509_REQ_new_ex(libctx, NULL)) != NULL, "CSR-allocation");
    CHECK(X509_REQ_set_version(req, 0) && X509_REQ_set_pubkey(req, key), "CSR-public-key");
    CHECK(X509_NAME_add_entry_by_txt(X509_REQ_get_subject_name(req), "CN", MBSTRING_ASC,
                                     (const unsigned char *)"localhost", -1, -1, 0), "CSR-subject");
    CHECK((md = EVP_MD_CTX_new()) != NULL, "CSR-context");
    CHECK(EVP_DigestSignInit_ex(md, NULL, NULL, libctx, NULL, key, NULL) > 0
          && X509_REQ_sign_ctx(req, md) > 0 && X509_REQ_verify(req, key) == 1, "CSR-sign-verify");
    CHECK(X509_REQ_get_signature_nid(req) == OBJ_txt2nid(oid), "CSR-sig-OID");
    printf("STEP certificates\n");
    CHECK((root = issue(libctx, ca_key, NULL, ca_key)) != NULL, "self-signed-CA");
    CHECK((cert = issue(libctx, key, root, ca_key)) != NULL, "CA-issues-leaf");
    CHECK(X509_get_signature_nid(root) == OBJ_txt2nid(oid), "CA-sig-OID");
    CHECK(X509_get_signature_nid(cert) == OBJ_txt2nid(oid), "certificate-sig-OID");
    CHECK((decoded = reload_cert(libctx, cert)) != NULL, "certificate-DER-reload");
    X509_free(cert);
    cert = decoded;
    decoded = NULL;
    CHECK((decoded = reload_cert(libctx, root)) != NULL, "CA-DER-reload");
    X509_free(root);
    root = decoded;
    decoded = NULL;
    CHECK(verify_chain(libctx, root, cert), "certificate-chain-verification");
    printf("PASS CERT %s oid=%s codecs=DER+PEM CSR=verified chain=verified\n", algorithm, oid);
    if (strcmp(argv[6], "-") != 0) {
        BIO *out = BIO_new_file(argv[6], "w");

        CHECK(out != NULL, "example-output");
        i = PEM_write_bio_PrivateKey(out, key, NULL, NULL, 0, NULL, NULL)
            && PEM_write_bio_X509(out, cert) && PEM_write_bio_X509(out, root);
        BIO_free(out);
        CHECK(i, "example-PEM");
    }
    if (strcmp(argv[5], "excluded") == 0) {
        CHECK((sslctx = SSL_CTX_new_ex(libctx, NULL, TLS_method())) != NULL, "excluded-context");
        CHECK(SSL_CTX_set1_sigalgs_list(sslctx, argv[7]) == 0, "oversized-TLS-sig-rejected");
        ERR_clear_error();
        printf("PASS TLS-excluded %s sig-capacity-exceeds-65535\n", algorithm);
    } else {
        CHECK(handshake(libctx, key, cert, root, argv[7], code, "X25519"), "sig-with-X25519");
        if (strcmp(argv[8], "-") != 0)
            CHECK(handshake(libctx, key, cert, root, argv[7], code, argv[8]), "sig-with-NGCC-KEM");
        else
            printf("SKIP sig-with-NGCC-KEM: no enabled group allowed by runtime policy\n");
    }
    ret = 0;
end:
    if (ret != 0)
        ERR_print_errors_fp(stderr);
    SSL_CTX_free(sslctx);
    EVP_MD_CTX_free(md);
    X509_REQ_free(req);
    X509_free(decoded);
    X509_free(cert);
    X509_free(root);
    EVP_PKEY_free(pub);
    EVP_PKEY_free(copy);
    EVP_PKEY_free(key);
    EVP_PKEY_free(ca_key);
    OSSL_PROVIDER_unload(ngcc);
    OSSL_PROVIDER_unload(def);
    OSSL_LIB_CTX_free(libctx);
    return ret;
}
