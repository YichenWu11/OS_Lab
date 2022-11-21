#include <cstring>
#include <stack>
#include <vector>

#include <Ext2.h>
#include <Log.h>

/*

*/

/*
    type:
        - r+: write and load  文件必须存在
        - w+: write and load  如果文件不存在则创建一个，如果存在则以覆盖方式写入

    filetype:
        - 0: unknown
        - 1: common file
        - 2: directory
        - 3: char device
        - 4: block device
        - 5: pipe
        - 6: socket
        - 7: symbol pointer

    filemode:
        - 高8位(high_i_mode)是目录项中文件类型码的一个拷贝
        - 低8位(low_i_mode)中的最低3位分别用来标识rwx3个属性
        - 高5位不用，用0填充
*/

#define DIR_R_MODE     0b0000001000000100
#define DIR_W_MODE     0b0000001000000010
#define DIR_RW_MODE    0b0000001000000110

#define FILE_R_MODE    0b0000000100000100
#define FILE_W_MODE    0b0000000100000010
#define FILE_X_MODE    0b0000000100000001
#define FILE_RW_MODE   0b0000000100000110
#define FILE_RWX_MODE  0b0000000100000111

#define IS_READABLE    0b0000000000000100
#define IS_WRITEABLE   0b0000000000000010
#define IS_EXEABLE     0b0000000000000001

// ***************************************************************************
// load and update

// load/update of super_block
void Ext2::updateSuperBlock() {
    fseek(fp, SUPER_BLOCK_START_ADDR, SEEK_SET);
    fwrite(super_block, SUPER_BLOCK_SIZE, 1, fp);  // 1 ---> only 1 group
    fflush(fp);
}

void Ext2::loadSuperBlock() {
    fseek(fp, SUPER_BLOCK_START_ADDR, SEEK_SET);
    fread(super_block, SUPER_BLOCK_SIZE, 1, fp);
}

// load/update of group_desc
void Ext2::updateGroupDesc() {   
    fseek(fp, GROUP_DESC_START_ADDR, SEEK_SET);
    fwrite(group_desc_table, GROUP_DESC_SIZE, 1, fp);  // 1 ---> only 1 group
    fflush(fp);
}

void Ext2::loadGroupDesc() {
    fseek(fp, GROUP_DESC_START_ADDR, SEEK_SET);
    fread(group_desc_table, GROUP_DESC_SIZE, 1, fp);
}


// load/update of block_bitmap
void Ext2::updateBlockBitmap() {   
    fseek(fp, BLOCK_BITMAP_START_ADDR, SEEK_SET);
    fwrite(block_bit_buf, BLOCK_SIZE, 1, fp);
    fflush(fp);    
}

void Ext2::loadBlockBitmap() {
    fseek(fp, BLOCK_BITMAP_START_ADDR, SEEK_SET);
    fread(block_bit_buf, BLOCK_SIZE, 1, fp);
}


// load/update of inode_bitmap
void Ext2::updateInodeBitmap() {    
    fseek(fp, INODE_BITMAP_START_ADDR, SEEK_SET);
    fwrite(inode_bit_buf, BLOCK_SIZE, 1, fp);
    fflush(fp); 
}

void Ext2::loadInodeBitmap() {
    fseek(fp, INODE_BITMAP_START_ADDR, SEEK_SET);
    fread(inode_bit_buf, BLOCK_SIZE, 1, fp);
}


// load/update of inode
void Ext2::updateInode(size_t idx) {    
    fseek(fp, INODE_TABLE_START_ADDR+idx*INODE_SIZE, SEEK_SET);
    fwrite(inode_buf, INODE_SIZE, 1, fp);
    fflush(fp);
}

void Ext2::loadInode(size_t idx) {
    fseek(fp, INODE_TABLE_START_ADDR+idx*INODE_SIZE, SEEK_SET);
    fread(inode_buf, INODE_SIZE, 1, fp);
}


// load/update of data_block
void Ext2::updateDataBlock(size_t idx) {
    fseek(fp, DATA_BLOCK_START_ADDR+idx*BLOCK_SIZE, SEEK_SET);
    fwrite(data_buf, BLOCK_SIZE, 1, fp);
    fflush(fp);
}

void Ext2::loadDataBlock(size_t idx) {
    fseek(fp, DATA_BLOCK_START_ADDR+idx*BLOCK_SIZE, SEEK_SET);
    fread(data_buf, BLOCK_SIZE, 1, fp);
}

// load/update of dir_entry_block
void Ext2::updateDirDataBlock(size_t idx) {
    fseek(fp, DATA_BLOCK_START_ADDR+idx*BLOCK_SIZE, SEEK_SET);
    fwrite(dir_buf, BLOCK_SIZE, 1, fp);
    fflush(fp);
}

void Ext2::loadDirDataBlock(size_t idx) {
    fseek(fp, DATA_BLOCK_START_ADDR+idx*BLOCK_SIZE, SEEK_SET);
    fread(dir_buf, BLOCK_SIZE, 1, fp);
}

// load/update of level1_index_block_buf
void Ext2::updateIndexBlock_Level1(size_t idx) {
    fseek(fp, DATA_BLOCK_START_ADDR+idx*BLOCK_SIZE, SEEK_SET);
    fwrite(level1_index_block_buf, BLOCK_SIZE, 1, fp);
    fflush(fp);
}

void Ext2::loadIndexBlock_Level1(size_t idx) {
    fseek(fp, DATA_BLOCK_START_ADDR+idx*BLOCK_SIZE, SEEK_SET);
    fread(level1_index_block_buf, BLOCK_SIZE, 1, fp);
}


// load/update of level2_index_block_buf
void Ext2::updateIndexBlock_Level2(size_t idx) {
    fseek(fp, DATA_BLOCK_START_ADDR+idx*BLOCK_SIZE, SEEK_SET);
    fwrite(level2_index_block_buf, BLOCK_SIZE, 1, fp);
    fflush(fp);
}

