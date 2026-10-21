# ngcc_provider 使用方法

Provider 加载名为 `ngcc`，算法属性为 `provider=ngcc`。

先在[方案汇总](NGCC_PROPOSALS_SUMMARY.md)的“调用名称：EVP 与 TLS”列查找实际名称：`EVP` 名称用于 C API 和 `-algorithm`，`TLS group` 用于 `-groups`，`TLS sigalg` 用于 `-sigalgs`。标为“未注册”的参数集不能用于相应 TLS 操作；还需确认该实例已在构建时启用。宿主支持范围和测试状态也见该表。

以下示例使用 `kem-zen-128` 和 `sig-01-aigis-sig2`，可按表中名称替换。

## 1. 命令行加载和算法查询

以下命令在 `ngcc.so` 所在目录执行，`openssl` 使用目标 OpenSSL/Tongsuo 的命令行程序。

```sh
# 加载 provider，查看加载结果。
openssl list -providers \
  -provider-path . -provider default -provider ngcc

# 列出 ngcc 提供的 KEM 和签名算法。
openssl list -kem-algorithms -signature-algorithms \
  -provider-path . -provider default -provider ngcc \
  -propquery 'provider=ngcc'
```

## 2. C：KEM 和签名

以下仅列关键调用，省略错误处理和资源释放。使用 `<openssl/provider.h>`、`<openssl/evp.h>` 和 `<openssl/crypto.h>`，链接 `-lcrypto`。

加载 provider：

```c
OSSL_LIB_CTX *libctx = OSSL_LIB_CTX_new();

/* 设置 ngcc.so 所在目录，加载所需 provider。 */
OSSL_PROVIDER_set_default_search_path(libctx, "/path/to/ngcc_provider");
OSSL_PROVIDER *def = OSSL_PROVIDER_load(libctx, "default");
OSSL_PROVIDER *ngcc = OSSL_PROVIDER_load(libctx, "ngcc");
```

KEM 密钥生成、封装和解封装：

```c
/* 生成接收方的 KEM 密钥对。 */
EVP_PKEY *key = EVP_PKEY_Q_keygen(libctx, "provider=ngcc", "kem-zen-128");
EVP_PKEY_CTX *enc = EVP_PKEY_CTX_new_from_pkey(libctx, key, "provider=ngcc");
EVP_PKEY_CTX *dec = EVP_PKEY_CTX_new_from_pkey(libctx, key, "provider=ngcc");
size_t ctlen, sslen, recovered_len;

EVP_PKEY_encapsulate_init(enc, NULL);
/* 查询密文和共享秘密所需的缓冲区大小。 */
EVP_PKEY_encapsulate(enc, NULL, &ctlen, NULL, &sslen);
unsigned char *ct = OPENSSL_malloc(ctlen);
unsigned char *ss = OPENSSL_malloc(sslen);
unsigned char *recovered = OPENSSL_malloc(sslen);
recovered_len = sslen;

/* 使用公钥封装，得到密文 ct 和共享秘密 ss。 */
EVP_PKEY_encapsulate(enc, ct, &ctlen, ss, &sslen);
/* 使用私钥从密文恢复共享秘密，并核对两端结果。 */
EVP_PKEY_decapsulate_init(dec, NULL);
EVP_PKEY_decapsulate(dec, recovered, &recovered_len, ct, ctlen);
int ok = sslen == recovered_len && CRYPTO_memcmp(ss, recovered, sslen) == 0;
```

封装方只需接收方公钥，解封装方持有私钥。公钥传输使用 `EVP_PKEY_get_raw_public_key()` 导出、`EVP_PKEY_new_raw_public_key_ex()` 导入；两端得到的共享秘密不在网络上传输。

签名和验签：

```c
/* 生成签名密钥对。 */
EVP_PKEY *key = EVP_PKEY_Q_keygen(libctx, "provider=ngcc", "sig-01-aigis-sig2");
EVP_MD_CTX *sign_ctx = EVP_MD_CTX_new();
EVP_MD_CTX *verify_ctx = EVP_MD_CTX_new();
const unsigned char msg[] = "hello ngcc";
size_t siglen;

/* 初始化签名；摘要名称传 NULL，由方案处理消息。 */
EVP_DigestSignInit_ex(sign_ctx, NULL, NULL, libctx, "provider=ngcc", key, NULL);
/* 查询签名缓冲区大小，再对消息签名。 */
EVP_DigestSign(sign_ctx, NULL, &siglen, msg, sizeof(msg) - 1);
unsigned char *sig = OPENSSL_malloc(siglen);
EVP_DigestSign(sign_ctx, sig, &siglen, msg, sizeof(msg) - 1);

/* 使用公钥验签，返回 1 表示验证成功。 */
EVP_DigestVerifyInit_ex(verify_ctx, NULL, NULL, libctx, "provider=ngcc", key, NULL);
int ok = EVP_DigestVerify(verify_ctx, sig, siglen, msg, sizeof(msg) - 1) == 1;
```

验签只需公钥。流式调用使用 `EVP_DigestSignUpdate()` / `EVP_DigestSignFinal()`，验签对应 `EVP_DigestVerifyUpdate()` / `EVP_DigestVerifyFinal()`。

## 3. 命令行：签名和验签

