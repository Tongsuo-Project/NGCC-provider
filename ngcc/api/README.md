# NGCC-provider ABI glue

This directory is the Tongsuo provider ABI glue (`link_*.h`, `link_shim.c`,
fallback `drng.c` / `auxfunc.c`). It is not author submission source.

Author reference implementations live in the private
[NGCC-candidates](https://github.com/Tongsuo-Project/NGCC-candidates)
submodule at `ngcc/candidates` (`kem-NN` / `sign-NN`). Initialize it with:

```sh
git submodule update --init
```

NGCC-provider compiles those trees through `make/ref.mk` and this glue.
Do not edit algorithm bytes in NGCC-candidates; do not build the candidates
repo's own Makefiles or patches as part of `ngcc.so`.