void Ext2::loadIndexBlock_Level2(size_t idx) {
    fseek(fp, DATA_BLOCK_START_ADDR+idx*BLOCK_SIZE, SEEK_SET);
    fread(level2_index_block_buf, BLOCK_SIZE, 1, fp);
}

// ***************************************************************************


// ***************************************************************************
// alloc and free

size_t Ext2::allocDataBlock() {
    uint16_t idx  = last_alloc_block;
    uint8_t  mask = 0b10000000;
    int      flag = 0;
    
    if (group_desc_table[0].bg_free_blocks_count == 0) {
        ERROR("No Free Block to be allocated!!!\n");
        return static_cast<size_t>(-1);
    }

    loadBlockBitmap();

    idx /= 8;  // 除以8得到块的idx

    // 找空闲块
    while (block_bit_buf[idx] == 0b11111111)
        idx = (idx + 1) % DIR_ENTRY_NUM_PER_BLOCK;

    // find a bit with the value 1
    while (block_bit_buf[idx] & mask) {
        mask >>= 1;
        ++flag;
    }

    block_bit_buf[idx] = block_bit_buf[idx] + mask;
    updateBlockBitmap();

    last_alloc_block   = idx*8 + flag;

    group_desc_table[0].bg_free_blocks_count--;
    updateGroupDesc();

    return last_alloc_block;
}

void   Ext2::freeDataBlock(size_t idx) {
    uint16_t bit_idx;
    bit_idx =  idx / 8;  // 除以8得到块idx
    loadBlockBitmap();

    // bitset 的 index 是从最低位到最高位
    std::bitset<8> target = block_bit_buf[bit_idx];
    target.flip(7 - idx % 8);
    block_bit_buf[bit_idx] = target.to_ulong();

    updateBlockBitmap();

    group_desc_table[0].bg_free_blocks_count++;
    updateGroupDesc();
}

size_t Ext2::allocInode() {
    uint16_t idx  = last_alloc_inode;
    uint8_t  mask = 0b10000000;
    int      flag = 0;

    if (group_desc_table[0].bg_free_inodes_count == 0) {
        ERROR("No Free Inode to be allocated!!!\n");
        return static_cast<size_t>(-1);
    }

    loadInodeBitmap();

    idx /= 8;  // 除以8得到块idx

    while (inode_bit_buf[idx] == 0b11111111) //先看该字节的8个位是否已经填满
        idx = (idx + 1) % DIR_ENTRY_NUM_PER_BLOCK;

    // find a bit with the value 1
    while (inode_bit_buf[idx] & mask) {
        mask >>= 1;
        ++flag;
    }

    inode_bit_buf[idx] = inode_bit_buf[idx] + mask;
    updateInodeBitmap();

    last_alloc_inode   = idx*8 + flag;

    group_desc_table[0].bg_free_inodes_count--;
    updateGroupDesc();

    return last_alloc_inode;
}

void   Ext2::freeInode(size_t idx) {
    uint16_t bit_idx;
    bit_idx =  idx / 8;
    loadInodeBitmap();

    std::bitset<8> target = inode_bit_buf[bit_idx];
    target.flip(7 - idx % 8);
    inode_bit_buf[bit_idx] = target.to_ulong();

    updateInodeBitmap();

    group_desc_table[0].bg_free_inodes_count++;
    updateGroupDesc();
}

// ***************************************************************************


// ***************************************************************************
// init

void Ext2::init() {
    memset(&file_open_table,  0, sizeof(file_open_table));
    strcpy(current_path, "[yichenwu11@path: /");
    loadSuperBlock();
    if(strcmp(super_block[0].sb_volume_name, VOLUME_NAME)) {
        init_fs();
    }
}

void Ext2::init_fs() {
    INFO("Init the File System now...\n");
    // clear
    memset(&super_block,      0, sizeof(super_block));
    memset(&group_desc_table, 0, sizeof(group_desc_table));
    memset(&inode_buf,        0, sizeof(inode_buf));
    memset(&block_bit_buf,    0, sizeof(block_bit_buf));
    memset(&inode_bit_buf,    0, sizeof(inode_bit_buf));
    memset(&dir_buf,          0, sizeof(dir_buf));
    memset(&data_buf,         0, sizeof(data_buf));
    memset(&file_open_table,  0, sizeof(file_open_table));

    // clear the whole disk
    for (int i = 0; i < TOTAL_BLOCK_NUM; ++i) fwrite(data_buf, BLOCK_SIZE, 1, fp);

    loadSuperBlock();
    loadGroupDesc();

    strcpy(&(super_block[0].sb_volume_name[0]), VOLUME_NAME);
    super_block[0].sb_disk_size = BLOCK_SIZE*TOTAL_BLOCK_NUM;
    super_block[0].sb_size_per_block = BLOCK_SIZE;
    super_block[0].sb_blocks_per_group = TOTAL_BLOCK_NUM;
    updateSuperBlock();

    strcpy(&(group_desc_table[0].bg_volume_name[0]), VOLUME_NAME);
    group_desc_table[0].bg_block_bitmap = BLOCK_BITMAP_START_ADDR; 
    group_desc_table[0].bg_inode_bitmap = INODE_BITMAP_START_ADDR; 
    group_desc_table[0].bg_inode_table  = INODE_TABLE_START_ADDR;   
    group_desc_table[0].bg_free_blocks_count = DATA_BLOCK_NUM;
    group_desc_table[0].bg_free_inodes_count = INODE_NUM; 
    group_desc_table[0].bg_used_dirs_count   = 0;
    updateGroupDesc();

    loadInode(0);          // 0号 inode给根目录

    inode_buf[0].i_mode   = DIR_RW_MODE;  // rw
    inode_buf[0].i_blocks = 0;
    inode_buf[0].i_size   = DIR_ENTRY_SIZE * 2; // . and ..
    inode_buf[0].i_atime  = 0;
    inode_buf[0].i_ctime  = 0;
    inode_buf[0].i_mtime  = 0;
    inode_buf[0].i_dtime  = 0;
    inode_buf[0].i_block[0] = allocDataBlock();  // index 0
    inode_buf[0].i_blocks++;

    current_dir = allocInode();
    updateInode(current_dir); 

    // init dir
    loadDirDataBlock(0);   // 0号 dataBlock 给根目录

    dir_buf[0].inode = current_dir; // 根目录的 . 和 .. 都是根目录
    dir_buf[0].name_len  = 1;
    dir_buf[0].file_type = 2; // directory
    strcpy(dir_buf[0].name, ".");

    dir_buf[1].inode = current_dir;
    dir_buf[1].name_len  = 1;
    dir_buf[1].file_type = 2; // directory
    strcpy(dir_buf[1].name, "..");
    updateDirDataBlock(inode_buf[0].i_block[0]);  // 根目录

    INFO("Init Done!!!\n");
    INFO("-------------------------------------\n");
}

