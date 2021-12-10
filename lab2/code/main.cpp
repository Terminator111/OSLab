#include <iostream>
#include <queue>
#include <sstream>
#include <string.h>
#include <vector>

using namespace std;

typedef unsigned char u8;   //1 byte
typedef unsigned short u16; //2 byte
typedef unsigned int u32;   //4 byte

#pragma pack(1) //1字节对齐

// 引导扇区
struct Fat12Header
{
    char BS_OEMName[8];     // OEM字符串，必须为8个字符，不足以空格填空
    u16 BPB_BytsPerSec;     // 每扇区字节数
    u8 BPB_SecPerClus;      // 每簇占用的扇区数
    u16 BPB_RsvdSecCnt;     // Boot占用的扇区数
    u8 BPB_NumFATs;         // FAT表的记录数
    u16 BPB_RootEntCnt;     // 最大根目录文件数
    u16 BPB_TotSec16;       // 每个FAT占用扇区数
    u8 BPB_Media;           // 媒体描述符
    u16 BPB_FATSz16;        // 每个FAT占用扇区数
    u16 BPB_SecPerTrk;      // 每个磁道扇区数
    u16 BPB_NumHeads;       // 磁头数
    u32 BPB_HiddSec;        // 隐藏扇区数
    u32 BPB_TotSec32;       // 如果BPB_TotSec16是0，则在这里记录
    u8 BS_DrvNum;           // 中断13的驱动器号
    u8 BS_Reserved1;        // 未使用
    u8 BS_BootSig;          // 扩展引导标志
    u32 BS_VolID;           // 卷序列号
    char BS_VolLab[11];     // 卷标，必须是11个字符，不足以空格填充
    char BS_FileSysType[8]; // 文件系统类型，必须是8个字符，不足填充空格
};

#pragma pack() // 取消指定对齐，恢复缺省对齐（8字节对齐）

// 根目录条目
struct RootEntry
{
    char DIR_Name[11]; // 文件名
    u8 DIR_Attr;       // 文件属性——目录:0x10 文件:0x20(windows下是0x00 草…)
    u8 reserve[10];    // 保留位
    u16 DIR_WrtTime;   // 最后一次写入时间
    u16 DIR_WrtDate;   // 最后一次写入日期
    u16 DIR_FstClus;   // 开始簇号
    u32 DIR_FileSize;  // 文件大小
};

struct FileNode
{
    string name;
    string path;
    int attr;          // 文件属性：0--目录，1--文件
    int fileSize;      // 文件的大小，如果是目录的话，值为0
    int startClus;     // 文件或目录的起始簇
    int directFileNum; // 文件直接子文件数量
    int direcDirNum;   // 文件直接子目录数量
    vector<FileNode *> childFiles;
};

struct Command
{
    string order;
    string para;
    string path;
};

extern "C"
{
    void my_print(const char *str);
    void my_print_red(const char *str);
}

void readHeader(FILE *);
void readRootEntries(FILE *);
void readFile(FILE *, FileNode *, int);
void readChildEntries(FILE *, FileNode *, int);
int getNextCluster(FILE *, int);
string getFileName(RootEntry *);
Command parseCommand(string);
void doLs(Command);
void printLS(FileNode *, bool);
void doCat(FILE *, Command);
int findFile(string);

const char *imgName = "a.img";
Fat12Header *header;
FileNode *rootPtr; // 指向所有文件
FileNode *fileArray[10000];
int fileArraySize = 0;
int RootStartSectors = 0; // 根目录区起始扇区
int DataStartSectors = 0; // 数据区起始扇区

int main()
{
    FILE *fat12 = fopen(imgName, "rb");
    readHeader(fat12);
    readRootEntries(fat12);
    // 读取用户输入
    string command;
    while (true)
    {
        my_print(">");
        getline(cin, command);
        if (command == "exit")
            break;
        Command cmd = parseCommand(command);
        if (cmd.order == "error")
        {
            my_print(cmd.para.c_str());
            my_print("\n");
            continue;
        }
        else if (cmd.order == "ls")
            doLs(cmd);
        else
            doCat(fat12, cmd);
    }
    //system("pause");
    return 0;
}

/**
 * 读取引导扇区内容，存入结构体中
*/
void readHeader(FILE *fat12)
{
    header = (struct Fat12Header *)malloc(sizeof(struct Fat12Header)); // 为header分配空间!!!!!!!!!!!!!!!
    fseek(fat12, 3, SEEK_SET);                                         // 从第3个字节开始读
    fread(header, 1, sizeof(Fat12Header), fat12);
    RootStartSectors = header->BPB_RsvdSecCnt + header->BPB_NumFATs * header->BPB_FATSz16;                                           // 引导 + FAT
    DataStartSectors = RootStartSectors + ((header->BPB_BytsPerSec - 1) + (header->BPB_RootEntCnt * 32)) / (header->BPB_BytsPerSec); // 根目录起始 + 大小
}

