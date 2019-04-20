// filesys.cc
//	Routines to manage the overall operation of the file system.
//	Implements routines to map from textual file names to files.
//
//	Each file in the file system has:
//	   A file header, stored in a sector on disk
//		(the size of the file header data structure is arranged
//		to be precisely the size of 1 disk sector)
//	   A number of data blocks
//	   An entry in the file system directory
//
// 	The file system consists of several data structures:
//	   A bitmap of free disk sectors (cf. bitmap.h)
//	   A directory of file names and file headers
//
//      Both the bitmap and the directory are represented as normal
//	files.  Their file headers are located in specific sectors
//	(sector 0 and sector 1), so that the file system can find them
//	on bootup.
//
//	The file system assumes that the bitmap and directory files are
//	kept "open" continuously while Nachos is running.
//
//	For those operations (such as Create, Remove) that modify the
//	directory and/or bitmap, if the operation succeeds, the changes
//	are written immediately back to disk (the two files are kept
//	open during all this time).  If the operation fails, and we have
//	modified part of the directory and/or bitmap, we simply discard
//	the changed version, without writing it back to disk.
//
// 	Our implementation at this point has the following restrictions:
//
//	   there is no synchronization for concurrent accesses
//	   files have a fixed size, set when the file is created
//	   files cannot be bigger than about 3KB in size
//	   there is no hierarchical directory structure, and only a limited
//	     number of files can be added to the system
//	   there is no attempt to make the system robust to failures
//	    (if Nachos exits in the middle of an operation that modifies
//	    the file system, it may corrupt the disk)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.
#ifndef FILESYS_STUB

#include "copyright.h"
#include "debug.h"
#include "disk.h"
#include "pbitmap.h"
#include "directory.h"
#include "filehdr.h"
#include "filesys.h"

// Sectors containing the file headers for the bitmap of free sectors,
// and the directory of files.  These file headers are placed in well-known
// sectors, so that they can be located on boot-up.
#define FreeMapSector 0
#define DirectorySector 1

// Initial file sizes for the bitmap and directory; until the file system
// supports extensible files, the directory size sets the maximum number
// of files that can be loaded onto the disk.
#define FreeMapFileSize (NumSectors / BitsInByte)
#define NumDirEntries 10
#define DirectoryFileSize (sizeof(DirectoryEntry) * NumDirEntries)

//----------------------------------------------------------------------
// FileSystem::FileSystem
// 	Initialize the file system.  If format = TRUE, the disk has
//	nothing on it, and we need to initialize the disk to contain
//	an empty directory, and a bitmap of free sectors (with almost but
//	not all of the sectors marked as free).
//
//	If format = FALSE, we just have to open the files
//	representing the bitmap and the directory.
//
//	"format" -- should we initialize the disk?
//----------------------------------------------------------------------

FileSystem::FileSystem(bool format)
{
    DEBUG(dbgFile, "Initializing the file system.");
    if (format)
    {
        PersistentBitmap *freeMap = new PersistentBitmap(NumSectors);
        Directory *directory = new Directory(NumDirEntries);
        FileHeader *mapHdr = new FileHeader;
        FileHeader *dirHdr = new FileHeader;

        DEBUG(dbgFile, "Formatting the file system.");

        // First, allocate space for FileHeaders for the directory and bitmap
        // (make sure no one else grabs these!)
        freeMap->Mark(FreeMapSector);
        freeMap->Mark(DirectorySector);

        // Second, allocate space for the data blocks containing the contents
        // of the directory and bitmap files.  There better be enough space!

        ASSERT(mapHdr->Allocate(freeMap, FreeMapFileSize));
        ASSERT(dirHdr->Allocate(freeMap, DirectoryFileSize));
        // Flush the bitmap and directory FileHeaders back to disk
        // We need to do this before we can "Open" the file, since open
        // reads the file header off of disk (and currently the disk has garbage
        // on it!).

        DEBUG(dbgFile, "Writing headers back to disk.");
        mapHdr->WriteBack(FreeMapSector);
        dirHdr->WriteBack(DirectorySector);

        // OK to open the bitmap and directory files now
        // The file system operations assume these two files are left open
        // while Nachos is running.

        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);

        // Once we have the files "open", we can write the initial version
        // of each file back to disk.  The directory at this point is completely
        // empty; but the bitmap has been changed to reflect the fact that
        // sectors on the disk have been allocated for the file headers and
        // to hold the file data for the directory and bitmap.

        DEBUG(dbgFile, "Writing bitmap and directory back to disk.");
        freeMap->WriteBack(freeMapFile); // flush changes to disk
        directory->WriteBack(directoryFile);
        if (debug->IsEnabled('f'))
        {
            freeMap->Print();
            directory->Print();
        }
        delete freeMap;
        delete directory;
        delete mapHdr;
        delete dirHdr;
    }
    else
    {
        // if we are not formatting the disk, just open the files representing
        // the bitmap and directory; these are left open while Nachos is running
        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);
    }
}