// ***************************************************************************


// ***************************************************************************
// search

void Ext2::inputTargetProcess(std::string) {

}

// search file in current dir
uint16_t Ext2::searchFile(
    char const tar[9], 
    char file_type, 
    uint16_t& inode_idx,
    uint16_t& block_idx, 
    uint16_t& dir_idx)
{
    loadInode(current_dir);

    for (uint16_t b_idx = 0; b_idx < inode_buf[0].i_blocks; ++b_idx) {
        loadDirDataBlock(inode_buf[0].i_block[b_idx]);
        for (uint16_t d_idx = 0; d_idx < DIR_ENTRY_NUM_PER_BLOCK; ++d_idx) {
            if (dir_buf[d_idx].file_type != file_type || strcmp(dir_buf[d_idx].name, tar)) 
                continue;
            inode_idx = dir_buf[d_idx].inode;
            block_idx = b_idx;
            dir_idx   = d_idx;
            return 1;
        }
    }

    return 0;
}

// search file in file_open_table
uint16_t Ext2::searchInTable(uint16_t inode) {
    uint16_t f_idx = 0;
    while (f_idx < FOPEN_TABLE_MAX && file_open_table[f_idx++] != inode);
    return (f_idx == FOPEN_TABLE_MAX) ? 0 : 1;
}

// ***************************************************************************


// ***************************************************************************
// ext2_operations

void Ext2::ext2_cd(char const tar[9]) {
    std::string target(tar);
    std::string space_delimiter = "/";
    std::vector<std::string> inputs{};

    size_t pos = 0;
    while ((pos = target.find(space_delimiter)) != std::string::npos) {
        inputs.push_back(target.substr(0, pos));
        target.erase(0, pos + space_delimiter.length());
    }
    inputs.push_back(target.substr(0, pos));

    for (auto &ele : inputs)
        ext2_cd_impl(ele.data());
}

void Ext2::ext2_cd_impl(char const tar[9]) {
    uint16_t inode_idx, block_idx, dir_idx, is_find;

    is_find = searchFile(tar, 2, inode_idx, block_idx, dir_idx);

    if(is_find) {
        current_dir = inode_idx;
        // case 1 : 
        if (!strcmp(tar, "..") && dir_buf[dir_idx - 1].name_len && dir_buf[dir_idx - 1].inode) {
            current_path[strlen(current_path) - dir_buf[dir_idx - 1].name_len - 1] = '\0';
            current_dirlen = dir_buf[dir_idx].name_len;
        }
        // case 2 : .
        else if (!strcmp(tar, ".")) {
        	return;
        }
        // case 3 : xxx
        else if (strcmp(tar, "..")) {
            current_dirlen=strlen(tar);
            strcat(current_path, tar);
            strcat(current_path, "/");
        }
    }
    else {
    	WARN("directory not exsit!!!\n");
    }    
}

void Ext2::preProcess(uint16_t tar_node, uint16_t len, FileType type, std::string file_name) {
    loadInode(tar_node);

    if(type == FileType::DIR) {
        // dir
        inode_buf[0].i_size     = DIR_ENTRY_SIZE * 2;
        inode_buf[0].i_blocks   = 1;
        inode_buf[0].i_block[0] = allocDataBlock();

        loadDirDataBlock(inode_buf[0].i_block[0]);

        dir_buf[0].inode = tar_node;
        dir_buf[0].name_len = len;
        dir_buf[1].inode = current_dir;
        dir_buf[1].name_len = current_dirlen;
        dir_buf[0].file_type = dir_buf[1].file_type = 2;

        for(int i = 2; i < DIR_ENTRY_NUM_PER_BLOCK; ++i) dir_buf[i].inode = 0;

        strcpy(dir_buf[0].name, ".");
        strcpy(dir_buf[1].name, "..");

        updateDirDataBlock(inode_buf[0].i_block[0]);

        // 目录的权限默认为 可读可写
        inode_buf[0].i_mode = DIR_RW_MODE;
    }
    else {
        // file
        inode_buf[0].i_size   = 0;
        inode_buf[0].i_blocks = 0;
        inode_buf[0].i_size   = 0;
        inode_buf[0].i_atime = 0;
        inode_buf[0].i_ctime = 0;   	
        inode_buf[0].i_mtime = 0;   	
        inode_buf[0].i_dtime = 0;
        // 根据文件后缀设置 mode
        size_t pos = 0;
        pos = file_name.find(".");
        if (pos == std::string::npos) {
            inode_buf[0].i_mode = FILE_X_MODE;
        }
        else {
            file_name.erase(0, pos + 1);
            if (file_name == "txt" || file_name == "md")
                inode_buf[0].i_mode = FILE_RW_MODE;
            else if (file_name == "exe")
                inode_buf[0].i_mode = FILE_X_MODE;
            else
                inode_buf[0].i_mode = FILE_R_MODE;
        }
    }

    updateInode(tar_node);
}

