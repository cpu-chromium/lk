/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

/* some assembly #defines, need to match the structure below */
#define __MMU_INITIAL_MAPPING_PHYS_OFFSET 0
#define __MMU_INITIAL_MAPPING_VIRT_OFFSET 4
#define __MMU_INITIAL_MAPPING_SIZE_OFFSET 8
#define __MMU_INITIAL_MAPPING_FLAGS_OFFSET 12
#define __MMU_INITIAL_MAPPING_SIZE        20

/* flags for initial mapping struct */
#define MMU_INITIAL_MAPPING_TEMPORARY     (0x1)
#define MMU_INITIAL_MAPPING_FLAG_UNCACHED (0x2)
#define MMU_INITIAL_MAPPING_FLAG_DEVICE   (0x4)

#ifndef ASSEMBLY

#include <sys/types.h>
#include <stdint.h>
#include <compiler.h>
#include <list.h>
#include <stdlib.h>
#include <arch.h>
#include <arch/mmu.h>

__BEGIN_CDECLS

#define PAGE_ALIGN(x) ALIGN(x, PAGE_SIZE)
#define IS_PAGE_ALIGNED(x) IS_ALIGNED(x, PAGE_SIZE)

struct mmu_initial_mapping {
    paddr_t phys;
    vaddr_t virt;
    size_t  size;
    unsigned int flags;
    const char *name;
};

/* assert that the assembly macros above match this struct */
STATIC_ASSERT(__offsetof(struct mmu_initial_mapping, phys) == __MMU_INITIAL_MAPPING_PHYS_OFFSET);
STATIC_ASSERT(__offsetof(struct mmu_initial_mapping, virt) == __MMU_INITIAL_MAPPING_VIRT_OFFSET);
STATIC_ASSERT(__offsetof(struct mmu_initial_mapping, size) == __MMU_INITIAL_MAPPING_SIZE_OFFSET);
STATIC_ASSERT(__offsetof(struct mmu_initial_mapping, flags) == __MMU_INITIAL_MAPPING_FLAGS_OFFSET);
STATIC_ASSERT(sizeof(struct mmu_initial_mapping) == __MMU_INITIAL_MAPPING_SIZE);

/* platform or target must fill out one of these to set up the initial memory map
 * for kernel and enough IO space to boot
 */
extern struct mmu_initial_mapping mmu_initial_mappings[];

/* core per page structure */
typedef struct vm_page {
    struct list_node node;

    uint flags : 8;
    uint ref : 24;
} vm_page_t;

#define VM_PAGE_FLAG_NONFREE  (0x1)

/* physical allocator */
typedef struct pmm_arena {
    struct list_node node;
    const char *name;

    uint flags;
    uint priority;

    paddr_t base;
    size_t  size;

    size_t free_count;

    struct vm_page *page_array;
    struct list_node free_list;
} pmm_arena_t;

#define PMM_ARENA_FLAG_KMAP (0x1) // this arena is already mapped and useful for kallocs

status_t pmm_add_arena(pmm_arena_t *arena);
uint pmm_alloc_pages(uint count, struct list_node *list);
uint pmm_alloc_range(paddr_t address, uint count, struct list_node *list);
int pmm_free(struct list_node *list);

    /* allocate a run of pages out of the kernel area and return the mapped pointer */
void *pmm_alloc_kpages(uint count, struct list_node *list);
static inline void *pmm_alloc_kpage(void) { return pmm_alloc_kpages(1, NULL); }

/* physical to virtual */
void *paddr_to_kvaddr(paddr_t pa);

/* virtual allocator */
typedef struct vmm_aspace {
    struct list_node node;
    char name[32];

    uint flags;

    vaddr_t base;
    size_t  size;

    struct list_node region_list;
} vmm_aspace_t;

typedef struct vmm_region {
    struct list_node node;
    char name[32];

    uint flags;
    uint arch_mmu_flags;

    vaddr_t base;
    size_t  size;

    struct list_node page_list;
} vmm_region_t;

#define VMM_REGION_FLAG_RESERVED 0x1
#define VMM_REGION_FLAG_PHYSICAL 0x2

/* grab a handle to the kernel address space */
extern vmm_aspace_t _kernel_aspace;
static inline vmm_aspace_t *vmm_get_kernel_aspace(void) {
    return &_kernel_aspace;
}

/* reserve a chunk of address space to prevent allocations from that space */
status_t vmm_reserve_space(vmm_aspace_t *aspace, const char *name, size_t size, vaddr_t vaddr);

/* allocate a region of virtual space that maps a physical piece of address space.
   the physical pages that back this are not allocated from the pmm. */
status_t vmm_alloc_physical(vmm_aspace_t *aspace, const char *name, size_t size, void **ptr, paddr_t paddr, uint vmm_flags, uint arch_mmu_flags);

/* allocate a region of memory backed by newly allocated contiguous physical memory  */
status_t vmm_alloc_contiguous(vmm_aspace_t *aspace, const char *name, size_t size, void **ptr, uint vmm_flags, uint arch_mmu_flags);

/* allocate a region of memory backed by newly allocated physical memory */
status_t vmm_alloc(vmm_aspace_t *aspace, const char *name, size_t size, void **ptr, uint vmm_flags, uint arch_mmu_flags);

#define VMM_FLAG_VALLOC_SPECIFIC 0x1

__END_CDECLS

#endif // !ASSEMBLY