#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <stdbool.h>

#include "xv6lib/types.h"
#include "xv6lib/fs.h"
#include "xv6lib/xv6stat.h"
#define BLOCK_SIZE (BSIZE)

#define T_DIR 1  // Directory
#define T_FILE 2 // File
#define T_DEV 3  // Special device

char *translate_address(const char *baseaddr, int block_no)
{
    return (char *)(baseaddr + block_no * BLOCK_SIZE);
}
int check_inode_type(int input_type)
{
    return (input_type == 0 || input_type == T_DIR || input_type == T_FILE || input_type == T_DEV);
}

int check_direct_addr_ranges(const struct dinode *dip, uint nblocks)
{
    int i;
    for (i = 0; i < NDIRECT; i++)
    {
        if (dip->addrs[i] >= nblocks)
        {
            return 0;
        }
    }
    return 1;
}
int check_indirect_addr_ranges(const char *baseddr, const struct dinode *dip, uint nblocks)
{
    if (dip->addrs[NDIRECT] == 0)
    {
        return 1;
    }
    if (dip->addrs[NDIRECT] > nblocks)
    {
        return 0;
    }
    uint *indirect_block = (uint *)translate_address(baseddr, dip->addrs[NDIRECT]);
    int BLOCK_NUM_BYTES = 4;
    if (indirect_block)
    {
        int i;
        for (i = 0; i < BLOCK_SIZE / BLOCK_NUM_BYTES; i++)
        {
            if (indirect_block[i] >= nblocks)
            {
                return 0;
            }
        }
    }
    return 1;
}

void check_inode_addr_ranges(const char *baseaddr, const struct dinode *dip, uint nblocks)
{
    if (!dip->type)
    {
        return;
    }
    if (!check_direct_addr_ranges(dip, nblocks))
    {
        fprintf(stderr, "ERROR: bad direct address in inode.\n");
        exit(1);
    }
    if (!check_indirect_addr_ranges(baseaddr, dip, nblocks))
    {
        fprintf(stderr, "ERROR: bad indirect address in inode.\n");
        exit(1);
    }
}

int check_inode_bitmap_consistency(const char *baseaddr, const struct dinode *dip, int *databitmap)
{
    if (dip->type == 0)
    {
        return 1;
    }
    int i;
    for (i = 0; i < NDIRECT; i++)
    {
        if ((dip->addrs[i] != 0) && databitmap[dip->addrs[i]] == 0)
        {
            return 0;
        }
    }
    if (dip->addrs[NDIRECT] == 0)
    {
        return 1;
    }
    uint *indirect_block = (uint *)translate_address(baseaddr, dip->addrs[NDIRECT]);
    int BLOCK_NUM_BYTES = 4;
    if (databitmap[dip->addrs[NDIRECT]] == 0)
    {
        return 0;
    }
    if (indirect_block)
    {
        for (i = 0; i < BLOCK_SIZE / BLOCK_NUM_BYTES; i++)
        {
            uint ib = indirect_block[i];
            if (ib && databitmap[ib] == 0)
            {
                return 0;
            }
        }
    }
    return 1;
}

int check_if_datablock_falsely_marked_in_use(const char *addr, int *databitmap)
{
    struct superblock *sb = (struct superblock *)translate_address(addr,1);
    const struct dinode *dip = (struct dinode *)translate_address(addr,2);
    int inodeDataBitset[sb->nblocks];
    memset(inodeDataBitset, 0, sizeof(inodeDataBitset));
    inodeDataBitset[0] = 1;
    inodeDataBitset[1] = 1;
    inodeDataBitset[2] = 1;
    const int datablockstarting = 4+ (sb->ninodes/IPB);
    int i, j, k;
    for(i = 0; i<datablockstarting ; i++){
        inodeDataBitset[i] = 1;
    }
    int BLOCK_NUM_BYTES = 4;
    for (i = 1; i < sb->ninodes; i++)
    {
        if (dip[i].type)
        {
            for (j = 0; j < NDIRECT + 1; j++)
            {
                if (dip[i].addrs[j] < sb->nblocks)
                {
                    inodeDataBitset[dip[i].addrs[j]] = 1;
                }
            }
            if (dip[i].addrs[NDIRECT] && dip[i].addrs[NDIRECT] < sb->nblocks)
            {
                inodeDataBitset[dip[i].addrs[NDIRECT]] = 1;
                uint *indirect_block = (uint *)translate_address(addr, dip[i].addrs[NDIRECT]);
                for (k = 0; k < BLOCK_SIZE / BLOCK_NUM_BYTES; k++)
                {
                    if (indirect_block[k] < sb->nblocks)
                    {
                        inodeDataBitset[indirect_block[k]] = 1;
                    }
                }
            }
        }
    }
    for (i = 0; i < sb->nblocks; i++)
    {
        if (databitmap[i] == 1 && inodeDataBitset[i] == 0)
        {
            return 0;
        }
    }
    return 1;
}