```sh
# 限制新文件权限，并准备待签名消息。
umask 077
printf 'hello ngcc' > message.txt

# 生成签名私钥。
openssl genpkey -algorithm sig-01-aigis-sig2 -out sig.key \
  -provider-path . -provider default -provider ngcc

# 从私钥导出公钥，供验签方使用。
openssl pkey -in sig.key -pubout -out sig.pub \
  -provider-path . -provider default -provider ngcc

# 对原始消息签名，结果写入 message.sig。
openssl pkeyutl -sign -rawin \
  -inkey sig.key -in message.txt -out message.sig \
  -provider-path . -provider default -provider ngcc

# 使用公钥验证消息及签名。
openssl pkeyutl -verify -rawin -pubin \
  -inkey sig.pub -in message.txt -sigfile message.sig \
  -provider-path . -provider default -provider ngcc
```

签名命令不传 `-digest`。当前 PEM/DER 编解码用于签名密钥；KEM 的独立封装/解封装使用上面的 EVP API，命令行 TLS 通过 `-groups` 选择 KEM。

## 4. 命令行：自签名 CA 和服务端证书

生成独立的 CA 密钥和自签名根证书：

```sh
# 生成 CA 私钥和自签名根证书，允许签发证书及 CRL。
openssl req -config /dev/null -new -x509 -newkey sig-01-aigis-sig2 -noenc \
  -keyout ca.key -out ca.pem -subj '/CN=NGCC Root CA' -days 365 \
  -addext 'basicConstraints=critical,CA:TRUE' \
  -addext 'keyUsage=critical,keyCertSign,cRLSign' \
  -provider-path . -provider default -provider ngcc
```

使用第 3 节的 `sig.key` 创建服务端 CSR，再由 CA 签发：

```sh
# 使用服务端签名私钥生成证书签名请求。
openssl req -config /dev/null -new -key sig.key \
  -out server.csr -subj '/CN=localhost' \
  -provider-path . -provider default -provider ngcc

# 设置服务端证书用途和 localhost 主机名。
cat > server.ext <<'EOF'
basicConstraints=critical,CA:FALSE
keyUsage=critical,digitalSignature
extendedKeyUsage=serverAuth
subjectAltName=DNS:localhost
EOF

# 用 CA 私钥签发服务端证书。
openssl x509 -req -in server.csr -CA ca.pem -CAkey ca.key \
  -set_serial 2 -days 365 -extfile server.ext -out server.pem \
  -provider-path . -provider default -provider ngcc

# 验证证书链、根证书自签名、服务端用途及主机名。
openssl verify \
  -provider-path . -provider default -provider ngcc \
  -CAfile ca.pem -check_ss_sig -purpose sslserver \
  -verify_hostname localhost server.pem
```

## 5. 命令行：TLS 1.3

服务端终端：

```sh
# 启动 TLS 1.3 服务端，使用指定 KEM 交换密钥、指定签名算法认证。
openssl s_server -accept 127.0.0.1:8443 -tls1_3 -www \
  -cert server.pem -key sig.key -groups kem-zen-128 -sigalgs sig-01-aigis-sig2 \
  -provider-path . -provider default -provider ngcc
```

客户端终端：

```sh
# 连接服务端，校验证书链及主机名；验证失败则终止握手。
openssl s_client -connect 127.0.0.1:8443 -servername localhost \
  -tls1_3 -CAfile ca.pem -verify_return_error -verify_hostname localhost \
  -groups kem-zen-128 -sigalgs sig-01-aigis-sig2 \
  -provider-path . -provider default -provider ngcc
```

`-groups` 指定临时密钥交换 KEM，`-sigalgs` 指定认证签名算法。两端都加载 `ngcc`；仅选择已注册 TLS capability 的参数集。

## 6. C：TLS 1.3

使用 `<openssl/ssl.h>`，链接 `-lssl -lcrypto`。`libctx` 已加载 provider，`fd` 是已连接的 socket：

```c
SSL_CTX *ctx = SSL_CTX_new_ex(libctx, NULL, TLS_client_method());

/* 仅允许 TLS 1.3，选择密钥交换 KEM 和认证签名算法。 */
SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION);
SSL_CTX_set_max_proto_version(ctx, TLS1_3_VERSION);
SSL_CTX_set1_groups_list(ctx, "kem-zen-128");
SSL_CTX_set1_sigalgs_list(ctx, "sig-01-aigis-sig2");
/* 加载信任的 CA，并要求验证服务端证书。 */
SSL_CTX_load_verify_locations(ctx, "ca.pem", NULL);
SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

SSL *ssl = SSL_new(ctx);
SSL_set_fd(ssl, fd);
/* SNI 指定目标站点，set1_host 设置证书主机名校验。 */
SSL_set_tlsext_host_name(ssl, "localhost");
SSL_set1_host(ssl, "localhost");
/* 发起握手，返回 1 表示成功。 */
int ok = SSL_connect(ssl) == 1;
```

服务端改用 `TLS_server_method()` 和 `SSL_accept()`，在握手前加载证书及私钥：

```c
/* 加载服务端证书链及私钥，并检查两者是否匹配。 */
SSL_CTX_use_certificate_chain_file(ctx, "server.pem");
SSL_CTX_use_PrivateKey_file(ctx, "sig.key", SSL_FILETYPE_PEM);
SSL_CTX_check_private_key(ctx);
```

证书链文件按叶子证书、中间 CA 的顺序排列。握手后使用 `SSL_write_ex()` / `SSL_read_ex()` 收发数据，使用 `SSL_shutdown()` 关闭连接。