void Ext2::ext2_mkdir(char const tar[9]) {
    uint16_t tar_node, idx1, idx2, idx3, flag;

    loadInode(current_dir);
    if (!searchFile(tar, 2, idx1, idx2, idx3))
    {
        if (inode_buf[0].i_size == 4096) {
            WARN("Current Directory Buffer has been full!!!\n");
            return;
        }
        flag = 1;
        if (inode_buf[0].i_size != inode_buf[0].i_blocks*BLOCK_SIZE) 
        {
            idx1 = 0;
            while(flag && idx1 < inode_buf[0].i_blocks)
            {
                loadDirDataBlock(inode_buf[0].i_block[idx1]);
                // 从 2 开始是因为根目录下 . 和 .. 的根目录 inode 为 0
                idx2 = 2;
                while (idx2 < DIR_ENTRY_NUM_PER_BLOCK)
                {
                    if(dir_buf[idx2].inode == 0)
                    {
                        flag = 0; 
                        break;
                    }
                    ++idx2;
                }
                ++idx1;
            }

            tar_node = dir_buf[idx2].inode = allocInode();

            dir_buf[idx2].name_len  = strlen(tar);
            dir_buf[idx2].file_type = static_cast<uint8_t>(FileType::DIR);
            strcpy(dir_buf[idx2].name, tar);

            // 因为 idx1 多加了一次，所以在这里要减去 1
            updateDirDataBlock(inode_buf[0].i_block[idx1 - 1]);
        }
        else {
            // 需要分配新的数据块来存储目录项
            inode_buf[0].i_block[inode_buf[0].i_blocks] = allocDataBlock();
            inode_buf[0].i_blocks++;
            loadDirDataBlock(inode_buf[0].i_block[inode_buf[0].i_blocks - 1]);

            tar_node = dir_buf[0].inode = allocInode();
            dir_buf[0].name_len = strlen(tar);
            dir_buf[0].file_type = static_cast<uint8_t>(FileType::DIR);
            strcpy(dir_buf[0].name,tar);
           
            for(int i = 1 ; i < DIR_ENTRY_NUM_PER_BLOCK; ++i)
            {
            	dir_buf[i].inode = 0;
            }
            updateDirDataBlock(inode_buf[0].i_block[inode_buf[0].i_blocks - 1]);
        }

        inode_buf[0].i_size += DIR_ENTRY_SIZE;  // the size of the new dir_entry

        updateInode(current_dir);
        
        preProcess(tar_node, strlen(tar), FileType::DIR);
        INFO("Dir Create Success!!!\n");
    }
    else {
        WARN("Directory has alloady existed!\n");
    }
}

// remove directory
void Ext2::ext2_rmdir(char const tar[9]) {
    uint16_t tar_node, inode_idx, block_idx, dir_idx;

    if (!strcmp(tar, "..") || !strcmp(tar,".")) {
        WARN(". and .. can not be deleted!\n");
        return;
    }

    loadInode(current_dir);

    if (searchFile(tar, 2, inode_idx, block_idx, dir_idx)) {
        loadInode(inode_idx);
        // 如果该目录下只有 . 和 ..
        if (inode_buf[0].i_size == DIR_ENTRY_SIZE * 2) {
            inode_buf[0].i_size   = 0;
            inode_buf[0].i_blocks = 0;

            freeDataBlock(inode_buf[0].i_block[0]);
            loadInode(current_dir);
            loadDirDataBlock(inode_buf[0].i_block[block_idx]);
            freeInode(dir_buf[dir_idx].inode);
            dir_buf[dir_idx].inode = 0;
            dir_buf[dir_idx].file_type = 0;
            dir_buf[dir_idx].name_len = 0;
            memset(&(dir_buf[dir_idx].name), 0, sizeof(dir_buf[dir_idx].name));
            updateDirDataBlock(inode_buf[0].i_block[block_idx]);
            inode_buf[0].i_size -= DIR_ENTRY_SIZE;  // free a dir_entry

            for (int m = 0; m < inode_buf[0].i_blocks; ++m) {
                loadDirDataBlock(inode_buf[0].i_block[m]);
                int empty_num = 0;
                for (int n = 0; n < DIR_ENTRY_NUM_PER_BLOCK; ++n) 
                    if (!dir_buf[n].inode && !dir_buf[n].name_len) ++empty_num;

                if (empty_num == DIR_ENTRY_NUM_PER_BLOCK) {
                    freeDataBlock(inode_buf[0].i_block[m]);
                    inode_buf[0].i_blocks--;
                    for (int k = m; k < inode_buf[0].i_blocks - 1; ++k) {
                        inode_buf[0].i_block[k] = inode_buf[0].i_block[k + 1];
                    }
                }
            }

            updateInode(current_dir);
            INFO("Remove Dir Success!!!\n");
            return;
        }
        else {
            int tmp = current_dir;
            int i;
            i = (current_dir == 0) ? 2 : 0;
            for (; i < inode_buf[0].i_blocks; ++i) {
                loadDirDataBlock(inode_buf[0].i_block[i]);
                for(int m = 0; m < DIR_ENTRY_NUM_PER_BLOCK; ++m)
                {
                    if(!strcmp(dir_buf[m].name, ".") || !strcmp(dir_buf[m].name, "..") || dir_buf[m].inode == 0)
                        continue;
                    if(dir_buf[m].file_type == 2)
                    {
                        strcpy(current_path, tar);
                        current_dir = inode_idx;
                        ext2_rmdir(dir_buf[m].name);
                    }
                    else if(dir_buf[m].file_type == 1)
                    {
                        current_dir = inode_idx;
                        ext2_rm(dir_buf[m].name);
                    }
                }
                if(inode_buf[0].i_size == DIR_ENTRY_SIZE * 2)
                {
                    current_dir = tmp;
                    ext2_rmdir(tar);
                }
            }
            current_dir = tmp;
            INFO("Remove Dir Success!!!\n");
            return;
        }   
    }
    else {
        WARN("Directory does not exsit!!!\n");
    }
}

