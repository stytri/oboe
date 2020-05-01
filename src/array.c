/*
MIT License

Copyright (c) 2019 Tristan Styles

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include "array.h"
#include "bitmac.h"
#include "bits.h"
#include "gc.h"

//------------------------------------------------------------------------------

struct node {
	uint64_t  map;
	uintptr_t ptr[];
};

enum {
	BITS_PER_HASH = 64,
	BITS_PER_NODE = 6,
	NODE_BIT_MASK = (1U << BITS_PER_NODE) - 1,
};

enum {
	LEAF_MAX_SIZE = INT_MAX / 2
};

static inline bool
node_is_leaf(
	uintptr_t node
) {
	return isnot_tagged(node);
}

static inline bool
is_leaf_last_entry(
	uintptr_t node
) {
	return isnot_tagged(node);
}

static inline size_t
to_index(
	uintptr_t p
) {
	return (size_t)detag(p);
}

static inline uintptr_t
to_pointer(
	uintptr_t i
) {
	return entag(i);
}

static void
node_free(
	struct node *node,
	bool         is_leaf
) {
	if(!is_leaf) for(int n = popcount64(node->map); n-- > 0; ) {
		node_free(untag_pointer(node->ptr[n]), node_is_leaf(node->ptr[n]));
	}

	free(node);
	return;
}

//------------------------------------------------------------------------------

bool
array_expand(
	Array arr,
	size_t size,
	size_t count
) {
	size_t const buf_max = BIT_ROUND(SIZE_MAX/2) / size;

	if(arr && ((buf_max - arr->capacity) >= count)) {
		count = capacity_of(arr->length + count);

		if(buf_max >= count) {
			size *= count;

			void *p = realloc(arr->base, size);
			if(p) {
				arr->base     = p;
				arr->capacity = count;
				return true;
			}
		}
	}

	return false;
}

void
array_clear(
	Array arr
) {
	if(arr) {
		arr->length = 0;
	}
}

void
array_free(
	Array arr
) {
	if(arr) {
		if(arr->map != (uintptr_t)NULL) {
			node_free(untag_pointer(arr->map), node_is_leaf(arr->map));

			arr->map = (uintptr_t)NULL;
		}

		free(arr->base);

		arr->capacity = 0;
		arr->length   = 0;
		arr->base     = NULL;

	}
}

//------------------------------------------------------------------------------

size_t
array_map_index(
	Array       arr,
	uint64_t    hash,
	size_t      index
) {
	uintptr_t uip = to_pointer(index);
	if(to_index(uip) != index) {
		return ~(size_t)0;
	}

	if(arr->map != (uintptr_t)NULL) {
		bool         is_leaf =  node_is_leaf(arr->map);
		struct node *node    =  untag_pointer(arr->map);
		uintptr_t   *here    = &arr->map;

		for(int o = 0; o += BITS_PER_NODE; ) {
			struct node *leaf;
			size_t       z;

			if(is_leaf) {
				leaf = node;

				if(leaf->map == hash) {
					// we have found our leaf node
					// so we need to append an entry,
					// unless it already exists
					int i;

					for(i = 0; i < LEAF_MAX_SIZE; ++i) {
						if(untag(leaf->ptr[i]) == uip) {
							// already entered
							return index;
						}

						if(is_leaf_last_entry(leaf->ptr[i])) {
							// we want to insert after the last entry
							++i;
							break;
						}
					}
					if(i == LEAF_MAX_SIZE) {
						return ~(size_t)0;
					}

					if(is_at_capacity(i)) {
						// expand if filled to capacity
						z    = sizeof(struct node) + ((i * 2) * sizeof(uintptr_t));
						leaf = realloc(leaf, z);
						if(!leaf) {
							return ~(size_t)0;
						}
						// ensure pointer to here is updated
						*here = (uintptr_t)leaf;
					}
					leaf->ptr[i-1] = tag(leaf->ptr[i-1]); // this entry is no longer the last entry
					leaf->ptr[i]   = uip;

					return index;
				}

				// create a new branch node
				// with 1 entry
				z    = sizeof(struct node) + sizeof(uintptr_t);
				node = malloc(z);
				if(!node) {
					return ~(size_t)0;
				}

				// move this leaf node down a level

				int      const i = (leaf->map >> o) & NODE_BIT_MASK;
				uint64_t const b = UINT64_C(1) << i;

				node->map    = b;
				node->ptr[0] = (uintptr_t)leaf;

				// ensure pointer to here is updated and tagged as a branch node
				*here   =  tag_pointer(node);

			}	// if(is_leaf)

			int      const i = (hash >> o) & NODE_BIT_MASK;
			uint64_t const b = UINT64_C(1) << i;
			uint64_t const x = node->map & (b - 1);
			int      const j = popcount64(x);

			if(!(node->map & b)) {
				// unoccupied slot

				int k = popcount64(node->map);
				if(is_at_capacity(k)) {
					// expand node if filled to capacity
					z    = sizeof(struct node) + ((k * 2) * sizeof(uintptr_t));
					node = realloc(node, z);
					if(!node) {
						return ~(size_t)0;
					}
					// ensure pointer to here is updated and tagged as a branch node
					*here = tag_pointer(node);
				}

				// create new leaf node
				// with 1 entry
				z    = sizeof(struct node) + sizeof(uintptr_t);
				leaf = malloc(z);
				if(!leaf) {
					return ~(size_t)0;
				}

				leaf->map    = hash;
				leaf->ptr[0] = uip;

				// make room at the insertion point
				// by shifting entries up one
				for(; k > j; --k) {
					node->ptr[k] = node->ptr[k-1];
				}

				// and insert
				node->map   |= b;
				node->ptr[j] = (uintptr_t)leaf;

				return index;
			}

			here    = &node->ptr[j];
			is_leaf =  node_is_leaf(node->ptr[j]);
			node    =  untag_pointer(node->ptr[j]);

		}	// for

	} else {
		// no entries
		// so create an initial leaf node

		size_t       z    = sizeof(struct node) + sizeof(uintptr_t);
		struct node *leaf = malloc(z);
		if(leaf) {
			leaf->map    = hash;
			leaf->ptr[0] = uip;
			arr->map     = (uintptr_t)leaf;

			return index;
		}
	}

	return ~(size_t)0;
}

size_t
array_get_index(
	Array       arr,
	uint64_t    hash,
	int       (*cmp)(
		Array       arr,
		size_t      index,
		void const *key,
		size_t      n
	),
	void const *key,
	size_t      n
) {
	if(arr->map != (uintptr_t)NULL) {
		bool               is_leaf = node_is_leaf(arr->map);
		struct node const *node    = untag_pointer(arr->map);

		for(int o = 0; o += BITS_PER_NODE; ) {
			if(is_leaf) {
				if(node->map == hash) {
					for(int i = 0; i < LEAF_MAX_SIZE; ++i) {

						size_t const index = to_index(node->ptr[i]);
						if(cmp(arr, index, key, n) == 0) {
							return index;
						}

						if(is_leaf_last_entry(node->ptr[i])) {
							break;
						}
					}
				}
				break;
			}

			int      const i = (hash >> o) & NODE_BIT_MASK;
			uint64_t const b = UINT64_C(1) << i;
			uint64_t const x = node->map & (b - 1);
			int      const j = popcount64(x);

			if(!(node->map & b)) {
				break;
			}

			is_leaf = node_is_leaf(node->ptr[j]);
			node    = untag_pointer(node->ptr[j]);

		}	// for
	}

	return ~(size_t)0;
}

