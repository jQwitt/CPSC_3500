// JACK WITT
// CPSC 3500: File System
// Implements the file system commands that are available to the shell.

#include <cstring>
#include <iostream>
#include <math.h>

using namespace std;

#include "FileSys.h"
#include "BasicFileSys.h"
#include "Blocks.h"

// mounts the file system
void FileSys::mount() {
    bfs.mount();
    curr_dir = 1;
    /* DEBUG
    for (int i = 2; i < NUM_BLOCKS; i++) {
        bfs.reclaim_block(i);
    } */
}

// unmounts the file system
void FileSys::unmount() {
    bfs.unmount();
}

// make a directory DONE
void FileSys::mkdir(const char *name) {
  if (check_name(name)) {
        cout << "Err: Name is Too Long" << endl;
        return;
    }
    if (!exists_in_current(name)) {
        int num = bfs.get_free_block();
        if (num) {
            struct dirblock_t newDir;
            struct dirblock_t currDir;
            newDir.magic = DIR_MAGIC_NUM;
            newDir.num_entries = 0;

            bfs.read_block(curr_dir, (void *) &currDir);
            if (currDir.num_entries == MAX_DIR_ENTRIES) {
                cout << "Err: Directory is Full";
                return;
            }
            currDir.num_entries++;
            currDir.dir_entries[currDir.num_entries - 1].block_num = num;
            int i = 0;
            while (name[i] != '\0' && i < MAX_FNAME_SIZE - 1) {
                currDir.dir_entries[currDir.num_entries - 1].name[i] = name[i];
                i++;
            }
            currDir.dir_entries[currDir.num_entries - 1].name[i] = '\0'; //append null term
            bfs.write_block(num, (void *) &newDir);
            bfs.write_block(curr_dir, (void *) &currDir);
            cout << "new dir @ block " << num; //DEBUG
        } else {
            cout << "Err: Disk is Full";
        }
    } else {
        cout << "Err: Directory Exists";
    }
    cout << endl;
}

// switch to a directory DONE
void FileSys::cd(const char *name) {
    dirblock_t d;
    bfs.read_block(curr_dir, (void *) &d);
    for (int i = 0; i < d.num_entries; i++) {
        if (strcmp(name, d.dir_entries[i].name) == 0) {
            if (is_directory(d.dir_entries[i].block_num)) {
                curr_dir = d.dir_entries[i].block_num;
                return;
            } else {
                cout << "Err: Not a Directory";
            }
        }
    }
    cout << "Err: Directory Does Not Exist";
    cout << endl;
}

// switch to home directory DONE
void FileSys::home() {
    curr_dir = 1;
}

// remove a directory DONE
void FileSys::rmdir(const char *name) {
    if (exists_in_current(name)) {
        dirblock_t curr;
        bfs.read_block(curr_dir, (void *) &curr);

        //iterate through all entries
        for (int i = 0; i < curr.num_entries; i++) {
            if (strcmp(curr.dir_entries[i].name, name) == 0) {
                int num = curr.dir_entries[i].block_num;
                if (is_directory(num)) {
                    dirblock_t to_rm;
                    bfs.read_block(num, (void *) &to_rm);
                    if (to_rm.num_entries == 0) {
                        int a = 0;
                        for (a = i; a < curr.num_entries - 1; a++) {
                            strcpy(curr.dir_entries[a].name, curr.dir_entries[a + 1].name);
                            curr.dir_entries[a].block_num = curr.dir_entries[a + 1].block_num;
                        }
                        curr.dir_entries[a].block_num = 0;
                        curr.num_entries--;
                        bfs.write_block(curr_dir, (void *) &curr);
                        bfs.reclaim_block(num);
                        cout << "rm dir @ block " << num << endl; //DEBUG
                    } else {
                        cout << "Err: DIR[ " << name << " ] is Not Empty" << endl;
                    }
                } else {
                    cout << "Err: Not a Directory" << endl;
                }
                return;
            }
        }
    } else {
        cout << "Err: Directory Does Not Exist" << endl;
    }
}