// 创建文件
void Ext2::ext2_touch(char const tar[9], FileType type) {
    uint16_t inode_idx, block_idx, dir_idx;
    uint16_t tar_node;
    loadInode(current_dir);
    if (!searchFile(tar, static_cast<uint8_t>(type), inode_idx, block_idx, dir_idx)) {
        if (inode_buf->i_size == 4096) {
            WARN("Current Directory has no free block to allocated!!!\n");
        }
        int flag = 1;
        if (inode_buf[0].i_size != inode_buf[0].i_blocks*BLOCK_SIZE) {
            int i = 0;
            int j = 0;
            while(flag && i < inode_buf[0].i_blocks) {
                loadDirDataBlock(inode_buf[0].i_block[i]);
                j = 2;
                while(j < DIR_ENTRY_NUM_PER_BLOCK) {
                    if(dir_buf[j].inode == 0) {
                        flag = 0;
                        break;
                    }
                    ++j;
                }
                ++i;
            }
            tar_node = dir_buf[j].inode = allocInode();
            dir_buf[j].name_len  = strlen(tar);
            dir_buf[j].file_type = static_cast<uint8_t>(type);
            strcpy(dir_buf[j].name, tar);
            updateDirDataBlock(inode_buf[0].i_block[i-1]);
        }
        else {
            inode_buf[0].i_block[inode_buf[0].i_blocks] = allocDataBlock();
            inode_buf[0].i_blocks++;
            loadDirDataBlock(inode_buf[0].i_block[inode_buf[0].i_blocks-1]);

            tar_node = dir_buf[0].inode = allocInode();

            dir_buf[0].name_len  = strlen(tar);
            dir_buf[0].file_type = static_cast<uint8_t>(type);
            strcpy(dir_buf[0].name, tar);

            for(int i = 1; i < DIR_ENTRY_NUM_PER_BLOCK; ++i) dir_buf[flag].inode=0;

            updateDirDataBlock(inode_buf[0].i_block[inode_buf[0].i_blocks-1]);
        }
        inode_buf[0].i_size += DIR_ENTRY_SIZE;  // a new dir_entry
        updateInode(current_dir);
        
        preProcess(tar_node, strlen(tar), type, tar);

        INFO("File Touch Success!!!\n");
    }
    else {
        WARN("The file has already existed!!!\n");
    }
}

// 删除文件
// type: 默认为 1 <COMMON FILE>
void Ext2::ext2_rm(char const tar[9], FileType type) {
    uint16_t inode_idx, block_idx, dir_idx;
    if (searchFile(tar, static_cast<uint8_t>(type), inode_idx, block_idx, dir_idx)) {
        loadInode(inode_idx);

        if (searchInTable(inode_idx)) 
            ext2_close(tar, type);

        for (int i = 0; i < inode_buf[0].i_blocks; ++i)
            freeDataBlock(inode_buf[0].i_block[i]);

        memset(&(inode_buf[0]), 0, sizeof(inode_buf[0]));
        freeInode(inode_idx);
        
        loadInode(current_dir);
        loadDirDataBlock(inode_buf[0].i_block[block_idx]);

        memset(&(dir_buf[dir_idx]), 0, sizeof(dir_buf[dir_idx]));
        updateDirDataBlock(inode_buf[0].i_block[block_idx]);

        inode_buf[0].i_size -= DIR_ENTRY_SIZE;

        for (int i = 1; i < inode_buf[0].i_blocks; ++i) {
            bool is_empty = false;
            int  e_idx = 0;
            loadDirDataBlock(inode_buf[0].i_block[i]);
            for (int j = 0; j < DIR_ENTRY_NUM_PER_BLOCK; ++j) {
                if (dir_buf[j].inode) break;
                if (j == DIR_ENTRY_NUM_PER_BLOCK - 1) is_empty = true;
            }

            if (is_empty) {
                freeDataBlock(inode_buf[0].i_block[i]);
                for (int k = i; k < inode_buf[0].i_blocks - 1; ++k) {
                    inode_buf[0].i_block[k] = inode_buf[0].i_block[k + 1];
                }
            }
        }
        updateInode(current_dir);
        INFO("Remove File Success!!!\n");
    }
    else {
        WARN("This file does not exsit!!!\n");
    }
}

void Ext2::ext2_open(char const tar[9], FileType type) {
    uint16_t inode_idx, block_idx, dir_idx;

    if (searchFile(tar, static_cast<uint8_t>(type), inode_idx, block_idx, dir_idx)) {
        if (searchInTable(inode_idx))
            INFO("This file has been open!!!\n");
        else {
            for (int i = 0; i < FOPEN_TABLE_MAX; ++i) {
                if (file_open_table[i] == 0) {
                    file_open_table[i] = inode_idx;
                    file_open_name_set.emplace(dir_buf[dir_idx].name);
                    INFO("File Open Success!!!\n");
                    return;
                }
            }
            WARN("file_open_table has been full!!!\n");
        }
    }
    else {
        WARN("This file does not exsit!!!\n");
    }
}

