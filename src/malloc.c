/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: larlena <larlena@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/05/27 14:35:58 by larlena           #+#    #+#             */
/*   Updated: 2023/06/20 19:17:14 by larlena          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_libc.h"

t_heap_head *g_heap = NULL;

inline static bool is_meta_block__(size_t data);
inline static bool is_free_block__(size_t data);
inline static void *get_free_block__(size_t size);
inline static void mark_block_as_free__(size_t *data);
inline static void mark_block_as_occupied__(size_t *data);
inline static t_block_head *get_next_block__(t_block_head *block);
inline static t_block_head *get_prev_block__(t_block_head *block);
inline static t_block_tail *get_current_block_tail__(t_block_head *block);
static void free_heap__(t_heap_head *heap);
static t_block_head *merge_adjacent_free_blocks__(t_block_head *block);
static void *get_first_free_block__(t_block_head *block,
				    t_block_head *(*iterate)(t_block_head *));

void *malloc(size_t size) {
	t_block_head *dst;

	if ((size == 0) || ((dst = get_free_block__(size)) == NULL)) {
		return NULL;
	}
	mark_block_as_occupied__(&dst->data);
	dst->next->prev = dst->prev;
	dst->prev->next = dst->next;
	get_current_block_tail__(dst)->data = dst->data;
	return BLOCK_SHIFT(dst);
}

void free(void *ptr) {
	t_block_head *block = ptr - sizeof(t_block_head);

	if (ptr == NULL) {
		return;
	}
	mark_block_as_free__(&block->data);
	block = merge_adjacent_free_blocks__(block);
	get_current_block_tail__(block)->data = block->data;
	block->next = get_first_free_block__(block, get_next_block__);
	block->prev = get_first_free_block__(block, get_prev_block__);
	block->prev->next = block;
	block->next->prev = block;
	if (is_meta_block__(get_next_block__(block)->data) &&
	    is_meta_block__(get_prev_block__(block)->data)) {
		free_heap__((void *)block->prev - sizeof(t_heap_head));
	}
}

/*
============================
	SHARED UTILS
============================
*/

inline static bool is_tiny_heap__(unsigned char flag) {
	return flag & TINY;
}

inline static bool is_small_heap__(unsigned char flag) {
	return flag & SMALL;
}

inline static bool is_large_heap__(unsigned char flag) {
	return flag & LARGE;
}

inline static bool is_tiny_block__(size_t size) {
	return size <= TINY_BLOCK_SIZE;
}

inline static bool is_small_block__(size_t size) {
	return size <= SMALL_BLOCK_SIZE;
}

inline static bool is_large_block__(size_t size) {
	return size > SMALL_BLOCK_SIZE;
}

inline static bool is_free_block__(size_t data) {
	return data & BLOCK_FREE;
}

inline static void mark_block_as_free__(size_t *data) {
	*data |= BLOCK_FREE;
}

inline static void mark_block_as_occupied__(size_t *data) {
	*data &= (__SIZE_MAX__ << 0x1);
}

inline static size_t round_size__(size_t size) {
	return (size & (__SIZE_MAX__ << BLOCK_FLAG_OFFSET)) +
	       (1 << BLOCK_FLAG_OFFSET);
}

inline static void set_block_size__(size_t *data, size_t size) {
	*data =
	    (size & (__SIZE_MAX__ << BLOCK_FLAG_OFFSET)) | (*data & BLOCK_FREE);
}

inline static size_t get_block_size__(size_t data) {
	return data & (__SIZE_MAX__ << BLOCK_FLAG_OFFSET);
}

inline static t_block_tail *get_current_block_tail__(t_block_head *block) {
	return (void *)block + get_block_size__(block->data) +
	       sizeof(t_block_head);
}

inline static t_block_tail *get_previous_block_tail__(t_block_head *block) {
	return (void *)block - sizeof(t_block_tail);
}

inline static t_block_head *get_next_block__(t_block_head *block) {
	return (void *)block +
	       (__BLOCK_METADATA_SIZE__ + get_block_size__(block->data));
}

inline static t_block_head *get_prev_block__(t_block_head *block) {
	return (void *)block -
	       (__BLOCK_METADATA_SIZE__ +
		get_block_size__(get_previous_block_tail__(block)->data));
}

static size_t get_size_of_heap__(size_t block_size, unsigned char block_type) {
	if (is_tiny_heap__(block_type)) {
		return TINY_HEAP_ALLOCATION_SIZE;
	} else if (is_small_heap__(block_type)) {
		return SMALL_HEAP_ALLOCATION_SIZE;
	} else {
		return block_size + __HEAP_METADATA_SIZE__;
	}
}

static unsigned char get_type_of_heap__(size_t block_size) {
	if (is_tiny_block__(block_size)) {
		return TINY;
	} else if (is_small_block__(block_size)) {
		return SMALL;
	} else {
		return LARGE;
	}
}

static void init_block__(t_block_head *block, size_t total_size) {
	block->prev = NULL;
	block->next = NULL;
	block->data = BLOCK_FREE;
	set_block_size__(&block->data, total_size);
	get_current_block_tail__(block)->data = block->data;
}

