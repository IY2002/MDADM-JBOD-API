#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "mdadm.h"
#include "jbod.h"
#include "net.h"

// universal variable to know if jbod is mounted
//-1 is unmounted , 1 is mounted
int mounted = -1;

// creates the op command for jbod based on inputs
uint32_t opCreator(int command, int diskNum, int blockID)
{
  uint32_t op = 0;
  op = op | (command << 26);
  op = op | (diskNum << 22);
  op = op | blockID;
  return op;
}

//helper functions for figuring out the disk num, block num and byte offset
int diskNumFinder(uint32_t addr){
  return addr / JBOD_DISK_SIZE;
}

int blockNumFinder(uint32_t addr){
  return (addr / JBOD_NUM_BLOCKS_PER_DISK) % JBOD_BLOCK_SIZE;
}

int byteOffsetFinder(uint32_t addr){
  return addr % JBOD_BLOCK_SIZE;
}

//runs the jbod command with the necessary input
int jbodRun(int cmd, int diskNum, int blockNum, uint8_t *buf){
  return jbod_client_operation(opCreator(cmd, diskNum, blockNum), buf);
}

//read block into buffer without moving the I/O operation pointer
//useful for read so the block data stays the same when a write doesnt cover the entire block
void copyBlockToBuffer(int blockNum, int diskNum, uint8_t * buffer){
  jbodRun(JBOD_READ_BLOCK, 0, 0, buffer);
  jbodRun(JBOD_SEEK_TO_BLOCK, diskNum, blockNum, NULL);
}

int mdadm_mount(void)
{
  if (mounted == 1) 
    return -1;
  mounted = 1;
  jbodRun(JBOD_MOUNT, 0, 0, NULL);
  return 1;
}

int mdadm_unmount(void)
{
  if (mounted == -1) 
    return -1;
  mounted = -1;
  jbodRun(JBOD_UNMOUNT, 0, 0, NULL);
  return 1;
}



// returns number of bytes read
// calculate strating disk and ending disk and increment till all is read
// read all the blocks needed, take only the wanted memory and put them in buffer
int mdadm_read(uint32_t addr, uint32_t len, uint8_t *buf)
{
  // error checking
  if (mounted == -1)
    return -1;
  if (len > 1024)
    return -1;
  if ((addr + len) > 16 * 256 * 256)
    return -1;
  if (buf == NULL && len != 0)
    return -1;
  if (buf == NULL && len == 0)
    return len;

  // calculation for starting position of data
  int currentDisk = diskNumFinder(addr);
  int currentBlock = blockNumFinder(addr);
  int byteOffset = byteOffsetFinder(addr);

  // setup IO to correct disk and block
  jbodRun(JBOD_SEEK_TO_DISK, currentDisk, currentBlock, NULL);  
  jbodRun(JBOD_SEEK_TO_BLOCK, currentDisk, currentBlock, NULL); 

  // buffer to hold the read from jbod operations
  uint8_t buffer[JBOD_BLOCK_SIZE];

  // keep a running tally of how many bytes read vs how many still need to be read
  int bytesToRead = len;
  int bytesRead = 0;

  // keep reading while there is still more bits t0 be read
  while (bytesToRead > 0)
  {

    // to account for data that starts in the middle of a jbod block
    int numBytes = 256 - byteOffset;

    // if not the whole buffer si needed but only a small part of it
    if (numBytes >= bytesToRead)
      numBytes = bytesToRead;

    //if block is in cache it is read, otherwise it is read from jbod and stored in cache
    if (cache_lookup(currentDisk, currentBlock, buffer) == -1){
      jbodRun(JBOD_READ_BLOCK, 0, 0, buffer);
      cache_insert(currentDisk, currentBlock, buffer);
    }

    // copy the number of bytes wanted from buffer to the end of the buf
    memcpy(&buf[bytesRead], &buffer[byteOffset], numBytes);

    // can only have a byteoffset on the first read other than that read starts at byte 0
    byteOffset = 0;

    // keep tally running
    bytesToRead -= numBytes;
    bytesRead += numBytes;

    // keep tally of current block
    currentBlock++;

    // check to see if we need to move onto next disk
    if (currentBlock % JBOD_NUM_BLOCKS_PER_DISK == 0)
    {
      currentDisk++;
      jbodRun(JBOD_SEEK_TO_DISK, currentDisk, 0, NULL);
      currentBlock = 0;
    }
  }
  return len;
}

int mdadm_write(uint32_t addr, uint32_t len, const uint8_t *buf) {
    // error checking
  if (mounted == -1)
    return -1;
  if (len > 1024)
    return -1;
  if ((addr + len) > 16 * 256 * 256)
    return -1;
  if (buf == NULL && len != 0)
    return -1;
  if (buf == NULL && len == 0)
    return len;

  // calculation for starting position of data
  int currentDisk = diskNumFinder(addr);
  int currentBlock = blockNumFinder(addr);
  int byteOffset = byteOffsetFinder(addr);

  // setup IO to correct disk and block
  jbodRun(JBOD_SEEK_TO_DISK, currentDisk, currentBlock, NULL);  
  jbodRun(JBOD_SEEK_TO_BLOCK, currentDisk, currentBlock, NULL); 

  // buffer to hold the data to written to block
  uint8_t buffer[JBOD_BLOCK_SIZE];

  // keep a running tally of how many bytes written vs how many still need to be written
  int bytesToWrite = len;
  int bytesWritten = 0;

  // keep writting while there is still more bits to be written
  while (bytesToWrite > 0)
  {

    // to account for data that starts in the middle of a jbod block
    int numBytes = 256 - byteOffset;

    // if the whole buffer is not needed but only a small part of it
    if (numBytes > bytesToWrite)
      numBytes = bytesToWrite;

    //this function copies the current block into my temporary buffer so only the necessary parts of the block are overwritten.
    copyBlockToBuffer(currentBlock, currentDisk, buffer);
    
    //copy from user given buffer to temporary buffer hte bytes that will be written in this loop
    memcpy(&buffer[byteOffset], &buf[bytesWritten], numBytes);
    
    //write data in block 
    jbodRun(JBOD_WRITE_BLOCK, 0, 0, buffer);
    
    // can only have a byteoffset on the first write other than that the write starts at byte 0 in the block
    byteOffset = 0;

    // update the running tally
    bytesToWrite -= numBytes;
    bytesWritten += numBytes;

    // keep tally of current block
    currentBlock++;

    // check to see if we need to move onto next disk
    if (currentBlock % JBOD_NUM_BLOCKS_PER_DISK == 0)
    {
      currentDisk++;
      jbodRun(JBOD_SEEK_TO_DISK, currentDisk, 0, NULL);
      currentBlock = 0;
    }
  }

  return len;
}
