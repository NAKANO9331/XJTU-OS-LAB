#include <stdio.h>
#include "string.h"
#include "stdlib.h"
#include "time.h"
#include <sys/ioctl.h>
#include <sys/file.h>
#include <termios.h>
#include <unistd.h> // 用于 STDIN_FILENO

// 文件系统相关宏定义
#define blocks 4611            // 总块数 (1+1+1+512+4096)
#define blocksiz 512           // 每块大小 (字节数)
#define inodesiz 64            // 索引节点大小 (字节数)
#define data_begin_block 515   // 数据块起始块号
#define dirsiz 32              // 目录项长度 (字节数)
#define EXT2_NAME_LEN 15       // 文件名最大长度
#define PATH "MY_DISK"           // 虚拟磁盘文件路径

// 组描述符结构体，描述文件系统组的相关信息，占 68 字节
typedef struct ext2_group_desc {
    char bg_volume_name[16];  // 卷名
    int bg_block_bitmap;      // 块位图所在块号
    int bg_inode_bitmap;      // 索引节点位图所在块号
    int bg_inode_table;       // 索引节点表起始块号
    int bg_free_blocks_count; // 组内空闲块数
    int bg_free_inodes_count; // 组内空闲索引节点数
    int bg_used_dirs_count;   // 组内目录数
    char password[16];             // 文件系统密码
    char bg_pad[36];          // 填充 
} ext2_group_desc;

// 索引节点结构体，描述文件或目录的元数据，占 64 字节
typedef struct ext2_inode {
    int i_mode;     // 文件类型和访问权限 (1: 普通文件, 2: 目录)
    int i_blocks;   // 文件内容占用的数据块数量
    int i_size;     // 文件大小 (字节)
    time_t i_atime; // 文件最近访问时间
    time_t i_ctime; // 文件创建时间
    time_t i_mtime; // 文件最近修改时间
    time_t i_dtime; // 文件删除时间
    int i_block[8]; // 数据块指针数组 (包括直接索引和间接索引)
    char i_pad[36]; // 填充 
} ext2_inode;

// 目录项结构体，描述目录中的一个文件或子目录，占 32 字节
typedef struct ext2_dir_entry {
    int inode;                // 关联的索引节点号
    int rec_len;              // 目录项长度
    int name_len;             // 文件名长度
    int file_type;            // 文件类型 (1: 普通文件, 2: 目录)
    char name[EXT2_NAME_LEN]; // 文件名
    char dir_pad;             // 填充
} ext2_dir_entry;

// 全局变量定义
ext2_group_desc group_desc;      // 组描述符实例
ext2_inode inode;                // 索引节点实例
ext2_dir_entry dir;              // 目录项实例 (存储文件或目录的元数据)
FILE *f;                         // 文件指针 (用于文件系统操作)
unsigned int last_allco_inode = 0; // 上次分配的索引节点号
unsigned int last_allco_block = 0; // 上次分配的数据块号

/**********第一部分**********/
/**********初始化模拟文件系统程序设计**********/

/*从文件系统中读取根目录的 inode 数据，并将其存储到 cu 指针所指的 ext2_inode 结构体中。*/
int initialize(ext2_inode *cu)
{
    f = fopen(PATH, "r+");                          // 打开文件以读写模式操作
    fseek(f, 3 * blocksiz, 0);                      // 定位到第 3 个块，读取根目录的 inode
    fread(cu, sizeof(ext2_inode), 1, f);            // 将数据读入 cu 指针所指的结构体
    fclose(f);                                      // 关闭文件
    return 0;
}

/*捕获用户输入字符，不显示输入内容。*/
int getch() {
    int ch; // 用于保存读取的字符
    struct termios oldt, newt; // 终端参数结构体，用于保存和修改终端设置

    // 获取当前终端的参数设置，并保存到 `oldt` 中
    // STDIN_FILENO 是标准输入（文件描述符 0）
    tcgetattr(STDIN_FILENO, &oldt);
    // 复制当前终端设置到 `newt`，准备修改
    newt = oldt;
    // 修改终端设置：
    // 关闭回显（ECHO）: 输入字符不会在终端显示
    // 关闭规范模式（ICANON）：输入字符不需要按 Enter 键即可读取
    newt.c_lflag &= ~(ECHO | ICANON);
    // 将修改后的参数立即生效到终端，TCSANOW 表示立即生效
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    // 从标准输入读取单个字符
    ch = getchar();
    // 恢复原始终端设置（如启用回显和规范模式）
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    // 返回用户输入的字符
    return ch;
}

/*退出显示函数，输出感谢信息并退出*/
void exitdisplay()
{
    printf("感谢使用~ 拜拜捏！\n");
    return;
}

int format(ext2_inode *current);
/*初始化文件系统,如果文件系统初始化成功，返回 0;如果文件系统初始化失败，返回 1*/
int initfs(ext2_inode *cu)
{
    f = fopen(PATH, "r+");
    if (f == NULL) // 如果文件系统文件不存在
    {
        char ch; // 用于存储用户输入的命令
        int i;
        printf("未找到文件系统。是否要创建一个新的文件系统？\n[Y/N]");
        i = 1;
        while (i) // 循环等待用户输入
        {
            scanf("%c", &ch); // 获取用户输入
            switch (ch)
            {
            case 'Y':
            case 'y': // 用户选择创建新文件系统
                if (format(cu) != 0) // 格式化文件系统
                    return 1; // 格式化失败，返回 1
                f = fopen(PATH, "r"); // 打开文件
                i = 0; // 停止循环
                break;
            case 'N':
            case 'n': // 用户选择不创建文件系统
                exitdisplay(); // 显示退出信息
                return 1; // 返回 1，表示初始化失败
            default:
                printf("无效命令，请重新输入\n"); // 输入无效命令，提示用户
                break;
            }
        }
    }

    // 如果文件存在，读取文件系统信息
    fseek(f, 0, SEEK_SET);
    fread(&group_desc, sizeof(ext2_group_desc), 1, f); // 读取组描述符
    fseek(f, 3 * blocksiz, SEEK_SET);
    fread(&inode, sizeof(ext2_inode), 1, f); // 读取根目录的索引节点
    fclose(f);

    initialize(cu); // 初始化当前目录
    return 0; // 返回 0，表示初始化成功
}

/**********第二部分**********/
/**********文件系统级函数及其子函数设计**********/

/*查找空闲索引节点*/
int FindInode()
{
    FILE *fp = NULL;
    unsigned int zero[blocksiz / 4]; // 用于保存inode位图
    int i;
    while (fp == NULL)
        fp = fopen(PATH, "r+"); // 打开文件系统
    fseek(fp, 2 * blocksiz, SEEK_SET); // 定位到inode位图
    fread(zero, blocksiz, 1, fp); // 读取inode位图

    for (i = last_allco_inode; i < (last_allco_inode + blocksiz / 4); i++) // 遍历inode位图
    {
        if (zero[i % (blocksiz / 4)] != 0xffffffff) // 检查是否有空闲的inode
        {
            unsigned int j = 0x80000000, k = zero[i % (blocksiz / 4)], l = i;
            for (i = 0; i < 32; i++) // 遍历32位，找到空闲的位
            {
                if (!(k & j)) // 判断当前位是否空闲
                {
                    zero[l % (blocksiz / 4)] |= j; // 将空闲位置为已占用
                    group_desc.bg_free_inodes_count -= 1; // 更新组描述符中的空闲inode计数
                    fseek(fp, 0, 0); // 定位到组描述符块
                    fwrite(&group_desc, sizeof(ext2_group_desc), 1, fp); // 写回组描述符

                    fseek(fp, 2 * blocksiz, SEEK_SET); // 定位到inode位图
                    fwrite(zero, blocksiz, 1, fp); // 更新inode位图

                    last_allco_inode = l % (blocksiz / 4); // 记录最后分配的inode
                    fclose(fp);
                    return l % (blocksiz / 4) * 32 + i; // 返回空闲inode编号
                }
                else
                    j /= 2; // 检查下一位
            }
        }
    }
    fclose(fp);
    return -1; // 没有空闲inode
}

