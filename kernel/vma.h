#define PROT_READ    0x1
#define PROT_WRITE   0x2
#define MAP_PRIVATE  0x1
#define MAP_SHARED   0x2
struct vma
{
  void *ostart;
  void *start;
  void *end;
  int flags;
  int prot;
  int valid;
  int offset;
  struct file* file;
};
