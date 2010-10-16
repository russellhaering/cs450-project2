#include "obj.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
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

char *resize_linebuf(char *lbuf, off_t size)
{
  lbuf = (char *) realloc(lbuf, size);
  if (lbuf == NULL) {
    perror("Error allocating space for line buffer");
  }

  return lbuf;
}

struct obj_data *load_obj_file(char *filename)
{
  int fd, ps, i, idx;
  char *fbuf, *bptr, *lbuf, *lptr, *eptr, fmask;
  struct stat stats;
  off_t fsize, lbufsize, lbufcnt;
  struct obj_data *d;
  struct vertex *v;
  struct vertex_texture *vt;
  struct vertex_normal *vn;
  struct face *f;


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

  lbufsize = LINEBUF_SIZE;
  lbufcnt = 0;
  lbuf = resize_linebuf(NULL, LINEBUF_SIZE);

  eptr = fbuf + fsize; 
  bptr = fbuf;

  while (bptr < eptr) {
    while (bptr < eptr && *bptr != '\n' && *bptr != '\r') {
      lbuf[lbufcnt] = *bptr;
      lbufcnt++;

      if (lbufcnt == lbufsize) {
        lbufsize += LINEBUF_SIZE;
        lbuf = resize_linebuf(lbuf, lbufsize);
      }
      bptr++;
    }

    // There is still at least 1 free slot in the lbuf
    lbuf[lbufcnt] = '\0';
    lbufcnt++;

    while (bptr < eptr && *bptr == '\n' || *bptr == '\r') {
      bptr++;
    }

    lbufcnt = 0;

    switch (lbuf[0]) {
      case 'v':
        switch (lbuf[1]) {
          case ' ':
            lptr = lbuf + 1;
            v = (struct vertex *) malloc(sizeof(struct vertex));
            v->x = strtof(lptr, &lptr);
            v->y = strtof(lptr, &lptr);
            v->z = strtof(lptr, &lptr);
            dyn_array_append(d->vs, v);
            break;

          case 't':
            lptr = lbuf + 2;;
            vt = (struct vertex_texture *) malloc(sizeof(struct vertex_texture));
            vt->u = strtof(lptr, &lptr);
            vt->v = strtof(lptr, &lptr);
            vt->w = strtof(lptr, &lptr);
            dyn_array_append(d->vts, vt);
            break;

          case 'n':
            lptr = lbuf + 2;
            vn = (struct vertex_normal *) malloc(sizeof(struct vertex_normal));
            vn->x = strtof(lptr, &lptr);
            vn->y = strtof(lptr, &lptr);
            vn->z = strtof(lptr, &lptr);
            dyn_array_append(d->vns, vn);
            break;

          default:
            printf("Unrecognized vertex type in .obj file\n");
        }
        break;

      case 'f':
        lptr = lbuf + 1;
        if (*lptr == ' ') {
          f = (struct face *) malloc(sizeof(struct face));
          f->vts = NULL;
          f->vns = NULL;

          // Figure out how many vertices there are
          i = 0;
          ps = 1;
          while (*lptr != '\0') {
            if (isspace(*lptr)) {
              ps = 1;
            }
            else {
              if (ps) {
                i++;
              }
              ps = 0;
            }
            lptr++;
          }
          f->count = i;
          // TODO: Allocate these lazily
          f->vs = (struct vertex **) malloc(f->count * sizeof(struct vertex *));

          lptr = lbuf + 1;
          i = 0;
          while (i < f->count) {
            // Store the vertex
            idx = strtol(lptr, &lptr, 10);
            idx--;
            if (idx >= 0) {
              if (idx < d->vs->count) {
                f->vs[i] = (struct vertex *) d->vs->items[idx];
              }
              else {
                printf("Invalid vertex index\n");
              }
            }
            else {
              printf("This .obj loader doesn't support negative indices\n");
            }

            if (*lptr == '/') {
              // Store the vertex texture (if there is one)
              if (*(lptr + 1) != '/') {
                lptr++;
                // Lazily allocate space for these on first run
                if (i == 0) {
                  f->vts = (struct vertex_texture **) malloc(f->count * sizeof(struct vertex_texture *));
                }

                if (f->vts != NULL) {
                  idx = strtol(lptr, &lptr, 10);
                  idx--;
                  if (idx >= 0) {
                    if (idx < d->vts->count) {
                      f->vts[i] = (struct vertex_texture *) d->vts->items[idx];
                    }
                    else {
                      printf("Invalid vertex index\n");
                    }
                  }
                  else {
                    printf("This .obj loader doesn't support negative indices\n");
                  }
                }
                else {
                  printf("Unexpected vertex texture, ignoring\n");
                }
              }
              else {
                lptr++;
              }

              // Store the vertex normal (if there is one)
              if (*lptr == '/') {
                lptr++;
                // Lazily allocate space for these on first run
                if (i == 0) {
                  f->vns = (struct vertex_normal **) malloc(f->count * sizeof(struct vertex_normal *));
                }

                if (f->vns != NULL) {
                  idx = strtol(lptr, &lptr, 10);
                  idx--;
                  if (idx >= 0) {
                    if (idx < d->vns->count) {
                      f->vns[i] = (struct vertex_normal *) d->vns->items[idx];
                    }
                    else {
                      printf("Invalid vertex index\n");
                    }
                  }
                  else {
                    printf("This .obj loader doesn't support negative indices\n");
                  }
                }
                else {
                  printf("Unexpected vertex normal, ignoring\n");
                }
              }
            }
            i++;
          }
          dyn_array_append(d->faces, f);
        }
        else {
          printf("Unrecognized line in .obj file\n");
        }
        break;

      default:
        printf("Unrecognized line in .obj file\n");
    }
  }
}