//----------------------------------------------------------------------
// FileSystem::Create
// 	Create a file in the Nachos file system (similar to UNIX create).
//	Since we can't increase the size of files dynamically, we have
//	to give Create the initial size of the file.
//
//	The steps to create a file are:
//	  Make sure the file doesn't already exist
//        Allocate a sector for the file header
// 	  Allocate space on disk for the data blocks for the file
//	  Add the name to the directory
//	  Store the new file header on disk
//	  Flush the changes to the bitmap and the directory back to disk
//
//	Return TRUE if everything goes ok, otherwise, return FALSE.
//
// 	Create fails if:
//   		file is already in directory
//	 	no free space for file header
//	 	no free entry for file in directory
//	 	no free space for data blocks for the file
//
// 	Note that this implementation assumes there is no concurrent access
//	to the file system!
//
//	"name" -- name of file to be created
//	"initialSize" -- size of file to be created
//----------------------------------------------------------------------

bool FileSystem::Create(char *name, int initialSize)//新建一个文件
{
    Directory *currentDirectory;
    OpenFile *currentDirectoryFile = directoryFile;//打开目录文件
    PersistentBitmap *freeMap;
    FileHeader *hdr;
    int sector;
    bool success;

    DEBUG(dbgFile, "Creating file " << name << " size " << initialSize);

    currentDirectory = new Directory(NumDirEntries);//分配目录内存空间
    currentDirectory->FetchFrom(currentDirectoryFile);//从目录文件读取目录信息
    //currentDirectory->Print();
    vector<string> folderStrVec;//新建向量用以存储访问每个子目录时的名字
    char* tmpStr = strtok(name, "/");
    while (tmpStr != NULL)
    {
        folderStrVec.push_back(string(tmpStr));
        tmpStr = strtok(NULL, "/");
    }//完成对目录名的解析
    /* +++++ LAB3 +++++ */
    // 洞7:begin
    int i;
    for (i = 0; i < folderStrVec.size() - 1; i++) {
    sector=currentDirectory->Find((char *) folderStrVec[i].c_str(), false);//此处为非数据恢复性查找
    if(sector==-1) return false;
    else{
	currentDirectoryFile=new OpenFile(sector);
	currentDirectory = new Directory(NumDirEntries);
	currentDirectory->FetchFrom(currentDirectoryFile);
    }
    }
    // 洞7:end
    /* +++++++++++++++++ */

    name = (char *) folderStrVec[i].c_str();

    if (currentDirectory->Find(name) != -1)
        success = FALSE; // file is already in directory
    else
    {
        freeMap = new PersistentBitmap(freeMapFile, NumSectors);
        sector = freeMap->FindAndSet(); // find a sector to hold the file header
        if (sector == -1)
            success = FALSE; // no free block for file header
        else if (!currentDirectory->Add(name, sector))
            success = FALSE; // no space in directory
        else
        {
            hdr = new FileHeader;
            if (!hdr->Allocate(freeMap, initialSize))
                success = FALSE; // no space on disk for data
            else
            {
                success = TRUE;
                // everthing worked, flush all changes back to disk
                hdr->WriteBack(sector);
                currentDirectory->WriteBack(currentDirectoryFile);
                freeMap->WriteBack(freeMapFile);
            }
            delete hdr;
        }
        delete freeMap;
    }
    delete currentDirectory;
    if (currentDirectoryFile != directoryFile)
        delete currentDirectoryFile;
    //printf("create %s  %d\n",name ,success);
    return success;
}


