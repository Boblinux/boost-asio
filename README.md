# 简单分布式文件存储系统

## Mini Program 中Client部分的使用

### 类接口

定义File类，File类中包含BlockFile类，File和BlockFile包含MD5类，封装了文件信息、文件块信息，实现发送文件接口。代码如下：
```c++
class File
{

public:

File(int fd, int conn, ll fileSize) 
    : fd_(fd),
      conn_(conn),
      fileSize_(fileSize),
      md5_(fd_, fileSize_, 0),
      blockNum_((ll)(fileSize_ % PAGE_SIZE == 0 ? fileSize_/PAGE_SIZE:fileSize_/PAGE_SIZE+1)),
      uploadId_(std::vector<bool>(blockNum_, false)),
      downloadId_(std::vector<bool>(blockNum_, false))
{
    initFile();
}
~File() {}
    
std::string getFileMD5()

unsigned long getFileBlockNum() {return blockNum_; }

std::vector<BlockFile*> getFileBlockFile() {return BlockFileList_; }

void initFile()

void sendFileinfo(std::string fileName)

void sendFileBlockMD5info()

void sendFileBlock()

void updateuploadId(std::string idstr)

void updatedownloadId(std::string idstr)

private:
    int                         fd_; 
    int                         conn_; 
    ll                          fileSize_;
    Md5                         md5_; 
    unsigned long               blockNum_;
    std::vector<BlockFile*>     BlockFileList_;
    std::vector<bool>           uploadId_; 
    std::vector<bool>           downloadId_;  
};

class BlockFile
{

public:

BlockFile(int fd, int conn, ll blockSize, int id, off_t offset) 
    : fd_(fd),
      conn_(conn),
      blockSize_(blockSize),
      id_(id),
      offset_(offset),
      md5_(fd_, blockSize_, offset_)
{
    
}
~BlockFile() {}
    
ll getBlockSize()

std::string getBlockFileMD5()

int getBlockFileid()

off_t getBlockFileOffset()

void sendBlockFile()

private:
    int             fd_;
    int             conn_; 
    ll              blockSize_;
    uint32_t        id_;
    off_t           offset_;
    Md5             md5_;
};


class Md5
{

public:

Md5(int fd, ll blockSize, off_t offset) 
    : fd_(fd),
      blockSize_(blockSize),
      offset_(offset)
{
}

~Md5() {}
    
std::string getMD5() 

private:
    std::string     md5_;
    int             fd_; 
    ll              blockSize_;
    off_t           offset_;
      
};
```
### 文件upLoad核心逻辑

1、发送文件信息（文件名、文件块数量、文件MD5）；
2、接收信息（ACK、Success、块ID组成的字符串），如果接收到的信息是ACK，开始执行4过程；如果是Success，直接retrue；如果是块ID组成的字符串，开始执行3过程；
3、将ID组成的字符分解update进File类中，将已经发送过的ID置1；
4、发送文件块MD5；
5、发送块文件。
6，结束
```c++
void upLoadFile(int conndfd, std::string argv)
{
    static char buffer[BUFFERSIZE];
    bzero(buffer, sizeof(buffer)); 
    int n;
    int fd = open(argv.data(), O_RDONLY);
    ll file_size = getFileSize(fd);
    Md::File File(fd, conndfd, file_size); 
    int blockNum = File.getFileBlockNum();
    std::cout << "File num: " << blockNum << " File MD5: "<< File.getFileMD5()  << std::endl;
    File.sendFileinfo(argv); //1、seng Fileinfo
    if((n = recv(conndfd, buffer, BUFFERSIZE, 0)) > 0)
    {
        std::string idstr = buffer;
        bzero(buffer, sizeof(buffer)); 
        //std::cout << idstr << std::endl;
        if(idstr == "success")
        {
            return ;  //5、success
        }
        
        std::string::size_type position = idstr.find(':');
        if(position != idstr.npos)
        {
            File.updateuploadId(idstr); //2、if file existence, updateuploadId
        }
        else
        {
            File.sendFileBlockMD5info(); //3、else sendFileBlockMD5info 
        }
    }
    File.sendFileBlock();   //4、sendFileBlock
    while((n = recv(conndfd, buffer, BUFFERSIZE, 0)) > 0)
    {
        std::cout << buffer << std::endl;
        std::string idstr = buffer;
        bzero(buffer, sizeof(buffer));
        if(idstr == "success")
        {
            break;  //5、success
        }
        else
        {
            File.updateuploadId(idstr); // goto 2、updateuploadId
            File.sendFileBlock(); 
        }
    }
}

```

