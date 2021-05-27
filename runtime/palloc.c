/*****************************************************************************/
/* Persistent memory storage. */
/*****************************************************************************/

#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include "runtime.h"
#include "../build/magic.h"

#define PERSISTENT_SIZE (1024L * 1024L * 1024L * 128L)
#define BOTTOM_ADDR ((void*)0x0000001000000000)
#define FTABLE_SIZE 64

/*****************************************************************************/
/* Persistent constants. */
/*****************************************************************************/

void*** pconsts = NULL;

/*****************************************************************************/
/* Gensym. */
/*****************************************************************************/

uint64_t* gid;

uint64_t SKIP_genSym(uint64_t unused) {
  return __atomic_fetch_add(gid, 1, __ATOMIC_RELAXED);
}

/*****************************************************************************/
/* Global locking. */
/*****************************************************************************/

pthread_mutexattr_t* gmutex_attr;
pthread_mutex_t* gmutex = (void*)1234;

// This is only used for debugging purposes
static int sk_is_locked = 0;

void sk_check_has_lock() {
  if(!sk_is_locked) {
    fprintf(stderr, "INTERNAL ERROR: unsafe operation\n");
    SKIP_throw(NULL);
  }
}

void sk_global_lock_init() {
  pthread_mutexattr_init(gmutex_attr);
  pthread_mutexattr_setpshared(gmutex_attr, PTHREAD_PROCESS_SHARED);
  pthread_mutexattr_setrobust(gmutex_attr, PTHREAD_MUTEX_ROBUST);
  pthread_mutex_init(gmutex, gmutex_attr);
}

void sk_global_lock() {
  if(gmutex == NULL) {
    return;
  }

  int code = pthread_mutex_lock(gmutex);
  sk_is_locked = 1;

  if(code == 0) {
    return;
  }

  if(code == EOWNERDEAD) {
    pthread_mutex_consistent(gmutex);
    return;
  }

  fprintf(stderr, "Internal error: locking failed\n");
  exit(44);
}

void sk_global_unlock() {
  if(gmutex == NULL) {
    return;
  }

  int code = pthread_mutex_unlock(gmutex);
  sk_is_locked = 0;

  if(code == 0) {
    return;
  }

  fprintf(stderr, "Internal error: unlocking failed\n");
  exit(44);
}

/*****************************************************************************/
/* The global information structure. */
/*****************************************************************************/

typedef struct {
  void* ginfo_array;
  void* ftable[FTABLE_SIZE];
  void* context;
  char* begin;
  char* head;
  char* end;
  char* fileName;
  char* break_ptr;
} ginfo_t;

ginfo_t **ginfo_root = NULL;
ginfo_t **ginfo = NULL;

char* sk_context_get_unsafe() {
  char* context = (*ginfo)->context;

  if(context != NULL) {
    sk_incr_ref_count(context);
  }

  return context;
}

char* SKIP_context_get() {
  sk_global_lock();
  char* context = sk_context_get_unsafe();
  sk_global_unlock();
  return context;
}

void sk_context_set(char* obj) {
  sk_global_lock();
  (*ginfo)->context = obj;
  sk_global_unlock();
}

void sk_context_set_unsafe(char* obj) {
  (*ginfo)->context = obj;
}

/*****************************************************************************/
/* File name parser (from the command line arguments). */
/*****************************************************************************/

static char* parse_args(int argc, char** argv, int* is_init) {
  int i;
  int idx = -1;

  for(i = 1; i < argc; i++) {
    if(strcmp(argv[i], "--data") == 0 || strcmp(argv[i], "--init") == 0) {
      if(strcmp(argv[i], "--init") == 0) {
        *is_init = 1;
      }
      if(i + 1 >= argc) {
        fprintf(stderr, "Error: --data/--init expects a file name");
        exit(102);
      }
      if(idx != -1) {
        fprintf(stderr, "Error: incompatible --data/--init options");
        exit(103);
      }
      idx = i+1;
    }
  }

  if(idx == -1) {
    return NULL;
  }
  else {
    return argv[idx];
  }
}

/*****************************************************************************/
/* Staging/commit. */
/*****************************************************************************/

