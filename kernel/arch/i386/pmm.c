#include <stdio.h>
#include <kernel/pmm.h>

uint8_t* bitmap_addr;

void PMM_initialize(multiboot_info_t* mbi) {
    uint8_t bitmap[mbi->mmap_length];
    bitmap_addr = bitmap;
    uint32_t* mmap_addr = mbi->mmap_addr;
    return;
}

uint32_t pmm_alloc_frame(void) {

}
void pmm_free_frame(uint32_t frame_addr) {

}
uint32_t pmm_get_free_frame_count(void) {

}
uint32_t pmm_get_total_frame_count(void) {

}