/*查找空闲块*/
int FindBlock()
{
    FILE *fp = NULL;
    unsigned int zero[blocksiz / 4]; // 用于保存块位图
    int i;
    while (fp == NULL)
        fp = fopen(PATH, "r+"); // 打开文件系统
    fseek(fp, 1 * blocksiz, SEEK_SET); // 定位到块位图
    fread(zero, blocksiz, 1, fp); // 读取块位图

    for (i = last_allco_block; i < (last_allco_block + blocksiz / 4); i++) // 遍历块位图
    {
        if (zero[i % (blocksiz / 4)] != 0xffffffff) // 检查是否有空闲块
        {
            unsigned int j = 0x80000000, k = zero[i % (blocksiz / 4)], l = i;
            for (i = 0; i < 32; i++) // 遍历32位，找到空闲的位
            {
                if (!(k & j)) // 判断当前位是否空闲
                {
                    zero[l % (blocksiz / 4)] |= j; // 将空闲位置为已占用
                    group_desc.bg_free_blocks_count -= 1; // 更新组描述符中的空闲块计数
                    fseek(fp, 0, 0); // 定位到组描述符块
                    fwrite(&group_desc, sizeof(ext2_group_desc), 1, fp); // 写回组描述符

                    fseek(fp, 1 * blocksiz, SEEK_SET); // 定位到块位图
                    fwrite(zero, blocksiz, 1, fp); // 更新块位图

                    last_allco_block = l % (blocksiz / 4); // 记录最后分配的块
                    fclose(fp);
                    return l % (blocksiz / 4) * 32 + i; // 返回空闲块编号
                }
                else
                    j /= 2; // 检查下一位
            }
        }
    }
    fclose(fp);
    return -1; // 没有空闲块
}

// 删除指定的 inode 节点，并更新 inode 位图
void DelInode(int len) // len 是 inode 号
{
    unsigned int zero[blocksiz / 4], i;
    int j;
    f = fopen(PATH, "r+"); // 打开文件系统路径
    fseek(f, 2 * blocksiz, SEEK_SET); // 定位到 inode 位图的起始位置
    fread(zero, blocksiz, 1, f); // 读取 inode 位图到 zero 数组
    i = 0x80000000; // 用于按位操作的初始值
    for (j = 0; j < len % 32; j++) 
        i = i / 2; // 将 i 右移，定位到对应的位
    zero[len / 32] = zero[len / 32] ^ i; // 通过按位异或清除指定 inode
    fseek(f, 2 * blocksiz, SEEK_SET); // 定位回 inode 位图的起始位置
    fwrite(zero, blocksiz, 1, f); // 写回更新后的 inode 位图
    fclose(f); // 关闭文件
}

// 删除指定的数据块，并更新块位图
void DelBlock(int len)
{
    unsigned int zero[blocksiz / 4], i;
    int j;
    f = fopen(PATH, "r+"); // 打开文件系统路径
    fseek(f, 1 * blocksiz, SEEK_SET); // 定位到块位图的起始位置
    fread(zero, blocksiz, 1, f); // 读取块位图到 zero 数组
    i = 0x80000000; // 用于按位操作的初始值
    for (j = 0; j < len % 32; j++) 
        i = i / 2; // 将 i 右移，定位到对应的位
    zero[len / 32] = zero[len / 32] ^ i; // 通过按位异或清除指定块
    fseek(f, 1 * blocksiz, SEEK_SET); // 定位回块位图的起始位置
    fwrite(zero, blocksiz, 1, f); // 写回更新后的块位图
    fclose(f); // 关闭文件
}

// 增加一个数据块到当前文件中，支持直接索引、一级索引和二级索引
void add_block(ext2_inode *current, int i, int j) // i 表示数据块序号，j 是新分配的数据块号
{
    FILE *fp = NULL;
    while (fp == NULL)
        fp = fopen(PATH, "r+"); // 打开文件系统路径

    if (i < 6) // 使用直接索引
    {
        current->i_block[i] = j; // 将新数据块号直接写入 i_block 数组
    }
    else
    {
        i = i - 6; // 索引号减去直接索引的数量
        if (i == 0) // 为一级索引分配新数据块
        {
            current->i_block[6] = FindBlock(); // 分配一个新的数据块存储一级索引
            fseek(fp, data_begin_block * blocksiz + current->i_block[6] * blocksiz, SEEK_SET);
            fwrite(&j, sizeof(int), 1, fp); // 写入第一个数据块号
        }
        else if (i < 128) // 一级索引中分配数据块
        {
            fseek(fp, data_begin_block * blocksiz + current->i_block[6] * blocksiz + i * 4, SEEK_SET);
            fwrite(&j, sizeof(int), 1, fp); // 写入对应数据块号
        }
        else // 二级索引
        {
            i = i - 128; // 索引号减去一级索引的数量
            if (i == 0) // 分配二级索引块
            {
                current->i_block[7] = FindBlock(); // 分配一个新的二级索引块
                fseek(fp, data_begin_block * blocksiz + current->i_block[7] * blocksiz, SEEK_SET);
                i = FindBlock(); // 为二级索引块分配一个新的数据块
                fwrite(&i, sizeof(int), 1, fp); 
                fseek(fp, data_begin_block * blocksiz + i * blocksiz, SEEK_SET);
                fwrite(&j, sizeof(int), 1, fp); // 写入数据块号
            }
            if (i % 128 == 0) // 当前索引需要分配新的块
            {
                fseek(fp, data_begin_block * blocksiz + current->i_block[7] * blocksiz + i / 128 * 4, SEEK_SET);
                i = FindBlock(); // 分配新的块
                fwrite(&i, sizeof(int), 1, fp); 
                fseek(fp, data_begin_block * blocksiz + i * blocksiz, SEEK_SET);
                fwrite(&j, sizeof(int), 1, fp); // 写入数据块号
            }
            else // 二级索引中已存在块
            {
                fseek(fp, data_begin_block * blocksiz + current->i_block[7] * blocksiz + i / 128 * 4, SEEK_SET);
                fread(&i, sizeof(int), 1, fp); // 读取二级索引块号
                fseek(fp, data_begin_block * blocksiz + i * blocksiz + i % 128 * 4, SEEK_SET);
                fwrite(&j, sizeof(int), 1, fp); // 写入数据块号
            }
        }
    }
}

