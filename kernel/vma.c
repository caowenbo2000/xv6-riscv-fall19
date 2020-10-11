
#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "vma.h"

struct
{
  struct spinlock lock;
  struct vma vma[NVMA];
}vmatable;

void vmainit(void)
{
  initlock(&vmatable.lock,"vmatable_lock");
}

struct vma *vmaalloc(void)
{
  struct vma* v;
  v=0;
  acquire(&vmatable.lock);
  for(int i=0;i<NVMA;i++)
  {
    if(vmatable.vma[i].valid==0)
	{
	  v=&vmatable.vma[i];
	  v->valid=1;
	  break;
	}
  }
  release(&vmatable.lock);
  return v;
}

