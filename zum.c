/*
 * zum 1.00 - free more disk space by making holes in files.
 *
 * Oleg Kibirev * April 1995 * oleg@gd.cs.CSUFresno.EDU
 *
 * This code is covered by General Public License, version 2 or any later
 * version of your choice. You should recieve file "COPYING" which contains
 * text of the license with any distribution of this program; if you don't 
 * have it, a copy is available from ftp.gnu.ai.mit.edu.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <fcntl.h>
#include <errno.h>
#include <utime.h>
#include <alloca.h>
#include <unistd.h>

extern int errno;

static char suffix[] = "__zum__";

static int zero_copy(int fds, int fdd, int size)
{
  char *ms;
  char *bp, *p, *ep;
  
  lseek(fdd, 0L, SEEK_SET);
  if(ftruncate(fdd, 0) < 0) {
    perror("ftruncate");
    return -1;
  }
  
  if((ms = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fds, 0)) == -1) {
    perror("mmap");
    return -1;
  }

  p = ms; ep = ms + size;
  
  while(p < ep) {
    for(bp = p; p < ep && *p; p++);
    if(p != bp && write(fdd, bp, p-bp) != p-bp) {
	perror("write");
	munmap(ms, size);
	return -1;
      }
    for(bp = p; p < ep && !*p; p++);
    if(p != bp)
      lseek(fdd, p-bp, SEEK_CUR);
  }
  munmap(ms, size);
  return ftruncate(fdd, size);
}


static void zero_unmap(char *file)
{
  static int fds = -1, fdd = -1;
  char *dest;
  struct stat st, std;

  if(fds > 0) { close(fds); fds = -1; }
  if(fdd > 0) { close(fdd); fdd = -1; }

  if((fds = open(file, O_RDONLY)) < 0 || lstat(file, &st) < 0) {
    perror(file);
    return;
  }

  if(!S_ISREG(st.st_mode)) {
    fprintf(stderr, "%s: Not a regular file.\n", file);
    return;
  }

  fputs(file, stdout); fflush(stdout);

  dest = alloca(strlen(file) + sizeof(suffix));
  sprintf(dest, "%s%s", file, suffix);
  if((fdd = open(dest, O_RDWR|O_CREAT|O_EXCL, 0600)) < 0) {
    perror(dest);
    return;
   }

  if(zero_copy(fds, fdd, st.st_size) < 0)
    return;

  if(fstat(fdd, &std) < 0) {
    perror(dest);
    unlink(dest);
    return;
  }

  if(std.st_blocks >=  st.st_blocks) {
    unlink(dest);
    putchar('\n'); fflush(stdout);
    return;
  }

  printf(" [%uK] ", (st.st_blocks-std.st_blocks)*st.st_blksize/1024);
  fflush(stdout);

  if(st.st_nlink == 1) {
    unlink(file);
    if(rename(dest, file) < 0) {
      perror("rename");
      return;
    }
    fputs(" [1 link]\n", stdout); fflush(stdout);
  } else {
    close(fds);
    if((fds = open(file, O_RDWR)) < 0) {
      perror(file);
      unlink(dest);
      return;
    }
    if(zero_copy(fdd, fds, st.st_size) < 0)
      return;

    unlink(dest);
    fputs(" [many links]\n", stdout); fflush(stdout);
  }

  /* 
   * Don't check chown errors, since after all this might not be root.
   */
  chown(file, st.st_uid, st.st_gid);
  if(chmod(file, st.st_mode) < 0)
    perror("Can't reset mode");
  
  {
    struct utimbuf b;
    b.actime  = st.st_atime;
    b.modtime = st.st_mtime;
    utime(file, &b);
  }
}

main(int argc, char **argv)
{
  char *p;

  if(argc > 1)
    while(p = *(++argv))
      zero_unmap(p);
  else {
    char buf[MAXPATHLEN];
    while(gets(buf))
      zero_unmap(buf);
  }
  return 0;
}