// list the contents of current directory DONE
void FileSys::ls() {
    dirblock_t d;
    bfs.read_block(curr_dir, (void *) &d);
    if (d.num_entries == 0) {
        cout << " ";
    } else {
        int a = 0;
        for (int i = 0; i < d.num_entries; i++) {
            while (d.dir_entries[i].name[a] != '\0' && a < MAX_FNAME_SIZE) {
                cout << d.dir_entries[i].name[a];
                a++;
            }
            if (is_directory(d.dir_entries[i].block_num))
                cout << '/';
            a = 0;
            cout << " ";
        }
    }
    cout << endl;
}

// create an empty data file DONE
void FileSys::create(const char *name) {
    if (!check_name(name)) {
        if (!exists_in_current(name)) {
            //fetch
            dirblock_t curr;
            bfs.read_block(curr_dir, (void *) &curr);
            int num = bfs.get_free_block();
            if (num) {
                if (curr.num_entries < MAX_DIR_ENTRIES) {
                    //init
                    inode_t newNode;
                    newNode.magic = INODE_MAGIC_NUM;
                    newNode.size = 0;
                    //update curr dir
                    strcpy(curr.dir_entries[curr.num_entries].name, name);
                    curr.dir_entries[curr.num_entries].block_num = num;
                    curr.num_entries++;
                    bfs.write_block(curr_dir, (void *) &curr);
                    bfs.write_block(num, (void *) &newNode);
                    cout << "new file @ block " << num << endl; //DEBUG
                } else {
                    cout << "Err: Current Directory is Full" << endl;
                }
            } else {
                cout << "Err: Disk is Full" << endl;
            }
        } else {
            cout << "Err: File exists" << endl;
        }
    } else {
        cout << "Err: Name is Too Long" << endl;
    }
}

// append data to a data file WIP - ADD OUT OF RANGE ERR
void FileSys::append(const char *name, const char *data) {
    if (exists_in_current(name)) {
        int len = strlen(data);
        if (len <= MAX_FILE_SIZE) {
            //find file in curr
            int curr_num = 0, i = 0;
            dirblock_t curr;
            bfs.read_block(curr_dir, (void *) &curr);
            while (i < curr.num_entries) {
                if (strcmp(curr.dir_entries[i].name, name) == 0) {
                    curr_num = curr.dir_entries[i].block_num;
                }
                i++;
            }
            if (!is_directory(curr_num)) {
                int pos = 0, index;
                int debug = 0;
                bool needNew = false;
                //loop logic: empty file or (block space to use or need new)
                while (pos < len) {
                  inode_t file; 
                    bfs.read_block(curr_num, (void *) &file);
                    index = floor(file.size / BLOCK_SIZE);
                    if (file.size == 0) {
                        int dataBlockNum = bfs.get_free_block();
                        if (dataBlockNum) {
                            datablock_t dataB;
                            bfs.read_block(dataBlockNum, (void *) &dataB);
                            file.blocks[0] = dataBlockNum; //update file
                            //write data until none left
                            for (int i = 0; i < BLOCK_SIZE && i < len; i++) {
                                dataB.data[i] = data[pos];
                                //cout << "new " << dataB.data[i] << endl;
                                pos++;
                                file.size++;
                            }
                            bfs.write_block(curr_num, (void *) &file);
                            bfs.write_block(dataBlockNum, (void *) &dataB);
                            cout << "new data block @ " << dataBlockNum << endl; //DEBUG
                        } else {
                            cout << "Err: Disk is Full" << endl;
                        }
                    } else {
                        if (file.size % BLOCK_SIZE == 0 || needNew) {
                            needNew = false;
                            //make new
                            int newBlock = bfs.get_free_block();
                            if (newBlock) {
                                datablock_t dataBlk;
                                bfs.read_block(newBlock, (void *) &dataBlk);
                                file.blocks[index] = newBlock; //update file
                                //write data until none left
                                for (int i = 0; i < BLOCK_SIZE && pos < len; i++) {
                                    dataBlk.data[i] = data[pos];
                                    //cout << "new " << dataBlk.data[i] << endl;
                                    pos++;
                                    file.size++;
                                }
                                bfs.write_block(curr_num, (void *) &file);
                                bfs.write_block(newBlock, (void *) &dataBlk);
                                cout << "new data block @ " << newBlock << endl; //DEBUG
                            } else {
                                cout << "Err: Disk is Full" << endl;
                            }
                        } else {
                            //use existing at index
                            datablock_t dataBlk;
                            index = floor(file.size / BLOCK_SIZE);
                            int blkNum = file.blocks[index];
                            bfs.read_block(blkNum, (void *) &dataBlk);
                            int start = strlen(dataBlk.data);
                            if (start < BLOCK_SIZE) {
                                cout << start;
                                //write data until none left
                                for (int i = start; i < BLOCK_SIZE && pos < len; i++) {
                                    dataBlk.data[i] = data[pos];
                                    //cout << "reuse " << dataBlk.data[i] << endl;
                                    pos++;
                                    file.size++;
                                    //INF LOOP??
                                }
                                bfs.write_block(curr_num, (void *) &file);
                                bfs.write_block(blkNum, (void *) &dataBlk);
                                cout << "used data block @ " << blkNum << endl; //DEBUG
                            } else {
                                needNew = true;
                            }
                        }
                    }
                }
            } else {
                cout << "Err: File is a Directory" << endl;
            }
        } else {
            cout << "Err: Exceeds Max File Size (" << MAX_FILE_SIZE << ")" << endl;
        }
    } else {
        cout << "Err: File Does Not Exist" << endl;
    }
}

