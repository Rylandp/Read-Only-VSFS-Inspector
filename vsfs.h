#ifndef VSFS_H
#define VSFS_H

#include <stdint.h>

#define VSFS_MAGIC             0x56534653U /* "VSFS" */
#define VSFS_BLOCK_SIZE        1024U
#define VSFS_TOTAL_BLOCKS      64U
#define VSFS_NUM_INODES        64U
#define VSFS_ROOT_INODE        0U
#define VSFS_DIRECT_PTRS       8U
#define VSFS_NAME_MAX          28U
#define VSFS_INVALID_INODE     0xFFFFFFFFU

#define VSFS_INODE_BITMAP_BLK  1U
#define VSFS_DATA_BITMAP_BLK   2U
#define VSFS_INODE_TABLE_START 3U
#define VSFS_INODE_TABLE_BLKS  4U
#define VSFS_DATA_START        7U
#define VSFS_DATA_BLOCKS       (VSFS_TOTAL_BLOCKS - VSFS_DATA_START) /* 57 */

#define VSFS_INODES_PER_BLK    (VSFS_BLOCK_SIZE / sizeof(vsfs_inode_t))   /* 16 */
#define VSFS_DIRENTS_PER_BLK   (VSFS_BLOCK_SIZE / sizeof(vsfs_dirent_t))  /* 32 */
#define VSFS_INDIRECT_PTRS     (VSFS_BLOCK_SIZE / sizeof(uint32_t))       /* 256 */

typedef enum {
    VSFS_FREE = 0,
    VSFS_FILE = 1,
    VSFS_DIR  = 2
} vsfs_type_t;

typedef struct {
    uint32_t magic;
    uint32_t total_blocks;
    uint32_t num_inodes;
    uint32_t inode_bitmap_block;
    uint32_t data_bitmap_block;
    uint32_t inode_table_start;
    uint32_t inode_table_blocks;
    uint32_t data_start;
    uint32_t data_blocks;
    uint8_t  reserved[VSFS_BLOCK_SIZE - 9 * sizeof(uint32_t)];
} vsfs_superblock_t;

typedef struct {
    uint32_t type;
    uint32_t size;
    uint32_t link_count;
    uint32_t direct[VSFS_DIRECT_PTRS];
    uint32_t indirect;
    uint32_t reserved[4];
} vsfs_inode_t;

typedef struct {
    uint32_t inode;
    char     name[VSFS_NAME_MAX];
} vsfs_dirent_t;

#endif /* VSFS_H */
