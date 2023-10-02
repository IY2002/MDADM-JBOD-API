#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cache.h"
#include "mdadm.h"

static cache_entry_t *cache = NULL;
static int cache_size = 0;
static int clock = 0;
static int num_queries = 0;
static int num_hits = 0;
static bool is_cache_on = false;
static int num_items_in_cache = 0;

//function to lookup if a certain block is in cache
bool in_cache(int disk_num, int block_num){
  for ( int i =0; i < num_items_in_cache; i++){
    if (cache[i].disk_num == disk_num && cache[i].block_num == block_num) return true;
  }
  return false;
}

int cache_create(int num_entries) {
  //error checking
  if (is_cache_on) return -1;
  if (num_entries < 2) return -1;
  if (num_entries > 4096) return -1;

  cache_size = num_entries;
  is_cache_on = true;
  //allocate room for cache
  cache = malloc(sizeof(cache_entry_t) * num_entries);
  return 1;
}

int cache_destroy(void) {
  //error checking
  if (!is_cache_on) return -1;
  cache_size = 0;
  is_cache_on = false;
  free(cache);
  cache = NULL;
  return 1;
}

int cache_lookup(int disk_num, int block_num, uint8_t *buf) {
  //error checking
  if (!is_cache_on) return -1;
  if (buf == NULL) return -1;
  
  num_queries++;
  clock++;

  for(int i = 0; i < num_items_in_cache; i++){
    // hit in the cache
    if (cache[i].disk_num == disk_num && cache[i].block_num == block_num){

      //copy block to buffer
      memcpy(buf, cache[i].block, JBOD_BLOCK_SIZE);

      //update accesstime to current time and number of hits
      cache[i].access_time = clock;
      num_hits++;

      return 1;
    }
  }
  return -1;
}

void cache_update(int disk_num, int block_num, const uint8_t *buf) {
  if (!is_cache_on) return;
  for(int i = 0; i < num_items_in_cache; i++){
    //block is in cache
    if (cache[i].disk_num == disk_num && cache[i].block_num == block_num){
      //update block contents and access time
      memcpy(cache[i].block, buf,  JBOD_BLOCK_SIZE);
      cache[i].access_time = clock;
      return;
    }
  }
}

int cache_insert(int disk_num, int block_num, const uint8_t *buf) {
  //error checking
  if (!is_cache_on) return -1;
  if (block_num < 0 || block_num > JBOD_NUM_BLOCKS_PER_DISK) return -1;
  if (disk_num < 0 || disk_num >= JBOD_NUM_DISKS) return -1;
  if (buf == NULL) return -1;
  if (in_cache(disk_num, block_num)) {
    cache_update(disk_num, block_num, buf);
    return -1;
  }
  clock++;

  //create the new entry in the cache
  cache_entry_t new_cache_entry;
  new_cache_entry.block_num = block_num;
  new_cache_entry.disk_num = disk_num;
  new_cache_entry.access_time = clock;
  new_cache_entry.valid = true;

  //cache isnt full yet so no need to replace anything
  if (num_items_in_cache < cache_size){

    //simply insert item at the end of cache
    cache[num_items_in_cache] = new_cache_entry;

    //copy block to the cache entry
    memcpy(cache[num_items_in_cache].block, buf, JBOD_BLOCK_SIZE);

    num_items_in_cache++;
    return 1;
  }

  //cache is full
  //have to find oldest block in cache and replace with new block
  int oldest_entry_index = 0;
  for(int i = 0; i < cache_size; i++){
    if (cache[i].access_time < cache[oldest_entry_index].access_time){
      oldest_entry_index = i;
    } 
  }

  //replace oldest entry in cache
  cache[oldest_entry_index] = new_cache_entry;

  //copy block to the cache entry
  memcpy(cache[oldest_entry_index].block, buf, JBOD_BLOCK_SIZE);
  
  return 1;
}

bool cache_enabled(void) {
  return is_cache_on;
}

void cache_print_hit_rate(void) {
  fprintf(stderr, "Hit rate: %5.1f%%\n", 100 * (float) num_hits / num_queries);
}
