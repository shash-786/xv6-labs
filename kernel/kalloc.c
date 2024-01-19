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
} kmem;

struct  {
  struct spinlock lock;
  int count[(PGROUNDUP(PHYSTOP)-KERNBASE)/PGSIZE];
} reference_count_helper;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&reference_count_helper.lock, "reference_count_helper");
  for(int i=0; i<(PGROUNDUP(PHYSTOP)-KERNBASE)/PGSIZE; i++) {
    reference_count_helper.count[i] = 1;
  }
  freerange(end, (void*)PHYSTOP);
}


void
increase_reference_of_page(void* pa) {
  acquire(&reference_count_helper.lock);
  reference_count_helper.count[PA2INDX(pa)]++;
  release(&reference_count_helper.lock);
}

void
decrease_refernce_of_page(void* pa) {
  acquire(&reference_count_helper.lock);
  reference_count_helper.count[PA2INDX(pa)]--;
  release(&reference_count_helper.lock);
}

int
get_reference_count(void* pa) {
  int count;
  acquire(&reference_count_helper.lock);
  count = reference_count_helper.count[PA2INDX(pa)];
  release(&reference_count_helper.lock);
  return count;
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");
  if(get_reference_count(pa) <= 0) {
    panic("Already Freed calling func --> kfree");
  }
  decrease_refernce_of_page(pa);
  if(get_reference_count(pa) > 0) {
    return;
  }
  struct run *r;
  
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);
  r = (struct run*)pa;
  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void*
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk
    acquire(&reference_count_helper.lock);
    reference_count_helper.count[PA2INDX(r)] = 1;
    release(&reference_count_helper.lock);
  }
  return (void*)r;
}
