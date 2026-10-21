#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static uint32_t weaver_q;
static int dt;

static uint32_t compress_q(uint32_t x)
{
    uint32_t num_buckets = 1u << dt;
    return (uint32_t)(((uint64_t)x * num_buckets + weaver_q / 2) / weaver_q) & (num_buckets - 1);
}

static int parse_args(int argc, char **argv, const char **out_path)
{
    int i;
    int have_q = 0;
    int have_d = 0;

    for(i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-q") == 0 && i + 1 < argc) {
            weaver_q = (uint32_t)strtoul(argv[++i], NULL, 10);
            have_q = 1;
        } else if(strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            dt = (int)strtol(argv[++i], NULL, 10);
            have_d = 1;
        } else if(strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            *out_path = argv[++i];
        } else {
            fprintf(stderr, "Usage: %s -q <modulus> -d <dt> -o <output.h>\n", argv[0]);
            return 1;
        }
    }

    if(!have_q || !have_d || !*out_path) {
        fprintf(stderr, "Usage: %s -q <modulus> -d <dt> -o <output.h>\n", argv[0]);
        return 1;
    }

    if(dt < 1 || dt > 12) {
        fprintf(stderr, "dt must be in [1,12]\n");
        return 1;
    }

    return 0;
}

static uint32_t find_bucket_lo(uint32_t y, uint32_t sz)
{
    uint32_t lo, t;

    for(lo = 0; lo < weaver_q; lo++) {
        for(t = 0; t < sz; t++) {
            if(compress_q(lo + t) != y)
                break;
        }
        if(t == sz)
            return lo;
    }

    fprintf(stderr, "ERROR: no bucket_lo for bucket %u size %u\n", y, sz);
    exit(1);
    return 0;
}

static void emit_header(FILE *out)
{
    uint32_t num_buckets = 1u << dt;
    uint16_t count[4096];
    uint32_t bucket_lo[4096];
    uint32_t y;
    uint32_t x;
    uint32_t sum = 0;
    uint8_t max_size = 0;
    char guard[64];

    memset(count, 0, sizeof(count));

    for(x = 0; x < weaver_q; x++)
        count[compress_q(x)]++;

    for(y = 0; y < num_buckets; y++) {
        bucket_lo[y] = find_bucket_lo(y, count[y]);
        sum += count[y];
        if(count[y] > max_size)
            max_size = (uint8_t)count[y];
    }

    if(sum != weaver_q) {
        fprintf(stderr, "ERROR: bucket sizes sum to %u, expected %u\n", sum, weaver_q);
        exit(1);
    }

    if(max_size > 8) {
        fprintf(stderr, "ERROR: max bucket_size %u > 8 (3-bit Lemire sampling)\n", max_size);
        exit(1);
    }

    fprintf(stderr, "q=%u d=%d buckets=%u max_bucket_size=%u sum=%u\n",
            weaver_q, dt, num_buckets, max_size, sum);

    snprintf(guard, sizeof(guard), "INVQ_TABLE_D%d_H", dt);

    fprintf(out, "/* Auto-generated: q=%u, d=%d. Do not edit. */\n", weaver_q, dt);
    fprintf(out, "#ifndef %s\n", guard);
    fprintf(out, "#define %s\n", guard);
    fprintf(out, "#include <stdint.h>\n");
    fprintf(out, "static const uint16_t invq_d%d_bucket_lo[%u] = {\n", dt, num_buckets);

    for(y = 0; y < num_buckets; y++) {
        fprintf(out, "%s%4u", (y % 16 == 0) ? "    " : " ", bucket_lo[y]);
        if(y < num_buckets - 1)
            fprintf(out, ",");
        if((y + 1) % 16 == 0)
            fprintf(out, "\n");
    }
    if(num_buckets % 16 != 0)
        fprintf(out, "\n");
    fprintf(out, "};\n");

    fprintf(out, "static const uint8_t invq_d%d_bucket_size[%u] = {\n", dt, num_buckets);
    for(y = 0; y < num_buckets; y++) {
        fprintf(out, "%s%3u", (y % 16 == 0) ? "    " : " ", count[y]);
        if(y < num_buckets - 1)
            fprintf(out, ",");
        if((y + 1) % 16 == 0)
            fprintf(out, "\n");
    }
    if(num_buckets % 16 != 0)
        fprintf(out, "\n");
    fprintf(out, "};\n");
    fprintf(out, "#endif\n");
}

int main(int argc, char **argv)
{
    const char *out_path = NULL;
    FILE *out;

    if(parse_args(argc, argv, &out_path) != 0)
        return 1;

    out = fopen(out_path, "w");
    if(!out) {
        perror(out_path);
        return 1;
    }

    emit_header(out);
    fclose(out);
    return 0;
}
