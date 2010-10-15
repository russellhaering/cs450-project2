#include "obj.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>


int dyn_array_init(struct dyn_array *arr, long size)
{
  arr->size = size;
  arr->increment = size;
  arr->count = 0;
  arr->items = (void **) malloc(size * sizeof(void *));

  if (arr->items == NULL) {
    perror("Error allocating space for dyn_arr items");
    return -1;
  }

  return 0;
}

int dyn_array_append(struct dyn_array *arr, void *item)
{
  long index = arr->count;
  arr->count++;

  if (arr->count > arr->size) {
    arr->size += arr->increment;
    arr->items = (void **) realloc(arr->items, arr->size * sizeof(void *));
 
    if (arr->items == NULL) {
      perror("Error allocating additional space for dyn_arr items");
      return -1;
    }
  }

  arr->items[index] = item;
  return index;
}

int dyn_array_trim(struct dyn_array *arr)
{
  arr->size = arr->count;
  arr->items = (void **) realloc(arr->items, arr->size * sizeof(void *));
}

void *dyn_array_get(struct dyn_array *arr, long index)
{
  if (index >= arr->count) {
    return NULL;
  }
  else {
    return arr->items[index];
  }
}

char *next_line(char *bptr)
{
  // Consume the rest of the current line
  do {
    bptr++;
  } while (*bptr != '\r' || *bptr != '\n');

  // Consume the line termination character(s)
  do {
    bptr++;
  } while (*bptr == '\r' || *bptr == '\n');

  return bptr;
}

struct obj_data *load_obj_file(char *filename)
{
  int fd;
  char *fbuf, *bptr;
  struct stat stats;
  off_t fsize;
  struct obj_data *d;

  fd = open(filename, O_RDONLY);
  if (fd < 0) {
    perror("Error opening data file");
    return NULL;
  }

  if (fstat(fd, &stats) < 0) {
    perror("Error stat-ing data file");
    return NULL;
  }

  fsize = stats.st_size;
  fbuf = (char *) mmap(0, fsize, PROT_READ, MAP_SHARED, fd, 0);

  if (fbuf == MAP_FAILED) {
    perror("Error mmap-ing file");
    return NULL;
  }

  madvise(fbuf, fsize, MADV_SEQUENTIAL);
  bptr = fbuf;

  d = (struct obj_data *) malloc(sizeof(struct obj_data));
  if (d == NULL) {
    perror("Error allocating space for .obj metadata");
    munmap(fbuf, fsize);
    return NULL;
  }

  d->vs = (struct dyn_array *) malloc(sizeof(struct dyn_array));
  if (d->vs == NULL) {
    free(d);
    munmap(fbuf, fsize);
    return NULL;
  }

  d->vts = (struct dyn_array *) malloc(sizeof(struct dyn_array));
  if (d->vts == NULL) {
    free(d->vs);
    free(d);
    munmap(fbuf, fsize);
    return NULL;
  }

  d->vns = (struct dyn_array *) malloc(sizeof(struct dyn_array));
  if (d->vns == NULL) {
    free(d->vts);
    free(d->vs);
    free(d);
    munmap(fbuf, fsize);
    return NULL;
  }

  d->faces = (struct dyn_array *) malloc(sizeof(struct dyn_array));
  if (d->faces == NULL) {
    free(d->vns);
    free(d->vts);
    free(d->vs);
    free(d);
    munmap(fbuf, fsize);
  }

  dyn_array_init(d->vs, 500);
  dyn_array_init(d->vts, 100);
  dyn_array_init(d->vns, 100);
  dyn_array_init(d->faces, 500);

  struct vertex *v;
  struct vertex_texture *vt;
  struct vertex_normal *vn;

  while (1) {
    switch (*bptr) {
      case '#':
        bptr = next_line(bptr);
        break;

      case 'v':
        bptr++;

        switch (*bptr) {
          case ' ':
            v = (struct vertex *) malloc(sizeof(struct vertex));
            v->x = strtof(bptr, &bptr);
            v->y = strtof(bptr, &bptr);
            v->z = strtof(bptr, &bptr);
            dyn_array_append(d->vs, v);
            break;

          case 't':
            bptr++;
            vt = (struct vertex_texture *) malloc(sizeof(struct vertex_texture));
            vt->u = strtof(bptr, &bptr);
            vt->v = strtof(bptr, &bptr);
            vt->w = strtof(bptr, &bptr);
            dyn_array_append(d->vts, vt);
            break;

          case 'n':
            bptr++;
            vn = (struct vertex_normal *) malloc(sizeof(struct vertex_normal));
            vn->x = strtof(bptr, &bptr);
            vn->y = strtof(bptr, &bptr);
            vn->z = strtof(bptr, &bptr);
            dyn_array_append(d->vns, vn);
            break;

          default:
            printf("Unrecognized vertex type in .obj file\n");
        }
        break;

      case 'f':
        //TODO: Read faces
        printf("Face found in .obj file, ignored\n");
        break;
      
      default:
        printf("Unrecognized line in .obj file\n");
    }
  }
}
