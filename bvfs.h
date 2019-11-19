#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>


struct inode {
  int size;
  char filename[32];
  timeval timestamp;
  short references[128];
} typedef inode;


struct fileDescriptor {
  int cursor;
  int mode;
  inode* file;
} typedef fileDescriptor;


//the file descriptor for the file serving as the file system
int fsFile;

//an array of inodes
inode* inodes;

//the free node pointed to by the superblock
int freeNode;

//file descriptor table and the information needed for it
fileDescriptor* fdTable;
int fdSize;
int fdCapacity;

// Prototypes
int bv_init(const char *fs_fileName);
// TODO
int bv_destroy();
// TODO
int bv_open(const char *fileName, int mode);
// TODO
int bv_close(int bvfs_FD);
// TODO
int bv_write(int bvfs_FD, const void *buf, size_t count);
// TODO
int bv_read(int bvfs_FD, void *buf, size_t count);
// TODO
int bv_unlink(const char* fileName);
// TODO
void bv_ls();


void debug() {
  for(int i=0; i<256; i++) {
    printf("%d: %d : %d\n", i, inodes[i].size, inodes[i].references[2]);
  }
  printf("Freenode at: %d\n", freeNode);
  printf("fdcapcity: %d\n", fdCapacity);
  printf("fdsize: %d\n", fdSize);

  int i=freeNode;
  int temp;
  while(i!=0) {
    lseek(fsFile, i, SEEK_SET);
    read(fsFile, (void*)&temp, sizeof(int));
    if( i-512 == temp) {
      printf("good: %d\n", temp);
    }
    else {
      printf("VERY, VERY BAD\n");
      printf("%d %d\n", i, temp);
    }
    i = temp;
  }
}


void initGlobals() {
  //read all of the inodes
  inodes = (inode*)malloc(256*sizeof(inode));
  for(int i=0; i<256; i++) {
    lseek(fsFile, i*512, SEEK_SET);
    read(fsFile, (void*)(inodes+i), sizeof(inode));
  }

  //read whatever the superblock points to
  lseek(fsFile, 256*512, SEEK_SET);
  read(fsFile, (void*)&freeNode, sizeof(int));

  //set up the file descriptor table
  fdTable = (fileDescriptor*)malloc(8*sizeof(fileDescriptor));
  fdSize = 0;
  fdCapacity = 8;
  for(int i=0; i<fdCapacity; i++) {
    fdTable[i].cursor = -1;
  }
}

void growfdTable() {
  //make a new table with a higher capacity
  fdCapacity *= 2;
  fileDescriptor* newTable = (fileDescriptor*)malloc(fdCapacity*sizeof(fileDescriptor));

  //copy data over to the new table
  for(int i=0; i<fdSize; i++) {
    newTable[i] = fdTable[i];
  }

  //populate the rest of the table with empty descriptors
  for(int i=fdSize; i<fdCapacity; i++) {
    newTable[i].cursor = -1;
  }

  //delete the old table and set new new table as the table to use;
  free(fdTable);
  fdTable = newTable;
}

int getFD() {
  if(fdSize == fdCapacity) {
    //grow the table and return the fd at index size
    growfdTable();
    fdTable[fdSize].cursor = 0;
    return fdSize++;
  }
  else {
    //look for the first empty fd
    for(int i=0; i<fdCapacity; i++) {
      if(fdTable[i].cursor == -1) {
        fdTable[i].cursor = 0;
        fdSize++;
        return i;
      }
    }
  }
}

void closeFD(int index) {
  if(index < 0 || index > fdCapacity) {
    return;
  }
  else if(fdTable[index].cursor != -1) {
    fdTable[index].cursor = -1;
    fdSize--;
  }
}


short getBlock() {
  int retBlock = freeNode;

  //0 means that there is no more free blocks
  //if we're at 0:
  //  we want to return 0
  //  we don't want to read in whatever garbage is at 0
  if(freeNode != 0) {
    //write to the free node
    lseek(fsFile, freeNode, SEEK_SET);
    read(fsFile, (void*)&freeNode, sizeof(int));

    //write the new free node to the superblock
    lseek(fsFile, 256*512, SEEK_SET);
    write(fsFile,(void*)&freeNode,sizeof(int));
  }

  return (short)(retBlock/512);
}

void freeBlock(int index) {
  //index should be the index of the block.
  //NOT the offset in the file
  int fileOffset = index*512;

  //set the new free node up at its offset
  lseek(fsFile, fileOffset, SEEK_SET);
  write(fsFile, (void*)&freeNode, sizeof(int));
  freeNode = fileOffset;

  //set the new freeNode in the superBlock
  lseek(fsFile, 256*512, SEEK_SET);
  write(fsFile, (void*)freeNode, sizeof(int));
}


