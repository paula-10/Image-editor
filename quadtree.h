/* RUSEN Paula - 312CD */
#ifndef QUAD_TREE_H
#define QUAD_TREE_H

typedef struct image {
	int width;
	int height;
	unsigned char *data;
} *image_t;

typedef struct subimage {
	int x;
	int y;
	int size;
} subimage_t;

typedef struct quadtree {
	int factor;
	struct image *image;
	struct quadtreenode *root;
} *quadtree_t;

typedef struct quadtreenode {
	struct subimage sub_image;			// declaratia unui bloc
	struct quadtreenode *children[4];		// declaratia vectorului de fii
} *quadtreenode_t;

#endif  // QUAD_TREE_H