### 文件downLoad核心逻辑

1、downLoadMD5从proxy将文件信息获取回来，包括块id、id对应的md5；
2、downLoadFileBlock从storage中将文件对应的所有块取回来，并且分别建立小文件；
3、combinateFile在本地对文件进行组合，删除文件，并且重命名。
```c++
void downLoadMD5(int conndfd, std::vector<std::string>& MD5str, std::string argv)
{
  const int BUFFSIZE = 1024*1024;
  char BUFFER[BUFFSIZE];
  ::read(conndfd, BUFFER, 8);
  // std::cout <<  BUFFER << '\n';
  
  std::string Buff = "";
  uint32_t bsbe = htonl(argv.size());
  Buff.append(static_cast<const char*>(static_cast<const void*>(&bsbe)), 4);
  Buff.append(argv.data(), argv.size());
  ::send(conndfd, Buff.data(), Buff.size(), 0); 

  bzero(BUFFER, BUFFSIZE);
  int n, location=0, resvsize = 4;
  while((n = ::read(conndfd, BUFFER+location, resvsize)) < resvsize)
  {
     location += n;
     resvsize -= n;
  }
  int32_t be32 = *static_cast<const int32_t*>(
                  static_cast<const void*>(std::string(BUFFER, 4).data()));
  resvsize = ntohl(be32);
  //std::cout<<"resvsize: " << resvsize << 'n';
  location=0;
  bzero(BUFFER, BUFFSIZE);
  while((n = ::read(conndfd, BUFFER+location, resvsize)) < resvsize)
  {
     location += n;
     resvsize -= n;
  }
  std::string str = std::string(BUFFER, ntohl(be32));
  for(int i=0; i+32 <= str.size(); i += 32)
  {
    MD5str.push_back(str.substr(i,32));
  }    
}

void downLoadFileBlock(int conndfd, std::string argv)
{
  std::string sendBuff = "";
  uint32_t bsbe = htonl(argv.size());
  sendBuff.append(static_cast<const char*>(static_cast<const void*>(&bsbe)), 4);
  sendBuff.append(argv.data(), argv.size());
  ::send(conndfd, sendBuff.data(), sendBuff.size(), 0); 
  
  Reactor::Buffer Buff;
  while(true)
  {
    Buff.readFd(conndfd);
    
    while(Buff.readableBytes() >= 4)
    {
      const void *tmp = Buff.peek();
      int32_t be32 = *static_cast<const int32_t*>(tmp);
      size_t BlockFileSize = boost::asio::detail::socket_ops::host_to_network_long(be32);
      //std::cout << BlockFileSize << '\n';
      if(Buff.readableBytes() >= 4+36+BlockFileSize)
      {
        // std::cout << BlockFileSize << '\n';
        Buff.retrieve(4);
        std::string FileContent = Buff.readAsBlock(36+BlockFileSize);
        if(getstringMD5(FileContent.substr(36)) == FileContent.substr(0,32))
        {
          // std::cout <<  FileContent.substr(0,32) << '\n';
          // std::cout <<  getstringMD5(FileContent.substr(36)) << '\n';
          if(lsMD5InFile(FileContent.substr(0,32)))
          {
            continue;
          }
          else
          {
            std::lock_guard<std::mutex> guard(mux);
            blockMD5Num.push_back(FileContent.substr(0,32));
            //std::cout << blockMD5Num.size() << ':' << MD5str.size() << '\n';
            std::string file = "./" + argv + '/' + FileContent.substr(0,36);
            // std::cout << file << '\n';
            int fd = open(file.data(), O_RDWR|O_CREAT);
            chmod(file.data(), 0777);
            ::write(fd, FileContent.substr(36).data(), FileContent.substr(36).size());
            close(fd);
            // std::cout << "resv block success" << '\n';
            printf("\rresv block[%.2lf%%]:", (blockMD5Num.size()*100.0) / (MD5str.size()));
          }
        }
        else
        {
          std::cout << "file resv error" << '\n';
        }
      }
      else
      {
        break;
      }
    }
  }
}

void combinateFile(std::vector<std::string> MD5str, const char *basePath)
{
  
  std::string fileName = std::string("./") + basePath + std::string("/");
  // std::cout << fileName << '\n';
  int fd = ::open(fileName.substr(0, fileName.size()-2).data(), O_RDWR | O_CREAT);
  if(-1 == fd)
  {
    std::cout << "create file errror\n";
    perror("ferror");
  }

  for(int i=0; i<MD5str.size(); i++)
  {
    std::string files = fileName + MD5str[i] + ".txt";
    conbFileblock(files.data(), fd, MD5str[i].substr(0, 32));
    printf("\rcombined block[%.2lf%%]:", (i*100.0) / (MD5str.size()));
  }
  printf("\rcombined block[%.2lf%%]:", 100.0);
  remove_dir(basePath);
  rename(fileName.substr(0, fileName.size()-2).data(), basePath);
}
```

