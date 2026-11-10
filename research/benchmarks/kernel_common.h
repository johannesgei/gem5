#ifndef RESEARCH_KERNEL_COMMON_H
#define RESEARCH_KERNEL_COMMON_H

#ifdef GEM5_ROI
#include <gem5/m5ops.h>

static inline void
roi_begin(void)
{
    m5_reset_stats(0, 0);
}

static inline void
roi_end(void)
{
    m5_dump_stats(0, 0);
}

#else

static inline void
roi_begin(void)
{
}

static inline void
roi_end(void)
{
}

#endif /* GEM5_ROI */

#endif /* RESEARCH_KERNEL_COMMON_H */
