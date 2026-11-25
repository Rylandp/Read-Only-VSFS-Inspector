#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vsfs.h"

/*
 * Central state for one read-only inspection pass.
 * All bitmaps and inodes are loaded in main() before walk_dir() and
 * check_all_inodes_and_blocks() run.
 */
typedef struct {
    FILE *fp;
    vsfs_superblock_t sb;
    uint8_t inode_bm[VSFS_BLOCK_SIZE];
    uint8_t data_bm[VSFS_BLOCK_SIZE];
    vsfs_inode_t inodes[VSFS_NUM_INODES];
    uint32_t actual_refs[VSFS_NUM_INODES];   /* Actual dir-entry refs per inode */
    uint8_t visited_dir[VSFS_NUM_INODES];    /* Cycle guard for walk_dir() */
    uint32_t block_refs[VSFS_TOTAL_BLOCKS];  /* Number of owners per data block */
    int32_t block_owner[VSFS_TOTAL_BLOCKS];  /* First inode claiming each block */
    uint32_t errors;
} fs_ctx_t;

/* Return bit 'idx' (0 or 1) from a 1024-byte bitmap block. */
int get_bit(const uint8_t *bm, uint32_t idx) {
    return (bm[idx / 8U] >> (idx % 8U)) & 1U;
}

/* Read block 'blk' (1024 bytes) into 'buf'. Returns 0 on success, -1 on error. */
static int read_block(FILE *fp, uint32_t blk, void *buf) {
    if (fseek(fp, (long)blk * (long)VSFS_BLOCK_SIZE, SEEK_SET) != 0) {
        return -1;
    }
    if (fread(buf, VSFS_BLOCK_SIZE, 1, fp) != 1) {
        return -1;
    }
    return 0;
}

/* Human-readable inode type name for ENTRY and ERROR lines. */
const char *type_name(uint32_t type) {
    switch (type) {
    case VSFS_FILE: return "FILE";
    case VSFS_DIR:  return "DIR";
    default:        return "FREE";
    }
}

/*
 * TODO (Stage 1): Validate the superblock.
 * Verify sb->magic == VSFS_MAGIC and that total_blocks, num_inodes,
 * inode_bitmap_block, data_bitmap_block, inode_table_start,
 * inode_table_blocks, data_start, and data_blocks match vsfs.h constants.
 * Return 0 if valid, -1 if invalid.
 */
static int validate_superblock(const vsfs_superblock_t *sb) {
    (void)sb;
    fprintf(stderr, "TODO statimg.c: implement validate_superblock, walk_dir, and checks\n");
    return -2;
}

/*
 * TODO (Stage 3/4): Validate a single data-block pointer 'blk' owned by 'inum'.
 * 1. If blk < data_start or blk >= total_blocks:
 *      print "ERROR invalid_block_ptr inode=<inum> block=<blk>\n", increment errors, return.
 * 2. If data_bm bit for blk is 0:
 *      print "ERROR unmarked_block inode=<inum> block=<blk>\n", increment errors.
 * 3. If block_refs[blk] > 0:
 *      print "ERROR duplicate_block block=<blk> first_inode=<owner> second_inode=<inum>\n",
 *      increment errors.
 *    Else record block_owner[blk] = (int32_t)inum.
 * 4. Increment block_refs[blk].
 */
void check_block_ptr(fs_ctx_t *ctx, uint32_t inum, uint32_t blk) {
    (void)ctx;
    (void)inum;
    (void)blk;
}

/*
 * TODO (Stage 2/3): Print "blocks=[b0,b1,...]" (and " indirect=<blk>" if
 * ino->indirect != 0) for inode 'inum'.
 * Include nonzero direct[0..7], the indirect block itself if nonzero, and all
 * nonzero uint32_t pointers stored inside the indirect block.
 */
void print_inode_blocks(fs_ctx_t *ctx, uint32_t inum) {
    (void)ctx;
    (void)inum;
    printf("blocks=[]");
}

/*
 * TODO (Stage 2): Recursively walk directory 'dir_inum' at 'dir_path'.
 * - Mark ctx->visited_dir[dir_inum] = 1.
 * - For each nonzero direct block of the directory inode:
 *   - Read VSFS_DIRENTS_PER_BLK directory entries.
 *   - Skip unused entries (inode == VSFS_INVALID_INODE).
 *   - Copy ent.name safely into a null-terminated char[VSFS_NAME_MAX + 1] buffer.
 *   - Build full_path ("/<name>" if dir_path is "/", else "<dir_path>/<name>").
 *   - If target >= num_inodes or inodes[target].type == VSFS_FREE:
 *       print "ERROR dangling_dirent path=<full_path> inode=<target>\n",
 *       increment ctx->errors, and continue.
 *   - Increment ctx->actual_refs[target] (for ALL valid entries, including "." and "..").
 *   - If name is "." or "..", continue without printing or recursing.
 *   - Print:
 *       "ENTRY path=<full_path> inode=<target> type=<type> size=<size> links=<link_count> "
 *     followed by print_inode_blocks(ctx, target) and "\n".
 *   - If target is VSFS_DIR:
 *       if ctx->visited_dir[target] is already 1:
 *         print "ERROR dir_cycle path=<full_path> inode=<target>\n", increment ctx->errors;
 *       else:
 *         recurse with walk_dir(ctx, target, full_path).
 */
