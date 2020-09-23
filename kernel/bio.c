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

struct {
  struct spinlock lock[BUCKET];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // head.next is most recently used.
  struct buf head[BUCKET];
} bcache;

void
binit(void)
{
  struct buf *b;


  // Create linked list of buffers
  //inital the all head
  for(int i=0;i<BUCKET;i++)
  {
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
    initsleeplock(&bcache.head[i].lock,"bache.head.lock");
    initlock(&bcache.lock[i],"beche.lock");
  }
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head[0].next;
    b->prev = &bcache.head[0];
    initsleeplock(&b->lock, "beche.buffer.lock");
    bcache.head[0].next->prev = b;
    bcache.head[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  uint hash_id = blockno%BUCKET;
  acquire(&bcache.lock[hash_id]);

  // Is the block already cached?
  for(b = bcache.head[hash_id].next; b != &bcache.head[hash_id]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[hash_id]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached; recycle an unused buffer int first situation 
  for(b = bcache.head[hash_id].prev; b != &bcache.head[hash_id]; b = b->prev){
    if(b->refcnt == 0) { 
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock[hash_id]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  //the second situation when cannt find buffer , steal buffer from other head
  for(uint i=0;i<BUCKET;i++)
  {
    if(i==hash_id) continue;
    acquire(&bcache.lock[i]);
    for(b = bcache.head[i].prev; b != &bcache.head[i]; b = b->prev){
      if(b->refcnt == 0) { 
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;

        //change list
        //delete in origin list
        b->next->prev=b->prev;
        b->prev->next=b->next;
        //add int new list
        b->next=bcache.head[hash_id].next;
        b->prev=&bcache.head[hash_id]; 
        bcache.head[hash_id].next->prev=b;
        bcache.head[hash_id].next=b;
        
        release(&bcache.lock[hash_id]);
        release(&bcache.lock[i]);
        acquiresleep(&b->lock);
        return b;
      }
    }
     release(&bcache.lock[i]);
     release(&bcache.lock[hash_id]);
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b->dev, b, 0);
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
  virtio_disk_rw(b->dev, b, 1);
}

// Release a locked buffer.
// Move to the head of the MRU list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");
  uint hash_id = b->blockno%BUCKET;
  releasesleep(&b->lock);

  acquire(&bcache.lock[hash_id]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head[hash_id].next;
    b->prev = &bcache.head[hash_id];
    bcache.head[hash_id].next->prev = b;
    bcache.head[hash_id].next = b;
  }
  
  release(&bcache.lock[hash_id]);
}

void
bpin(struct buf *b) {
  uint hash_id = b->blockno%BUCKET;
  acquire(&bcache.lock[hash_id]);
  b->refcnt++;
  release(&bcache.lock[hash_id]);
}

void
bunpin(struct buf *b) {
  uint hash_id = b->blockno%BUCKET;
  acquire(&bcache.lock[hash_id]);
  b->refcnt--;
  release(&bcache.lock[hash_id]);
}


