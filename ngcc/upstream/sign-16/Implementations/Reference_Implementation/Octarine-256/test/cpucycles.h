#ifndef CPUCYCLES_H
#define CPUCYCLES_H
#include <stdint.h>
#include <linux/perf_event.h>

/* [PROFILE] A thread-attached, pinned cycles:uk PMU event; never TSC. */
extern struct perf_event_mmap_page *arcane_pmu_page;
_Noreturn void arcane_pmu_fail(const char *message);

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

static inline uint64_t cpucycles_elapsed(uint64_t start, uint64_t end, uint64_t overhead) {
  if (end < start || end - start < overhead)
    arcane_pmu_fail("invalid cycle delta");
  return end - start - overhead;
}

uint64_t cpucycles_overhead(void);
#endif
