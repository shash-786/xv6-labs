// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define HASH(block_no) (block_no%NBUCKETS)
extern uint ticks;

struct {
  struct spinlock lock;
  struct buf buf[NBUF];
  struct buf hash_bucket[NBUCKETS];
  struct spinlock bucket_lock[NBUCKETS];
  int size;

} bcache;

void
binit(void)
{
  struct buf *b;

  bcache.size = 0;
  initlock(&bcache.lock, "bcache");

  for(int i=0; i<NBUCKETS; i++){
    initlock(&bcache.bucket_lock[i], "bcache.bucket_lock");
  }

  for(b=bcache.buf; b < bcache.buf+NBUF; b++) {
    initsleeplock(&b->lock,"buffer");
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
	// Look through the hashed bucket for cached copy
	
	  int hash_id = HASH(blockno);
	  acquire(&bcache.bucket_lock[hash_id]);

	  struct buf* b = bcache.hash_bucket[hash_id].next;
	  while(b) {
		  if(b->dev == dev && b->blockno == blockno) {
			  b->refcnt++;
			  release(&bcache.bucket_lock[hash_id]);
			  acquiresleep(&b->lock);
			  return b;
		  }
		  b = b -> next;
	  }

	  acquire(&bcache.lock);
	  if(bcache.size < NBUF) {
		  b = &bcache.buf[bcache.size++];
		  release(&bcache.lock);

		  b->refcnt = 1;
		  b->dev = dev;
		  b->blockno = blockno;
		  b->valid = 0;
		  b->next = bcache.hash_bucket[hash_id].next;
		  bcache.hash_bucket[hash_id].next = b; 

		  release(&bcache.bucket_lock[hash_id]);
		  acquiresleep(&b->lock);

		  return b;
	  }
	  release(&bcache.lock);
	  release(&bcache.bucket_lock[hash_id]);


	 // No empty buffers left nor did we find a cached copy
	 // Find a Least Recently used buffer
	 
	 struct buf *prev, *min, *minpre;
	 for(int i=0; i<NBUCKETS; i++) {

	 	acquire(&bcache.bucket_lock[hash_id]);
		uint mintimestamp = 0x8fffffff;
		b = bcache.hash_bucket[hash_id].next;
		prev = &bcache.hash_bucket[hash_id];
		while(b) {
		  if(hash_id == HASH(blockno) && b->dev == dev && b->blockno == blockno){
			b->refcnt++;
			release(&bcache.bucket_lock[hash_id]);
			acquiresleep(&b->lock);
			return b;
		  }
		  if(b->refcnt == 0 && b->timestamp < mintimestamp) {
			minpre = prev;
			min = b; 
			mintimestamp = b -> timestamp;
		  }
		  prev = b;
		  b = b -> next;
		}
		
		if(min) {
		  min -> dev = dev;
		  min -> blockno = blockno;
		  min -> valid = 0;
		  min -> refcnt = 1;

		  if(hash_id != HASH(blockno)) {
			  minpre -> next = min -> next;
			  release(&bcache.bucket_lock[hash_id]);
			  hash_id = HASH(blockno);
			  acquire(&bcache.bucket_lock[hash_id]);
			  min -> next = bcache.hash_bucket[hash_id].next;
			  bcache.hash_bucket[hash_id].next = min;
		  }
		  release(&bcache.bucket_lock[hash_id]);
	          acquiresleep(&min->lock);
		  return min;
		}
		
		hash_id++;
		if(hash_id == NBUCKETS)
			hash_id = 0;

	 } 
	 panic("No buffers! bget");

}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  int hash_id = HASH(b->blockno);
  acquire(&bcache.bucket_lock[hash_id]);
  b->refcnt--;
  if (b->refcnt == 0) {
	  b->timestamp = ticks;
  }
  release(&bcache.bucket_lock[hash_id]);
}

void
bpin(struct buf *b) {
	int hash_id = HASH(b->blockno);
	acquire(&bcache.bucket_lock[hash_id]);
	b->refcnt++;
	release(&bcache.bucket_lock[hash_id]);
}

void
bunpin(struct buf *b) {
	int hash_id = HASH(b->blockno);
	acquire(&bcache.bucket_lock[hash_id]);
	b->refcnt--;
	release(&bcache.bucket_lock[hash_id]);
}