/*
 * int bv_init(const char *fs_fileName);
 *
 * Initializes the bvfs file system based on the provided file. This file will
 * contain the entire stored file system. Invocation of this function will do
 * one of two things:
 *
 *   1) If the file (fs_fileName) exists, the function will initialize in-memory
 *   data structures to help manage the file system methods that may be invoked.
 *
 *   2) If the file (fs_fileName) does not exist, the function will create that
 *   file as the representation of a new file system and initialize in-memory
 *   data structures to help manage the file system methods that may be invoked.
 *
 * Input Parameters
 *   fs_fileName: A c-string representing the file on disk that stores the bvfs
 *   file system data.
 *
 * Return Value
 *   int:  0 if the initialization succeeded.
 *        -1 if the initialization failed (eg. file not found, access denied,
 *           etc.). Also, print a meaningful error to stderr prior to returning.
 */
int bv_init(const char *fs_fileName) {
  // Get partition name from command line argument
  fsFile = open(fs_fileName, O_CREAT | O_RDWR | O_EXCL, 0644);
  if (fsFile < 0 && errno == EEXIST) {
    // File already exists. Open it and read info
    fsFile = open(fs_fileName, O_CREAT | O_RDWR , S_IRUSR | S_IWUSR);
    initGlobals();
  }
  else if (fsFile < 0){
    // Something bad occurred. Just print the error
    printf("%s", strerror(errno));
    return -1;
  }
  else {
    // File did not previously exist but it does now. Write data to it
    inode tempNode;
    tempNode.size = -1;
    int lastBlock = 512*16383;

    // write empty inodes by setting their size to 0
    for(int i=0; i<256; i++) {
      lseek(fsFile, i*512, SEEK_SET);
      write(fsFile, (void*)&tempNode, sizeof(inode));
    }

    // write superblock pointer
    lseek(fsFile, 256*512, SEEK_SET);
    write(fsFile, (void*)&lastBlock, sizeof(int));

    //write pointers for each block to the one before it
    //the block closest to the superblock points to null
    for(int i= 16383*512; i>257*512; i-=512) {
      lastBlock = i-512;
      lseek(fsFile, i, SEEK_SET);
      write(fsFile, (void*)&lastBlock, sizeof(int));
    }
    lastBlock = 0;
    lseek(fsFile, 257*512, SEEK_SET);
    write(fsFile, (void*)&lastBlock, sizeof(int));

    initGlobals();
  }

  return 0;
}






/*
 * int bv_destroy();
 *
 * This is your opportunity to free any dynamically allocated resources and
 * perhaps to write any remaining changes to disk that are necessary to finalize
 * the bvfs file before exiting.
 *
 * Return Value
 *   int:  0 if the clean-up process succeeded.
 *        -1 if the clean-up process failed (eg. bv_init was not previously,
 *           called etc.). Also, print a meaningful error to stderr prior to
 *           returning.
 */
int bv_destroy() {
  close(fsFile);
  free(inodes);
  free(fdTable);
  return 0;
}







// Available Modes for bvfs (see bv_open below)
int BV_RDONLY = 0;
int BV_WCONCAT = 1;
int BV_WTRUNC = 2;

/*
 * int bv_open(const char *fileName, int mode);
 *
 * This function is intended to open a file in either read or write mode. The
 * above modes identify the method of access to utilize. If the file does not
 * exist, you will create it. The function should return a bvfs file descriptor
 * for the opened file which may be later used with bv_(close/write/read).
 *
 * Input Parameters
 *   fileName: A c-string representing the name of the file you wish to fetch
 *             (or create) in the bvfs file system.
 *   mode: The access mode to use for accessing the file
 *           - BV_RDONLY: Read only mode
 *           - BV_WCONCAT: Write only mode, appending to the end of the file
 *           - BV_WTRUNC: Write only mode, replacing the file and writing anew
 *
 * Return Value
 *   int: >=0 Greater-than or equal-to zero value representing the bvfs file
 *           descriptor on success.
 *        -1 if some kind of failure occurred. Also, print a meaningful error to
 *           stderr prior to returning.
 */