// 返回目录的存储位置偏移量，每个目录项占 32 字节
int dir_entry_position(int dir_entry_begin, int i_block[8]) // dir_entry_begin 表示目录项的相对起始字节
{
    int dir_blocks = dir_entry_begin / 512;   // 目录项跨越的块数
    int block_offset = dir_entry_begin % 512; // 当前块内的字节偏移量
    int a; // 临时变量用于存储索引值
    FILE *fp = NULL;

    if (dir_blocks <= 5) // 如果目录项在直接索引块中
    {
        return data_begin_block * blocksiz + i_block[dir_blocks] * blocksiz + block_offset;
    }
    else // 如果目录项在间接索引块中
    {
        while (fp == NULL) // 确保文件成功打开
            fp = fopen(PATH, "r+");

        dir_blocks -= 6; // 减去直接索引块数量

        if (dir_blocks < 128) // 一级间接索引处理（128 为单块可容纳的索引数）
        {
            fseek(fp, data_begin_block * blocksiz + i_block[6] * blocksiz + dir_blocks * 4, SEEK_SET);
            fread(&a, sizeof(int), 1, fp);
            return data_begin_block * blocksiz + a * blocksiz + block_offset;
        }
        else // 二级间接索引处理
        {
            dir_blocks -= 128; // 减去一级间接索引块数量

            // 定位二级间接索引块并读取其值
            fseek(fp, data_begin_block * blocksiz + i_block[7] * blocksiz + (dir_blocks / 128) * 4, SEEK_SET);
            fread(&a, sizeof(int), 1, fp);

            // 定位对应的一级间接索引块并读取最终地址
            fseek(fp, data_begin_block * blocksiz + a * blocksiz + (dir_blocks % 128) * 4, SEEK_SET);
            fread(&a, sizeof(int), 1, fp);

            return data_begin_block * blocksiz + a * blocksiz + block_offset;
        }

        fclose(fp); // 关闭文件
    }
}

// 为当前目录寻找一个空目录条目位置并返回绝对地址
int FindEntry(ext2_inode *current)
{
    FILE *fout = NULL;
    int location;       // 条目的绝对位置
    int block_location; // 块号
    int temp;           // 每个block可以存放的INT数量
    int remain_block;   // 剩余块数
    location = data_begin_block * blocksiz; // 从数据块起始位置开始计算
    temp = blocksiz / sizeof(int); // 计算每个块可以存放多少个INT
    fout = fopen(PATH, "r+"); // 以读写模式打开文件
    if (current->i_size % blocksiz == 0) // 如果当前目录的大小是块的整数倍，说明当前块已满，需要增加一个新块
    {
        add_block(current, current->i_blocks, FindBlock()); // 增加一个新的数据块
        current->i_blocks++; // 更新块计数
    }
    if (current->i_blocks < 6) // 前6个块为直接索引块
    {
        location += current->i_block[current->i_blocks - 1] * blocksiz; // 计算当前目录条目的位置
        location += current->i_size % blocksiz; // 在当前块内找到位置
    }
    else if (current->i_blocks < temp + 5) // 一级索引
    {
        block_location = current->i_block[6]; // 获取一级索引的块号
        fseek(fout, (data_begin_block + block_location) * blocksiz + (current->i_blocks - 6) * sizeof(int), SEEK_SET); 
        fread(&block_location, sizeof(int), 1, fout); // 读取块号
        location += block_location * blocksiz; // 更新位置
        location += current->i_size % blocksiz; // 在该块内找到位置
    }
    else // 二级索引
    {
        block_location = current->i_block[7]; // 获取二级索引的块号
        remain_block = current->i_blocks - 6 - temp; // 计算剩余块数
        fseek(fout, (data_begin_block + block_location) * blocksiz + (int)((remain_block - 1) / temp + 1) * sizeof(int), SEEK_SET); 
        fread(&block_location, sizeof(int), 1, fout); // 读取二级索引块号
        remain_block = remain_block % temp; // 计算剩余块数
        fseek(fout, (data_begin_block + block_location) * blocksiz + remain_block * sizeof(int), SEEK_SET); 
        fread(&block_location, sizeof(int), 1, fout); // 读取块号
        location += block_location * blocksiz; // 更新位置
        location += current->i_size % blocksiz + dirsiz; // 在该块内找到位置并加上目录条目大小
    }
    current->i_size += dirsiz; // 更新当前目录的大小
    fclose(fout); // 关闭文件
    return location; // 返回找到的空目录条目的位置
}

/*登录函数，用于验证输入的密码与存储的密码是否匹配，如果密码匹配，则返回 0；如果密码不匹配，则返回非零值*/
int login()
{
    char psw[16]; // 用于存储输入的密码
    printf("请输入密码（原始密码为9331）：");
    scanf("%s", psw); // 输入密码
    return strcmp(group_desc.password, psw); // 比较输入的密码与存储的密码
}

int Open(ext2_inode *current, char *name);//声明Open函数

/*获取当前目录的目录名，参数 cs_name_name：用于存储获取到的目录名，参数 node：当前目录的索引节点*/
void getstring(char *cs_name, ext2_inode node)
{
    ext2_inode current = node; // 当前目录节点
    int i, j;
    ext2_dir_entry dir; // 目录项
    f = fopen(PATH, "r+"); // 打开文件系统

    // 打开父目录
    Open(&current, ".."); // current指向父目录（上级目录）

    // 查找当前目录中的"."（表示当前目录）项，获取其对应的索引节点
    for (i = 0; i < node.i_size / 32; i++)
    {
        fseek(f, dir_entry_position(i * 32, node.i_block), SEEK_SET); // 定位到当前目录项
        fread(&dir, sizeof(ext2_dir_entry), 1, f); // 读取目录项
        if (!strcmp(dir.name, ".")) // 如果目录项为"."，即当前目录
        {
            j = dir.inode; // 保存该目录项对应的索引节点
            break;
        }
    }

    // 查找父目录中与当前目录相对应的目录项，获取其名称
    for (i = 0; i < current.i_size / 32; i++)
    {
        fseek(f, dir_entry_position(i * 32, current.i_block), SEEK_SET); // 定位到父目录项
        fread(&dir, sizeof(ext2_dir_entry), 1, f); // 读取目录项
        if (dir.inode == j) // 如果该目录项的索引节点与当前目录相同
        {
            strcpy(cs_name, dir.name); // 将目录名复制到传入的字符串 cs_name 中
            return; // 返回
        }
    }
}

/**********第三部分**********/
/**********命令层函数的设计**********/

/*打开指定目录并将其设置为当前目录,current 指向新打开的当前目录（ext2_inode）*/
int Open(ext2_inode *current, char *name)
{
    FILE *fp = NULL;
    int i;

    while (fp == NULL) // 确保文件成功打开
        fp = fopen(PATH, "r+");

    for (i = 0; i < (current->i_size / 32); i++) // 遍历当前目录中的所有目录项
    {
        // 定位当前目录项的存储位置
        fseek(fp, dir_entry_position(i * 32, current->i_block), SEEK_SET);
        fread(&dir, sizeof(ext2_dir_entry), 1, fp);

        if (!strcmp(dir.name, name)) // 匹配目标目录名
        {
            if (dir.file_type == 2) // 如果是目录类型
            {
                // 读取目标目录的索引节点信息
                fseek(fp, 3 * blocksiz + dir.inode * sizeof(ext2_inode), SEEK_SET);
                fread(current, sizeof(ext2_inode), 1, fp);
                fclose(fp); // 关闭文件
                return 0;   // 打开成功
            }
        }
    }

    fclose(fp); // 关闭文件
    return 1;   // 打开失败
}