// display the contents of a data file DONE
void FileSys::cat(const char *name) {
    if (exists_in_current(name)) {
        dirblock_t curr;
        bfs.read_block(curr_dir, (void *) &curr);
        //find instance
        int block;
        for (int i = 0; i < curr.num_entries; i++) {
            if (strcmp(curr.dir_entries[i].name, name) == 0) {
                block = curr.dir_entries[i].block_num;
                break;
            }
        }
        if (!is_directory(block)) {
            inode_t file;
            bfs.read_block(block, (void *) &file);
            if (file.size == 0) {
                cout << " " << endl;
            } else {
                int pos = 0;
                for (int a = 0; a < floor(file.size / BLOCK_SIZE) + 1; a++) {
                    datablock_t to_read;
                    bfs.read_block(file.blocks[a], (void *) &to_read);
                    for (int b = 0; b < BLOCK_SIZE && pos < file.size; b++) {
                        cout << to_read.data[b];
                        pos++;
                    }
                }
                cout << endl;
            }
        } else {
            cout << "Err: File is a Directory" << endl;
        }
    } else {
        cout << "Err: File Does Not Exist" << endl;
    }
}

// display the last N bytes of the file DONE
void FileSys::tail(const char *name, unsigned int n) {
    if (exists_in_current(name)) {
        dirblock_t curr;
        bfs.read_block(curr_dir, (void *) &curr);
        //find instance
        int block;
        for (int i = 0; i < curr.num_entries; i++) {
            if (strcmp(curr.dir_entries[i].name, name) == 0) {
                block = curr.dir_entries[i].block_num;
                break;
            }
        }
        if (!is_directory(block)) {
            inode_t file;
            bfs.read_block(block, (void *) &file);
            if (file.size == 0) {
                cout << " " << endl;
            } else {
                int pos = file.size - n, index = 0, limit = floor(file.size / BLOCK_SIZE) + 1;
                if (pos < 0) pos = 0; //overflow
                index = floor(pos / BLOCK_SIZE);
                int start = pos;
                for (int a = index; a < limit; a++) {
                    datablock_t to_read;
                    bfs.read_block(file.blocks[a], (void *) &to_read);
                    int b = start;
                    while(b < BLOCK_SIZE && pos < file.size) {
                        cout << to_read.data[b];
                        b++;
                        pos++;

                    }
                    start = 0;
                }
                cout << endl;
            }
        } else {
            cout << "Err: File is a Directory" << endl;
        }
    } else {
        cout << "Err: File Does Not Exist" << endl;
    }
}

