# ngcc_provider

`ngcc_provider` 将 NGCC 提案的参考实现接入 OpenSSL provider 接口，提供 KEM、签名、签名密钥编解码及 TLS 1.3 能力。提案算法源码编译到一个 `ngcc.so` 中，provider 加载名为 `ngcc`。

## 免责声明

**本仓库仅用于测试，不能用于实际生产环境。**

方案代码均来自 NGCC 提案团队提交的实现。本仓库仅进行接口适配，不对方案的密码学安全性或实现安全性作任何保证，也不对方案安全性承担责任。测试通过不代表方案安全，已知安全问题也不作为本仓库的统一排除条件。

## 文档

- [方案汇总](NGCC_PROPOSALS_SUMMARY.md)：已接入及未接入的参数集、EVP/TLS 调用名称、源码来源、测试结果和限制。
- [使用说明](USAGE.md)：C API 和命令行示例，包括 KEM、签名验签、证书签发和 TLS 1.3。

## 构建

构建需要 Perl、make、C 工具链和 Tongsuo/OpenSSL 开发文件。

在仓库根目录构建：

```sh
./config
make -j6
```

`OPENSSL_PREFIX` 指定 OpenSSL 或 Tongsuo 的安装目录。默认路径为 macOS Apple Silicon 的 `/opt/homebrew/opt/openssl@3`、macOS Intel 的 `/usr/local/opt/openssl@3`、Linux 的 `/usr`。安装在其他位置时使用 `./config OPENSSL_PREFIX=/path/to/openssl`。

默认启用不依赖 GMP 的 189 个参数集；指定 GMP 路径后，无算法选项时启用全部 208 个参数集。也可以按[方案汇总](NGCC_PROPOSALS_SUMMARY.md)中的调用名称选择：

```sh
# 只启用这两个实例
./config enable-kem-zen-128 enable-sig-01-aigis-sig2

# 从默认选择中排除这两个实例（与上面的启用示例二选一）
./config no-kem-zen-128 no-sig-01-aigis-sig2
make -j6
```

多个 `enable-` 取并集；只有 `no-` 时从默认选择中排除，提供 GMP 路径后则从全部参数集中排除。同一实例同时启用和禁用、未知名称或空选择均报错。每次配置重新计算选择，不叠加上次选项；重新配置后直接运行 `make`。

`./config --list` 列出目标，`./config --list-algorithms` 列出名称。配置支持 `CC=...`、`LD=...`、`CFLAGS=...`、`CPPFLAGS=...`、`LDFLAGS=...`、`GMP_STATIC=...` 等参数。默认识别当前系统；交叉编译需显式提供目标及配套工具链、依赖库。

产物为 `ngcc.so`。

### GMP

以下方案默认不启用。需要将它们纳入全量构建，或通过 `enable-` 单独启用时，必须指定 GMP 路径：

- QIMEN-PIKE：2 个参数集，`kem-31-ngcc-1`、`kem-31-ngcc-2`。
- SQISign2Dsquare：12 个参数集，`sig-sqisign2dsq-*`。
- SQIsign2D-push1/2：2 个已接入参数集，`sig-26-sqisign2d-lvl1`、`sig-26-sqisign2d-lvl3`。
- SQIsignTriangle：3 个参数集，`sig-27-sqisigntriangle-lvl1`、`sig-27-sqisigntriangle-lvl2`、`sig-27-sqisigntriangle-lvl5`。

只选择其他方案时不需要 GMP。提供 GMP 路径后，仍可用 `no-` 排除上述参数集。

需要 GMP 时，可指定源码目录：

```sh
./config --with-gmp-source=/path/to/gmp-source
```

GMP 源码目录须包含 `configure` 且未配置过；GMP 构建另需 `m4`。
配置完成后，ngcc_provider 的 `make` 在 `build/gmp/` 中构建 PIC 静态库并合入 `ngcc.so`，不修改 GMP 源码目录。已有 PIC 静态 GMP 时，可用 `GMP_ROOT=/path/to/gmp-install` 指定；它与源码选项互斥。每次配置需重新传入所需的 GMP 选项。交叉编译时可用 `GMP_HOST=目标三元组` 指定 GMP 目标。

## 测试

测试工具另需 Python 3.10+，使用 `OPENSSL_PREFIX` 中的宿主：

```sh
make verify-sources
make test JOBS=6 TEST_TIMEOUT=1800
```

测试覆盖 EVP、TLS、证书及互通；实际执行范围和跳过项以[方案汇总](NGCC_PROPOSALS_SUMMARY.md#测试与来源快照)为准。默认按历史完整用例墙钟时间跳过超过 1 秒的参数集，这不是单次算法运算的性能指标。`FULL_TEST=1` 运行全部已启用参数；要覆盖全部 208 个参数集，需先提供 GMP 路径并且不使用算法过滤选项。跳过不计为通过。
