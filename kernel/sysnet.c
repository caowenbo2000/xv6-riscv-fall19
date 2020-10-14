//
// network system calls.
//

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "net.h"

struct sock {
  struct sock *next; // the next socket in the list
  uint32 raddr;      // the remote IPv4 address
  uint16 lport;      // the local UDP port number
  uint16 rport;      // the remote UDP port number
  struct spinlock lock; // protects the rxq
  struct mbufq rxq;  // a queue of packets waiting to be received
};

static struct spinlock lock;
static struct sock *sockets;

void
sockinit(void)
{
  initlock(&lock, "socktbl");
}

int
sockalloc(struct file **f, uint32 raddr, uint16 lport, uint16 rport)
{
  struct sock *si, *pos;

  si = 0;
  *f = 0;
  if ((*f = filealloc()) == 0)
    goto bad;
  if ((si = (struct sock*)kalloc()) == 0)
    goto bad;

  // initialize objects
  si->raddr = raddr;
  si->lport = lport;
  si->rport = rport;
  initlock(&si->lock, "sock");
  mbufq_init(&si->rxq);
  (*f)->type = FD_SOCK;
  (*f)->readable = 1;
  (*f)->writable = 1;
  (*f)->sock = si;

  // add to list of sockets
  acquire(&lock);
  pos = sockets;
  while (pos) {
    if (pos->raddr == raddr &&
        pos->lport == lport &&
	pos->rport == rport) {
      release(&lock);
      goto bad;
    }
    pos = pos->next;
  }
  si->next = sockets;
  sockets = si;
  release(&lock);
  return 0;

bad:
  if (si)
    kfree((char*)si);
  if (*f)
    fileclose(*f);
  return -1;
}

//
// Your code here.
//
// Add and wire in methods to handle closing, reading,
// and writing for network sockets.
//

int sockread(struct sock *so, uint64 addr,int n)
{
  acquire(&so->lock);
  while(mbufq_empty(&so->rxq))
  {
    sleep(&so->rxq,&so->lock);
  }
  
  struct mbuf* mbuf = so->rxq.head;
  char *buf = mbuf->head;
  int dofree = 0;
  if(n>=mbuf->len)
  {
    mbufq_pophead(&so->rxq);
    n = mbuf->len;
	dofree=1;
  }
  else 
  {
    mbufpull(mbuf,n);
  }
  release(&so->lock);
  if(copyout(myproc()->pagetable,addr,buf,n)==-1)
  {
    if(dofree)
	  mbuffree(mbuf);
	return -1;
  }
  if(dofree)
    mbuffree(mbuf);
  return n;
  
}

int sockwrite(struct sock *so,uint64 addr ,int n)
{
  if(n>MAX_UDP_PAYLOAD)
    return -1;
  struct mbuf *mbuf = mbufalloc(MBUF_DEFAULT_HEADROOM);
  char *buf = mbuf->head;
  if(copyin(myproc()->pagetable,buf,addr,n)==-1)
  {
    mbuffree(mbuf);
	return -1;
  }
  mbufput(mbuf,n);
  net_tx_udp(mbuf,so->raddr,so->lport,so->rport);;
  return n;
}


void sockclose(struct sock* so)
{
  acquire(&lock);
  struct sock *prev,*pos;
  prev=pos=sockets;
  while(pos)
  {
    if(pos == so)
	{
	  if(prev==pos)
	    sockets = pos->next;
	  else 
	    prev->next = pos->next;
	  release(&lock);
	  struct mbuf *m=pos->rxq.head;
	  struct mbuf *to_free;
	  while(m)
      {
	    to_free = m;
	    m = m->next;
	    mbuffree(to_free);
	  }
	  kfree((void *)so);
	  return ;
	}
	prev = pos;
	pos = pos->next;
  }
  release(&lock);
  panic("cannt find");
  
}
// called by protocol handler layer to deliver UDP packets
void
sockrecvudp(struct mbuf *m, uint32 raddr, uint16 lport, uint16 rport)
{
  //
  // Your code here.
  //
  // Find the socket that handles this mbuf and deliver it, waking
  // any sleeping reader. Free the mbuf if there are no sockets
  // registered to handle it.
  //
  struct sock* pos;
  acquire(&lock);
  pos = sockets;
  while(pos)
  {
    if(pos->raddr==raddr&&
	   pos->lport==lport&&
	   pos->rport==rport)
	{
      acquire(&pos->lock);
	  mbufq_pushtail(&pos->rxq,m);
	  release(&pos->lock);
	  release(&lock);
	  wakeup(&pos->rxq);
	  return;
	}
	pos=pos->next;

  }
  mbuffree(m);
  release(&lock);
}