/*关闭当前目录,仅更新最后访问时间，返回上一目录作为新的当前目录*/
int Close(ext2_inode *current)
{
    time_t now;
    ext2_dir_entry parent_entry; // 父目录项信息
    FILE *fout;

    fout = fopen(PATH, "r+"); // 打开文件进行读写
    time(&now); // 获取当前时间

    current->i_atime = now; // 更新最后访问时间

    // 定位并读取当前目录对应的目录项
    fseek(fout, (data_begin_block + current->i_block[0]) * blocksiz, SEEK_SET);
    fread(&parent_entry, sizeof(ext2_dir_entry), 1, fout);

    // 更新索引节点信息到文件系统
    fseek(fout, 3 * blocksiz + (parent_entry.inode) * sizeof(ext2_inode), SEEK_SET);
    fwrite(current, sizeof(ext2_inode), 1, fout);

    fclose(fout); // 关闭文件

    // 打开父目录并设置为当前目录
    return Open(current, "..");
}

/*从当前目录中读取文件内容，name为文件名*/
int Read(ext2_inode *current, char *name) {
    FILE *fp = NULL;
    int i;
    while (fp == NULL)
        fp = fopen(PATH, "r+"); // 打开文件系统

    int fd = fileno(fp);
    if (flock(fd, LOCK_SH) == -1) { // 添加共享锁
        perror("无法加读锁");
        fclose(fp);
        return -1;
    }

    for (i = 0; i < (current->i_size / 32); i++) {
        fseek(fp, dir_entry_position(i * 32, current->i_block), SEEK_SET);
        fread(&dir, sizeof(ext2_dir_entry), 1, fp);
        if (!strcmp(dir.name, name)) {
            if (dir.file_type == 1) {
                time_t now;
                ext2_inode node;
                char content_char;
                fseek(fp, 3 * blocksiz + dir.inode * sizeof(ext2_inode), SEEK_SET);
                fread(&node, sizeof(ext2_inode), 1, fp);

                for (i = 0; i < node.i_size; i++) {
                    fseek(fp, dir_entry_position(i, node.i_block), SEEK_SET);
                    fread(&content_char, sizeof(char), 1, fp);
                    if (content_char == 0xD)
                        printf("\n");
                    else
                        printf("%c", content_char);
                }
                printf("\n");

                time(&now);
                node.i_atime = now;
                fseek(fp, 3 * blocksiz + dir.inode * sizeof(ext2_inode), SEEK_SET);
                fwrite(&node, sizeof(ext2_inode), 1, fp);

                flock(fd, LOCK_UN); // 释放锁
                fclose(fp);
                return 0;
            }
        }
    }

    flock(fd, LOCK_UN); // 释放锁
    fclose(fp);
    return 1; // 文件未找到
}


/*向目录 'current' 中的文件 'name' 写入数据。如果该文件在目录中不存在，则提示用户先创建文件*/
int Write(ext2_inode *current, char *name) {
    FILE *fp = NULL;
    ext2_dir_entry dir;
    ext2_inode node;
    time_t now;
    char str;
    int i;

    while (fp == NULL)
        fp = fopen(PATH, "r+");

    int fd = fileno(fp);
    if (flock(fd, LOCK_EX) == -1) { // 添加独占锁
        perror("无法加写锁");
        fclose(fp);
        return -1;
    }

    while (1) {
        for (i = 0; i < (current->i_size / 32); i++) {
            fseek(fp, dir_entry_position(i * 32, current->i_block), SEEK_SET);
            fread(&dir, sizeof(ext2_dir_entry), 1, fp);
            if (!strcmp(dir.name, name)) {
                if (dir.file_type == 1) {
                    fseek(fp, 3 * blocksiz + dir.inode * sizeof(ext2_inode), SEEK_SET);
                    fread(&node, sizeof(ext2_inode), 1, fp);
                    break;
                }
            }
        }
        if (i < current->i_size / 32)
            break;

        printf("该文件不存在，请先创建文件\n");
        flock(fd, LOCK_UN); // 释放锁
        fclose(fp);
        return 0;
    }

    str = getch();
    while (str != 27) {
        printf("%c", str);

        if (!(node.i_size % 512)) {
            add_block(&node, node.i_size / 512, FindBlock());
            node.i_blocks += 1;
        }

        fseek(fp, dir_entry_position(node.i_size, node.i_block), SEEK_SET);
        fwrite(&str, sizeof(char), 1, fp);

        node.i_size += sizeof(char);

        if (str == 0x0d)
            printf("%c", 0x0a);

        str = getch();
        if (str == 27)
            break;
    }

    time(&now);
    node.i_mtime = now;
    node.i_atime = now;

    fseek(fp, 3 * blocksiz + dir.inode * sizeof(ext2_inode), SEEK_SET);
    fwrite(&node, sizeof(ext2_inode), 1, fp);

    flock(fd, LOCK_UN); // 释放锁
    fclose(fp);
    printf("\n");
    return 0;
}