/* +++++ LAB3 +++++ */
// 洞8:begin
bool FileSystem::CreateFolder(char *name)
{
    Directory *currentDirectory;
    OpenFile *currentDirectoryFile = directoryFile;//打开目录文件
    PersistentBitmap *freeMap;
    FileHeader *hdr;
    int sector;
    bool success;
     //printf("success\n");
    DEBUG(dbgFile, "Creating folder " << name );

    currentDirectory = new Directory(NumDirEntries);//分配目录内存空间
    currentDirectory->FetchFrom(currentDirectoryFile);//从目录文件读取目录信息
   // currentDirectory->Print();
    vector<string> folderStrVec;//新建向量用以存储访问每个子目录时的名字
    //printf("complete str %s\n",name);
    char* tmpStr = strtok(name, "/");
    while (tmpStr != NULL)
    {
        folderStrVec.push_back(string(tmpStr));
        tmpStr = strtok(NULL, "/");
    }//完成对目录名的解析
  
    int i;
    for (i = 0; i < folderStrVec.size() - 1; i++) {
     //printf("success2,%d\n%s\n%s\n",folderStrVec.size(),(char *) folderStrVec[0].c_str(),(char *) folderStrVec[1].c_str());
      sector=currentDirectory->Find((char *) folderStrVec[i].c_str(), false);//此处为非数据恢复性查找
      //printf("find result:%d\n",sector);
    if(sector==-1) 
    {
      //printf("can't find dir");
      return false;
    }
      else{
	currentDirectoryFile=new OpenFile(sector);
	currentDirectory = new Directory(NumDirEntries);
	currentDirectory->FetchFrom(currentDirectoryFile);
	//currentDirectory->Print();
    }
    }

    name = (char *) folderStrVec[i].c_str();
    //printf("now name :%s\n",name);
    
    if (currentDirectory->Find(name,false) != -1){
        success = FALSE; // file is already in directory
        //printf("already in\n");
    }
    else
    {
        freeMap = new PersistentBitmap(freeMapFile, NumSectors);
        sector = freeMap->FindAndSet(); // find a sector to hold the file header
        if (sector == -1)
            success = FALSE; // no free block for file header
        else if (!currentDirectory->Add(name, sector))
            success = FALSE; // no space in directory
        else
        {
            hdr = new FileHeader;
            if (!hdr->Allocate(freeMap, DirectoryFileSize))
                success = FALSE; // no space on disk for data
            else
            {
                //printf("success\n");
		success = TRUE;
                // everthing worked, flush all changes back to disk
                hdr->WriteBack(sector);
		Directory *newDirectory = new Directory(NumDirEntries);
		OpenFile *newdirectoryFile = new OpenFile(sector);
		newDirectory->WriteBack(newdirectoryFile);
                currentDirectory->WriteBack(currentDirectoryFile);
                freeMap->WriteBack(freeMapFile);
            }
            delete hdr;
        }
        delete freeMap;
    }
    delete currentDirectory;
    if (currentDirectoryFile != directoryFile)
        delete currentDirectoryFile;
    //printf("create %s  %d\n",name ,success);
   //if(success) printf("return true\n");
    return success;
}
// 洞8:end
/* +++++++++++++++++ */


//----------------------------------------------------------------------
// FileSystem::Open
// 	Open a file for reading and writing.
//	To open a file:
//	  Find the location of the file's header, using the directory
//	  Bring the header into memory
//
//	"name" -- the text name of the file to be opened
//----------------------------------------------------------------------

OpenFile *
FileSystem::Open(char *name)
{
    int i;
    Directory *directory = new Directory(NumDirEntries);
    OpenFile *openFile = NULL;
    int sector;
    DEBUG(dbgFile, "Opening file" << name);
    directory->FetchFrom(directoryFile);

    OpenFile *currentDirectoryFile = directoryFile;
    vector<string> folderStrVec;
    char* tmpStr = strtok(name, "/");
    while (tmpStr != NULL)
    {
        folderStrVec.push_back(string(tmpStr));
        tmpStr = strtok(NULL, "/");
    }
    /* +++++ LAB3 +++++ */
    // 洞9:begin
    //   将name包含的目录名解析出来，然后从根目录开始跳转定位到当前目录。
    //   解析出来的目录名可以用表达式”(char*)folderStrVec[i]”来访问。
    //   若name中包含的目录不存在，则认为失败并返回FALSE。

    for (i = 0; i < folderStrVec.size() - 1; i++) {
	sector=directory->Find((char *) folderStrVec[i].c_str(), false);//此处为非数据恢复性查找
	if(sector==-1) return false;
	else{
	    currentDirectoryFile=new OpenFile(sector);
	    directory = new Directory(NumDirEntries);
	    directory->FetchFrom(currentDirectoryFile);
    }
    }
    // 洞9:end
    /* +++++++++++++++++ */

    name = (char *) folderStrVec[i].c_str();

    sector = directory->Find(name);
    /* +++++ LAB3 +++++ */
    // 洞6:begin
    //   你需要修改并填补这里
    //ASSERT(sector!=-1);
    if(sector!=-1){
    openFile = new OpenFile(sector);
    openFile->setId(-1);
    for(i=3;openFile->getId()==-1;i++)
	if(getFile(i)==NULL){
	    openFile->setId(i);
	    addFile(openFile->getId(),openFile);
	}
    }
    // 洞6:end
    /* +++++++++++++++++ */
    delete directory;
    if (currentDirectoryFile != directoryFile)
        delete currentDirectoryFile;
    return openFile; // return NULL if not found or reached max file number
}