## Mini Program 中Proxy部分的使用

Proxy的状态`StatuOperate`是一个枚举类型`enum{sw, up, down}`，初始化状态为`sw`。

- `sw`状态：Proxy等待客户端发送请求类型（`"upppload"`或`"download"`）
  `"upppload"`：Proxy的状态`StatuOperate`从`sw--->up`
  `"download"`：Proxy的状态`StatuOperate`从`sw--->down`

- `up`状态：Proxy（Status：up）进入`upLoadFile`函数
- `down`状态：Proxy（Status：down）进入`downLoad`函数

#### 文件上传

Proxy的`upLoadFile`函数里上传分为4个状态：`enum{upstatus1, upstatus2, upstatus3, upstatus4}`;

- upstatus1(初始)：
  进入`initFileNumber`函数。向3个storage发送`"upppload"`（storage收到进入up状态，进入的`upLoad()`函数的状态1，调用`mkfile`函数等待proxy文件名数据包），并查看文件名是否在mysql存在（是否已上传过）：
      1. 没有就新建文件名的表：
         - 返回客户端`"ACK"`(客户端进入`sendFileBlockMD5info()`)
         - 向storage发送filenameLen(4)+filename（mkdir状态的storage新建文件名的文件夹准备存储）- upstatu1--->upstatu2
      2. 已存在表，检查表中文件block上传状态：
         - 全部已传完，返回客户端`"success"`(客户端关闭连接)，upstatu1--->upstatu4(完成并退出)
         - 未传完，返回已完成的blockid的str(客户端据此进行更新id表并发送block)，upstatu1--->upstatu3（用于断点续传）

- upstatu2：
  进入`initFileMD5()`函数，将客户端发来的所有blockmd5的str每32个字节进行解析成一个block的md5存在mysql。upstatu2--->upstatu3

- upstatu3:
  进入`sendBlock()`函数，接收来自客户端的block（解析出blocksize(4)、blockid(4)、blockdata），对其blockdata进行md5校验，结果与mysql查询结果进行对比。正确之后将blocksize(4) + blockmd5(32) + blockdata发送到对应的storage端，并在Proxy端更新已上传的blockid（通过blockmd5进行哈希取余决定）（storage在saveblock函数等待block）

- upstatu4：完成并退出

#### 文件下载

Proxy接收到来自客户端请求类型为`"download"`，Proxy的状态`StatuOperate`从`sw--->down`，进入`downLoad`函数。将MySQL中存的相应文件名的所有block的md5值合并成字符串(blocknum(4) + blockmd5str(32*blocknum))，返回给客户端，客户端继续与存储端进行文件的下载。

主要工作：Proxy与客户端以及存储端协议对接和协议解析，并调用mysql接口的数据块存储和查询功能。对客户端发来的协议包解析并调用与mysql交互，进行下一步动作。

#### 遇到的问题：

