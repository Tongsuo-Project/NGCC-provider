/* Exercise real libssl TLS handshakes over a BIO pair, without TCP ports.
 * The server certificate is classical ECDSA; the negotiated group alone is
 * the tested NGCC KEM. Wire callbacks check the actual key_share ID and size. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/provider.h>
#include <openssl/core_names.h>
#include <openssl/err.h>
#include <openssl/x509.h>

#define REQUIRE(c, stage)                                                     \
    do {                                                                      \
        if (!(c)) {                                                           \
            fprintf(stderr, "FAIL %s\n", stage);                              \
            goto end;                                                         \
        }                                                                     \
    } while (0)
typedef struct {
    unsigned int id;
    size_t pklen, ctlen;
    int client_shares, server_shares, retries, bad;
} TRACE;

static unsigned int u16(const unsigned char *p)
{
    return ((unsigned int)p[0] << 8) | p[1];
}

/* Read complete handshake messages supplied by libssl, including their header. */
static void trace_message(int write_p, int version, int type, const void *data,
                          size_t length, SSL *ssl, void *arg)
{
    TRACE *t = arg;
    const unsigned char *p = data;
    size_t pos, end, n;
    int client;
    (void)version;
    (void)ssl;
    if (!write_p || type != SSL3_RT_HANDSHAKE || length < 39)
        return;
    client = p[0] == SSL3_MT_CLIENT_HELLO;
    if (!client && p[0] != SSL3_MT_SERVER_HELLO)
        return;
    pos = 38;
    n = p[pos++];
    if (pos + n + 3 > length) {
        t->bad = 1;
        return;
    }
    pos += n;
    if (client) {
        n = u16(p + pos);
        pos += 2;
        if (pos + n + 1 > length) {
            t->bad = 1;
            return;
        }
        pos += n;
        n = p[pos++];
        if (pos + n + 2 > length) {
            t->bad = 1;
            return;
        }
        pos += n;
    } else {
        pos += 3; /* cipher suite and compression */
    }
    if (pos + 2 > length) {
        t->bad = 1;
        return;
    }
    end = pos + 2 + u16(p + pos);
    pos += 2;
    if (end != length) {
        t->bad = 1;
        return;
    }
    while (pos + 4 <= end) {
        unsigned int ext = u16(p + pos);
        n = u16(p + pos + 2);
        pos += 4;
        if (pos + n > end) {
            t->bad = 1;
            return;
        }
        if (ext == TLSEXT_TYPE_key_share) {
            size_t off = pos, limit = pos + n;
            if (!client && n == 2) {
                if (u16(p + pos) == t->id)
                    ++t->retries;
                else
                    t->bad = 1;
            } else {
                if (client) {
                    if (n < 2 || u16(p + off) != n - 2) {
                        t->bad = 1;
                        return;
                    }
                    off += 2;
                }
                while (off + 4 <= limit) {
                    unsigned int id = u16(p + off);
                    size_t bytes = u16(p + off + 2);
                    off += 4;
                    if (off + bytes > limit) {
                        t->bad = 1;
                        return;
                    }
                    if (id == t->id) {
                        if (bytes != (client ? t->pklen : t->ctlen))
                            t->bad = 1;
                        if (client)
                            ++t->client_shares;
                        else
                            ++t->server_shares;
                    }
                    off += bytes;
                }
                if (off != limit)
                    t->bad = 1;
            }
        }
        pos += n;
    }
}

static int list_group(const OSSL_PARAM params[], void *arg)
{
    OSSL_LIB_CTX *libctx = arg;
    SSL_CTX *ctx = NULL;
    EVP_KEYMGMT *keymgmt = NULL;
    EVP_KEM *kem = NULL;
    const OSSL_PARAM *p;
    const char *name, *algorithm;
    unsigned int id, bits, iskem;
    int min, max, dtls, ok;
#define GET(field, fn, out)                                                   \
    do {                                                                      \
        p = OSSL_PARAM_locate_const(params, field);                           \
        if (p == NULL || !fn(p, &out))                                        \
            return 0;                                                         \
    } while (0)
    GET(OSSL_CAPABILITY_TLS_GROUP_NAME, OSSL_PARAM_get_utf8_string_ptr, name);
    GET(OSSL_CAPABILITY_TLS_GROUP_ALG, OSSL_PARAM_get_utf8_string_ptr, algorithm);
    GET(OSSL_CAPABILITY_TLS_GROUP_ID, OSSL_PARAM_get_uint, id);
    GET(OSSL_CAPABILITY_TLS_GROUP_SECURITY_BITS, OSSL_PARAM_get_uint, bits);
    GET(OSSL_CAPABILITY_TLS_GROUP_IS_KEM, OSSL_PARAM_get_uint, iskem);
    GET(OSSL_CAPABILITY_TLS_GROUP_MIN_TLS, OSSL_PARAM_get_int, min);
    GET(OSSL_CAPABILITY_TLS_GROUP_MAX_TLS, OSSL_PARAM_get_int, max);
    GET(OSSL_CAPABILITY_TLS_GROUP_MIN_DTLS, OSSL_PARAM_get_int, dtls);
#undef GET
    if (iskem != 1 || min != TLS1_3_VERSION || max != TLS1_3_VERSION || dtls != -1)
        return 0;
    keymgmt = EVP_KEYMGMT_fetch(libctx, name, "provider=ngcc");
    kem = EVP_KEM_fetch(libctx, name, "provider=ngcc");
    ctx = SSL_CTX_new_ex(libctx, NULL, TLS_method());
    ok = strcmp(name, algorithm) == 0 && keymgmt != NULL && kem != NULL
        && ctx != NULL && SSL_CTX_set1_groups_list(ctx, name);
    EVP_KEYMGMT_free(keymgmt);
    EVP_KEM_free(kem);
    SSL_CTX_free(ctx);
    if (!ok)
        return 0;
    printf("GROUP %s %s %u %u\n",name,algorithm,id,bits);
    return 1;
}

