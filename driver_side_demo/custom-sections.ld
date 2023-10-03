#define SHMEM_RESERVED_HEADERS 4K
#define SHMEM_RESERVED_SIZE (16 * 1K)

SECTION_PROLOGUE (shmem0, 20040400 (NOLOAD),)
{
    __shmem0_start = .;
    KEEP(*(.shared.vring.*));
    __shmem0_pool_start = .;
    __shmem0_end = __shmem0_start + (SHMEM_RESERVED_SIZE - SHMEM_RESERVED_HEADERS);
} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)