/*创建目录，type=1 创建文件、type=2 创建目录、current 当前目录的索引节点、name 文件名或目录名*/
int Create(int type, ext2_inode *current, char *name)
{
    FILE *fout = NULL;
    int i;
    int block_location;     // block location
    int node_location;      // node location
    int dir_entry_location; // dir entry location
    time_t now;
    ext2_inode ainode;
    ext2_dir_entry aentry, bentry; // bentry保存当前系统的目录体信息
    time(&now);
    fout = fopen(PATH, "r+");
    node_location = FindInode(); // 寻找空索引

    // 检查是否存在重复文件或目录名称
    for (i = 0; i < current->i_size / dirsiz; i++)
    {
        fseek(fout, dir_entry_position(i * sizeof(ext2_dir_entry), current->i_block), SEEK_SET);
        fread(&aentry, sizeof(ext2_dir_entry), 1, fout);
        if (aentry.file_type == type && !strcmp(aentry.name, name))
            return 1;
    }

    fseek(fout, (data_begin_block + current->i_block[0]) * blocksiz, SEEK_SET);
    fread(&bentry, sizeof(ext2_dir_entry), 1, fout); // current's dir_entry
    if (type == 1)  //文件
    {
        ainode.i_mode = 1;
        ainode.i_blocks = 0; //文件暂无内容
        ainode.i_size = 0;   //初始文件大小为 0
        ainode.i_atime = now;
        ainode.i_ctime = now;
        ainode.i_mtime = now;
        ainode.i_dtime = 0;
        for (i = 0; i < 8; i++)
        {
            ainode.i_block[i] = 0;
        }
        for (i = 0; i < 24; i++)
        {
            ainode.i_pad[i] = (char)(0xff);
        }
    }
    else //目录
    {
        ainode.i_mode = 2;   //目录
        ainode.i_blocks = 1; //目录 当前和上一目录
        ainode.i_size = 64;  //初始大小 32*2=64 //一旦新建一个目录，该目录下就有"."和".."
        ainode.i_atime = now;
        ainode.i_ctime = now;
        ainode.i_mtime = now;
        ainode.i_dtime = 0;
        block_location = FindBlock();
        ainode.i_block[0] = block_location;
        for (i = 1; i < 8; i++)
        {
            ainode.i_block[i] = 0;
        }
        for (i = 0; i < 24; i++)
        {
            ainode.i_pad[i] = (char)(0xff);
        }
        //当前目录
        aentry.inode = node_location;
        aentry.rec_len = sizeof(ext2_dir_entry);
        aentry.name_len = 1;
        aentry.file_type = 2;
        strcpy(aentry.name, ".");
        printf("创建了.dir\n");
        aentry.dir_pad = 0;
        fseek(fout, (data_begin_block + block_location) * blocksiz, SEEK_SET);
        fwrite(&aentry, sizeof(ext2_dir_entry), 1, fout);
        //上一级目录
        aentry.inode = bentry.inode;
        aentry.rec_len = sizeof(ext2_dir_entry);
        aentry.name_len = 2;
        aentry.file_type = 2;
        strcpy(aentry.name, "..");
        aentry.dir_pad = 0;
        fwrite(&aentry, sizeof(ext2_dir_entry), 1, fout);
        printf("创建了..dir\n");
        //一个空条目
        aentry.inode = 0;
        aentry.rec_len = sizeof(ext2_dir_entry);
        aentry.name_len = 0;
        aentry.file_type = 0;
        aentry.name[EXT2_NAME_LEN] = 0;
        aentry.dir_pad = 0;
        fwrite(&aentry, sizeof(ext2_dir_entry), 14, fout); //清空数据块
    }                                                      // end else
    //保存新建inode
    fseek(fout, 3 * blocksiz + (node_location) * sizeof(ext2_inode), SEEK_SET);
    fwrite(&ainode, sizeof(ext2_inode), 1, fout);
    // 将新建inode 的信息写入current 指向的数据块
    aentry.inode = node_location;
    aentry.rec_len = dirsiz;
    aentry.name_len = strlen(name);
    if (type == 1)
    {
        aentry.file_type = 1;
    } //文件
    else
    {
        aentry.file_type = 2;
    } //目录
    strcpy(aentry.name, name);
    aentry.dir_pad = 0;
    dir_entry_location = FindEntry(current);
    fseek(fout, dir_entry_location, SEEK_SET); //定位条目位置
    fwrite(&aentry, sizeof(ext2_dir_entry), 1, fout);

    //保存current 的信息,bentry 是current 指向的block 中的第一条
    //ext2_inode cinode;
    fseek(fout, 3 * blocksiz + (bentry.inode) * sizeof(ext2_inode), SEEK_SET);

    // fread(&cinode, sizeof(ext2_inode), 1, fout);
    // printf("after_cinode.i_size: %d\n", cinode.i_size);

    fwrite(current, sizeof(ext2_inode), 1, fout);
    fclose(fout);
    return 0;
}

/*在当前目录删除目录或文件*/
int Delete(int type, ext2_inode *current, char *name)
{
    FILE *fout = NULL;
    int i, j, t, k, flag;
    int node_location, dir_entry_location, block_location, e_location;
    int block_location2, block_location3;
    ext2_inode cinode;
    ext2_dir_entry centry, dentry, eentry;
    // 初始化删除条目的空结构
    dentry.inode = 0;
    dentry.rec_len = sizeof(ext2_dir_entry);
    dentry.name_len = 0;
    dentry.file_type = 0;
    strcpy(dentry.name, "");
    dentry.dir_pad = 0;

    fout = fopen(PATH, "r+");
    t = (int)(current->i_size / dirsiz); // 计算当前目录条目数量
    flag = 0; // 用于标记是否找到目标文件或目录

    // 查找目录项，定位到目标文件或目录
    for (i = 0; i < t; i++)
    {
        dir_entry_location = dir_entry_position(i * dirsiz, current->i_block);
        fseek(fout, dir_entry_location, SEEK_SET);
        fread(&centry, sizeof(ext2_dir_entry), 1, fout);
        if ((strcmp(centry.name, name) == 0) && (centry.file_type == type))
        {
            flag = 1;
            j = i;
            break;
        }
    }
    // 如果找到了目标文件或目录
    if (flag)
    {
        node_location = centry.inode;  // 获取inode号
        fseek(fout, 3 * blocksiz + node_location * sizeof(ext2_inode), SEEK_SET); // 定位到inode位置
        fread(&cinode, sizeof(ext2_inode), 1, fout); // 读取inode信息

        // 删除目录
        if (type == 2)
        {
            while (cinode.i_size > 2 * dirsiz) // 删除目录中的内容，至少保留当前目录项和"."目录项
            {
                fseek(fout, dir_entry_position(cinode.i_size - dirsiz, cinode.i_block), SEEK_SET);
                fread(&eentry, sizeof(ext2_dir_entry), 1, fout);    
                Delete(eentry.file_type, &cinode, eentry.name); // 递归删除子目录或文件
            }

            // 删除当前目录的块和inode
            DelBlock(cinode.i_block[0]);
            DelInode(node_location);

            // 更新当前目录条目，删除目录项
            dir_entry_location = dir_entry_position(current->i_size - dirsiz, current->i_block);
            fseek(fout, dir_entry_location, SEEK_SET);
            fread(&centry, dirsiz, 1, fout); // 读取最后一条目录项
            fseek(fout, dir_entry_location, SEEK_SET);
            fwrite(&dentry, dirsiz, 1, fout); // 清空该位置

            // 释放多余的数据块
            dir_entry_location -= data_begin_block * blocksiz;
            if (dir_entry_location % blocksiz == 0)
            {
                DelBlock(dir_entry_location / blocksiz);
                current->i_blocks--;
                if (current->i_blocks == 6)
                    DelBlock(current->i_block[6]);
                else if (current->i_blocks == (blocksiz / sizeof(int) + 6))
                {
                    int a;
                    fseek(fout, data_begin_block * blocksiz + current->i_block[7] * blocksiz, SEEK_SET);
                    fread(&a, sizeof(int), 1, fout);
                    DelBlock(a);
                    DelBlock(current->i_block[7]);
                }
                else if (!((current->i_blocks - 6 - blocksiz / sizeof(int)) % (blocksiz / sizeof(int))))
                {
                    int a;
                    fseek(fout, data_begin_block * blocksiz + current->i_block[7] * blocksiz + ((current->i_blocks - 6 - blocksiz / sizeof(int)) / (blocksiz / sizeof(int))), SEEK_SET);
                    fread(&a, sizeof(int), 1, fout);
                    DelBlock(a);
                }
            }
            current->i_size -= dirsiz;

            // 如果删除的条目不是最后一条，用最后一条目录项覆盖删除项
            if (j * dirsiz < current->i_size)
            {
                dir_entry_location = dir_entry_position(j * dirsiz, current->i_block);
                fseek(fout, dir_entry_location, SEEK_SET);
                fwrite(&centry, dirsiz, 1, fout);
            }
            printf("目录 %s 被删掉了!\n", name);
        }
        // 删除文件
        else
        {
            // 删除直接指向的块
            for (i = 0; i < 6; i++)
            {
                if (cinode.i_blocks == 0)
                    break;
                block_location = cinode.i_block[i];
                DelBlock(block_location); // 删除块
                cinode.i_blocks--;
            }

            // 删除一级索引中的块
            if (cinode.i_blocks > 0)
            {
                block_location = cinode.i_block[6];
                fseek(fout, (data_begin_block + block_location) * blocksiz, SEEK_SET);
                for (i = 0; i < blocksiz / sizeof(int); i++)
                {
                    if (cinode.i_blocks == 0)
                        break;
                    fread(&block_location2, sizeof(int), 1, fout);
                    DelBlock(block_location2); // 删除块
                    cinode.i_blocks--;
                }
                DelBlock(block_location); // 删除一级索引
            }

            // 删除二级索引中的块
            if (cinode.i_blocks > 0)
            {
                block_location = cinode.i_block[7];
                for (i = 0; i < blocksiz / sizeof(int); i++)
                {
                    fseek(fout, (data_begin_block + block_location) * blocksiz + i * sizeof(int), SEEK_SET);
                    fread(&block_location2, sizeof(int), 1, fout);
                    fseek(fout, (data_begin_block + block_location2) * blocksiz, SEEK_SET);
                    for (k = 0; i < blocksiz / sizeof(int); k++)
                    {
                        if (cinode.i_blocks == 0)
                            break;
                        fread(&block_location3, sizeof(int), 1, fout);
                        DelBlock(block_location3); // 删除块
                        cinode.i_blocks--;
                    }
                    DelBlock(block_location2); // 删除二级索引
                }
                DelBlock(block_location); // 删除一级索引
            }

            // 删除文件的inode
            DelInode(node_location);
            // 更新当前目录的条目
            dir_entry_location = dir_entry_position(current->i_size - dirsiz, current->i_block);
            fseek(fout, dir_entry_location, SEEK_SET);
            fread(&centry, dirsiz, 1, fout); // 读取最后一条目录项
            fseek(fout, dir_entry_location, SEEK_SET);
            fwrite(&dentry, dirsiz, 1, fout); // 清空该位置

            // 释放数据块
            dir_entry_location -= data_begin_block * blocksiz;
            if (dir_entry_location % blocksiz == 0)
            {
                DelBlock(dir_entry_location / blocksiz);
                current->i_blocks--;
                if (current->i_blocks == 6)
                    DelBlock(current->i_block[6]);
                else if (current->i_blocks == (blocksiz / sizeof(int) + 6))
                {
                    int a;
                    fseek(fout, data_begin_block * blocksiz + current->i_block[7] * blocksiz, SEEK_SET);
                    fread(&a, sizeof(int), 1, fout);
                    DelBlock(a);
                    DelBlock(current->i_block[7]);
                }
                else if (!((current->i_blocks - 6 - blocksiz / sizeof(int)) % (blocksiz / sizeof(int))))
                {
                    int a;
                    fseek(fout, data_begin_block * blocksiz + current->i_block[7] * blocksiz + ((current->i_blocks - 6 - blocksiz / sizeof(int)) / (blocksiz / sizeof(int))), SEEK_SET);
                    fread(&a, sizeof(int), 1, fout);
                    DelBlock(a);
                }
            }
            current->i_size -= dirsiz;

            // 如果删除的条目不是最后一条，用最后一条目录项覆盖删除项
            if (j * dirsiz < current->i_size)
            {
                dir_entry_location = dir_entry_position(j * dirsiz, current->i_block);
                fseek(fout, dir_entry_location, SEEK_SET);
                fwrite(&centry, dirsiz, 1, fout);
            }

            printf("文件 %s 被删掉了!\n", name);
        }

        // 更新当前目录inode
        fseek(fout, (data_begin_block + current->i_block[0]) * blocksiz, SEEK_SET);
        fread(&centry, sizeof(ext2_dir_entry), 1, fout);
        fseek(fout, 3 * blocksiz + (centry.inode) * sizeof(ext2_inode), SEEK_SET);
        fwrite(current, sizeof(ext2_inode), 1, fout); // 更新目录inode
    }
    fclose(fout);
}

