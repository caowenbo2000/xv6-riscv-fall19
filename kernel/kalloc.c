// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  char *start;
  uint8 *ref;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  uint64 page_num=(PHYSTOP - (uint64)end) >> 12; // a page is 1 << 12,and the ref of it is 1 << 8
  kmem.ref = (uint8 *)end;
  kmem.start = end + (1<<8) * page_num;
  kmem.start = (char *)PGROUNDUP((uint64)kmem.start);
  memset(kmem.ref,0, page_num * sizeof(uint));
  freerange(kmem.start, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < kmem.start || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;

  uint64 index = ((uint64)pa - (uint64)kmem.start)>>12;//reset ref num
  kmem.ref[index] = 0;
  
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  uint64 index = ((uint64)r - (uint64)kmem.start)>>12;//set ref num
  if(r)
  {
    kmem.freelist = r->next;
    kmem.ref[index] = 1;
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}


int kborrow(void *pa)
{
	if((uint64)pa%PGSIZE||(char *)pa<kmem.start||(uint64)pa>PHYSTOP)
	panic("kborrow");
    uint64 index = ((uint64)pa - (uint64)kmem.start)>>12;//add ref num
	acquire(&kmem.lock);
	
    kmem.ref[index]++;
    int	ret = (int) kmem.ref[index];

	release(&kmem.lock);
//	printf("borrow:ret is %d\n",ret);
	return ret;
	
}

void kdrop(void *pa)
{
	if((uint64)pa%PGSIZE||(char *)pa<kmem.start||(uint64)pa>PHYSTOP)
	panic("kdrop");
    uint64 index = ((uint64)pa - (uint64)kmem.start)>>12;//minus ref num
	acquire(&kmem.lock);

    int do_free = --kmem.ref[index];
	release(&kmem.lock);
	
	if(do_free==0)
	kfree(pa);
	
}
	
