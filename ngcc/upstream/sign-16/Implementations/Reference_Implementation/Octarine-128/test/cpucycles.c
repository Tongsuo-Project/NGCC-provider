#define _GNU_SOURCE
#include <errno.h>
#include <inttypes.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "cpucycles.h"

struct perf_event_mmap_page *arcane_pmu_page;
static int pmu_fd = -1;
static int initial_cpu = -1;
struct pmu_reading { uint64_t value, enabled, running; };

_Noreturn void arcane_pmu_fail(const char *message) {
  fprintf(stderr, "[pmu-error] %s (errno=%d: %s)\n", message, errno, strerror(errno));
  fflush(NULL);
  _Exit(90);
}

static struct pmu_reading read_event(void) {
  struct pmu_reading data;
  if (read(pmu_fd, &data, sizeof(data)) != sizeof(data))
    arcane_pmu_fail("read pinned PMU event");
  return data;
}

static void __attribute__((constructor)) pmu_init(void) {
  cpu_set_t affinity;
  CPU_ZERO(&affinity);
  if (sched_getaffinity(0, sizeof(affinity), &affinity) || CPU_COUNT(&affinity) != 1)
    arcane_pmu_fail("benchmark must be bound to exactly one logical CPU");
  initial_cpu = sched_getcpu();
  struct perf_event_attr attr;
  memset(&attr, 0, sizeof(attr));
  attr.size = sizeof(attr);
  attr.type = PERF_TYPE_HARDWARE;
  attr.config = PERF_COUNT_HW_CPU_CYCLES;
  attr.disabled = 1;
  attr.pinned = 1;
  attr.exclude_user = 0;
  attr.exclude_kernel = 0; /* [PROFILE] Include syscall/kernel CPU cycles. */
  attr.exclude_hv = 1;
  attr.read_format = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
  pmu_fd = syscall(SYS_perf_event_open, &attr, 0, -1, -1, PERF_FLAG_FD_CLOEXEC);
  if (pmu_fd < 0) arcane_pmu_fail("perf_event_open cycles:uk");
  arcane_pmu_page = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ, MAP_SHARED, pmu_fd, 0);
  if (arcane_pmu_page == MAP_FAILED) arcane_pmu_fail("mmap perf metadata");
  if (ioctl(pmu_fd, PERF_EVENT_IOC_RESET, 0) || ioctl(pmu_fd, PERF_EVENT_IOC_ENABLE, 0))
    arcane_pmu_fail("enable PMU event");
  (void)cpucycles();
  fprintf(stderr, "[pmu-start] event=PERF_COUNT_HW_CPU_CYCLES mode=user+kernel exclude_user=0 exclude_kernel=0 pinned=1 cpu=%d rdpmc=1 width=%u index=%u\n",
          initial_cpu, arcane_pmu_page->pmc_width, arcane_pmu_page->index);
}

static void __attribute__((destructor)) pmu_finish(void) {
  if (pmu_fd < 0) return;
  if (ioctl(pmu_fd, PERF_EVENT_IOC_DISABLE, 0)) arcane_pmu_fail("disable PMU event");
  struct pmu_reading data = read_event();
  struct rusage usage;
  getrusage(RUSAGE_SELF, &usage);
  int cpu = sched_getcpu();
  fprintf(stderr, "[pmu-end] cpu=%d cycles=%" PRIu64 " enabled_ns=%" PRIu64
          " running_ns=%" PRIu64 " voluntary_cs=%ld involuntary_cs=%ld\n",
          cpu, data.value, data.enabled, data.running, usage.ru_nvcsw, usage.ru_nivcsw);
  if (cpu != initial_cpu || !data.value || !data.running || data.enabled != data.running)
    arcane_pmu_fail("CPU changed or PMU did not run for 100% of enabled time");
  /* Descriptors and mappings are released by process exit; no files removed. */
}

uint64_t cpucycles_overhead(void) {
  uint64_t overhead = UINT64_MAX;
  for (unsigned int i = 0; i < 100000; i++) {
    uint64_t t0 = cpucycles();
    __asm__ volatile ("" ::: "memory");
    uint64_t t1 = cpucycles();
    if (t1 < t0) arcane_pmu_fail("non-monotonic PMU counter");
    if (t1 - t0 < overhead) overhead = t1 - t0;
  }
  fprintf(stderr, "[pmu-overhead] cycles=%" PRIu64 " trials=100000 statistic=min\n", overhead);
  return overhead;
}
