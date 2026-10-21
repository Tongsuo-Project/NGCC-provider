# Using NGCC-provider

The provider load name is `ngcc`; algorithm properties are `provider=ngcc`.

Look up the algorithm name you want to test in the "call name" column of the [proposal summary](NGCC_PROPOSALS_SUMMARY.md): the `EVP` name is used in C tests and as the command-line `-algorithm`, `TLS group` is used with command-line `-groups`, and `TLS sigalg` is used with command-line `-sigalgs`.

The examples below use `kem-41-zen-128` and `sig-01-aigis-sig2`; substitute names from the table as needed.

## 1. Loading the provider and querying algorithms from the command line

Run the following commands in the directory that contains `ngcc.so`.

```sh
# Load the provider and inspect the load result.
openssl list -providers \
  -provider-path . -provider default -provider ngcc

# List the KEM and signature algorithms provided by ngcc.
openssl list -kem-algorithms -signature-algorithms \
  -provider-path . -provider default -provider ngcc \
  -propquery 'provider=ngcc'
```



## 2. Command-line test: sign and verify

```sh
# Restrict new-file permissions and prepare the message to be signed.
umask 077
printf 'hello ngcc' > message.txt

# Generate a signature private key.
openssl genpkey -algorithm sig-01-aigis-sig2 -out sig.key \
  -provider-path . -provider default -provider ngcc

# Export the public key from the private key for the verifier.
openssl pkey -in sig.key -pubout -out sig.pub \
  -provider-path . -provider default -provider ngcc

# Sign the raw message and write the result to message.sig.
openssl pkeyutl -sign -rawin \
  -inkey sig.key -in message.txt -out message.sig \
  -provider-path . -provider default -provider ngcc

# Verify the message and signature with the public key.
openssl pkeyutl -verify -rawin -pubin \
  -inkey sig.pub -in message.txt -sigfile message.sig \
  -provider-path . -provider default -provider ngcc
```

The sign command does not take `-digest`. Current PEM/DER encoding is used for signature keys; standalone KEM encapsulate/decapsulate uses the EVP API above, and command-line TLS selects KEMs via `-groups`.

## 3. Command-line test: self-signed CA and server certificate

Generate a separate CA key and a self-signed root certificate:

```sh
# Generate a CA private key and self-signed root certificate, allowing certificate and CRL issuance.
openssl req -config /dev/null -new -x509 -newkey sig-01-aigis-sig2 -noenc \
  -keyout ca.key -out ca.pem -subj '/CN=NGCC Root CA' -days 365 \
  -addext 'basicConstraints=critical,CA:TRUE' \
  -addext 'keyUsage=critical,keyCertSign,cRLSign' \
  -provider-path . -provider default -provider ngcc
```

Create a server CSR with `sig.key` from section 2, then have the CA issue the certificate:

```sh
# Generate a certificate signing request with the server signature private key.
openssl req -config /dev/null -new -key sig.key \
  -out server.csr -subj '/CN=localhost' \
  -provider-path . -provider default -provider ngcc

# Set the server certificate usage and the localhost hostname.
cat > server.ext <<'EOF'
basicConstraints=critical,CA:FALSE
keyUsage=critical,digitalSignature
extendedKeyUsage=serverAuth
subjectAltName=DNS:localhost
EOF

# Issue the server certificate with the CA private key.
openssl x509 -req -in server.csr -CA ca.pem -CAkey ca.key \
  -set_serial 2 -days 365 -extfile server.ext -out server.pem \
  -provider-path . -provider default -provider ngcc

# Verify the certificate chain, root self-signature, server usage, and hostname.
openssl verify \
  -provider-path . -provider default -provider ngcc \
  -CAfile ca.pem -check_ss_sig -purpose sslserver \
  -verify_hostname localhost server.pem
```

## 4. Command-line test: TLS 1.3 handshake

Server terminal:

```sh
# Start a TLS 1.3 server using the specified KEM for key exchange and the specified signature algorithm for authentication.
openssl s_server -accept 127.0.0.1:8443 -tls1_3 -www \
  -cert server.pem -key sig.key -groups kem-41-zen-128 -sigalgs sig-01-aigis-sig2 \
  -provider-path . -provider default -provider ngcc
```

Client terminal:

```sh
# Connect to the server and check the certificate chain and hostname; abort the handshake on verification failure.
openssl s_client -connect 127.0.0.1:8443 -servername localhost \
  -tls1_3 -CAfile ca.pem -verify_return_error -verify_hostname localhost \
  -groups kem-41-zen-128 -sigalgs sig-01-aigis-sig2 \
  -provider-path . -provider default -provider ngcc
```

`-groups` selects the ephemeral key-exchange KEM; `-sigalgs` selects the authentication signature algorithm. Both ends load `ngcc`; only select parameter sets that have a registered TLS capability.

## 5. C tests: KEM and signatures

Use `<openssl/provider.h>`, `<openssl/evp.h>`, and `<openssl/crypto.h>`, and link `-lcrypto`.

Load the provider:

```c
OSSL_LIB_CTX *libctx = OSSL_LIB_CTX_new();

/* Set the directory that contains ngcc.so and load the required providers. */
OSSL_PROVIDER_set_default_search_path(libctx, "/path/to/NGCC-provider");
OSSL_PROVIDER *def = OSSL_PROVIDER_load(libctx, "default");
OSSL_PROVIDER *ngcc = OSSL_PROVIDER_load(libctx, "ngcc");
```

