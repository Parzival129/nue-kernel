#ifndef _KERNEL_PMM_H
#define _KERNEL_PMM_H

#include <stdint.h>
#include <kernel/multiboot.h>

void PMM_initialize(multiboot_info_t* mbi);
uint32_t pmm_alloc_frame(void);
void pmm_free_frame(uint32_t frame_addr);
uint32_t pmm_get_free_frame_count(void);
uint32_t pmm_get_total_frame_count(void);

#endif
