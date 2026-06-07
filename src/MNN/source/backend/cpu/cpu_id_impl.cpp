//
//  cpu_id_impl.cpp  — InitCpuFlags for libyuv, needed by FunctionDispatcher
//
#include "x86_x64/cpu_id.h"
#include <intrin.h>

namespace libyuv {

int InitCpuFlags(void) {
    int flags = 0;
#if defined(_M_IX86) || defined(_M_X64)
    int cpuInfo[4];
    __cpuid(cpuInfo, 1);
    if (cpuInfo[3] & (1 << 26)) flags |= kCpuHasSSE2;
    if (cpuInfo[2] & (1 << 0))  flags |= kCpuHasSSSE3;
    if (cpuInfo[2] & (1 << 19)) flags |= kCpuHasSSE41;
    if (cpuInfo[2] & (1 << 20)) flags |= kCpuHasSSE42;
    if (cpuInfo[2] & (1 << 28)) flags |= kCpuHasAVX;
    __cpuidex(cpuInfo, 7, 0);
    if (cpuInfo[1] & (1 << 5))  flags |= kCpuHasAVX2;
#endif
    return flags | kCpuInitialized;
}

} // namespace libyuv