int bv_open(const char *fileName, int mode) {
  //deal with long file names
  if(strlen(fileName) > 31) {
    fprintf(stderr,"file name '%s' too long for open\n", fileName);
    return -1;
  }
  else if(mode < 0 || mode > 2) {
    fprintf(stderr,"invalid mode for open\n");
    return -1;
  }

  int found = 0;
  int inodeIndex;
  for(int i=0; i<256; i++) {
    if(strcmp(inodes[i].filename, fileName) == 0 && inodes[i].size != -1) {
      found = 0;
      inodeIndex = i;
      break;
    }
  }

  if(found) {
    int fd = getFD();
    fdTable[fd].file = inodes+inodeIndex;
    fdTable[fd].mode = mode;
    if(mode == BV_WTRUNC) {
      //find the number of blocks
      int numBlocks = (fdTable[fd].file->size/512)+1;
      if(fdTable[fd].file->size%512 == 0) {
        numBlocks--;
      }

      //work backwards to set block equal to each other
      //TODO modify to use freeBlock function
      for(int i=numBlocks-1; i>0; i--) {
        lseek(fsFile, fdTable[fd].file->references[i]*512, SEEK_SET);
        write(fsFile, (void*)&freeNode, sizeof(int));
        freeNode = fdTable[fd].file->references[i]*512;
      }

      //set the size of the inode to 0
      fdTable[fd].file->size = 0;
    }
    else if(mode == BV_WCONCAT) {
      //TODO set the cursor to the proper location
    }
    return fd;
  }
  else if(mode == BV_RDONLY) {
    fprintf(stderr, "file '%s' does not exist for reading", fileName);
    return -1;
  }
  else {
    int fd = getFD();
    //TODO find an empty inode
    //TODO set its size to -1 to make sure its file descriptor doesn't get taken
    return fd;
  }
}






/*
 * int bv_close(int bvfs_FD);
 *
 * This function is intended to close a file that was previously opened via a
 * call to bv_open. This will allow you to perform any finalizing writes needed
 * to the bvfs file system.
 *
 * Input Parameters
 *   fileName: A c-string representing the name of the file you wish to fetch
 *             (or create) in the bvfs file system.
 *
 * Return Value
 *   int:  0 if open succeeded.
 *        -1 if some kind of failure occurred (eg. the file was not previously
 *           opened via bv_open). Also, print a meaningful error to stderr
 *           prior to returning.
 */
int bv_close(int bvfs_FD) {
  //TODO look for the file in the file descriptors page
  //TODO see if the file has been written to at all (size of inode != -1)
  //TODO remove from file descriptor table
}







/*
 * int bv_write(int bvfs_FD, const void *buf, size_t count);
 *
 * This function will write count bytes from buf into a location corresponding
 * to the cursor of the file represented by bvfs_FD.
 *
 * Input Parameters
 *   bvfs_FD: The identifier for the file to write to.
 *   buf: The buffer containing the data we wish to write to the file.
 *   count: The number of bytes we intend to write from the buffer to the file.
 *
 * Return Value
 *   int: >=0 Value representing the number of bytes written to the file.
 *        -1 if some kind of failure occurred (eg. the file is not currently
 *           opened via bv_open). Also, print a meaningful error to stderr
 *           prior to returning.
 */
int bv_write(int bvfs_FD, const void *buf, size_t count) {
}






/*
 * int bv_read(int bvfs_FD, void *buf, size_t count);
 *
 * This function will read count bytes from the location corresponding to the
 * cursor of the file (represented by bvfs_FD) to buf.
 *
 * Input Parameters
 *   bvfs_FD: The identifier for the file to read from.
 *   buf: The buffer that we will write the data to.
 *   count: The number of bytes we intend to write to the buffer from the file.
 *
 * Return Value
 *   int: >=0 Value representing the number of bytes written to buf.
 *        -1 if some kind of failure occurred (eg. the file is not currently
 *           opened via bv_open). Also, print a meaningful error to stderr
 *           prior to returning.
 */
int bv_read(int bvfs_FD, void *buf, size_t count) {
}







/*
 * int bv_unlink(const char* fileName);
 *
 * This function is intended to delete a file that has been allocated within
 * the bvfs file system.
 *
 * Input Parameters
 *   fileName: A c-string representing the name of the file you wish to delete
 *             from the bvfs file system.
 *
 * Return Value
 *   int:  0 if the delete succeeded.
 *        -1 if some kind of failure occurred (eg. the file does not exist).
 *           Also, print a meaningful error to stderr prior to returning.
 */
int bv_unlink(const char* fileName) {
}







/*
 * void bv_ls();
 *
 * This function will list the contests of the single-directory file system.
 * First, you must print out a header that declares how many files live within
 * the file system. See the example below in which we print "2 Files" up top.
 * Then display the following information for each file listed:
 *   1) the file size in bytes
 *   2) the number of blocks occupied within bvfs
 *   3) the time and date of last modification (derived from unix timestamp)
 *   4) the name of the file.
 * An example of such output appears below:
 *    | 2 Files
 *    | bytes:  276, blocks: 1, Tue Nov 14 09:01:32 2017, bvfs.h
 *    | bytes: 1998, blocks: 4, Tue Nov 14 10:32:02 2017, notes.txt
 *
 * Hint: #include <time.h>
 * Hint: time_t now = time(NULL); // gets the current unix timestamp (32 bits)
 * Hint: printf("%s\n", ctime(&now));
 *
 * Input Parameters
 *   None
 *
 * Return Value
 *   void
 */
void bv_ls() {
}