/**
 * 读取根目录下文件
*/
void readRootEntries(FILE *fat12)
{
    if (rootPtr != NULL)
    {
        delete rootPtr;
        rootPtr = NULL;
    } // 避免内存泄露
    rootPtr = new FileNode; // 根目录
    rootPtr->name = "/";
    rootPtr->path = "/";
    rootPtr->fileSize = 0;
    rootPtr->startClus = 0;
    rootPtr->attr = 0;
    rootPtr->direcDirNum = 0;
    rootPtr->directFileNum = 0;
    fileArray[fileArraySize++] = rootPtr;

    int offset = RootStartSectors * header->BPB_BytsPerSec;
    for (int i = 0; i < header->BPB_RootEntCnt; i++)
    {
        readFile(fat12, rootPtr, offset);
        offset += 32;
    }
}

/**
 * 读取文件/目录节点
*/
void readFile(FILE *fat12, FileNode *root, int offset)
{
    RootEntry rootEntry;
    RootEntry *rootEntryPtr = &rootEntry;
    fseek(fat12, offset, SEEK_SET);
    fread(rootEntryPtr, 1, 32, fat12);
    if (rootEntryPtr->DIR_Name[0] == '\0') // 读到空文件
        return;
    // 判断文件名是否为大写字母和数字
    bool flag = true;
    for (int i = 0; i < 11; i++)
    {
        char tmp = rootEntryPtr->DIR_Name[i];
        if (!((tmp >= 'A' && tmp <= 'Z') || (tmp >= '0' && tmp <= '9') || tmp == ' '))
        {
            flag = false;
            break;
        }
    }
    if (!flag)
        return;

    FileNode *file = new FileNode;
    file->fileSize = rootEntryPtr->DIR_FileSize;
    file->startClus = rootEntryPtr->DIR_FstClus;
    root->childFiles.push_back(file);
    file->name = getFileName(rootEntryPtr);
    file->direcDirNum = 0;
    file->directFileNum = 0;
    if (rootEntryPtr->DIR_Attr == 0x10)
    { // 目录
        file->attr = 0;
        root->direcDirNum++;
        file->path = root->path;
        file->path += file->name;
        file->path += "/";
        // 递归读取子目录下的文件
        readChildEntries(fat12, file, file->startClus);
    }
    else
    { // 文件
        file->attr = 1;
        root->directFileNum++;
        file->path = root->path;
        file->path += file->name;
    }
    fileArray[fileArraySize++] = file;
}

/**
 * 递归读取子目录
*/
void readChildEntries(FILE *fat12, FileNode *root, int startClus)
{
    int curClus = startClus; // 当前簇号
    do
    {
        int nextClus = getNextCluster(fat12, curClus);
        int offset = DataStartSectors * header->BPB_BytsPerSec + (curClus - 2) * header->BPB_SecPerClus * header->BPB_BytsPerSec;
        for (int i = 0; i < header->BPB_SecPerClus * header->BPB_BytsPerSec; i += 32)
        {
            readFile(fat12, root, offset + i);
        }
        if (nextClus == -1)
            break;
        curClus = nextClus;
    } while (curClus != -1);
}

/**
 * 读取Fat表，获取下一个簇号（若无 返回-1）
*/
int getNextCluster(FILE *fat12, int curClus)
{
    if (curClus == 0)
    {
        return -1;
    }
    int base = header->BPB_RsvdSecCnt * header->BPB_BytsPerSec;
    int num1 = 0, num2 = 0; // 初始化！！！！！！！
    int *p1 = &num1;
    int *p2 = &num2;
    int start = curClus / 2 * 3; // 3 byte的起始位置
    int res = 0;
    if (curClus % 2 == 0)
    {
        fseek(fat12, base + start, SEEK_SET);
        fread(p1, 1, 1, fat12);
        fread(p2, 1, 1, fat12);
        res = ((num2 & 0x0f) << 8) + num1;
    }
    else
    {
        fseek(fat12, base + start + 1, SEEK_SET);
        fread(p1, 1, 1, fat12);
        fread(p2, 1, 1, fat12);
        res = ((num1 & 0xf0) >> 4) + (num2 << 4);
    }
    if (res >= 0xff8) // 最后一个簇
        return -1;
    if (res == 0xff7) // 坏簇
    {
        my_print("Broken Cluster!\n");
        return -1;
    }
    return res;
}

/**
 * 解析并获取文件名
*/
string getFileName(RootEntry *rootEntryPtr)
{
    string name;
    char tmp;
    for (int i = 0; i < 8; i++)
    {
        if ((tmp = rootEntryPtr->DIR_Name[i]) != ' ')
            name += tmp;
    }
    if (rootEntryPtr->DIR_Attr == 0x20 || rootEntryPtr->DIR_Attr == 0x00)
    {
        name += ".";
        for (int i = 8; i < 11; i++)
        {
            if ((tmp = rootEntryPtr->DIR_Name[i]) != ' ')
                name += tmp;
        }
    }
    name += "\0";
    return name;
}