void Ext2::ext2_close(char const tar[9], FileType type) {
    uint16_t inode_idx, block_idx, dir_idx;
    if (searchFile(tar, static_cast<uint8_t>(type), inode_idx, block_idx, dir_idx)) {
        if (searchInTable(inode_idx))
            for (int i = 0; i < FOPEN_TABLE_MAX; ++i) {
                if (file_open_table[i] == inode_idx) {
                    file_open_table[i] = 0;
                    file_open_name_set.erase(file_open_name_set.find(dir_buf[dir_idx].name));
                    INFO("File Close Success!!!\n");
                    return;
                }
            }
        else {
            WARN("File has not been open!!!\n");
        }
    }
    else {
        WARN("This file does not exsit!!!\n");
    }
}

void Ext2::ext2_read(char const tar[9]) {
    uint16_t node_idx, block_idx, dir_idx;
    if (!searchFile(tar, 1, node_idx, block_idx, dir_idx)) {
        WARN("This file doesn't exsit in curr directory!!!\n");
        return;
    }
    if (!searchInTable(node_idx)) {
        WARN("This file has not been open, open first before read/write!!!\n");
        return;
    }

    loadInode(node_idx);

    if (inode_buf[0].i_mode & IS_READABLE) {
        if (inode_buf[0].i_blocks == 0) {
            WARN("This file is empty!!!\n");
            return;
        }

        bool is_level1_use = inode_buf[0].i_blocks >= 7;
        bool is_level2_use = inode_buf[0].i_blocks >= 8;

        if (is_level1_use) {
            // 0 ----- 5 (6 个直接索引的数据块)
            for (int i = 0; i < 6; ++i) {
                loadDataBlock(inode_buf[0].i_block[i]);
                for (int j = 0; j < BLOCK_SIZE; ++j) {
                    if (data_buf[j] == '\0') break;
                    printf("%c", data_buf[j]);
                }
            }
            // 6 ----> 一级间接索引的数据块
            loadIndexBlock_Level1(inode_buf[0].i_block[6]);
            for (int i = 0; i < 32; ++i) {
                loadDataBlock(level1_index_block_buf[i]);
                for (int j = 0; j < BLOCK_SIZE; ++j) {
                    if (data_buf[j] == '\0') break;
                    printf("%c", data_buf[j]);
                }
            }

            // 如果存在二级间接索引
            if (is_level2_use) {
                // 7 ----> 二级间接索引的数据块
                loadIndexBlock_Level2(inode_buf[0].i_block[7]);
                for (int i = 0; i < 32; ++i) {
                    loadIndexBlock_Level1(level2_index_block_buf[i]);
                    for (int j = 0; j < 32; ++j) {
                        loadDataBlock(level1_index_block_buf[j]);
                        for (int k = 0; k < BLOCK_SIZE; ++k) {
                            if (data_buf[k] == '\0') break;
                            printf("%c", data_buf[k]);
                        }
                    }
                }
            }
            else {
                printf("\n");
            }
        }
        else {
            for (int i = 0; i < inode_buf[0].i_blocks; ++i) {
                loadDataBlock(inode_buf[0].i_block[i]);
                for (int j = 0; j < BLOCK_SIZE; ++j) {
                    if (data_buf[j] == '\0') break;
                    printf("%c", data_buf[j]);
                }
            }
            printf("\n");
        }
    }
    else {
        WARN("Read this file is not allowed!!!\n");
    }
}