/* 列出当前目录中的文件和子目录*/
void ls(ext2_inode *current)
{
    ext2_dir_entry dir; // 目录项
    int i, j;
    char timestr[150]; // 用于存储时间的字符串
    ext2_inode node;   // 文件或目录的索引节点

    // 打开文件
    f = fopen(PATH, "r+");
    printf("类型\t\t文件名\t\t创建时间\t\t\t最后访问时间\t\t\t修改时间\n");
    printf("\n注意！current->i_size:%d\n", current->i_size);

    // 遍历当前目录的所有条目
    for (i = 0; i < current->i_size / 32; i++)
    {
        fseek(f, dir_entry_position(i * 32, current->i_block), SEEK_SET); // 定位到目录项
        fread(&dir, sizeof(ext2_dir_entry), 1, f);  // 读取目录项
        fseek(f, 3 * blocksiz + dir.inode * sizeof(ext2_inode), SEEK_SET); // 定位到索引节点
        fread(&node, sizeof(ext2_inode), 1, f);     // 读取索引节点

        // 格式化时间字符串
        strcpy(timestr, "");
        strcat(timestr, asctime(localtime(&node.i_ctime))); // 创建时间
        strcat(timestr, asctime(localtime(&node.i_atime))); // 最后访问时间
        strcat(timestr, asctime(localtime(&node.i_mtime))); // 修改时间

        // 替换时间字符串中的换行符为制表符
        for (j = 0; j < strlen(timestr) - 1; j++)
            if (timestr[j] == '\n')
            {
                timestr[j] = '\t';
            }

        // 输出文件或目录的信息
        if (dir.file_type == 1)
            printf("文件\t\t%s\t\t%s", dir.name, timestr);
        else
            printf("目录\t\t%s\t\t%s", dir.name, timestr);
    }

    fclose(f); // 关闭文件
}

/*此函数用于修改文件系统的密码，如果密码修改成功，则返回 0；如果密码修改失败或用户取消修改，则返回 1*/
int Password()
{
    char psw[16], ch[10]; // 用于存储输入的密码和确认修改的命令
    printf("请输入旧密码：\n");
    scanf("%s", psw); // 输入当前密码
    if (strcmp(psw, group_desc.password) != 0) // 比对输入的旧密码与存储的密码
    {
        printf("密码错误！\n");
        return 1; // 密码错误，返回 1
    }
    while (1)
    {
        printf("请输入新密码：");
        scanf("%s", psw); // 输入新密码
        while (1)
        {
            printf("确定修改密码？[Y/N]");
            scanf("%s", ch); // 输入是否确认修改密码
            if (ch[0] == 'N' || ch[0] == 'n') // 用户选择取消修改
            {
                printf("您已取消密码修改\n");
                return 1; // 用户取消修改，返回 1
            }
            else if (ch[0] == 'Y' || ch[0] == 'y') // 用户确认修改
            {
                strcpy(group_desc.password, psw); // 更新密码
                f = fopen(PATH, "r+"); // 重新打开文件
                fseek(f, 0, 0); // 定位到文件开头
                fwrite(&group_desc, sizeof(ext2_group_desc), 1, f); // 保存新的密码
                fclose(f);
                return 0; // 密码修改成功，返回 0
            }
            else
                printf("无效命令\n"); // 输入无效命令，提示用户
        }
    }
}

 /*显示当前目录的绝对路径*/ 