void sk_staging() {
  if((*ginfo)->fileName == NULL) {
    return;
  }

  sk_global_lock();

  ginfo_root = ginfo;

  ginfo_t* array = (*ginfo)->ginfo_array;
  ginfo_t* ginfo_data = *ginfo;
  ginfo = malloc(sizeof(ginfo_t*));
  *ginfo = ginfo_data;

  if(*ginfo == array) {
    array[1] = array[0];
    *ginfo = &array[1];
  }
  else {
    array[0] = array[1];
    *ginfo = &array[0];
  }
}

void sk_commit() {
  if((*ginfo)->fileName == NULL) {
    return;
  }

  ginfo_t* array = (*ginfo)->ginfo_array;
  ginfo_t* ginfo_data = *ginfo;
  free(ginfo);
  ginfo = ginfo_root;
  __atomic_thread_fence(__ATOMIC_RELEASE);
  __atomic_store(ginfo, &ginfo_data, __ATOMIC_RELEASE);

  sk_global_unlock();
}

/*****************************************************************************/
/* Creates a new file mapping. */
/*****************************************************************************/

void sk_create_mapping(char* fileName) {
  if(access(fileName, F_OK) == 0) {
    fprintf(stderr, "ERROR: File %s already exists!\n", fileName);
    exit(21);
  }
  int fd = open(fileName, O_RDWR | O_CREAT, 0600);
  lseek(fd, PERSISTENT_SIZE, SEEK_SET);
  write(fd, "", 1);
  int prot = PROT_READ | PROT_WRITE;
  char* begin = mmap(BOTTOM_ADDR, PERSISTENT_SIZE, prot, MAP_SHARED | MAP_FIXED, fd, 0);
  char* end = begin + PERSISTENT_SIZE;

  if(begin == (void*)-1) {
    perror("ERROR (MMAP FAILED)");
    exit(23);
  }

  close(fd);

  char* head = begin;

  *(uint64_t*)head = MAGIC;
  head += sizeof(uint64_t);

  *(void**)head = begin;
  head += sizeof(void*);

  gmutex_attr = (pthread_mutexattr_t*) head;
  head += sizeof(pthread_mutexattr_t);

  gmutex = (pthread_mutex_t*)head;
  head += sizeof(pthread_mutex_t);

  ginfo_t* ginfo_data = (ginfo_t*)head;
  head += 2 * sizeof(ginfo_t);

  ginfo = (ginfo_t**)head;
  head += sizeof(ginfo_t*);

  gid = (uint64_t*)head;
  head += sizeof(uint64_t);

  pconsts = (void***)head;
  head += sizeof(void**);

  size_t fileName_length = strlen(fileName)+1;
  char* persistent_fileName = head;
  head += fileName_length;

  memcpy(persistent_fileName, fileName, fileName_length);

  *ginfo = ginfo_data;

  (*ginfo)->ginfo_array = ginfo_data;

  int i;
  for(i = 0; i < FTABLE_SIZE; i++) {
    (*ginfo)->ftable[i] = NULL;
  }

  if(head >= end) {
    fprintf(stderr, "Could not initialize memory\n");
    exit(31);
  }

  (*ginfo)->break_ptr = sbrk(0);

  // The head must be aligned!
  head = (char*)(((uintptr_t)head + (uintptr_t)(15)) & ~((uintptr_t)(15)));

  (*ginfo)->begin = begin;
  (*ginfo)->head = head;
  (*ginfo)->end = end;
  (*ginfo)->fileName = persistent_fileName;
  *gid = 1;
  *pconsts = NULL;

  sk_global_lock_init();
}

/*****************************************************************************/
/* Loads an existing mapping. */
/*****************************************************************************/