static void init_primary_block__(t_heap_head *heap, size_t heap_size) {
	size_t size = heap_size - sizeof(t_heap_head);
	t_block_head *first = HEAP_SHIFT(heap);
	t_block_head *last = (void *)first + size - __BLOCK_METADATA_SIZE__;
	t_block_head *block = (void *)first + __BLOCK_METADATA_SIZE__;

	init_block__(first, 0);
	first->data = BLOCK_OCCUPIED;
	init_block__(last, 0);
	last->data = BLOCK_OCCUPIED;
	init_block__(block, ((void *)last - (void *)first) -
				2 * __BLOCK_METADATA_SIZE__);
	first->next = block;
	first->prev = last;
	last->next = first;
	last->prev = block;
	block->next = last;
	block->prev = first;
}

/*
================================
	UTILS FOR MALLOC
================================
*/

static void *init_heap__(t_heap_head *heap, size_t size, size_t heap_type) {
	heap->total_size = size;
	heap->flags = heap_type;
	heap->next = g_heap;
	g_heap = heap;
	if (heap->next != NULL) {
		heap->next->prev = heap;
	}
	return heap;
}

static void *create_heap__(size_t size, size_t heap_type) {
	t_heap_head *new_heap;
	size_t heap_size = get_size_of_heap__(size, heap_type);

	if ((new_heap = mmap(NULL, heap_size, PROT_READ | PROT_WRITE,
			     MAP_PRIVATE | MAP_ANON, -1, 0)) == NULL) {
		return new_heap;
	}
	init_heap__(new_heap, heap_size, heap_type);
	init_primary_block__(new_heap, heap_size);
	return new_heap;
}

static void *find_free_heap__(t_heap_head *heap, size_t heap_type) {
	while (heap) {
		if (heap->flags == heap_type) {
			return heap;
		}
		heap = heap->next;
	}
	return heap;
}

static void *find_free_block__(t_heap_head *heap, size_t size) {
	t_block_head *it = ((t_block_head *)HEAP_SHIFT(heap))->next;
	t_block_head *buff = it;
	size_t it_size = get_block_size__(it->data);
	size_t buff_size = it_size;

	while (it_size) {
		if (((buff_size > it_size) && (it_size >= size)) &&
		    is_free_block__(it->data)) {
			buff = it;
			buff_size = get_block_size__(buff->data);
		}
		it = it->next;
		it_size = get_block_size__(it->data);
	}
	return get_block_size__(buff->data) < size ? NULL : buff;
}

static void trim_block__(t_block_head *block, size_t size) {
	size_t block_size = get_block_size__(block->data);
	t_block_head *buff;

	if (block_size - size >= __MIN_BLOCK_SIZE__) {
		buff = (void *)block + size + __BLOCK_METADATA_SIZE__;
		init_block__(buff, block_size - size - __BLOCK_METADATA_SIZE__);
		buff->next = block->next;
		buff->prev = block;
		block->next = buff;
		buff->next->prev = buff;
		set_block_size__(&block->data, size);
		get_current_block_tail__(block)->data = block->data;
	}
}

static void *get_free_block__(size_t input_size) {
	t_heap_head *heap = g_heap;
	t_block_head *block;
	size_t size = round_size__(input_size);
	size_t heap_type = get_type_of_heap__(size);

	while (true) {
		if ((heap = find_free_heap__(heap, heap_type)) == NULL) {
			if ((heap = create_heap__(size, heap_type)) == NULL) {
				return NULL;
			}
		}
		if ((block = find_free_block__(heap, size)) == NULL) {
			heap = heap->next;
		} else {
			break;
		}
	}
	trim_block__(block, size);
	return block;
}

/*
==============================
	UTILS FOR FREE
==============================
*/

inline static bool is_meta_block__(size_t data) { return data == 0; }

static void free_heap__(t_heap_head *heap) {
	if (heap->next) {
		heap->next->prev = heap->prev;
	}
	if (heap->prev) {
		heap->prev->next = heap->next;
	}
	if (heap == g_heap) {
		g_heap = heap->next;
	}
	munmap(heap, heap->total_size);
}

static void *get_first_free_block__(t_block_head *block,
				    t_block_head *(*iterate)(t_block_head *)) {
	block = iterate(block);
	while (block && (!is_free_block__(block->data) &&
			 !is_meta_block__(block->data))) {
		block = iterate(block);
	}
	return block;
}

static void merge_blocks__(t_block_head *first, t_block_head *second) {
	set_block_size__(&first->data, __BLOCK_METADATA_SIZE__ +
					   get_block_size__(first->data) +
					   get_block_size__(second->data));
	first->next = second->next;
	get_current_block_tail__(first)->data = first->data;
}

static t_block_head *merge_adjacent_free_blocks__(t_block_head *block) {
	t_block_head *buff;

	buff = get_next_block__(block);
	if (is_free_block__(buff->data)) {
		merge_blocks__(block, buff);
	}
	buff = get_prev_block__(block);
	if (is_free_block__(buff->data)) {
		merge_blocks__(buff, block);
		block = buff;
	}
	return block;
}