void loadbitmap(const char *baseaddr, uint size, int bitmap[size])
{
    struct superblock *sb = (struct superblock *)translate_address(baseaddr, 1);
    printf("size:%d ninodes:%d nblocks%d\n",sb->size,sb->ninodes,sb->nblocks );
    char* address = (char*)translate_address(baseaddr,3+(sb->ninodes/IPB)); //baseaddr + (((sb-> ninodes)/(IPB)) + 3)*BSIZE;
    //address += 4;
    int i, m;
    for (i = 0; i < size; i++)
    {
        if (i < (sb->size) - (sb->nblocks))
            bitmap[i] = 1;
        else
            bitmap[i] = 0;
    }
    for (i = 0; i < BPB; i++)
    {
        m = 1 << (i % 8);

        if ((address[i / 8] & m) != 0)
        {
            bitmap[i] = 1;
        }
    }
}



int check_root_dir(const char *addr)
{
    struct dinode *dip = (struct dinode *)(addr + IBLOCK((uint)0) * BLOCK_SIZE);
    struct dirent *de = (struct dirent *)(addr + dip[ROOTINO].addrs[0] * BLOCK_SIZE);
    return (de[0].inum == 1 && de[1].inum == 1);
}

int check_directory_format(const char *addr, int inode_num)
{
    struct dinode *inode_start = (struct dinode *)(addr + IBLOCK((uint)0) * BLOCK_SIZE);
    if (inode_start[inode_num].type == T_DIR)
    {
        struct dirent *de = (struct dirent *)translate_address(addr, inode_start[inode_num].addrs[0]);
        char str1[] = ".";
        char str2[] = "..";
        return (de[0].inum == inode_num && (!strcmp(de[0].name, str1)) && (!strcmp(de[1].name, str2)));
    }
    return 1;
}

int check_duplicate_direct_addr(const struct dinode *dip, int nblocks)
{
    if (dip == NULL)
    {
        return 0;
    }
    int i = 0;
    int isSeen[nblocks];
    for (i = 0; i < nblocks; i++)
    {
        isSeen[i] = 0;
    }
    for (i = 0; i < NDIRECT + 1; i++)
    {
        if (dip->addrs[i] && dip->addrs[i] < nblocks)
        {
            isSeen[dip->addrs[i]]++;
        }
    }
    for (i = 0; i < nblocks; i++)
    {
        if (isSeen[i] > 1)
        {
            return 0;
        }
    }
    return 1;
}

int check_duplicate_indirect_addr(const char *baseaddr, const struct dinode *dip, int nblocks)
{
    int i = 0;
    int isSeen[nblocks];
    for (i = 0; i < nblocks; i++)
    {
        isSeen[i] = 0;
    }
    if (dip->addrs[NDIRECT])
    {
        uint *indirect_block = (uint *)translate_address(baseaddr, dip->addrs[NDIRECT]);
        int BLOCK_NUM_BYTES = 4;
        for (i = 0; i < BLOCK_SIZE / BLOCK_NUM_BYTES; i++)
        {
            if (indirect_block[i] && indirect_block[i] < nblocks)
            {
                isSeen[indirect_block[i]]++;
            }
        }
    }
    for (i = 0; i < nblocks; i++)
    {
        if (isSeen[i] > 1)
        {
            return 0;
        }
    }
    return 1;
}

void dfs_directories_recursive(const char *baseaddr, const struct dinode *dip, const struct dirent *de, int *visit, uint *refcount);
void search_direct(const char *baseaddr, const struct dinode *dip, const int block_no, int *visit, uint *refcount)
{
    if (block_no)
    {
        const struct dirent *de = (struct dirent *)translate_address(baseaddr, block_no);
        while (de->inum)
        {
            // dont re-do dfs for parent's reference
            if (strcmp(de->name, "..") && strcmp(de->name, "."))
            {
                dfs_directories_recursive(baseaddr, dip, de, visit, refcount);
            }
            de++;
        }
    }
}
void search_inode(const char *baseaddr, const struct dinode *dip, const struct dinode *current_inode, int *visit, uint *refcount)
{
    if (current_inode->type == T_DIR)
    {
        int i;
        for (i = 0; i < NDIRECT; i++)
        {
            search_direct(baseaddr, dip, current_inode->addrs[i], visit, refcount);
        }
        if (current_inode->addrs[NDIRECT])
        {
            uint *indirect_block = (uint *)translate_address(baseaddr, current_inode->addrs[NDIRECT]);
            int BLOCK_NUM_BYTES = 4;
            int i;
            for (i = 0; i < BLOCK_SIZE / BLOCK_NUM_BYTES; i++)
            {
                search_direct(baseaddr, dip, indirect_block[i], visit, refcount);
            }
        }
    }
}

void dfs_directories_recursive(const char *baseaddr, const struct dinode *dip, const struct dirent *de, int *visit, uint *refcount)
{
    refcount[de->inum]++;
    // printf("de->name:%s ,de->inum:%d,refcount%d , vis %d\n",(char*)de->name, de->inum,refcount[de->inum],visit[de->inum]);
    if (de->inum && !visit[de->inum])
    {
        visit[de->inum] = 1;
        if (dip[de->inum].type == T_DIR)
        {
            search_inode(baseaddr, dip, &dip[de->inum], visit, refcount);
        }
    }
}

