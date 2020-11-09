#include "runtime.h"

void* memset(void* ptr, int value, size_t num) {
  size_t i;
  unsigned char* p = (unsigned char*)ptr;
  for(i = 0; i < num; i++) {
    p[i] = (unsigned char)value;
  }
  return ptr;
}

void* memcpy(void* dest, const void* src, size_t size) {
  void* result = dest;
  const char* end = (char*)src + size;
  const char* lend = (char*)src + (size / sizeof(long) * sizeof(long));

  while(src < (void*)lend) {
    *(long*)dest = *(long*)src;
    dest += sizeof(long);
    src += sizeof(long);
  }

  while(src < (void*)end) {
    *(char*)dest = *(char*)src;
    dest++;
    src++;
  }
  return result;
}

void print_int(uint64_t x) {
  char str[256];
  SkipInt i = 255;
  if(x == 0) {
    SKIP_print_char('0');
    SKIP_print_char('\n');
    return;
  }
  while(x != 0) {
    if(x % 10 == 0) str[i] = '0';
    if(x % 10 == 1) str[i] = '1';
    if(x % 10 == 2) str[i] = '2';
    if(x % 10 == 3) str[i] = '3';
    if(x % 10 == 4) str[i] = '4';
    if(x % 10 == 5) str[i] = '5';
    if(x % 10 == 6) str[i] = '6';
    if(x % 10 == 7) str[i] = '7';
    if(x % 10 == 8) str[i] = '8';
    if(x % 10 == 9) str[i] = '9';
    i--;
    x = x / 10;
  };
  for(i++; i < 256; i++) {
    SKIP_print_char(str[i]);
  };
  SKIP_print_char('\n');
}

void todo() {
  SKIP_throw(NULL);
}


char* SKIP_read_line() {
  uint32_t size = SKIP_read_line_fill();
  uint32_t i;
  char* result = SKIP_Obstack_alloc(size);

  for(i = 0; i < size; i++) {
    result[i] = SKIP_read_line_get(i);
  }

  result = sk_string_create(result, size);
  return result;
}
