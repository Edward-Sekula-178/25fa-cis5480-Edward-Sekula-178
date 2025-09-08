#include "./Vec.h"
#include <stdlib.h>
#include "./panic.h"

Vec vec_new(size_t initial_capacity, ptr_dtor_fn ele_dtor_fn) {
  Vec new = {
      .data = malloc(sizeof(ptr_t) * (initial_capacity)),
      .length = 0,
      .capacity = initial_capacity,
      .ele_dtor_fn = ele_dtor_fn,
  };
  return new;
}

ptr_t vec_get(Vec* self, size_t index) {
  if (index >= vec_len(self)) {
    panic("vec_get: index out of range");
  }
  return self->data[index];
}

void vec_set(Vec* self, size_t index, ptr_t new_ele) {
  if (index >= vec_len(self)) {
    panic("vec_set: index out of range");
  }
  if (self->ele_dtor_fn != NULL) {
    self->ele_dtor_fn(self->data[index]);
  }
  self->data[index] = new_ele;
}

void vec_push_back(Vec* self, ptr_t new_ele) {
  if (vec_len(self) == vec_capacity(self)) {
    // double size of array
    if (vec_is_empty(self)) {
      vec_resize(self, 1);
    } else {
      vec_resize(self, 2 * vec_capacity(self));
    }
  }

  self->data[vec_len(self)] = new_ele;
  self->length += 1;
}

bool vec_pop_back(Vec* self) {
  if (vec_is_empty(self)) {
    return false;
  }
  if (self->ele_dtor_fn != NULL) {
    self->ele_dtor_fn(self->data[vec_len(self) - 1]);
  }
  vec_len(self) -= 1;
  return true;
}

void vec_insert(Vec* self, size_t index, ptr_t new_ele) {
  if (index > vec_len(self)) {
    panic("vec_insert: index out of bounds");
  }

  if (vec_len(self) == vec_capacity(self)) {
    // double size of array
    if (vec_is_empty(self)) {
      vec_resize(self, 1);
    } else {
      vec_resize(self, 2 * vec_capacity(self));
    }
  }

  if (index != vec_len(self)) {
    for (size_t i = vec_len(self); i > index; i--) {
      self->data[i] = self->data[i - 1];
    }
  }
  self->data[index] = new_ele;
  self->length += 1;
}

void vec_erase(Vec* self, size_t index) {
  if (index >= vec_len(self)) {
    panic("vec_erase: index out of bounds");
  }

  if (self->ele_dtor_fn != NULL) {
    self->ele_dtor_fn(self->data[index]);
  }

  for (size_t i = index; i < vec_len(self); i++) {
    self->data[i] = self->data[i + 1];
  }

  vec_len(self) -= 1;
}

void vec_resize(Vec* self, size_t new_capacity) {
  ptr_t* new_data = malloc(sizeof(ptr_t) * new_capacity);
  if (new_data == NULL) {
    panic("vec_resize: allocation failure");
  }
  for (size_t i = 0; i < MAX(vec_len(self), new_capacity); i++) {
    new_data[i] = self->data[i];
  }
  free(self->data);
  self->data = new_data;
  vec_capacity(self) = new_capacity;
}

void vec_clear(Vec* self) {
  if (self->ele_dtor_fn != NULL && self->data != NULL) {
    for (size_t i = 0; i < vec_len(self); i++) {
      self->ele_dtor_fn(self->data[i]);
    }
  }
  vec_len(self) = 0;
}
void vec_destroy(Vec* self) {
  vec_clear(self);
  free(self->data);
  self->data = NULL;
  vec_capacity(self) = 0;
  vec_len(self) = 0;
  self->ele_dtor_fn = NULL;
}
