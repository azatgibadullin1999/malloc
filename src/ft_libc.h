/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_libc.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: larlena <larlena@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/05/27 15:40:48 by larlena           #+#    #+#             */
/*   Updated: 2023/06/17 12:36:57 by larlena          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MALLOC_FT_LIBC_H__
# define MALLOC_FT_LIBC_H__

# include <stdbool.h>
# include <stdio.h>
# include <sys/mman.h>
# include <sys/resource.h>
# include <unistd.h>

# define TINY_HEAP_ALLOCATION_SIZE ((size_t)(4 * getpagesize()))
# define TINY_BLOCK_SIZE ((size_t)(TINY_HEAP_ALLOCATION_SIZE / 128))
# define SMALL_HEAP_ALLOCATION_SIZE ((size_t)(16 * getpagesize()))
# define SMALL_BLOCK_SIZE ((size_t)(SMALL_HEAP_ALLOCATION_SIZE / 128))

# define HEAP_SHIFT(start) ((void *)start + sizeof(t_heap_head))
# define BLOCK_SHIFT(start) ((void *)start + sizeof(t_block_head))

# define BLOCK_FREE 0x1
# define BLOCK_OCCUPIED 0x0
# define BLOCK_FLAG_OFFSET 3

# define __BLOCK_METADATA_SIZE__ \
	((size_t)(sizeof(t_block_head) + sizeof(t_block_tail)))
# define __HEAP_METADATA_SIZE__ \
	((size_t)(sizeof(t_heap_head) + 3 * __BLOCK_METADATA_SIZE__))
# define __MIN_USER_DATA__ \
	((size_t)(0x1 << BLOCK_FLAG_OFFSET))
# define __MIN_BLOCK_SIZE__ \
	((size_t)(__BLOCK_METADATA_SIZE__ + __MIN_USER_DATA__))

# define TINY 0x01
# define SMALL 0x02
# define LARGE 0x04

typedef struct s_block_head {
	struct s_block_head *prev;
	struct s_block_head *next;
	size_t data;
} t_block_head;

typedef struct s_block_tail {
	size_t data;
} t_block_tail;

typedef struct s_heap_head {
	struct s_heap_head *prev;
	struct s_heap_head *next;
	size_t total_size;
	size_t flags;
} t_heap_head;

extern t_heap_head *g_heap;

void *malloc(size_t size);

#endif	// MALLOC_FT_LIBC_H__