//----------------------------------------------------------------------
// FileSystem::Remove
// 	Delete a file from the file system.  This requires:
//	    Remove it from the directory
//	    Delete the space for its header
//	    Delete the space for its data blocks
//	    Write changes to directory, bitmap back to disk
//
//	Return TRUE if the file was deleted, FALSE if the file wasn't
//	in the file system.
//
//	"name" -- the text name of the file to be removed
//----------------------------------------------------------------------

bool FileSystem::Remove(char *name)
{
    Directory *directory;
    PersistentBitmap *freeMap;
    FileHeader *fileHdr;
    int sector;
    int i;

    directory = new Directory(NumDirEntries);
    directory->FetchFrom(directoryFile);

    OpenFile *currentDirectoryFile = directoryFile;
    vector<string> folderStrVec;
    char* tmpStr = strtok(name, "/");
    while (tmpStr != NULL)
    {
        folderStrVec.push_back(string(tmpStr));
        tmpStr = strtok(NULL, "/");
    }
    /* +++++ LAB3 +++++ */
    // 洞10:begin
    //   将name包含的目录名解析出来，然后从根目录开始跳转定位到当前目录。
    //   解析出来的目录名可以用表达式”(char*)folderStrVec[i]”来访问。
    //   若name中包含的目录不存在，则认为失败并返回FALSE。

    for (i = 0; i < folderStrVec.size() - 1; i++) {
	sector=directory->Find((char *) folderStrVec[i].c_str(), false);//此处为非数据恢复性查找
	if(sector==-1) return false;
	else{
	     currentDirectoryFile=new OpenFile(sector);
	    directory = new Directory(NumDirEntries);
	    directory->FetchFrom(currentDirectoryFile);
	}
    }
    // 洞10:end
    /* +++++++++++++++++ */

    name = (char *) folderStrVec[i].c_str();

    sector = directory->Find(name);
    if (sector == -1)
    {
        delete directory;
        if (currentDirectoryFile != directoryFile)
            delete currentDirectoryFile;
        return FALSE; // file not found
    }
    fileHdr = new FileHeader;
    fileHdr->FetchFrom(sector);

    freeMap = new PersistentBitmap(freeMapFile, NumSectors);

    fileHdr->Deallocate(freeMap); // remove data blocks
    freeMap->Clear(sector);       // remove header block
    directory->Remove(name);

    freeMap->WriteBack(freeMapFile);     // flush to disk
    directory->WriteBack(currentDirectoryFile); // flush to disk
    delete fileHdr;
    delete directory;
    if (currentDirectoryFile != directoryFile)
        delete currentDirectoryFile;
    delete freeMap;
    return TRUE;
}