// 覆盖写入
void Ext2::ext2_write(char const tar[9]) {
    uint16_t node_idx, block_idx, dir_idx;
    if (!searchFile(tar, 1, node_idx, block_idx, dir_idx)) {
        WARN("This file doesn't exsit in curr directory!!!\n");
        return;
    }
    if (!searchInTable(node_idx)) {
        WARN("This file has not been open, open first before read/write!!!\n");
        return;
    }

    loadInode(node_idx);

    if (inode_buf[0].i_mode & IS_READABLE) {
        // input
        size_t tmp_idx = 0;
        std::string tmp;

        INFO("Input `$$` to end: \n\n");

        char is_end[2];

        while(1) {
            tmp.push_back(getchar());
            is_end[tmp_idx%2] = tmp[tmp_idx];
            // 暂定为读到 `$$` 结束
            if (is_end[0] == '$' && is_end[1] == '$') {
                tmp.pop_back();
                tmp.pop_back();
                tmp.push_back('\0');
                break;
            }
            if (tmp_idx >= MAX_FILE_CAPACITY - 1) {
                WARN("The Max file capacity is 4096!!!\n");
                break;
            }
            ++tmp_idx;
        }

        std::cin.get();
        
        int need_block_num = 
            (tmp.length() % BLOCK_SIZE) ?  (tmp.length() / BLOCK_SIZE + 1) : (tmp.length() / BLOCK_SIZE);

        // free first
        bool is_level1_use = inode_buf[0].i_blocks >= 7;
        bool is_level2_use = inode_buf[0].i_blocks >= 8;

        if (is_level1_use) {
            // free direct
            for (int i = 0; i < 6; ++i) 
                freeDataBlock(inode_buf[0].i_block[i]);
            loadIndexBlock_Level1(inode_buf[0].i_block[6]);
            for (int i = 0; i < 32; ++i) 
                freeDataBlock(level1_index_block_buf[i]);
            
            if (is_level2_use) {
                loadIndexBlock_Level2(inode_buf[0].i_block[7]);
                for (int i = 0; i < 32; ++i) {
                    loadIndexBlock_Level1(level2_index_block_buf[i]);
                    for (int j = 0; j < 32; ++j) 
                        freeDataBlock(level1_index_block_buf[j]);
                }
            }
        }
        else {
            for (int i = 0; i < inode_buf[0].i_blocks; ++i) 
                freeDataBlock(inode_buf[0].i_block[i]);
        }

        inode_buf[0].i_blocks = need_block_num;

        bool is_level1_need = need_block_num >= (6  + 1);
        bool is_level2_need = need_block_num >= (38 + 1);

        if (is_level1_need) {
            memset(&level1_index_block_buf, 0, sizeof(level1_index_block_buf));
            // alloc
            for (int i = 0; i < 7; ++i)
                inode_buf[0].i_block[i] = allocDataBlock();
            need_block_num -= 6;
            int to_alloc_num = (is_level2_need) ? 32 : need_block_num;
            for (int i = 0; i < to_alloc_num; ++i) {
                level1_index_block_buf[i] = allocDataBlock();
            }
            updateIndexBlock_Level1(inode_buf[0].i_block[6]);
            need_block_num -= to_alloc_num;
            
            if (is_level2_need) {
                memset(&level2_index_block_buf, 0, sizeof(level2_index_block_buf));

                // alloc
                inode_buf[0].i_block[7] = allocDataBlock();

                int to_alloc_level2_num = need_block_num / 32 + 1;

                for (int i = 0; i < to_alloc_level2_num; ++i) {
                    level2_index_block_buf[i] = allocDataBlock();
                    int to_alloc_blk_num = (need_block_num <= 32) ? need_block_num : 32;
                    loadIndexBlock_Level1(level2_index_block_buf[i]);
                    for (int j = 0; j <= need_block_num; ++j) {
                        level1_index_block_buf[j] = allocDataBlock();
                        need_block_num--;
                    }
                    updateIndexBlock_Level1(level2_index_block_buf[i]);
                }
                updateIndexBlock_Level2(inode_buf[0].i_block[7]);

                // write (需要一级间接和二级间接索引)

                // 直接和一级间接部分
                int glo_idx = 0;
                for (int i = 0; i < 6; ++i) {
                    loadDataBlock(inode_buf[0].i_block[i]);
                    memcpy(data_buf, tmp.data()+glo_idx*BLOCK_SIZE, BLOCK_SIZE);
                    updateDataBlock(inode_buf[0].i_block[i]);
                    ++glo_idx;
                }
                loadIndexBlock_Level1(inode_buf[0].i_block[6]);
                for (int i = 0; i < 32; ++i) {
                    loadDataBlock(level1_index_block_buf[i]);
                    memcpy(data_buf, tmp.data()+glo_idx*BLOCK_SIZE, BLOCK_SIZE);
                    updateDataBlock(level1_index_block_buf[i]);
                    ++glo_idx;
                }
                updateIndexBlock_Level1(inode_buf[0].i_block[6]);

                // 二级间接部分
                loadIndexBlock_Level2(inode_buf[0].i_block[7]);
                for (int i = 0; i < 32; ++i) {
                    if (!level2_index_block_buf[i]) {
                        loadIndexBlock_Level1(level2_index_block_buf[i]);
                        for (int j = 0; j < 32; ++j) {
                            if (!level1_index_block_buf[i]) {
                                if (j == 31 || !level1_index_block_buf[j+1])
                                    memcpy(data_buf, tmp.data()+glo_idx*BLOCK_SIZE, tmp.length()-i*BLOCK_SIZE);
                                else
                                   memcpy(data_buf, tmp.data()+glo_idx*BLOCK_SIZE, BLOCK_SIZE);
                                glo_idx++;
                            }
                        }
                        updateIndexBlock_Level1(level2_index_block_buf[i]);
                    }
                }
                updateIndexBlock_Level2(inode_buf[0].i_block[7]);
            }
            else {
                // write (需要一级间接索引)  
                int glo_idx = 0;
                for (int i = 0; i < 6; ++i) {
                    loadDataBlock(inode_buf[0].i_block[i]);
                    memcpy(data_buf, tmp.data()+glo_idx*BLOCK_SIZE, BLOCK_SIZE);
                    updateDataBlock(inode_buf[0].i_block[i]);
                    ++glo_idx;
                }
                loadIndexBlock_Level1(inode_buf[0].i_block[6]);
                for (int i = 0; i < 32; ++i) {
                    if (!level1_index_block_buf[i]) {
                        loadDataBlock(level1_index_block_buf[i]);
                        if (i == 31 || !level1_index_block_buf[i+1]) {
                            memcpy(data_buf, tmp.data()+glo_idx*BLOCK_SIZE, tmp.length()-i*BLOCK_SIZE);
                        }
                        else {
                            memcpy(data_buf, tmp.data()+glo_idx*BLOCK_SIZE, BLOCK_SIZE);
                        }
                        updateDataBlock(level1_index_block_buf[i]);
                        ++glo_idx;
                    }
                }
                updateIndexBlock_Level1(inode_buf[0].i_block[6]);
            }
        }
        else {
            // alloc
            inode_buf[0].i_blocks = need_block_num;
            for (int i = 0; i < need_block_num; ++i)
                inode_buf[0].i_block[i] = allocDataBlock();
            // write (不需要间接索引)
            for (int i = 0; i < inode_buf[0].i_blocks; ++i) {
                loadDataBlock(inode_buf[0].i_block[i]);
                if (i == inode_buf[0].i_blocks - 1)
                    memcpy(data_buf, tmp.data()+i*BLOCK_SIZE, tmp.length()-i*BLOCK_SIZE);
                else    
                    memcpy(data_buf, tmp.data()+i*BLOCK_SIZE, BLOCK_SIZE);
                updateDataBlock(inode_buf[0].i_block[i]);
            }
        }

        updateInode(node_idx);
    }
    else {
        WARN("Write to this file is not allowed!!!\n");
    }
}

// TODO: 追加文件内容而不是重写
void Ext2::ext2_append(char const tar[9]) {

}

std::string Ext2::type2Str(FileType type) {
    switch (type)
    {
        case FileType::UNKNOWN:
            return "<UNKNOWN>";
        case FileType::FILE:
            return "<FILE>";
        case FileType::DIR:
            return "<DIR>";
        default:
            return "<>";
    }
}