1. 上传时，proxy进行初始化file信息时发送filename数据包给storage，storage没有收到（没有进入mkfile函数或者阻塞在了mkfile）

2. 断点续传：上传中断再继续上传，storage端没有收到block。(Proxy发给storage的协议逻辑，已解决)

3. 下载时返回给客户端blockmd5str时，受MTU影响不能一次性发完，客户端接收数据缺失（返回包协议头加入包长度信息，客户端读到指定长度）

## Mini Program 中Storage部分的使用

### 存储文件
- storage端存储文件时主要包含三个状态：分别是mkfile，saveBlockFile，finishresv
- mkfile状态：当proxy端向storage发送存储请求时，storage处于默认的mkfile状态，进入mkfile()函数，获取传输文件的md5值，根据md5值创建文件夹，并将状态修改为状态2。如果过程中boost::asio::read出错，那就进入状态3
-  saveBlockFile状态：在进入saveBlockFile后，获取每个块的MD5值作为存储块的文件名，在做完MD5校验后存储块内容。如果过程中boost::asio::read出错，则进入状态3
-  finishresv状态：一直等待proxy发送的upload请求，如果接收到请求就进入状态1.
#### 创建存储块文件的文件夹
- 实现方法：根据文件名的MD5值创建文件夹
```c++
void mkfile()
  {
    boost::asio::read(socket_, boost::asio::buffer(&*head_.begin(), head_length), ec_);
    if(ec_)
    {
      std::cout << "mkfile error" << '\n';
      statu_ = statu3;
    }
    size_t fileLen = network_to_host_long(head_);
    
    boost::asio::read(socket_, boost::asio::buffer(&*data_.begin(), fileLen), ec_);
    if(ec_)
    {
      std::cout << "mkfile error" << '\n';
      statu_ = statu3;
    } 
    fileName_ += std::string(&*data_.begin(), fileLen) + '/';
    mkdir(fileName_.c_str(),S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
    std::cout << "start save file: " << fileName_ << '\n';
    statu_ = statu2;
  }
```
#### 存储块文件
- 实现方法：
```c++
void saveBlockFile()
  {
    while(true)
    { 
      boost::asio::read(socket_, boost::asio::buffer(&*head_.begin(), head_length), ec_);
      if(ec_)
      {
        std::cout << "saveBlockFile error" << '\n';
        statu_ = statu3;
      } 
      size_t blocksize = network_to_host_long(head_);
      //std::cout << "blocksize: " << blocksize << '\n';
      
      boost::asio::read(socket_, boost::asio::buffer(&*data_.begin(), 32), ec_);
      if(ec_)
      {
        std::cout << "saveBlockFile error" << '\n';
        statu_ = statu3;
      } 
      std::string blockmd5 = std::string(&*data_.begin(), 32);
      //std::cout << blockmd5 << '\n';
      if(blocksize == 0 && blockmd5 == "EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE")
      {
        std::cout << "\nsave file finish: " << fileName_ << '\n';
        break;
      }
      if(blocksize == 0 && blockmd5 == "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM")
      {
        continue;
      }
      boost::asio::read(socket_, boost::asio::buffer(&*data_.begin(), blocksize), ec_);
      if(ec_)
      {
        std::cout << "saveBlockFile error" << '\n';
        statu_ = statu3;
      } 
      std::string inputstr = std::string(&*data_.begin(), blocksize);
      std::string md5 = getstringMD5(inputstr);
      if(blockmd5 == getstringMD5(inputstr))
      {
        ++conter_;
        printf("\rreceive block nums: %d", conter_);
        std::ofstream f_out;
        std::string filename;
        filename = fileName_ + md5 + ".txt";
        f_out.open(filename, std::ios::out | std::ios::app); 
        f_out << inputstr;
        f_out.close();
      }
    }
  }
```

### 发送文件
- 存储端接收客户端的请求，download函数先去获取存储文件的文件夹名，然后调用readFileList函数遍历文件夹下的块文件，发送给客户端。