// delete a data file DONE
void FileSys::rm(const char *name) {
    if (exists_in_current(name)) {
        dirblock_t curr;
        bfs.read_block(curr_dir, (void *) &curr);
        //find instance
        int block;
        int nodeIndex = 0;
        for (int i = 0; i < curr.num_entries; i++) {
            if (strcmp(curr.dir_entries[i].name, name) == 0) {
                block = curr.dir_entries[i].block_num;
                nodeIndex = i;
                break;
            }
        }
        if (!is_directory(block)) {
            inode_t node;
            bfs.read_block(block, (void *) &node);
            if (node.size > 0) {
                int last = floor(node.size / BLOCK_SIZE) + 1;
                //remove all data blocks
                for (int i = 0; i < last; i++) {
                    bfs.reclaim_block(node.blocks[i]);
                    cout << "reclaimed block @ " << node.blocks[i] << endl; //DEBUG
                }
                bfs.write_block(block, (void *) &node);
            }
            //remove inode from curr_dir
            int num = curr.dir_entries[nodeIndex].block_num;
            for (int a = nodeIndex; a < curr.num_entries; a++) {
                strcpy(curr.dir_entries[a].name, curr.dir_entries[a + 1].name);
                curr.dir_entries[a].block_num = curr.dir_entries[a + 1].block_num;
            }
            bfs.reclaim_block(num);
            cout << "reclaimed block @ " << num << endl; //DEBUG
            curr.num_entries--;
            bfs.write_block(curr_dir, (void *) &curr);
        } else {
            cout << "Err: File is a Directory" << endl;
        }
    } else {
        cout << "Err: File Does Not Exist" << endl;
    }
}

// display stats about file or directory DONE
void FileSys::stat(const char *name) {
    if (exists_in_current(name)) {
        dirblock_t curr;
        bfs.read_block(curr_dir, (void *) &curr);
        //find instance
        int block;
        int i;
        for (i = 0; i < curr.num_entries; i++) {
            if (strcmp(curr.dir_entries[i].name, name) == 0) {
                block = curr.dir_entries[i].block_num;
                break;
            }
        }
        if (is_directory(block)) {
            //dir output
            cout << "Directory name: ";
            for (int a = 0; curr.dir_entries[i].name[a] != '\0' && a < MAX_FNAME_SIZE; a++) {
                cout << curr.dir_entries[i].name[a];
            }
            cout << '/' << endl;
            cout << "Directory block: " << block << endl;
        } else {
            //inode output
            inode_t node;
            bfs.read_block(block, (void *) &node);
            double size = node.size, blk = BLOCK_SIZE;
            double num = ceil(size / blk);
            cout << "Inode Block: " << block << endl;
            cout << "Bytes in File: " << node.size << endl;
            cout << "Number of Blocks: " << num+1 << endl;
            cout << "First Block: ";
            if (node.size == 0)
                cout << '0';
            else
                cout << node.blocks[0];
            cout << endl;
        }
    } else {
        cout << "Err: File Does Not Exist" << endl;
    }
}

// HELPER FUNCTIONS (optional)

// reads the magic num and returns true if directory DONE 
bool FileSys::is_directory(const int num) {
    dirblock_t d;
    bfs.read_block(num, (void *) &d);
    return d.magic == DIR_MAGIC_NUM;
}

// return true if the given name exists DONE
bool FileSys::exists_in_current(const char *name) {
    dirblock_t c;
    bfs.read_block(curr_dir, (void *) &c);
    for (int i = 0; i < c.num_entries; i++) {
        if (strcmp(name, c.dir_entries[i].name) == 0) {
            return true;
        }
    }
    return false;
}

// checks if name is within MAX_FNAME SIZE
bool FileSys::check_name(const char *name) {
    int i = 0;
    while (name[i] != '\0') {
        i++;
    }
    return i > MAX_FNAME_SIZE;
}
