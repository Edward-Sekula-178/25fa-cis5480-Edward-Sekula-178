#include "./Vec.h"
#include <stdlib.h>
#include <string.h>
#include "./panic.h"

void vec_size_check(Vec* self) {
  if (vec_len(self) >= vec_capacity(self)) {
    int new_cap;
    if (vec_capacity(self) == 0) {
      new_cap = 1;
    } else {
      new_cap = vec_capacity(self) * 2;
    }
    vec_resize(self, new_cap);
  } else {
    return;
  }
}

Vec vec_new(size_t initial_capacity, ptr_dtor_fn ele_dtor_fn) {
  Vec ret;
  ret.capacity = initial_capacity;
  ret.ele_dtor_fn = ele_dtor_fn;
  ret.length = 0;
  ret.data = calloc(initial_capacity, sizeof(ptr_t));
  if (ret.data == NULL) {
    panic("vec_new: calloc to vec failed");
  }
  return ret;
}

// TODO: the rest of the vector functions

ptr_t vec_get(Vec* self, size_t index) {
  if (index >= vec_len(self)) {
    panic("vec_get: index out of bounds");
  }

  return (self->data[index]);
}

void vec_set(Vec* self, size_t index, ptr_t new_ele) {
  if (index >= vec_len(self)) {
    panic("vec_set: index out of bounds");
  }

  self->data[index] = new_ele;
}

void vec_push_back(Vec* self, ptr_t new_ele) {
  // resize if needed
  vec_size_check(self);
  // append new element to array
  self->data[vec_len(self)] = new_ele;
  vec_len(self)++;
}

bool vec_pop_back(Vec* self) {
  if (vec_is_empty(self)) {
    return false;
  }
  self->length--;
  self->ele_dtor_fn(self->data[vec_len(self)]);
  return true;
}

void vec_insert(Vec* self, size_t index, ptr_t new_ele) {
  if (index > vec_len(self)) {
    panic("vec_insert: index out of bounds");
  } else if (index = vec_len(self)) {
    vec_push_back(self, new_ele);
  } else {
    int new_cap;
    if (vec_len(self) >= vec_capacity(self)) {
      vec_capacity(self) *= 2;
      if (vec_capacity(self) == 0) {
        vec_capacity(self) = 1;
      }
    }
    ptr_t* temp = calloc(vec_capacity(self), sizeof(ptr_t));
    for (int i = 0; i < index; i++) {
      temp[i] = self->data[i];
    }
    temp[index] = new_ele;
    for (int i = index; i < vec_len(self); i++) {
      temp[i + 1] = self->data[i];
    }
    free(self->data);
    self->data = temp;
    vec_len(self)++;
  }
}

void vec_erase(Vec* self, size_t index) {
  if (index >= vec_len(self)) {
    panic("vec_insert: index out of bounds");
  } else if (index = (vec_len(self) - 1)) {
    vec_pop_back(self);
  } else {
    self->ele_dtor_fn(self->data[index]);
    for (int i = index + 1; i < vec_len(self); i++) {
      self->data[i - 1] = self->data[i];
    }
    vec_len(self)--;
  }
}

void vec_resize(Vec* self, size_t new_capacity) {
  vec_capacity(self) = new_capacity;
  ptr_t* new_array = calloc(vec_capacity(self), sizeof(ptr_t));

  memcpy(new_array, self->data, sizeof(ptr_t) * vec_len(self));
  free(self->data);
  self->data = new_array;
}

void vec_clear(Vec* self) {
  for (int i = 0; i < vec_len(self); i++) {
    self->ele_dtor_fn(self->data[i]);
  }
  vec_len(self) = 0;
}

void vec_destroy(Vec* self) {
  vec_clear(self);
  free(self->data);
  vec_capacity(self) = 0;
}