static void walk_dir(fs_ctx_t *ctx, uint32_t dir_inum, const char *dir_path) {
    (void)ctx;
    (void)dir_inum;
    (void)dir_path;
}

/*
 * TODO (Stage 4): Run all inode and data-block consistency cross-checks.
 * 1. For each inode i in 0 .. sb.num_inodes - 1:
 *    - Compare get_bit(inode_bm, i) against (inodes[i].type != VSFS_FREE).
 *      On mismatch: print "ERROR inode_bitmap_mismatch inode=<i> bitmap=<bm> type=<type>\n",
 *      increment ctx->errors.
 *    - If inodes[i].type != VSFS_FREE:
 *      - Call check_block_ptr() on every nonzero direct[d].
 *      - If indirect != 0, call check_block_ptr() on indirect itself, then read
 *        the indirect block (if in range) and call check_block_ptr() on every
 *        nonzero pointer inside it.
 *      - If actual_refs[i] == 0:
 *          print "ERROR orphan_inode inode=<i>\n", increment ctx->errors.
 *      - Else if inodes[i].link_count != actual_refs[i]:
 *          print "ERROR link_count_mismatch inode=<i> expected=<link_count> actual=<actual_refs>\n",
 *          increment ctx->errors.
 * 2. For each data block b in sb.data_start .. sb.total_blocks - 1:
 *    - If get_bit(data_bm, b) && block_refs[b] == 0:
 *        print "ERROR leaked_block block=<b>\n", increment ctx->errors.
 */
static void check_all_inodes_and_blocks(fs_ctx_t *ctx) {
    (void)ctx;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <disk.img>\n", argv[0]);
        return 2;
    }

    fs_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    for (uint32_t b = 0; b < VSFS_TOTAL_BLOCKS; b++) {
        ctx.block_owner[b] = -1;
    }

    ctx.fp = fopen(argv[1], "rb");
    if (!ctx.fp) {
        fprintf(stderr, "statimg: cannot open '%s': %s\n",
                argv[1], strerror(errno));
        return 2;
    }

    if (read_block(ctx.fp, 0, &ctx.sb) != 0) {
        fprintf(stderr, "statimg: failed to read superblock from '%s'\n", argv[1]);
        fclose(ctx.fp);
        return 2;
    }

    int sb_rc = validate_superblock(&ctx.sb);
    if (sb_rc == -2) {
        fclose(ctx.fp);
        return 1;
    }
    if (sb_rc != 0) {
        printf("ERROR bad_superblock magic=0x%08x\n", ctx.sb.magic);
        printf("STATUS corrupt errors=1\n");
        fclose(ctx.fp);
        return 1;
    }

    if (read_block(ctx.fp, ctx.sb.inode_bitmap_block, ctx.inode_bm) != 0 ||
        read_block(ctx.fp, ctx.sb.data_bitmap_block, ctx.data_bm) != 0) {
        fprintf(stderr, "statimg: failed to read bitmaps\n");
        fclose(ctx.fp);
        return 2;
    }

    for (uint32_t b = 0; b < ctx.sb.inode_table_blocks; b++) {
        if (read_block(ctx.fp, ctx.sb.inode_table_start + b,
                       &ctx.inodes[b * VSFS_INODES_PER_BLK]) != 0) {
            fprintf(stderr, "statimg: failed to read inode table\n");
            fclose(ctx.fp);
            return 2;
        }
    }

    printf("SUPERBLOCK blocks=%u inodes=%u data_start=%u data_blocks=%u\n",
           ctx.sb.total_blocks, ctx.sb.num_inodes,
           ctx.sb.data_start, ctx.sb.data_blocks);

    walk_dir(&ctx, VSFS_ROOT_INODE, "/");
    check_all_inodes_and_blocks(&ctx);

    if (ctx.errors == 0) {
        printf("STATUS clean\n");
    } else {
        printf("STATUS corrupt errors=%u\n", ctx.errors);
    }

    fclose(ctx.fp);
    return ctx.errors == 0 ? 0 : 1;
}