/* +++++ LAB3 +++++ */
// 洞11:begin
//   将srcName包含的目录名解析出来，然后从根目录开始跳转定位到当前目录。
//   若srcName中包含的目录不存在，则认为失败并返回FALSE。
//   最后在当前目录下判断是否能够恢复（即目录表项是否还有条目的名字为待恢复文件的名字），
//   若能的话，恢复相应的文件并存放到原生Linux系统下的dstName文件中。
//   注：实现该函数可以参考FileSystem::Create函数。
bool FileSystem::Recover(char *srcName, char *dstName) {
    Directory *currentDirectory;
    OpenFile *currentDirectoryFile = directoryFile;//打开目录文件
    PersistentBitmap *freeMap;
    FileHeader *hdr;
    int sector,filelength;
    char *buf;
    
    currentDirectory = new Directory(NumDirEntries);//分配目录内存空间
    currentDirectory->FetchFrom(currentDirectoryFile);//从目录文件读取目录信息

    vector<string> folderStrVec;//新建向量用以存储访问每个子目录时的名字
    char* tmpStr = strtok(srcName, "/");
    while (tmpStr != NULL)
    {
        folderStrVec.push_back(string(tmpStr));
        tmpStr = strtok(NULL, "/");
    }//完成对目录名的解析
  
    int i;
    for (i = 0; i < folderStrVec.size() - 1; i++) {
    sector=currentDirectory->Find((char *) folderStrVec[i].c_str(), false);//此处为非数据恢复性查找
    if(sector==-1) return false;
    else{
	 currentDirectoryFile=new OpenFile(sector);
	 currentDirectory= new Directory(NumDirEntries);
	currentDirectory->FetchFrom(currentDirectoryFile);
    }
    }

    char *name = (char *) folderStrVec[i].c_str();
    //sector=currentDirectory->Find(name,false) ;
   // if(sector == -1) printf("it has been delete!\n");
    sector=currentDirectory->Find(name,true) ;
    if (sector == -1)
        return FALSE; // file is already in directory
    else
    {
       //printf("find recovery file!\n");
	OpenFile *RecoverFile =new OpenFile(sector);
	filelength=RecoverFile->getHdr()->FileLength();
	buf=new char[filelength];
	RecoverFile->Read(buf,filelength);
    }
    delete currentDirectory;
    if (currentDirectoryFile != directoryFile)
        delete currentDirectoryFile;
    //printf("create %s  %d\n",name ,success);
    
    FILE *out = fopen(dstName, "w");
    fprintf(out,"%s",buf);
    //printf("%s",buf);
    fclose(out);

    return true;
}
// 洞11:end
/* +++++++++++++++++ */

//----------------------------------------------------------------------
// FileSystem::List
// 	List all the files in the file system directory.
//----------------------------------------------------------------------

void FileSystem::List()
{
    Directory *directory = new Directory(NumDirEntries);

    directory->FetchFrom(directoryFile);
    directory->List();
    delete directory;
}

//----------------------------------------------------------------------
// FileSystem::Print
// 	Print everything about the file system:
//	  the contents of the bitmap
//	  the contents of the directory
//	  for each file in the directory,
//	      the contents of the file header
//	      the data in the file
//----------------------------------------------------------------------

void FileSystem::Print()
{
    FileHeader *bitHdr = new FileHeader;
    FileHeader *dirHdr = new FileHeader;
    PersistentBitmap *freeMap = new PersistentBitmap(freeMapFile, NumSectors);
    Directory *directory = new Directory(NumDirEntries);

    printf("Bit map file header:\n");
    bitHdr->FetchFrom(FreeMapSector);
    bitHdr->Print();

    printf("Directory file header:\n");
    dirHdr->FetchFrom(DirectorySector);
    dirHdr->Print();

    freeMap->Print();

    directory->FetchFrom(directoryFile);
    directory->Print();

    delete bitHdr;
    delete dirHdr;
    delete freeMap;
    delete directory;
}

/* ++++++++++++++++++ LAB3 可能需要阅读这里 ++++++++++++++++++ */
int
FileSystem::Close(int fileId){
    return removeFile(fileId);
}

int 
FileSystem::Write(char *buffer, int size, int fid){ 
    OpenFile* file = getFile(fid);
    if(file==NULL){
        printf("file did not open\n");
        return -1;
    }
    return file->Write(buffer, size);
}

int 
FileSystem::Read(char* buffer, int size, int fid){
    OpenFile* file = getFile(fid);
    if(file==NULL){
        printf("file did not open\n");
        return -1;
    }
    return file->Read(buffer,size);
}

OpenFile *
FileSystem::getFile(int fileId)
{
    return openedFile[fileId];
}

void FileSystem::addFile(int fileId, OpenFile *file)
{
    openedFile[fileId] = file;
}

int FileSystem::removeFile(int fileId)
{
    OpenFile *file = openedFile[fileId];
    if (file != NULL)
    {
        delete file;
        openedFile.erase(fileId);
        return 1;
    }
    return -1;
}

/* +++++++++++++++++++++++++++++++++++++++++++ */

#endif // FILESYS_STUB