KEM key generation, encapsulation, and decapsulation:

```c
/* Generate the recipient's KEM key pair. */
EVP_PKEY *key = EVP_PKEY_Q_keygen(libctx, "provider=ngcc", "kem-41-zen-128");
EVP_PKEY_CTX *enc = EVP_PKEY_CTX_new_from_pkey(libctx, key, "provider=ngcc");
EVP_PKEY_CTX *dec = EVP_PKEY_CTX_new_from_pkey(libctx, key, "provider=ngcc");
size_t ctlen, sslen, recovered_len;

EVP_PKEY_encapsulate_init(enc, NULL);
/* Query the buffer sizes required for the ciphertext and shared secret. */
EVP_PKEY_encapsulate(enc, NULL, &ctlen, NULL, &sslen);
unsigned char *ct = OPENSSL_malloc(ctlen);
unsigned char *ss = OPENSSL_malloc(sslen);
unsigned char *recovered = OPENSSL_malloc(sslen);
recovered_len = sslen;

/* Encapsulate with the public key to obtain ciphertext ct and shared secret ss. */
EVP_PKEY_encapsulate(enc, ct, &ctlen, ss, &sslen);
/* Recover the shared secret from the ciphertext with the private key and compare both sides. */
EVP_PKEY_decapsulate_init(dec, NULL);
EVP_PKEY_decapsulate(dec, recovered, &recovered_len, ct, ctlen);
int ok = sslen == recovered_len && CRYPTO_memcmp(ss, recovered, sslen) == 0;
```

The encapsulating party needs only the recipient's public key; the decapsulating party holds the private key. Transmit the public key by exporting with `EVP_PKEY_get_raw_public_key()` and importing with `EVP_PKEY_new_raw_public_key_ex()`. The shared secrets obtained on both sides are not sent on the network.

Sign and verify:

```c
/* Generate a signature key pair. */
EVP_PKEY *key = EVP_PKEY_Q_keygen(libctx, "provider=ngcc", "sig-01-aigis-sig2");
EVP_MD_CTX *sign_ctx = EVP_MD_CTX_new();
EVP_MD_CTX *verify_ctx = EVP_MD_CTX_new();
const unsigned char msg[] = "hello ngcc";
size_t siglen;

/* Initialize signing; pass NULL for the digest name so the scheme handles the message. */
EVP_DigestSignInit_ex(sign_ctx, NULL, NULL, libctx, "provider=ngcc", key, NULL);
/* Query the signature buffer size, then sign the message. */
EVP_DigestSign(sign_ctx, NULL, &siglen, msg, sizeof(msg) - 1);
unsigned char *sig = OPENSSL_malloc(siglen);
EVP_DigestSign(sign_ctx, sig, &siglen, msg, sizeof(msg) - 1);

/* Verify with the public key; a return value of 1 means verification succeeded. */
EVP_DigestVerifyInit_ex(verify_ctx, NULL, NULL, libctx, "provider=ngcc", key, NULL);
int ok = EVP_DigestVerify(verify_ctx, sig, siglen, msg, sizeof(msg) - 1) == 1;
```

Verification needs only the public key. Streaming calls use `EVP_DigestSignUpdate()` / `EVP_DigestSignFinal()`, and the matching verify calls are `EVP_DigestVerifyUpdate()` / `EVP_DigestVerifyFinal()`.

## 6. C tests: TLS 1.3 handshake

Use `<openssl/ssl.h>` and link `-lssl -lcrypto`. `libctx` already has the provider loaded, and `fd` is a connected socket:

```c
SSL_CTX *ctx = SSL_CTX_new_ex(libctx, NULL, TLS_client_method());

/* Allow TLS 1.3 only, and select the key-exchange KEM and authentication signature algorithm. */
SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION);
SSL_CTX_set_max_proto_version(ctx, TLS1_3_VERSION);
SSL_CTX_set1_groups_list(ctx, "kem-41-zen-128");
SSL_CTX_set1_sigalgs_list(ctx, "sig-01-aigis-sig2");
/* Load the trusted CA and require verification of the server certificate. */
SSL_CTX_load_verify_locations(ctx, "ca.pem", NULL);
SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

SSL *ssl = SSL_new(ctx);
SSL_set_fd(ssl, fd);
/* SNI names the target site; set1_host sets certificate hostname checking. */
SSL_set_tlsext_host_name(ssl, "localhost");
SSL_set1_host(ssl, "localhost");
/* Start the handshake; a return value of 1 means success. */
int ok = SSL_connect(ssl) == 1;
```

On the server, use `TLS_server_method()` and `SSL_accept()`, and load the certificate and private key before the handshake:

```c
/* Load the server certificate chain and private key, and check that they match. */
SSL_CTX_use_certificate_chain_file(ctx, "server.pem");
SSL_CTX_use_PrivateKey_file(ctx, "sig.key", SSL_FILETYPE_PEM);
SSL_CTX_check_private_key(ctx);
```

Order certificates in the chain file as leaf certificate then intermediate CA. After the handshake, send and receive with `SSL_write_ex()` / `SSL_read_ex()`, and close the connection with `SSL_shutdown()`.