void pwd (char *str, ext2_inode *current) {
    FILE *fout = NULL;
    char string[100]; // 用于存储路径字符串
    char *slash = "/"; // 用于拼接路径时的分隔符

    int node_location, dir_entry_location, block_location;
    ext2_inode cinode; // 当前目录的inode结构
    ext2_dir_entry pentry, centry;  // 上级目录条目、当前目录条目

    fout = fopen(PATH, "r+"); // 打开文件系统

    // 定位到当前目录的"."条目，即当前目录自身
    fseek(fout, (data_begin_block + current->i_block[0]) * blocksiz, SEEK_SET); 
    fread(&centry, sizeof(ext2_dir_entry), 1, fout); // 读取当前目录的条目信息

    // 定位到当前目录的".."条目，即上一级目录
    fseek(fout, (data_begin_block + current->i_block[0]) * blocksiz + dirsiz, SEEK_SET); 
    fread(&pentry, sizeof(ext2_dir_entry), 1, fout); // 读取上一级目录的条目信息

    // 定位到上一级目录的inode，并读取该inode的信息
    fseek(fout, 3 * blocksiz + (pentry.inode) * sizeof(ext2_inode), SEEK_SET);  
    fread(&cinode, sizeof(ext2_inode), 1, fout); // 读取上一级目录的inode信息

    // 获取上一级目录的路径并保存到string中
    getstring(string, cinode);
    
    // 更新当前目录为上一级目录
    current = &cinode;

    // 如果当前目录的inode与上一级目录的inode不相同，说明没有到达根目录，需要递归
    if (centry.inode != pentry.inode) {
        // 拼接当前目录路径到上级路径
        strcat(string, slash);
        strcat(string, str); // 将当前目录名加到路径字符串后面
        strcpy(str, string); // 更新路径

        // 递归调用pwd，继续向上查找上一级目录
        pwd(str, current);
    }
}

/*格式化模拟文件系统，包括初始化组描述符、位图和根目录。current 指向 ext2_inode 类型的指针，用于指向根目录。返回 0，表示成功。*/
int format(ext2_inode *current)
{
    FILE *fp = NULL;
    int i;
    unsigned int zero[blocksiz / 4];                // 用于填充块的零数组
    time_t now;
    time(&now);                                     // 获取当前时间
    // 保证文件打开成功
    while (fp == NULL)
        fp = fopen(PATH, "w+");                     // 打开文件以写模式操作
    // 初始化零数组
    for (i = 0; i < blocksiz / 4; i++)
        zero[i] = 0;
    // 清空所有数据块，将其初始化为零
    for (i = 0; i < blocks; i++)                   
    {
        fseek(fp, i * blocksiz, SEEK_SET);          // 定位到块的起始位置
        fwrite(&zero, blocksiz, 1, fp);            // 写入零数据
    }
    // 初始化组描述符
    strcpy(group_desc.bg_volume_name, "Volume_name"); // 设置卷名
    group_desc.bg_block_bitmap = 1;                  // 块位图所在块号
    group_desc.bg_inode_bitmap = 2;                  // 索引节点位图所在块号
    group_desc.bg_inode_table = 3;                   // 索引节点表起始块号
    group_desc.bg_free_blocks_count = 4095;          // 可用块数（除去根目录占用块）
    group_desc.bg_free_inodes_count = 4095;          // 可用索引节点数
    group_desc.bg_used_dirs_count = 1;               // 已用目录数
    strcpy(group_desc.password, "9331");                   // 设置默认密码
    
    // 将组描述符写入第一块
    fseek(fp, 0, SEEK_SET);
    fwrite(&group_desc, sizeof(ext2_group_desc), 1, fp);

    // 初始化块位图和索引节点位图，第一位标记为已用
    zero[0] = 0x80000000;                           
    fseek(fp, 1 * blocksiz, SEEK_SET);
    fwrite(&zero, blocksiz, 1, fp);                 // 写入块位图
    fseek(fp, 2 * blocksiz, SEEK_SET);
    fwrite(&zero, blocksiz, 1, fp);                 // 写入索引节点位图

    // 初始化索引节点表，设置根目录节点信息
    inode.i_mode = 2;                               // 目录类型
    inode.i_blocks = 1;                             // 占用块数
    inode.i_size = 64;                              // 目录大小
    inode.i_ctime = now;                            // 创建时间
    inode.i_atime = now;                            // 访问时间
    inode.i_mtime = now;                            // 修改时间
    inode.i_dtime = 0;                              // 删除时间（未删除）
    fseek(fp, 3 * blocksiz, SEEK_SET);
    fwrite(&inode, sizeof(ext2_inode), 1, fp);      // 写入索引节点表

    // 初始化根目录的 "." 和 ".." 目录项
    dir.inode = 0;                                  // 当前目录 inode 号
    dir.rec_len = 32;                               // 目录项长度
    dir.name_len = 1;                               // 名称长度
    dir.file_type = 2;                              // 类型（目录）
    strcpy(dir.name, ".");                          // 当前目录
    fseek(fp, data_begin_block * blocksiz, SEEK_SET);
    fwrite(&dir, sizeof(ext2_dir_entry), 1, fp);    // 写入当前目录

    dir.inode = 0;                                  // 根目录上级目录仍为自身
    dir.rec_len = 32;
    dir.name_len = 2;                               // 名称长度
    dir.file_type = 2;                              // 类型（目录）
    strcpy(dir.name, "..");                         // 上级目录
    fseek(fp, data_begin_block * blocksiz + dirsiz, SEEK_SET);
    fwrite(&dir, sizeof(ext2_dir_entry), 1, fp);    // 写入上级目录

    // 调用初始化函数，设置当前目录指针为根目录
    initialize(current);

    // 打印根目录 inode 大小调试信息
    printf("\n注意！inode.i_size:%d\n", inode.i_size);
    fclose(fp);                                     // 关闭文件
    return 0;
}

/**********第四部分**********/
/**********界面及main函数设计**********/

