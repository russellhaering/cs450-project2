#ifndef __OBJ_H
#define __OBJ_H

// Types of vertices

struct vertex {
  float x;
  float y;
  float z;
};

struct vertex_texture {
  float u;
  float v;
  float w;
};

struct vertex_normal {
  float x;
  float y;
  float z;
};

// Faces

struct face {
  int count;
  struct vertex *vs;
  struct vertex_texture *vts;
  struct vertex_normal *vns;
};

// Dynamic array

struct dyn_array {
  long size;
  long increment;
  long count;
  void **items;
};

// Internel representation of an .obj file

struct obj_data {
  struct dyn_array *vs;
  struct dyn_array *vts;
  struct dyn_array *vns;
  struct dyn_array *faces;
};

// Function prototypes
int dyn_array_init(struct dyn_array *arr, long size);
int dyn_array_append(struct dyn_array *arr, void *item);
int dyn_array_trim(struct dyn_array *arr);
void *dyn_array_get(struct dyn_array *arr, long index);

char *next_line(char *bptr);

struct obj_data *load_obj_file(char *filename);
#endif
