#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <user/syscall.h>
#include "devices/input.h"
#include "devices/shutdown.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/interrupt.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"

struct lock syscall_lock;


static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{

  lock_init(&syscall_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  check_valid_pointer((const void*) f->esp);

  int arg[3];

  switch(* (int *) f->esp)
  {  

    case SYS_HALT:
      {
	halt(); 
	break;
      }

    case SYS_EXIT:
      {
	get_stackargs(f->esp, &arg[0], 1);
	exit(arg[0]);
	break;
      }

    case SYS_EXEC:
      {
	get_stackargs(f->esp, &arg[0], 1);
	arg[0] = check_return_pageptr((const void *) arg[0]);
	f->eax = exec((const char *) arg[0]); 
	break;
      }

    case SYS_WAIT:
      {
	get_stackargs(f->esp, &arg[0], 1);
	f->eax = wait(arg[0]);
	break;
      }

    case SYS_CREATE:
      {
	get_stackargs(f->esp, &arg[0], 2);
	arg[0] = check_return_pageptr((const void *) arg[0]);
	f->eax = create((const char *)arg[0], (unsigned) arg[1]);
	break;
      }

    
    case SYS_REMOVE:
      {
	get_stackargs(f->esp, &arg[0], 1);
	arg[0] = check_return_pageptr((const void *) arg[0]);
	f->eax = remove((const char *) arg[0]);
	break;
      }

    case SYS_OPEN:
      {
	get_stackargs(f->esp, &arg[0], 1);
	arg[0] = check_return_pageptr((const void *) arg[0]);
	f->eax = open((const char *) arg[0]);
	break; 		
      }

    case SYS_FILESIZE:
      {
	get_stackargs(f->esp, &arg[0], 1);
	f->eax = filesize(arg[0]);
	break;
      }
    case SYS_READ:
      {
	get_stackargs(f->esp, &arg[0], 3);
	check_valid_buffer((void *) arg[1], (unsigned) arg[2]);
	arg[1] = check_return_pageptr((const void *) arg[1]);
	f->eax = read(arg[0], (void *) arg[1], (unsigned) arg[2]);
	break;
      }
    case SYS_WRITE:
      { 
	get_stackargs(f->esp, &arg[0], 3);
	check_valid_buffer((void *) arg[1], (unsigned) arg[2]);
	arg[1] = check_return_pageptr((const void *) arg[1]);
	f->eax = write(arg[0], (const void *) arg[1],(unsigned) arg[2]);
	break;
      }

    case SYS_SEEK:
      {
	get_stackargs(f->esp, &arg[0], 2);
	seek(arg[0], (unsigned) arg[1]);
	break;
      } 
    case SYS_TELL:
      { 
	get_stackargs(f->esp, &arg[0], 1);
	f->eax = tell(arg[0]);
	break;
      }

    case SYS_CLOSE:
      { 
	get_stackargs(f->esp, &arg[0], 1);
	close(arg[0]);
	break;
      }

   }
  
}

void halt (void)
{
  shutdown_power_off();
}


void exit (int status)
{
  
  thread_current()->child->status = status;
  printf ("%s: exit(%d)\n", thread_current()->name, status);
  
  thread_exit();
}


pid_t exec (const char *cmd_line)
{
  pid_t id = process_execute(cmd_line);
  struct childstruct* c = retrieve_child(id);
 
  sema_down(&c->forload);
  if (c->load == 1)
    return id;
  else if(c->load == -1)
    return -1;
  return -1;
   
}

int wait (pid_t pid)
{
  return process_wait(pid);
}


bool create (const char *file, unsigned initial_size)
{
  lock_acquire(&syscall_lock);
  bool created = filesys_create(file, initial_size);
  lock_release(&syscall_lock);
  return created;
}

bool remove (const char *file)
{
  lock_acquire(&syscall_lock);
  bool removed = filesys_remove(file);
  lock_release(&syscall_lock);
  return removed;
}

int open (const char *file)
{
  lock_acquire(&syscall_lock);
  struct file *f = filesys_open(file);
  if (f==NULL)
    {
      lock_release(&syscall_lock);
      return -1;
    }
  
  struct process_file *new_f = malloc(sizeof(struct process_file));
  new_f->file_address = f;
  new_f->fd = thread_current()->fd;
  //file_deny_write(new_f->file_address);
  list_push_back(&thread_current()->flist, &new_f->f_elem);
  thread_current()->fd=thread_current()->fd+1;


  int fd= new_f->fd;

  lock_release(&syscall_lock);
  return fd;
}

int filesize(int fd)

{

  lock_acquire(&syscall_lock);

  struct file *f=retrieve_file(fd);
  if(f==NULL)
    {
	lock_release(&syscall_lock);
	return -1;
    }
  int bytes= (int) file_length(f);
  lock_release(&syscall_lock);
  return bytes;

}

int read (int fd, void *buffer, unsigned size)
{
  lock_acquire(&syscall_lock);

  
  if (fd == 0)
  {
    lock_release(&syscall_lock);
    return (int) input_getc();
  }

  if (!list_empty(&thread_current()->flist))
  {
    struct file *f=retrieve_file(fd);
    if(f==NULL)
    {
	lock_release(&syscall_lock);
	return -1;
    }
    int bytes = (int) file_read(f, buffer, size);
    lock_release(&syscall_lock);
    return bytes;
   }

   else
   {
     lock_release(&syscall_lock);
     return -1;
   }

}


int write (int fd, const void *buffer, unsigned size)
{
  lock_acquire(&syscall_lock);

  
  if (fd == 1)
  {
    putbuf(buffer,size);
    lock_release(&syscall_lock);
    return (int) size;
  }

  if (!list_empty(&thread_current()->flist))
  {
    struct file *f=retrieve_file(fd);
    if(f==NULL)
    {
	lock_release(&syscall_lock);
	return 0;
    }
    int bytes = (int) file_write(f, buffer, size);
    lock_release(&syscall_lock);
    return bytes;
   }

   else
   {
     lock_release(&syscall_lock);
     return -1;
   }


}


void seek(int fd,unsigned position)
{

  lock_acquire(&syscall_lock);

  struct file *f=retrieve_file(fd);
  if(f==NULL)
    {
	lock_release(&syscall_lock);
	return;
    }
  file_seek(f,position);
  lock_release(&syscall_lock);

}


unsigned tell(int fd)
{
 lock_acquire(&syscall_lock);

  struct file *f=retrieve_file(fd);
  if(f==NULL)
    {
	lock_release(&syscall_lock);
	return -1;
    }
  unsigned position = (unsigned) file_tell(f);
  lock_release(&syscall_lock);
  return position;
   
}

void close (int fd)
{
  lock_acquire(&syscall_lock);

  if(list_empty(&thread_current()->flist))
  {
     lock_release(&syscall_lock);
     return;

  }

  for (struct list_elem *e = list_front(&thread_current()->flist); e != list_end (&thread_current()->flist); e = list_next (e))
  { 
      
      struct process_file *pf = list_entry (e, struct process_file, f_elem);
      if (pf->fd == fd)
      {
        file_close(pf->file_address);
        //file_allow_write(pf->file_address);
        list_remove(&pf->f_elem);
        free(pf);
        lock_release(&syscall_lock);
        return;
      }
  }
  
  lock_release(&syscall_lock);
  return;
}




void check_valid_pointer (const void *vaddr)
{
  if (vaddr==NULL || !is_user_vaddr(vaddr) || vaddr < (void *) 0x08048000)
    {

      exit(-1);
      
    }
}

void check_valid_buffer (void *buffer, unsigned size)
{
  unsigned i=0;
  char *ptr  = (char * )buffer;
  while(i<size)
    {
      check_valid_pointer((const void *) ptr);
      ptr++;
      i++;
    }
}

void get_stackargs (void *esp, int *arg, int count)
{
  int i=0;
  while(i<count)
  {
    int *stack_pointer=(int *)esp+(i+1);
    check_valid_pointer((const void *) stack_pointer);
    arg[i]=*stack_pointer;
    i++;
  }
}

int check_return_pageptr(const void *vaddr)
{
  void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
  if (ptr==NULL)
      {exit(-1);
       
       }
  return (int) ptr;
}



struct file* retrieve_file (int fd)
{
  struct thread *t = thread_current();

  for (struct list_elem *e = list_begin (&t->flist); e != list_end (&t->flist); e = list_next (e))
        {
          struct process_file *pf = list_entry (e, struct process_file, f_elem);
          if (fd == pf->fd)
		return pf->file_address;
	    
        }
  return NULL;
}



void close_allfiles(struct thread *t)
{ 
  
  if(t->execfile != NULL)
      file_close(t->execfile);
  struct list_elem *e;

	while(!list_empty(&t->flist))
	{
		e = list_pop_front(&t->flist);
		struct process_file *pf = list_entry (e, struct process_file, f_elem);
		list_remove(&pf->f_elem);
		free(pf);

	}

}



