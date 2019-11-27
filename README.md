# BVFS
## Overview
This project is a single-directory file system for Buena Vista University's 2019 Operating Systems class.

## Specifications
The file system meets the following specifications:
* **Total file system partitiion size**: 8,388,608 bytes
* **Total number of blocks in the partition**: 16,384 blocks
* **Total number of blocks available for use by the user**: 16, 127 blocks
* **Block Size**: 512 bytes
* **Maximum number of files**: 256 files
* **Maximum file size**: 65,536 bytes (128 blocks)
* **Maximum file name size**: 31 characters

## Functionality
Users will have the following functions, which are also outlined within bvfs.h:

#### Init
Takes a file name that the user specifies for the file system to be named. If the file does not exist, init will create the file and initialize it as a brand new file system. If the file does exist, init will read in the file systems information for the user to acess their already stored files.

#### Destroy
Finishes writing information to file and does all other necessary cleanup of an already initialized file system.

#### Open
Open a file descriptor in one of the following modes:
* Read (BV_RDONLY)
  * Open an already created file in order to read its data.
  * Trying to open a file that is not created will result in an error
* Overwrite (BV_WTRUNC)
  * Open file in order to write to it
  * If the file already exists, overwrite will delete the old file and begin writing from byte 0
  * If the file does not exist, overwrite will create the file and begin writing from byte 0
* Append (BV_WCONCAT)
  * Open file in order to write to it
  * If the file already exists, append will begin writing from the end of the file
  * If the file does not exist, append will create the file and begin writing from byte 0

#### Close
Close an already opened file descriptor

#### Write
Write to an open file descriptor. If the file is open in read mode, write will fail.

#### Read
Read from an open file descriptor. If the file is open in overwrite or append mode, read will fail.

#### Unlink
Delete a file from the file system.

#### ls
Outputs the following information on the the file system
* Number of files
* File information per file
  * Number of bytes
  * Number of blocks
  * Date last modified
  * File name

## Usage
To run the test suite provided, simple use the following command while in the home directory:
```bash
make run
```

Alternatively, you can run the test suite in the following way:
```bash
make
./tester.out
```

To run one of the 19 tests, simply invoke the Makefile and execute the desired test in the range of 0 to 18:
```bash
make
./tester.out 3
```
