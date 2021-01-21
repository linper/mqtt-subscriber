#include "glist.h"

struct glist{
	void **array;
	size_t count;
	size_t cap;
	size_t min_cap;
	float shrink_threshold;
	void (*free_cb)(void*);
};

//cap must be power of 2 and highier than 0, if not defaults to 16;
struct glist *new_glist(int cap)
{
	if (cap < 1 || cap % 2 == 1)
		cap = 16;
	struct glist *lst = (struct glist*)malloc(sizeof(struct glist));
	lst->count = 0;
	lst->cap = cap;
	lst->min_cap = cap;
	lst->shrink_threshold = 0.25;
	void **array = (void**)malloc(sizeof(void*)*cap);
	if (array == NULL)
		return NULL;
	lst->array = array;
	lst->free_cb = NULL;
	return lst;
}

void clear_glist(struct glist *lst)
{
	for (size_t i = 0; i < lst->count; i++){
		if (lst->free_cb != NULL)
			lst->free_cb(lst->array[i]);
		else
			free(lst->array[i]);
	}
	lst->count = 0;
}

void free_glist(struct glist *lst){
	if (lst != NULL){
		clear_glist(lst);
		free(lst->array);
		free(lst);
	}
}

void free_shallow_glist(struct glist *lst)
{
	if (lst != NULL){
		free(lst->array);
		free(lst);
	}
}

int __extend_glist(struct glist *lst){
	void **new_array = (void**)malloc(sizeof(void*)  *lst->cap*2);
	if (errno == ENOMEM)
		return -1;
	memcpy(new_array, lst->array, sizeof(void*) * lst->cap);
	free(lst->array);
	lst->array = new_array;
	lst->cap *= 2;
	return 0;
}

int __shrink_glist(struct glist *lst)
{
	if (lst->cap > 1){
		void **new_array = (void**)malloc(sizeof(void*)  *lst->cap / 2);
		if (errno == ENOMEM)
			return -1;
		memcpy(new_array, lst->array, sizeof(void*) * lst->cap / 2);
		free(lst->array);
		lst->array = new_array;
		lst->cap /= 2;
		return 0;
	}
	return -1;
}

int push_glist(struct glist *lst, void *value)
{
	if (lst->count == lst->cap && __extend_glist(lst) != 0)
		return -1;
	lst->array[lst->count++] = value;
	return 0;
}

int put_glist(struct glist *lst, void *value, size_t index)
{
	if (lst->count == lst->cap && __extend_glist(lst) != 0)
		return -1;
	if (index > lst->count)
		return -1;
	lst->array[index] = value;
	lst->count++;
	return 0;
}

void *pop_glist(struct glist *lst)
{
	void *value = lst->array[--lst->count];
	if (lst->count <= (float)lst->cap * lst->shrink_threshold && \
			lst->count > lst->min_cap && __shrink_glist(lst) != 0){
		errno = ENOMEM;
		return NULL;
	}
	return value;
}

void *get_glist(struct glist *lst, size_t index)
{
	if (index >= lst->count)
		return NULL;
	void *value = lst->array[index];
	return value;
}

void *remove_glist(struct glist *lst, size_t index)
{
	if (index > lst->count)
		return NULL;
	void *value = lst->array[index];
	for (size_t i = index; i < lst->count-1; i++)
		*(lst->array+i) = *(lst->array+i+1);
	lst->count--;
	if (lst->count <= (float)lst->cap * lst->shrink_threshold && \
			lst->count > lst->min_cap && __shrink_glist(lst) != 0)
		return NULL;
	return value;
}

int delete_glist(struct glist *lst, size_t index)
{
	if (index > lst->count)
		return -1;
	if (lst->free_cb != NULL)
		lst->free_cb(lst->array[index]);
	else
		free(lst->array[index]);
	for (size_t i = index; i < lst->count-1; i++)
		*(lst->array+i) = *(lst->array+i+1);
	lst->count--;
	if (lst->count <= (float)lst->cap * lst->shrink_threshold && \
			lst->count > lst->min_cap && __shrink_glist(lst) != 0)
		return -1;
	return 0;
}

size_t count_glist(struct glist *lst)
{
	return lst->count;
}

void **get_array_glist(struct glist *lst)
{
	return lst->array;
}

void set_free_cb_glist(struct glist *lst, void (*cb)(void*))
{
	if (lst)
		lst->free_cb = cb;
}