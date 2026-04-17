#include <stdio.h>
#include <kernel/pmm.h>

extern uint32_t _kernel_end; // Linked finds the address of the kernel end
uint8_t* bitmap_addr = (uint8_t*)(&_kernel_end); // where is the first available piece of memory after the kernel?

uint32_t num_frames = 0;
uint32_t bitmap_size_in_bytes = 0;

void PMM_initialize(multiboot_info_t* mbi) {

    uint32_t highest_address = 0;

    multiboot_mmap_entry_t* entry = (multiboot_mmap_entry_t*)mbi->mmap_addr; // identify first entry of mmap entries
    uint8_t* end = (uint8_t*)mbi->mmap_addr + mbi->mmap_length; // identify where the mmap entries end for the sake of looping through

    while (entry < end) {
        if (entry->type == 1) { // if the current entry is useable
            if (entry->addr + entry->len > highest_address) {
                highest_address = entry->addr + entry->len; // updates the currernt highest available address that RAM reaches
            }
        }
        entry = (multiboot_mmap_entry_t*)((uint8_t*)entry + entry->size + 4); // traverse to the next entry based off size of current entry which could vary
    }

    num_frames = highest_address / 4096; // calculates the number of frames based off the highest address and the size of each frame ( 4Kib)
    bitmap_size_in_bytes = num_frames / 8; // calculates the size of the bitmap in bytes based off how many frames there are since each frame availablity can be indicated by a single bit
    
    memset(bitmap_addr, 0xFF, bitmap_size_in_bytes); // Initializes the bitmap with all frames defaulting to 1 ( NOT useable )

    entry = (multiboot_mmap_entry_t*)mbi->mmap_addr;
    while (entry < end) { // KEY POINT: In the bitmap, logic is flipped, 1 = unavaiable memory, 0 = available memory, while in the mmap entries 1 = available and 0 = unavailable DONT GET CONFUSED
        if (entry->type == 1) { // if the mmap entry is labeled free
            uint32_t first_frame = (uint32_t)entry->addr / 4096; // find the address of the first frame in this entry
            uint32_t last_frame = ((uint32_t)entry->addr + (uint32_t)entry->len) / 4096; // find the address of the second frame in this entry

            for (uint32_t frame = first_frame; frame < last_frame; frame++) { // iterate through the frames in the entry
                uint32_t byte_index = frame / 8; // find byte of that frame in the bitmap
                uint32_t bit_index  = frame % 8; // find the bit within the byte of that frame in the bitmap (in phyical memory each frame is 4kib)
                bitmap_addr[byte_index] &= ~(1 << bit_index); // set that bit to 0, indicating its free in the bitmap
            }
        }
        entry = (multiboot_mmap_entry_t*)((uint8_t*)entry + entry->size + 4); // move to the next entry, based off the size of the current frame
    }

    // Mark specific regions in the bitmap as not available (1)

    // low memory (best practice to leave alone for other system related stuff)
    uint32_t current_addr = 0x0;
    while (current_addr < 0x100000) { // from 0x0 to 0x100000
        uint32_t frame = current_addr / 4096; 
        for (int i = 0; i < 8; i++) {
            bitmap_addr[frame / 8] |= (1 << (frame % 8)); // (frame / 8) -> what byte is this frames bit in? (frame % 8) -> what bit within that byte refers to this frame?
            frame++;
        }
        current_addr = current_addr + (8 * 4096);
    }

    // kernel itself
    current_addr = 0x100000;
    while (current_addr < (uint32_t)&_kernel_end) { // from 0x100000 to address of _kernel_end
        uint32_t frame = current_addr / 4096;
        for (int i = 0; i < 8; i++) {
            bitmap_addr[frame / 8] |= (1 << (frame % 8));
            frame++;
        }
        current_addr = current_addr + (8 * 4096);
    }

    // bitmap
    current_addr = (uint32_t)bitmap_addr;
    while (current_addr < (uint32_t)bitmap_addr + bitmap_size_in_bytes) { // from start of bitmap to end
        uint32_t frame = current_addr / 4096;
        for (int i = 0; i < 8; i++) {
            bitmap_addr[frame / 8] |= (1 << (frame % 8));
            frame++;
        }
        current_addr = current_addr + (8 * 4096);
    }

}

uint32_t pmm_alloc_frame(void) {
    uint8_t* current_byte = bitmap_addr;
    while (current_byte < bitmap_addr + bitmap_size_in_bytes) { // iterate through the bytes of the bitmap
        if (*current_byte != 0xFF) { // two layer search method, no need to search within the byte itself if it already is all unavailable (1)
            for (int i = 0; i < 8; i++) { // iterate through each bit
                if (!(*current_byte & (1 << i))) { // if the bit is 0
                    uint32_t frame = (current_byte - bitmap_addr) * 8 + i; // byte offset into bitmap * 8 bits + bit index = frame number
                    *current_byte |= (1 << i); // mark frame as used
                    return frame * 4096; // return physical address
                }
            }
        }
        current_byte++;
    }
    return 0; // no free frames

}
void pmm_free_frame(uint32_t frame_addr) {

}
uint32_t pmm_get_free_frame_count(void) {

}
uint32_t pmm_get_total_frame_count(void) {

}