void sk_load_mapping(char* fileName) {
  int fd = open(fileName, O_RDWR, 0600);

  if(fd == -1) {
    fprintf(stderr, "Error: could not open file (did you run --init?)\n");
    exit(25);
  }

  void* addr;
  uint64_t magic;
  lseek(fd, 0L, SEEK_SET);
  int magic_size = read(fd, &magic, sizeof(uint64_t));

  if(magic_size != sizeof(uint64_t) || magic != MAGIC) {
    fprintf(stderr, "Error: wrong file format\n");
    exit(23);
  }

  int bytes = read(fd, &addr, sizeof(void*));
  if(bytes != sizeof(void*)) {
    fprintf(stderr, "Error: could not read heap address\n");
    exit(24);
  }

  lseek(fd, 0L, SEEK_SET);

  int prot = PROT_READ | PROT_WRITE;
  lseek(fd, 0L, SEEK_END);
  size_t fsize = lseek(fd, 0, SEEK_CUR) - 1;
  char* begin = mmap(addr, fsize, prot, MAP_SHARED | MAP_FIXED, fd, 0);
  close(fd);

  if(begin == (void*)-1) {
    perror("ERROR (MMAP FAILED)");
    exit(23);
  }

  char* head = begin;
  head += sizeof(uint64_t);
  head += sizeof(void*);
  head += sizeof(pthread_mutexattr_t);
  gmutex = (pthread_mutex_t*)head;
  head += sizeof(pthread_mutex_t);
  head += 2 * sizeof(ginfo_t);
  ginfo = (ginfo_t**)head;
  head += sizeof(ginfo_t*);
  gid = (uint64_t*)head;
  head += sizeof(uint64_t);
  pconsts = (void***)head;
  head += sizeof(void**);
}

/*****************************************************************************/
/* Detects pointers that come from the binary. */
/*****************************************************************************/

int sk_is_static(void* ptr) {
  return (char*)ptr <= (*ginfo)->break_ptr;
}

/*****************************************************************************/
/* Free table. */
/*****************************************************************************/

size_t sk_bit_size(size_t size) {
  return (size_t)(sizeof(size_t) * 8 - __builtin_clzl(size - 1));
}

size_t sk_pow2_size(size_t size) {
  size = (size + (sizeof(void*) - 1)) & ~(sizeof(void*)-1);
  return (1 << sk_bit_size(size));
}

void sk_add_ftable(void* ptr, size_t size) {
  int slot = sk_bit_size(size);
  *(void**)ptr = (*ginfo)->ftable[slot];
  (*ginfo)->ftable[slot] = ptr;
}

void* sk_get_ftable(size_t size) {
  int slot = sk_bit_size(size);
  void** ptr = (*ginfo)->ftable[slot];
  if(ptr == NULL) {
    return ptr;
  }
  (*ginfo)->ftable[slot] = *(void**)(*ginfo)->ftable[slot];
  return ptr;
}

/*****************************************************************************/
/* No file initialization (the memory is not backed by a file). */
/*****************************************************************************/

static void sk_init_no_file() {
  ginfo = malloc(sizeof(ginfo_t*));
  *ginfo = malloc(sizeof(ginfo_t));
  (*ginfo)->break_ptr = sbrk(0);
  (*ginfo)->fileName = NULL;
  gmutex = NULL;
  gid = malloc(sizeof(uint64_t));
  pconsts = malloc(sizeof(void**));
  *gid = 1;
  *pconsts = NULL;
}

/*****************************************************************************/
/* Memory initialization. */
/*****************************************************************************/

void SKIP_memory_init(int argc, char** argv) {
  int is_create = 0;
  char* fileName = parse_args(argc, argv, &is_create);

  if(fileName == NULL) {
    sk_init_no_file();
    return;
  }
  if(is_create) {
    sk_create_mapping(fileName);
    return;
  }
  sk_load_mapping(fileName);
}

/*****************************************************************************/
/* Persistent alloc/free primitives. */
/*****************************************************************************/

size_t total_palloc_size = 0;

void* sk_palloc(size_t size) {
  if((*ginfo)->fileName == NULL) {
    return malloc(size);
  }
  sk_check_has_lock();
  size = sk_pow2_size(size);
  total_palloc_size += size;
  sk_cell_t* ptr = sk_get_ftable(size);
  if(ptr != NULL) {
    return ptr;
  }
  if((*ginfo)->head + size >= (*ginfo)->end) {
    fprintf(stderr, "Error: out of persistent memory.");
    exit(45);
  }
  void* result = (*ginfo)->head;
  (*ginfo)->head += size;
  return result;
}

void sk_pfree_size(void* chunk, size_t size) {
  if((*ginfo)->fileName == NULL) {
    free(chunk);
    return;
  }
  sk_check_has_lock();
  size = sk_pow2_size(size);
  total_palloc_size -= size;
  sk_add_ftable(chunk, size);
}