std::string Ext2::mode2Str(uint16_t mode) {
    std::string res;
    if (mode & IS_READABLE) 
        res.push_back('r');
    else
        res.push_back('_');
    if (mode & IS_WRITEABLE) 
        res.push_back('w');
    else
        res.push_back('_');
    if (mode & IS_EXEABLE) 
        res.push_back('x');
    else
        res.push_back('_');
    return res;
}

int Ext2::calcDirSize(uint16_t inode) {
    int res = 0;
    loadInode(inode);
    int blocks = inode_buf[0].i_blocks;

    res += inode_buf[0].i_blocks * BLOCK_SIZE;

    for (int idx = 0; idx < blocks; ++idx) {
        loadDirDataBlock(inode_buf[0].i_block[idx]);
        for (int jdx = 2; jdx < DIR_ENTRY_NUM_PER_BLOCK; ++jdx) {
            if (dir_buf[jdx].name_len) {
                if (dir_buf[jdx].file_type == static_cast<uint8_t>(FileType::DIR)) {
                    res += calcDirSize(dir_buf[jdx].inode);
                }
                else {
                    loadInode(dir_buf[jdx].inode);
                    res += inode_buf[0].i_blocks * BLOCK_SIZE;
                }
            }
        }
    }
    return res;
}

void Ext2::ext2_ll() {
    uint16_t inode_idx, b_idx = 0, d_idx;
    loadInode(current_dir);
    int blocks = inode_buf[0].i_blocks;
    while (b_idx < blocks) {
        uint16_t tatget_dir = inode_buf[0].i_block[b_idx];
        loadDirDataBlock(tatget_dir);
        d_idx = 0;
        while (d_idx < DIR_ENTRY_NUM_PER_BLOCK) {
            loadDirDataBlock(tatget_dir);
            if (d_idx < 2 || dir_buf[d_idx].name_len) {
                loadInode(dir_buf[d_idx].inode);
                printf("\033[0m\033[1;32m%-9s \033[0m", dir_buf[d_idx].name);
                printf("\033[0m\033[1;32m%-7s \033[0m", type2Str(FileType(dir_buf[d_idx].file_type)).c_str());
                printf("\033[0m\033[1;32m%-5s \033[0m", mode2Str(inode_buf[0].i_mode).c_str());
                if (dir_buf[d_idx].file_type == static_cast<uint8_t>(FileType::DIR)) {
                    printf("\033[0m\033[1;32m%dB\n\033[0m", calcDirSize(dir_buf[d_idx].inode));
                }
                else {
                    printf("\033[0m\033[1;32m%dB\n\033[0m", inode_buf[0].i_blocks * BLOCK_SIZE);
                }
            }
            ++d_idx;
        }
        ++b_idx;
    }
}

void Ext2::ext2_ls() {
    uint16_t inode_idx, b_idx = 0, d_idx;
    loadInode(current_dir);
    while (b_idx < inode_buf[0].i_blocks) {
        loadDirDataBlock(inode_buf[0].i_block[b_idx]);
        d_idx = 2;
        // 32 : 512 Byte / 16 Byte (size of dir_entry)
        while (d_idx < DIR_ENTRY_NUM_PER_BLOCK) {    
            if (dir_buf[d_idx].name_len) {
                if (dir_buf[d_idx].file_type == 2)
                    INFO(dir_buf[d_idx].name);
                else
                    printf("%s", dir_buf[d_idx].name);
                printf(" ");
            }    
            ++d_idx;
        }
        ++b_idx;
    }
    printf("\n");
}

void Ext2::ext2_l_open_file() {
    for (const auto& n : file_open_name_set) {
        INFO(n.c_str());
        INFO(" ");
    }
    INFO("\n");
}

void Ext2::ext2_chmod(char const tar[9]) {
    INFO("Input the mode you want to change: such as r__, rwx, __x:\n");

    std::string mode;

    std::cin >> mode;

    std::cin.get();

    uint16_t output = 0b0000000100000000;
    if (mode.length() > 3)
        ERROR("Mode Input Error!!! Please check your input!!!\n");
    for (int i = 0; i < mode.length(); ++i) {
        if (mode[i] == 'r') {
            output = output | 0b0000000100000100;
        }
        else if (mode[i] == 'w') {
            output = output | 0b0000000100000010;
        }
        else if (mode[i] == 'x') {
            output = output | 0b0000000100000001;
        }
        else if (mode[i] == '_') {
            // do nothing
        }
        else {
            ERROR("Mode Input Error!!! Please check your input!!!\n");
        }
    }
    ext2_chmod_impl(tar, output);
}


void Ext2::ext2_chmod_impl(char const tar[9], uint16_t mode) {
    uint16_t inode_idx, block_idx, dir_idx;

    if (searchFile(tar, static_cast<uint8_t>(FileType::FILE), inode_idx, block_idx, dir_idx)) {
        loadInode(inode_idx);
        inode_buf[0].i_mode = mode;
        updateInode(inode_idx);
        INFO("Success!!!\n");
    }
    else {
        WARN("This file does not exsit!!!\n");
    }    
}

// ***************************************************************************

void Ext2::showDiskInfo() {
    loadGroupDesc();

    printf("\033[0m\033[1;32mVolume Name      : %s\n\033[0m", group_desc_table[0].bg_volume_name);
    printf("\033[0m\033[1;32mFree Block Count : %d\n\033[0m", group_desc_table[0].bg_free_blocks_count);
    printf("\033[0m\033[1;32mFree INode Count : %d\n\033[0m", group_desc_table[0].bg_free_inodes_count);
}

void Ext2::printCurrPath() {
    printf("\033[0m\033[3;38m%s]\n\033[0m", Ext2::GetInstance().current_path);
    INFO("> ");
}