void validate_inode_directory_references(const char *baseaddr)
{
    const struct dinode *dip = (struct dinode *)(baseaddr + IBLOCK((uint)0) * BLOCK_SIZE);
    struct superblock *sb = (struct superblock *)translate_address(baseaddr, 1);
    int visit[sb->ninodes];
    uint reference_counts[sb->ninodes];
    memset(visit, 0, sizeof(visit));
    memset(reference_counts, 0, sizeof(reference_counts));
    const struct dirent *de = (struct dirent *)translate_address(baseaddr, dip[ROOTINO].addrs[0]);
    // reference_counts[de->inum]--; // remove initiator count.
    dfs_directories_recursive(baseaddr, dip, de, visit, reference_counts);
    int i;
    // check9
    for (i = ROOTINO; i < sb->ninodes; i++)
    {
        if (dip[i].type != 0 && visit[i] == 0)
        {
            fprintf(stderr, "ERROR: inode marked use but not found in a directory.\n");
            exit(1);
        }
    }
    // check10
    for (i = ROOTINO; i < sb->ninodes; i++)
    {
        if (dip[i].type == 0 && visit[i] != 0)
        {
            fprintf(stderr, "ERROR: inode referred to in directory but marked free.\n");
            exit(1);
        }
    }
    // check11
    for (i = ROOTINO; i < sb->ninodes; i++)
    {
        if ((uint)dip[i].nlink != reference_counts[i])
        {
            // printf("i:%d,refcount:%d,nlink%d\n",i,reference_counts[i],dip[i].nlink);
            fprintf(stderr, "ERROR: bad reference count for file.\n");
            exit(1);
        }
    }
    // check12
    for (i = ROOTINO; i < sb->ninodes; i++)
    {
        if (dip[i].type == T_DIR && reference_counts[i] > 1)
        {
            fprintf(stderr, "ERROR: directory appears more than once in file system.\n");
            exit(1);
        }
    }
}



void validate_fs_img(char *fs_img_path)
{
    int fd = open(fs_img_path, O_RDONLY);
    if (fd < 0)
    {
        fprintf(stderr, "image not found\n");
        exit(1);
    }
    struct stat file_info;
    fstat(fd, &file_info);
    const char *addr = mmap(NULL, file_info.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED)
    {
        perror("mmap failed");
        close(fd);
        exit(1);
    }
    struct superblock *sb = (struct superblock *)translate_address(addr,1);
    const struct dinode *dip = (struct dinode *)(addr + IBLOCK((uint)0) * BLOCK_SIZE);
    int databitmap[sb->size];
    loadbitmap(addr, sb->size, databitmap);
    // check 1
    int i;
    for (i = 1; i < sb->ninodes; i++)
    {
        if (!check_inode_type(dip[i].type))
        {
            fprintf(stderr, "ERROR: bad inode.\n");
            close(fd);
            exit(1);
        }
    }
    // check 2
    for (i = 1; i < sb->ninodes; i++)
    {
        if (dip[i].type)
        {
            check_inode_addr_ranges(addr, &dip[i], sb->nblocks);
        }
    }
    // check3
    if (!check_root_dir(addr))
    {
        fprintf(stderr, "ERROR: root directory does not exist.\n");
        close(fd);
        exit(1);
    }
    // check4
    for (i = 1; i < sb->ninodes; i++)
    {
        if (!check_directory_format(addr, i))
        {
            fprintf(stderr, "ERROR: directory not properly formatted.\n");
            close(fd);
            exit(1);
        }
    }
    // check5
    for (i = 1; i < sb->ninodes; i++)
    {
        if (!check_inode_bitmap_consistency(addr, &dip[i], databitmap))
        {
            fprintf(stderr, "ERROR: address used by inode but marked free in bitmap.\n");
            close(fd);
            exit(1);
        }
    }
    // check6
    if (!check_if_datablock_falsely_marked_in_use(addr, databitmap))
    {
        fprintf(stderr, "ERROR: bitmap marks block in use but it is not in use.\n");
        close(fd);
        exit(1);
    }
    // check7
    for (i = 1; i < sb->ninodes; i++)
    {
        if (!check_duplicate_direct_addr(&dip[i], sb->nblocks))
        {
            fprintf(stderr, "ERROR: direct address used more than once.\n");
            close(fd);
            exit(1);
        }
    }
    // chech8
    for (i = 1; i < sb->ninodes; i++)
    {
        if (!check_duplicate_indirect_addr(addr, &dip[i], sb->nblocks))
        {
            fprintf(stderr, "ERROR: indirect address used more than once.\n");
            close(fd);
            exit(1);
        }
    }
    validate_inode_directory_references(addr);
    close(fd);
    if (munmap((void *)addr, file_info.st_size) == -1)
    {
        perror("Error unmapping memory");
    }
}




int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: fcheck <file_system_image>\n");
        exit(1);
    }
    validate_fs_img(argv[1]);
}