#### 存储端遍历block，并发送给客户端
- 实现方式：根据文件名找到存储块的文件夹，遍历文件夹下的所有块，并发送给客户端
```c++
 void readFileList()
  {
    DIR *dir;
    if ((dir=opendir(fileName_.data())) == NULL)
    {
      perror("Open dir error...");
    }
    struct dirent *ptr;
    while ((ptr=readdir(dir)) != NULL)
    {
      if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)    //current dir OR parrent dir
        continue;
      else
      {
        std::string Name = ptr->d_name;
        std::string fileName = fileName_ + Name;
        int fd = open(fileName.data(),O_RDONLY);
        if(-1 == fd)
        {
          perror("ferror");
        }
        struct stat fs;
        fstat(fd, &fs);
        uint32_t bsbe = htonl(fs.st_size);
        std::string sendbuf = "";
        sendbuf.append(static_cast<const char*>(static_cast<const void*>(&bsbe)), head_length);
        sendbuf.append(Name.data(), Name.size());
        int n, location = 0, filesize = fs.st_size;
        char *buff = (char*)malloc(max_length*sizeof(char));
        while((n = read(fd, buff+location, filesize)) < filesize)
        {
          location += n;
          filesize -= n;
        }
        sendbuf.append(std::string(buff, filesize).data(), filesize);
        boost::asio::write(socket_, boost::asio::buffer(sendbuf.data(), sendbuf.size()), ec_);
        if(ec_)
        {
          return ;
        }
        close(fd);
      }
    }
    closedir(dir);
  }
```

## Mini Program 中MySQL部分的使用

### 简介

定义了`mysql_operation.h`头文件，文件中创建了`DBhelper`类，设计不同的接口实现了MySQL针对该羡慕的更新与查询工作；

​    数据库中分为总表和分表，总表名为`files`，分表名为文件名（示例中使用名为`AAAAA`的文件作为demo），如图所示：

```mysql
+----------------+
| Tables_in_FILE |
+----------------+
| AAAAA          |
| files          |
+----------------+
```

`files`表中记录了文件的ID、文件名、文件的MD5值，示例如图所示：

```mysql
+----+-------+----------------+-----------+
| id | name  | file_md5       | block_num |
+----+-------+----------------+-----------+
|  0 | AAAAA | A1B2C3D4E5F6G7 |         3 |
+----+-------+----------------+-----------+
```

用文件名命名的分表（例如表`AAAAA`）里面记录了每个块的ID、MD5值和上传状态，

```mysql
+----+----------------------------------+-------------+
| id | block_md5                        | upload_stat |
+----+----------------------------------+-------------+
|  3 | 0AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA1 |           1 |
|  1 | 0BBBBBBBBBBBBBBBBBBBBBBBBBBBBBB1 |           1 |
|  2 | 0CCCCCCCCCCCCCCCCCCCCCCCCCCCCCC1 |           1 |
+----+----------------------------------+-------------+
```

为了保证`files`表在数据库中是是唯一的，创建总表是手动进行的，没有在类中提供接口。

 

### 变量

MySQL配置：

`MYSQL mysql`

`std::string host`

`std::string user`

`std::string passwd`

`std::string db`

`unsigned int port`

记录查询结果：

`MYSQL_RES *query_result`

记录文件的数量

`int file_cnt=0`

### 构造函数和析构函数

构造函数在创建对象的时候完成3个功能：首先是通过列表初始化完成参数的配置，然后对MYSQL类型的对象进行初始化，最后完成数据库连接，实现如下：

```c++
DBhelper():host("localhost"),user("root"),passwd("jingxiangyi"), 
            port(3306), db("FILE"),query_result(nullptr)
{
    mysql_init(&mysql); //完成初始化
    conn_db(); //连接数据库
}
```

析构函数在对象生命周期结束的时候，关闭连接，释放资源，实现如下：

```c++
~DBhelper()
{
    mysql_close(&mysql); // 关闭连接
    std::cout << "Connection closed!" << std::endl;
}
```



### 接口
#### 查看文件是否上传：
- 实现方式：输入文件名字符串file_name，查询files表里面是否存在该文件的记录，并返回bool型结果

