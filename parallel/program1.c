#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
void* print_xs (void* unused)
{
  while (1)
  fputc ('x', stderr);
  return NULL;
}
int main ()
{
  int p;
  pthread_t thread_id;
  p = pthread_create (&thread_id, NULL, &print_xs, NULL);
  if (p != 0) { perror("Thread problem"); exit(1);}
  while (1)
  fputc ('o', stderr);
  return 0;
}

