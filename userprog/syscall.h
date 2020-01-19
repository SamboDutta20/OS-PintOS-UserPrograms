#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/synch.h"

struct process_file {
  struct file *file_address;
  int fd;
  struct list_elem f_elem;
};

struct file* retrieve_file (int fd);
void close_allfiles(struct thread *t);
void exit (int status);


void check_valid_pointer (const void *vaddr);
void check_valid_buffer (void *buffer, unsigned size);
void get_stackargs (void *esp, int *arg, int count);
int check_return_pageptr(const void *vaddr);



void syscall_init (void);

#endif /* userprog/syscall.h */
