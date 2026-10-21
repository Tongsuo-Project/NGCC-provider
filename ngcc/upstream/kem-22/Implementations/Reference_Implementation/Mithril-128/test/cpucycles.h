#ifndef CPUCYCLES_H
#define CPUCYCLES_H
#include <stdint.h>
#include <linux/perf_event.h>

/* [PROFILE] A thread-attached, pinned cycles:uk PMU event; never TSC. */
extern struct perf_event_mmap_page *arcane_pmu_page;
void arcane_pmu_fail(const char *message);
uint64_t arcane_pmu_read_syscall(void);
void arcane_pmu_dump_samples(const char *label, const uint64_t *samples,
                            unsigned long count, uint64_t overhead);

static inline uint64_t cpucycles(void) {
  struct perf_event_mmap_page *pc = arcane_pmu_page;
  for (;;) {
    uint32_t seq = __atomic_load_n(&pc->lock, __ATOMIC_ACQUIRE);
    if (seq & 1U) continue;
    uint32_t idx = pc->index;
    uint16_t width = pc->pmc_width;
    int64_t offset = pc->offset;
    if (!pc->cap_user_rdpmc || !idx || !width || width > 64)
      arcane_pmu_fail("RDPMC unavailable or pinned event not scheduled");
    uint32_t low, high;
    __asm__ volatile ("lfence\n\trdpmc\n\tlfence"
                      : "=a"(low), "=d"(high) : "c"(idx - 1) : "memory");
    uint64_t bits = ((uint64_t)high << 32) | low;
    int64_t pmc = (int64_t)(bits << (64 - width));
    pmc >>= 64 - width;
    uint64_t result = (uint64_t)offset + (uint64_t)pmc;
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
    if (__atomic_load_n(&pc->lock, __ATOMIC_RELAXED) == seq)
      return result;
  }
}

uint64_t cpucycles_overhead(void);
#endif