static X509 *certificate(OSSL_LIB_CTX *lib, EVP_PKEY **key)
{
    X509 *cert = NULL;
    X509_NAME *name;
    *key = EVP_PKEY_Q_keygen(lib, NULL, "EC", "P-256");
    if (*key == NULL || (cert = X509_new_ex(lib, NULL)) == NULL)
        return NULL;
    X509_set_version(cert, 2);
    ASN1_INTEGER_set(X509_get_serialNumber(cert), 1);
    X509_gmtime_adj(X509_getm_notBefore(cert), -60);
    X509_gmtime_adj(X509_getm_notAfter(cert), 3600);
    name = X509_get_subject_name(cert);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                              (const unsigned char *)"NGCC TLS test", -1, -1, 0);
    if (!X509_set_issuer_name(cert, name) || !X509_set_pubkey(cert, *key)
        || !X509_sign(cert, *key, EVP_sha256())) {
        X509_free(cert);
        cert = NULL;
    }
    return cert;
}

/* Return 1 when done, 0 when another peer turn is needed, -1 on failure. */
static int step(SSL *ssl)
{
    int rc, error;
    if (SSL_is_init_finished(ssl))
        return 1;
    rc = SSL_do_handshake(ssl);
    if (rc == 1)
        return 1;
    error = SSL_get_error(ssl, rc);
    return error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE ? 0 : -1;
}