/*模拟的 Shell 环境，程序进入一个无限循环，等待用户输入命令，并根据不同命令执行相应的操作。 */
void shellloop(ext2_inode currentdir)
{
    char command[10], var1[10], var2[128], path[10];
    ext2_inode temp;
    int i, j;
    char currentstring[20];
    // 命令表，存储支持的命令
    char ctable[14][10] = {"create", "delete", "cd", "close", "read", "write", "password", "format", "exit", "login", "logout", "ls", "pwd", "help"};

    // 无限循环，等待用户输入命令
    while (1)
    {
        // 获取当前目录名称并打印提示符
        getstring(currentstring, currentdir); 
        printf("\n[当前目录: %s]> ", currentstring);

        // 读取用户输入的命令
        scanf("%s", command);

        // 遍历命令表，找到对应的命令索引
        for (i = 0; i < 14; i++)
            if (!strcmp(command, ctable[i]))
                break;

        // 根据命令执行对应操作
        if (i == 0 || i == 1) // 创建或删除文件/目录
        {
            scanf("%s", var1); // 输入类型（f: 文件, d: 目录）
            scanf("%s", var2); // 输入文件/目录名称
            if (var1[0] == 'f')
                j = 1; // 文件
            else if (var1[0] == 'd')
                j = 2; // 目录
            else
            {
                printf("错误: 第一个参数必须是 [f/d]\n");
                continue;
            }

            if (i == 0) // 创建操作
            {
                if (Create(j, &currentdir, var2) == 1)
                    printf("失败: 无法创建 %s\n", var2);
                else
                    printf("成功: 创建了 %s\n", var2);
            }
            else // 删除操作
            {
                if (Delete(j, &currentdir, var2) == 1)
                    printf("失败: 无法删除 %s\n", var2);
                else
                    printf("成功: 删除了 %s\n", var2);
            }
        }
   else if (i == 2) // cd - Change Directory
        {
            scanf("%s", var2); // 输入目标目录
            i = 0;
            j = 0;
            temp = currentdir;
            while (1)
            {
                path[i] = var2[j]; // 构建路径
                if (path[i] == '/')
                {
                    if (j == 0)
                        initialize(&currentdir); // 如果是根目录，初始化
                    else if (i == 0) // 目录名中不能包含 '/'
                    {
                        printf("路径错误!\n");
                        break;
                    }
                    else // 进入指定目录
                    {
                        path[i] = '\0'; // 临时存储目录路径
                        if (Open(&currentdir, path) == 1)
                        {
                            printf("路径不对!\n");
                            currentdir = temp;
                        }
                    }
                    i = 0; // 重新设置路径
                }
                else if (path[i] == '\0') // 路径结束
                {
                    if (i == 0)
                        break;
                    if (Open(&currentdir, path) == 1)
                    {
                        printf("路径错误!\n");
                        currentdir = temp;
                    }
                    break;
                }
                else
                    i++; // 增加路径字符索引
                j++; // 增加用户输入字符索引
            }
        }
        else if (i == 3) // 关闭文件
        {
            scanf("%d", &i); // 输入要关闭的文件数量
            for (j = 0; j < i; j++)
                if (Close(&currentdir) == 1)
                {
                    printf("警告: 数量 %d 超过了打开的文件数\n", i);
                    break;
                }
        }
        else if (i == 4) // 读取文件
        {
            scanf("%s", var2); // 输入文件名
            if (Read(&currentdir, var2) == 1)
                printf("失败: 无法读取文件 %s\n", var2);
        }
        else if (i == 5) // 写入文件
        {
            printf("请输入要写入的数据，ESC结束\n");
            scanf("%s", var2); // 输入文件名
            if (Write(&currentdir, var2) == 1)
                printf("失败: 无法写入文件 %s\n", var2);
        }
        else if (i == 6) // 修改密码
            Password();
        else if (i == 7) //格式化
        {
            while (1)
            {
                printf("Do you want to format the filesystem?\n It will be dangerous to your data.\n");
                printf("[Y/N]");
                scanf("%s", var1);
                if (var1[0] == 'N' || var1[0] == 'n')
                    break;
                else if (var1[0] == 'Y' || var1[0] == 'y')
                {
                    format(&currentdir);
                    break;
                }
                else
                    printf("please input [Y/N]");
            }
        }
       else if (i == 8) // exit - 退出文件系统
        {
            while (1)
        {
        printf("你确定要退出文件系统吗？[Y/N]\n");
        scanf("%s", var2); // 读取用户输入
        if (var2[0] == 'N' || var2[0] == 'n') // 用户选择不退出
            break;
        else if (var2[0] == 'Y' || var2[0] == 'y') // 用户选择退出
            return; // 退出文件系统
        else
            printf("\n请输入 [Y/N]\n"); // 提示用户重新输入
         }
        }
        else if (i == 9) // 登录
            printf("错误: 您已经登录\n"); // 提示用户已登录
        else if (i == 10) // logout - 从文件系统退出
        {
            while (i)
            {
        printf("你确定要从文件系统退出吗？[Y/N]"); // 提示用户是否退出
        scanf("%s", var1); // 读取用户输入
        if (var1[0] == 'N' || var1[0] == 'n') // 用户选择不退出
            break;
        else if (var1[0] == 'Y' || var1[0] == 'y') // 用户选择退出
        {
            initialize(&currentdir); // 重新初始化文件系统
            while (1)
            {
                printf("请输入命令: ");
                scanf("%s", var2); // 读取用户输入的命令
                if (strcmp(var2, "login") == 0) // 如果输入的是 "login" 命令
                {
                    if (login() == 0) // 调用登录函数，如果登录成功
                    {
                        i = 0; // 设置 i 为 0，退出外部循环
                        break;
                    }
                }
                else if (strcmp(var2, "exit") == 0) // 如果输入的是 "exit" 命令
                    return; // 退出程序
            }
        }
        else
            printf("请输入 [Y/N]"); // 提示用户重新输入
            }
        }
        else if (i == 11) // ls - 列出目录内容
            ls(&currentdir); // 调用 ls 函数显示目录内容
        else if (i == 12) // pwd - 打印工作目录
        {
            char string[100];
            for (int j = 0; j < 100; j++)
                string[j] = 0; // 初始化字符串
            getstring(currentstring, currentdir); // 获取当前目录的目录名
            pwd(string, &currentdir); // 获取绝对路径
            strcat(string, currentstring); // 拼接当前目录名
            int i = 0;
            for (i = 0; string[i + 1]; i++)
                string[i] = string[i + 1]; // 去掉多余的 '/'
            string[i] = 0; // 添加字符串结尾标志
            printf("%s\n", string); // 输出绝对路径
        }
        else if (i == 13) // 帮助信息
        {
            printf("************************************************************************************\n");
            printf("*                             模拟 ext2 文件系统                                  *\n");
            printf("* 支持的命令：                                                                    *\n");
            printf("* 01.切换目录  : cd+目录名          02.创建目录   : create d+目录名                *\n");
            printf("* 03.创建文件  : create f+文件名    04.删除目录   : delete d+目录名                *\n");
            printf("* 05.删除文件  : delete f+文件名    06.读取文件   : read+文件名                    *\n");
            printf("* 07.写入文件  : write+文件名       08.显示路径   : pwd                            *\n");
            printf("* 09.关闭文件  : close+数量         10.修改密码   : password                       *\n");
            printf("* 11.列出项目  : ls                 12.帮助菜单   : help                           *\n");
            printf("* 13.格式化磁盘: format             14.退出系统   : exit                           *\n");
            printf("* 15.注销系统  : logout                                                            *\n");
            printf("************************************************************************************\n");
        }
        else
        {
            printf("错误: 无效命令，请输入 help 查看支持的命令。\n");
        }
    }
}

/*main函数，简化版的文件系统程序的主函数。*/
int main()
{
    ext2_inode cu; /* 当前用户的 inode 结构，表示用户所在的目录或文件系统状态 */
    
    // 输出欢迎信息，提示用户进入 Ext2 类似文件系统
    printf("你好呀!欢迎使用我的系统!\n");

    // 初始化文件系统，如果初始化失败则退出程序
    if (initfs(&cu) == 1)
        return 0;

    // 提示用户输入密码进行登录
    if (login() != 0) /* 登录失败时退出 */
    {
        // 密码错误时显示错误信息，并退出程序
        printf("密码不对 再见！\n");
        
        // 显示退出信息并结束程序
        exitdisplay();
        
        return 0;
    }

    // 登录成功后进入命令行交互模式
    shellloop(cu);

    // 退出文件系统并显示退出信息
    exitdisplay();
    
    // 正常结束程序
    return 0;
}