/**
 * 解析用户输入的命令
*/
Command parseCommand(string command)
{
    Command error = {.order = "error"};

    string order;
    string para;
    string path;
    string tmp;
    stringstream ss;
    ss << command;

    ss >> order; // read order
    // check order
    if (order != "ls" && order != "cat")
    {
        error.para = "Unknown Command!";
        return error;
    }
    // read parameter and path
    while (getline(ss, tmp, ' '))
    {
        if (tmp[0] == '-')
            // parameter
            para += tmp.substr(1);
        else
        {
            // path
            if (path.length() == 0)
                path = tmp;
            else
            {
                error.para = "Too Many Paths!";
                return error;
            }
        }
    }
    // check parameter
    if (para.length() != 0)
    {
        if (order == "cat")
        {
            error.para = "Invalid Parameter!";
            return error;
        }
        else
        {
            for (unsigned int i = 0; i < para.length(); i++)
            {
                if (para[i] != 'l')
                {
                    error.para = "Invalid Parameter!";
                    return error;
                }
            }
            para = "l";
        }
    }
    // check path —— in doX function

    Command cmd = {.order = order, .para = para, .path = path};
    return cmd;
}

/**
 * 处理ls和ls -l指令
*/
void doLs(Command command)
{
    // check path
    if (command.path.length() == 0 || command.path == ".")
        command.path = "/";

    int pos = findFile(command.path);
    if (pos == -1)
    {
        my_print("Invalid Path!\n");
        return;
    }

    FileNode *file = fileArray[pos];
    bool flag = command.para.length() > 0 ? true : false; //有-l参数则flag为true

    if (file->attr == 1)
    {
        my_print((file->name + "  ").c_str());
        if (flag)
            my_print(to_string(file->fileSize).c_str());
        my_print("\n");
        return;
    }
    else
    {
        printLS(file, flag); // 递归 遍历
    }
}

/**
 * 递归 遍历目录，并print
*/
void printLS(FileNode *file, bool flag)
{
    queue<FileNode *> waitDirs; // 待print的队列
    my_print(file->path.c_str());
    if (flag)
        my_print((" " + to_string(file->direcDirNum) + " " + to_string(file->directFileNum)).c_str());
    my_print(":\n");

    if (file->path != "/")
    {
        if (flag)
            my_print_red(".\n..\n");
        else
            my_print_red(".  ..  ");
    }

    for (auto iter = file->childFiles.cbegin(); iter != file->childFiles.cend(); iter++)
    {
        if ((*iter)->attr == 0)
        {
            waitDirs.push((*iter));
            my_print_red((*iter)->name.c_str());
        }
        else
        {
            my_print((*iter)->name.c_str());
        }
        my_print("  ");
        if (flag)
        {
            if ((*iter)->attr == 0)
                my_print((to_string((*iter)->direcDirNum) + " " + to_string((*iter)->directFileNum) + "\n").c_str());
            else
                my_print((to_string((*iter)->fileSize) + "\n").c_str());
        }
    }
    my_print("\n");
    while (!waitDirs.empty())
    {
        printLS(waitDirs.front(), flag);
        waitDirs.pop();
    }
}

/**
 * 处理cat指令
*/
void doCat(FILE *fat12, Command command)
{
    // check path
    if (command.path.length() == 0)
    {
        my_print("No File Input!\n");
        return;
    }

    if (command.path[0] != '/')
    {
        command.path = '/' + command.path;
    }

    int pos = findFile(command.path);
    if (pos == -1)
    {
        my_print("Invalid Path!\n");
        return;
    }

    FileNode *file = fileArray[pos];
    if (file->attr == 0)
    {
        my_print("Not A File!\n");
        return;
    }

    if (file->name.length() < 4 ||
        file->name.substr(file->name.length() - 4, 4) != ".TXT")
    {
        my_print("Only Txt Files!\n");
        return;
    }

    int curClus = file->startClus;
    int BytsPerClus = header->BPB_SecPerClus * header->BPB_BytsPerSec;
    do
    {
        int nextClus = getNextCluster(fat12, curClus);
        int offset = DataStartSectors * header->BPB_BytsPerSec + (curClus - 2) * BytsPerClus;
        char *tmp = new char[BytsPerClus]; // or malloc
        // The malloc function allocates a memory block of at least size bytes. The block may be larger than size bytes because of space required for alignment and maintenance information.
        // cout << strlen(tmp); --528
        fseek(fat12, offset, SEEK_SET);
        fread(tmp, 1, BytsPerClus, fat12);
        tmp[BytsPerClus] = '\0'; //!!!!!!!!!!!!!
        my_print(tmp);
        if (nextClus == -1)
            break;
        curClus = nextClus;
    } while (curClus != -1);
    my_print("\n");
}

/**
 * 根据path找File在fileArray中的索引（若无 pos=-1）
*/
int findFile(string path)
{
    for (int pos = 0; pos < fileArraySize; pos++)
    {
        if (fileArray[pos]->path == path || fileArray[pos]->path == (path + "/"))
            return pos;
    }
    return -1;
}