int main(int argc, char **argv)
{
    OSSL_LIB_CTX *lib = NULL;
    OSSL_PROVIDER *def = NULL, *ngcc = NULL;
    SSL_CTX *clientctx = NULL, *serverctx = NULL;
    SSL *client = NULL, *server = NULL;
    BIO *a = NULL, *b = NULL;
    EVP_PKEY *certkey = NULL;
    X509 *cert = NULL;
    TRACE trace = {0};
    unsigned char first[32], second[32], received[32];
    const char *group, *mode, *negotiated;
    char clientgroups[512];
    size_t bytes;
    int ret = 1, cs = 0, ss = 0, failed = 0;
    setbuf(stdout, NULL);
    if (argc != 3 && argc != 7)
        return 2;
    REQUIRE((lib = OSSL_LIB_CTX_new()), "libctx");
    REQUIRE(OSSL_PROVIDER_set_default_search_path(lib, argv[1]), "provider-path");
    REQUIRE((def = OSSL_PROVIDER_load(lib, "default")), "default-provider");
    REQUIRE((ngcc = OSSL_PROVIDER_load(lib, "ngcc")), "ngcc-provider");
    if (argc == 3) {
        REQUIRE(!strcmp(argv[2], "--list"), "list-argument");
        REQUIRE(OSSL_PROVIDER_get_capabilities(ngcc, "TLS-GROUP", list_group, lib), "capabilities");
        ret = 0;
        goto end;
    }
    group = argv[2];
    mode = argv[3];
    trace.id = (unsigned int)strtoul(argv[4], NULL, 0);
    trace.pklen = strtoul(argv[5], NULL, 0);
    trace.ctlen = strtoul(argv[6], NULL, 0);
    REQUIRE((clientctx = SSL_CTX_new_ex(lib, NULL, TLS_client_method())), "client-ctx");
    REQUIRE((serverctx = SSL_CTX_new_ex(lib, NULL, TLS_server_method())), "server-ctx");
    if (!strcmp(mode, "excluded")) {
        REQUIRE(SSL_CTX_set1_groups_list(clientctx, group) == 0, "excluded-group-rejected");
        ERR_clear_error();
        ret = 0;
        goto end;
    }
    REQUIRE((cert = certificate(lib, &certkey)), "certificate");
    REQUIRE(SSL_CTX_use_certificate(serverctx, cert) && SSL_CTX_use_PrivateKey(serverctx, certkey), "server-identity");
    REQUIRE(X509_STORE_add_cert(SSL_CTX_get_cert_store(clientctx), cert), "trust-certificate");
    SSL_CTX_set_verify(clientctx, SSL_VERIFY_PEER, NULL);
    SSL_CTX_set_options(serverctx, SSL_OP_NO_TICKET);
    REQUIRE(SSL_CTX_set_ciphersuites(clientctx, "TLS_AES_128_GCM_SHA256")
            && SSL_CTX_set_ciphersuites(serverctx, "TLS_AES_128_GCM_SHA256"), "ciphersuite");
    REQUIRE(SSL_CTX_set1_groups_list(serverctx, group), "server-group");
    snprintf(clientgroups, sizeof(clientgroups), !strcmp(mode,"hrr") ? "*X25519:%s" : "%s", group);
    REQUIRE(SSL_CTX_set1_groups_list(clientctx, clientgroups), "client-groups");
    {
        int version = !strcmp(mode,"tls12") ? TLS1_2_VERSION : TLS1_3_VERSION;
        REQUIRE(SSL_CTX_set_min_proto_version(clientctx, version) && SSL_CTX_set_max_proto_version(clientctx, version)
             && SSL_CTX_set_min_proto_version(serverctx, version) && SSL_CTX_set_max_proto_version(serverctx, version), "protocol");
    }
    SSL_CTX_set_msg_callback(clientctx, trace_message);
    SSL_CTX_set_msg_callback_arg(clientctx, &trace);
    SSL_CTX_set_msg_callback(serverctx, trace_message);
    SSL_CTX_set_msg_callback_arg(serverctx, &trace);
    REQUIRE((client = SSL_new(clientctx)) && (server = SSL_new(serverctx)), "ssl");
    REQUIRE(BIO_new_bio_pair(&a, 0, &b, 0), "bio-pair");
    SSL_set_bio(client, a, a);
    a = NULL;
    SSL_set_bio(server, b, b);
    b = NULL;
    SSL_set_connect_state(client);
    SSL_set_accept_state(server);
    for (int i = 0; i < 10000 && !(cs && ss); ++i) {
        cs = step(client);
        ss = step(server);
        if (cs < 0 || ss < 0) {
            failed = 1;
            break;
        }
    }
    if (!strcmp(mode, "tls12")) {
        REQUIRE(failed, "TLS12-KEM-must-not-negotiate");
        ERR_clear_error();
        ret = 0;
        goto end;
    }
    REQUIRE(!failed && cs == 1 && ss == 1, "TLS-handshake");
    REQUIRE(SSL_version(client) == TLS1_3_VERSION && SSL_version(server) == TLS1_3_VERSION, "TLS13");
    REQUIRE(SSL_get_verify_result(client) == X509_V_OK, "certificate-verification");
    negotiated = SSL_group_to_name(client, SSL_get_negotiated_group(client));
    REQUIRE(negotiated && !strcmp(negotiated, group), "negotiated-client-group");
    negotiated = SSL_group_to_name(server, SSL_get_negotiated_group(server));
    REQUIRE(negotiated && !strcmp(negotiated, group), "negotiated-server-group");
    REQUIRE(!trace.bad && trace.client_shares == 1 && trace.server_shares == 1, "wire-key-share");
    REQUIRE(trace.retries == (!strcmp(mode,"hrr") ? 1 : 0), "hello-retry-request");
    REQUIRE(SSL_export_keying_material(client, first, sizeof(first), "ngcc-test", 8, NULL, 0, 0)
         && SSL_export_keying_material(server, second, sizeof(second), "ngcc-test", 8, NULL, 0, 0)
         && !CRYPTO_memcmp(first, second, sizeof(first)), "exporter-match");
    REQUIRE(SSL_write_ex(client, first, sizeof(first), &bytes) && bytes == sizeof(first), "client-write");
    REQUIRE(SSL_read_ex(server, received, sizeof(received), &bytes) && bytes == sizeof(first)
         && !CRYPTO_memcmp(first, received, bytes), "server-read");
    REQUIRE(SSL_write_ex(server, second, sizeof(second), &bytes) && bytes == sizeof(second), "server-write");
    REQUIRE(SSL_read_ex(client, received, sizeof(received), &bytes) && bytes == sizeof(second)
         && !CRYPTO_memcmp(second, received, bytes), "client-read");
    printf("PASS TLS1.3 group=%s wire_id=0x%04X pk=%zu ct=%zu hrr=%d authenticated=1 exporter=match data=bidirectional\n",
           group, trace.id, trace.pklen, trace.ctlen, trace.retries);
    ret = 0;
end:
    if (ret)
        ERR_print_errors_fp(stderr);
    BIO_free(a);
    BIO_free(b);
    SSL_free(client);
    SSL_free(server);
    SSL_CTX_free(clientctx);
    SSL_CTX_free(serverctx);
    X509_free(cert);
    EVP_PKEY_free(certkey);
    OSSL_PROVIDER_unload(ngcc);
    OSSL_PROVIDER_unload(def);
    OSSL_LIB_CTX_free(lib);
    return ret;
}