```c++
bool check_file_exist(const std::string &file_name)
{
    std::string query_str = "SELECT * FROM files WHERE name=\'"+file_name+"\'";
    if(mysql_real_query(&mysql, query_str.c_str(), query_str.length())!=0){
        std::cout << "check_file_exist(): " 
                  << mysql_error(&mysql) << std::endl;
        
        return false;
    }
    return mysql_num_rows(mysql_store_result(&mysql))==1;
}
```

#### 为文件单独创建分表

- 实现方式：输入文件名字符串file_name，创建名为file_name的表

```c++
void db_add_file_table(const std::string &file_name)
{
    std::string query_str = "CREATE TABLE `" + file_name 
                                + "` (id int NOT NULL, block_md5 char(33) NOT NULL, "
                                + "upload_stat int NOT NULL, PRIMARY KEY (block_md5))"                                       + "ENGINE=InnoDB";
    if(mysql_real_query(&mysql, query_str.c_str(), 
                    query_str.length())!=0){
        std::cout << "db_add_file_table(): " 
                  << mysql_error(&mysql) << std::endl;
        return;
    }
}
```

#### 在总表中添加文件信息

- 实现方式：在files总表中添加一个名为file_name的文件，包括文件名、id号、md5值以及block数量等

```c++
void db_add_file_row(const std::string &file_name, 
                    const int &block_num, 
                    const std::string &file_md5)
{
    std::string query_str = "INSERT INTO files VALUES(" 
                            + std::to_string(file_cnt) + ", " // 文件的id号
                            + "\'" + file_name + "\', " 
                            + "\'" + file_md5 + "\', "
                            + std::to_string(block_num) + ")"; // 记录block的数量
    
    if(mysql_real_query(&mysql, query_str.c_str(), 
                    query_str.length())!=0){
        std::cout << "db_add_file_row(): " 
                  << mysql_error(&mysql) << std::endl;
        return;
    }
    file_cnt++; //文件的id号+1
}
```

#### 在数据库中创建需要上传的新文件

- 实现方式：调用`db_add_file_table`和`db_add_file_row`

```c++
void create_file(const std::string &file_name, const int &block_num, const std::string &file_md5)
{
    db_add_file_table(file_name);
    db_add_file_row(file_name, block_num, file_md5);
    std::cout << "File " << file_name << " created with " 
              << std::to_string(block_num) << " blocks" << std::endl;
}
```

#### 在分表中添加指定文件的一个block的信息

- 实现方式：输入文件名字符串file_name、块的信息，向分表中添加包含block信息的行，信息包括ID、MD5、上传状态(默认未上传)

```c++
void db_add_block_row(const std::string &file_name, const int &block_id, const std::string &block_md5){
    std::string query_str = "INSERT INTO " + file_name + " VALUES(" 
                            + std::to_string(block_id) + ", " // 块的id号
                            + "\'" + block_md5 + "\', " // 块的md5值
                            + "0)";// 块的上传状态（默认未完成为0）
    
    if(mysql_real_query(&mysql, query_str.c_str(), 
                    query_str.length())!=0){
        std::cout << "db_add_block_row(): " 
        << mysql_error(&mysql) << std::endl;
        return;
    }
}
```

#### 在分表中添加指定文件的所有的block信息

- 实现方式：输入文件名字符串file_name、所有块的MD5值构成的字符串，进行解析后，将每个块的MD5值作为输入调用`db_add_block_row`，完成针对每个块的添加操作

```c++
void set_block_md5(const std::string &file_name, const std::string &md5_string){
    int block_id = 0;
    while(block_id*32 < md5_string.length()){
        std::string temp_md5 = md5_string.substr(block_id*32, 32);
        //std::cout << temp_md5 << std::endl;
        db_add_block_row(file_name, block_id, temp_md5);
        block_id++;
    }
}
```

#### 更新block的上传状态

- 实现方式：输入文件名字符串file_name、对应block的id，进行状态更改

```c++
void set_block_uploaded(const std::string &file_name, const int &block_id){
    std::string query_str = "UPDATE " + file_name // 设置文件名，找到表
                            + " SET upload_stat=1 WHERE id=" 
                            + std::to_string(block_id); // 设置上传状态
    if(mysql_real_query(&mysql, query_str.c_str(), 
                    query_str.length())!=0){
        std::cout << "set_block_uploaded(): " 
        << mysql_error(&mysql) << std::endl;
        return;
    }
    //std::cout << "Updated block: " << std::to_string(block_id) << std::endl;
}
```

#### 检查文件上传状态

- 实现方式：输入文件名字符串file_name，检查上传状态为0的个数，返回结果为bool型

```c++
bool check_upload_status(const std::string &file_name){
    std::string query_str = "SELECT * FROM " + file_name + " WHERE upload_stat=0";
    if(mysql_real_query(&mysql, query_str.c_str(), query_str.length())!=0){
        std::cout << "check_upload_status(): " 
        << mysql_error(&mysql) << std::endl;
        return false;
    }
    return mysql_num_rows(mysql_store_result(&mysql))==0;
}
```

#### 根据ID查询block的MD5值

- 实现方式：输入文件名字符串file_name、对应block的id，返回block的MD5值

```c++
std::string get_block_md5(const int &block_id, const std::string &file_name){
    std::string query_str = "SELECT block_md5 FROM " + file_name + " WHERE id=" + std::to_string(block_id);
    if(mysql_real_query(&mysql, query_str.c_str(), 
                    query_str.length())!=0){
        std::cout << "get_block_md5(): " 
                  << mysql_error(&mysql) << std::endl;
        return "";
    }
    MYSQL_ROW row = mysql_fetch_row(mysql_store_result(&mysql));
    return row[0];
}
```

#### 查询上传成功的block_id

- 实现方式：查询上传状态为1的block，返回id值的字符串，中间用冒号隔开

```c++
std::string get_block_id_string(const std::string &file_name){
    std::string temp_string;
    std::string query_str = "SELECT id FROM " + file_name + " WHERE upload_stat=1";
    if(mysql_real_query(&mysql, query_str.c_str(), 
                    query_str.length())!=0){
        std::cout << "get_block_id_string(): " 
        << mysql_error(&mysql) << std::endl;
        return "";
    }
    query_result = mysql_store_result(&mysql);
    MYSQL_ROW row;
    int rows = mysql_num_rows(query_result);
    while (row = mysql_fetch_row(query_result)) {
            temp_string += std::string(row[0]) + ":";
    }
    if(temp_string.length()>0){
        temp_string.pop_back();
    }
    return temp_string;
}
```

#### 查询文件的block总数量

- 实现方式：输入文件名字符串，在files总表中查询，返回int型的结果

```c++
int get_block_num(const std::string &file_name){
    std::string query_str = "SELECT block_num FROM files WHERE name =\'" 
                            + file_name + "\'";
    if(mysql_real_query(&mysql, query_str.c_str(), 
                    query_str.length())!=0){
        std::cout << "get_block_nums(): " 
        << mysql_error(&mysql) << std::endl;
        return -1;
    }
    MYSQL_ROW row = mysql_fetch_row(mysql_store_result(&mysql));
    return atoi(row[0]);
}
```

#### 查询所有block的MD5值，拼接后返回字符串

- 实现方式：输入文件名，在分表中进行对id排序后的MD5查询，完成拼接操作后返回

```c++
std::string get_file_block_md5(const std::string &file_name){
    std::string query_str = "SELECT block_md5 FROM `" + file_name + "` ORDER BY id ASC" ;
    //std::string query_str = "SELECT * FROM AAAAA ";//WHERE id=0";
    if(mysql_real_query(&mysql, query_str.c_str(), 
                    query_str.length())!=0){
        std::cout << "get_file_block_md5(): " 
        << mysql_error(&mysql) << std::endl;
        return "";
    }

    std::string temp_string;
    query_result = mysql_store_result(&mysql);
    MYSQL_ROW row;
    int rows = mysql_num_rows(query_result);
    //cout << "The total rows is: " << rows << endl;
    int fields = mysql_num_fields(query_result);
    //cout << "The total fields is: " << fields << endl;
    while (row = mysql_fetch_row(query_result)) {
            temp_string += std::string(row[0]);
    }

    return temp_string;
}
```
