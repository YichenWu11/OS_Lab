#pragma once

/*
    Assume:
        - Only one group --> one group desc
        - Only one user
*/

#include <bitset>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <set>
#include <string>

#define VOLUME_NAME "Ext2FileSystem"

#define BLOCK_SIZE 512

#define SUPER_BLOCK_START_ADDR 0
#define SUPER_BLOCK_SIZE 32

#define GROUP_DESC_START_ADDR 512
#define GROUP_DESC_SIZE 32

#define BLOCK_BITMAP_START_ADDR 1024 // 512 * 2

#define INODE_BITMAP_START_ADDR 1536 // 512 * 3

#define INODE_TABLE_START_ADDR 2048 // 512 * 4
#define INODE_SIZE 64
#define INODE_NUM 4096

#define DATA_BLOCK_START_ADDR 264192 // 512 * 4 + 64 * 4096
#define DATA_BLOCK_NUM 4096

#define TOTAL_BLOCK_NUM 4612 // 4 + 512 + 4096

#define USER_NUM 1
#define FOPEN_TABLE_MAX 16

#define MAX_FILE_CAPACITY 4096

#define DIR_ENTRY_NUM_PER_BLOCK 32

#define DIR_ENTRY_SIZE 16

// 16 + 2 * 2 + 4 + 8 = 32 Byte
struct ext2_super_block {
    char sb_volume_name[16];
    uint32_t sb_disk_size; // uint32_t to store the large number
    uint16_t sb_size_per_block;
    uint16_t sb_blocks_per_group;
    char sb_pad[8];
};

// 16 + 2 * 6 + 4 = 32 Byte
struct ext2_group_desc {
    char bg_volume_name[16];       //卷名
    uint16_t bg_block_bitmap;      //保存块位图的块号
    uint16_t bg_inode_bitmap;      //保存索引结点位图的块号
    uint16_t bg_inode_table;       //索引结点表的起始块号
    uint16_t bg_free_blocks_count; //本组空闲块的个数
    uint16_t bg_free_inodes_count; //本组空闲索引结点的个数
    uint16_t bg_used_dirs_count;   //本组目录的个数
    char bg_pad[4];                //填充(0xff)
};

// 2 * 2 + 4 + 8 * 4 + 2 * 8 + 8 = 64 Byte
struct ext2_inode {
    uint16_t i_mode;     // 文件类型及访问权限
    uint16_t i_blocks;   // 文件的数据块个数
    uint32_t i_size;     // 大小(字节)
    uint64_t i_atime;    // 访问时间
    uint64_t i_ctime;    // 创建时间
    uint64_t i_mtime;    // 修改时间
    uint64_t i_dtime;    // 删除时间
    uint16_t i_block[8]; // 指向数据块的指针 --> [0,5](直接索引) [6](一级间接)
                         // [7](二级间接)
    char i_pad[8];       // 填充1(0xff)
};

// 2 * 3 + 1 + 9 = 16 Byte
struct ext2_dir_entry {
    uint16_t inode;
    uint16_t record_len;
    uint16_t name_len;
    char file_type;
    char name[9];
};

enum class FileType : uint8_t {
    UNKNOWN = 0,
    FILE,
    DIR,
    OTHER,
};

class Ext2 {
public:
    static Ext2& GetInstance() {
        static Ext2 instance;
        return instance;
    }

    void init();

    // load/update of super_block
    void updateSuperBlock();
    void loadSuperBlock();

    // load/update of group_desc
    void updateGroupDesc();
    void loadGroupDesc();

    // load/update of block_bitmap
    void updateBlockBitmap();
    void loadBlockBitmap();

    // load/update of inode_bitmap
    void updateInodeBitmap();
    void loadInodeBitmap();

    // load/update of inode
    void updateInode(size_t idx);
    void loadInode(size_t idx);

    // load/update of dir_entry_block
    void updateDirDataBlock(size_t idx);
    void loadDirDataBlock(size_t idx);

    // load/update of data_block
    void updateDataBlock(size_t idx);
    void loadDataBlock(size_t idx);

    // load/update of level1_index_block_buf
    void updateIndexBlock_Level1(size_t idx);
    void loadIndexBlock_Level1(size_t idx);

    // load/update of level2_index_block_buf
    void updateIndexBlock_Level2(size_t idx);
    void loadIndexBlock_Level2(size_t idx);

    // alloc and free
    size_t allocDataBlock();
    void freeDataBlock(size_t idx);

    size_t allocInode();
    void freeInode(size_t idx);

    // search
    uint16_t searchFile(char const tar[9], char file_type, uint16_t& inode_idx,
                        uint16_t& block_idx, uint16_t& dir_idx);
    uint16_t searchInTable(uint16_t inode);

    // operations
    void ext2_cd(char const tar[9]);
    void ext2_cd_impl(char const tar[9]);

    void ext2_mkdir(char const tar[9]);
    void ext2_rmdir(char const tar[9]);

    void ext2_touch(char const tar[9], FileType type = FileType::FILE);
    void ext2_rm(char const tar[9], FileType type = FileType::FILE);

    void ext2_chmod(char const tar[9]);
    void ext2_chmod_impl(char const tar[9], uint16_t mode);

    void ext2_open(char const tar[9], FileType type = FileType::FILE);
    void ext2_close(char const tar[9], FileType type = FileType::FILE);
    void ext2_read(char const tar[9]);
    void ext2_write(char const tar[9]);
    void ext2_append(char const tar[9]);

    void ext2_ls();
    void ext2_ll();
    void ext2_l_open_file();

    void showDiskInfo();

    // print
    void printCurrPath();

private:
    Ext2() {
        fp = fopen("./Ext2", "w+");
        assert(fp);
        fseek(fp, 0, SEEK_SET);
    }

    ~Ext2() {
        fclose(fp);
    }

    void init_fs();

    void preProcess(uint16_t tar, uint16_t len, FileType type,
                    std::string file_name = "");

    void inputTargetProcess(std::string);

    std::string type2Str(FileType type);
    std::string mode2Str(uint16_t mode);
    int calcDirSize(uint16_t inode);

private:
    ext2_super_block super_block[1];
    ext2_group_desc group_desc_table[1];
    ext2_inode inode_buf[1];
    uint8_t block_bit_buf[512] = {0};
    uint8_t inode_bit_buf[512] = {0};
    ext2_dir_entry dir_buf[32]; // 32 * 16 = 512 Byte
    char data_buf[BLOCK_SIZE];

    uint16_t file_open_table[FOPEN_TABLE_MAX];
    std::set<std::string> file_open_name_set;

    uint16_t level1_index_block_buf[256]; // 一级间接索引
    uint16_t level2_index_block_buf[256]; // 二级间接索引

    FILE* fp; // 用文件来模拟磁盘空间

    char current_path[256];     // 当前路径
    uint16_t current_dir{0};    // 当前目录
    uint16_t current_dirlen{0}; // 当前路径长度

    uint16_t last_alloc_block{0}; // 上次分配的数据块号
    uint16_t last_alloc_inode{0}; // 上次分配的节点号
};
