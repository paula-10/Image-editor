/* RUSEN Paula - 312CD */
#include <string.h>

#include "queue.h"
#define ERROR -1

image_t readPPM(FILE* input) {
	// read file's type
	char magic_number[3] = {0}, dummy;

	// read magic number
	fread(magic_number, sizeof(char), 2, input);
	if (strcmp(magic_number, "P6")) return NULL;
	fread(&dummy, sizeof(char), 1, input);

	// allocate image and read width and height
	image_t image = malloc(sizeof(struct image));
	if (image == NULL) return NULL;
	fscanf(input, "%d %d", &image->width, &image->height);
	fread(&dummy, sizeof(char), 1, input);

	// read maxval
	int maxval;
	fscanf(input, "%d", &maxval);
	fread(&dummy, sizeof(char), 1, input);

	// allocate image buffer data and read it
	image->data =
    	malloc(sizeof(unsigned char) * image->height * image->width * 3);
	if (image->data == NULL) {
    free(image);
    return NULL;
	}
	fread(image->data, sizeof(unsigned char), image->height * image->width * 3,
		input);
	return image;
}

void writePPM(image_t image, FILE* output) {
	fprintf(output, "%s\n%d %d\n%d\n", "P6", image->width, image->height, 255);
	fwrite(image->data, sizeof(unsigned char), image->height * image->width * 3,
        output);
}

int calculateMean(image_t image, int x, int y, int size) {
	long long red = 0, green = 0, blue = 0;
	for (int i = x; i < x + size; i++) {
		for (int j = y; j < y + size; j++) {
			red += image->data[3 * (i * image->width + j) + 0];
			green += image->data[3 * (i * image->width + j) + 1];
			blue += image->data[3 * (i * image->width + j) + 2];
		}
	}
	red /= size * size;
	green /= size * size;
	blue /= size * size;

	float mean = 0;
	for (int i = x; i < x + size; i++) {
		for (int j = y; j < y + size; j++) {
			mean += (red - image->data[3 * (i * image->width + j) + 0]) *
            	(red - image->data[3 * (i * image->width + j) + 0]);
			mean += (green - image->data[3 * (i * image->width + j) + 1]) *
            	(green - image->data[3 * (i * image->width + j) + 1]);
			mean += (blue - image->data[3 * (i * image->width + j) + 2]) *
            	(blue - image->data[3 * (i * image->width + j) + 2]);
    	}
	}
  	mean /= 3.f * size * size;
	return (int)mean;
}

quadtreenode_t initQuadTreeNode(int x, int y, int size) {
	quadtreenode_t quadtreenode = malloc(sizeof(struct quadtreenode));
	if (quadtreenode == NULL) return NULL;

	for (int i = 0; i < 4; i++) quadtreenode->children[i] = NULL;
	quadtreenode->sub_image.size = size;
	quadtreenode->sub_image.x = x;
	quadtreenode->sub_image.y = y;

	return quadtreenode;
}

quadtree_t initQuadTree(image_t image, int factor) {
	quadtree_t quadtree = malloc(sizeof(struct quadtree));
	if (quadtree == NULL) return NULL;
	quadtree->image = image;
	quadtree->factor = factor;

	// create root node with whole image window
	quadtree->root = initQuadTreeNode(0, 0, image->height);
	if (quadtree->root == NULL) {
		free(quadtree);
		return NULL;
	}

	return quadtree;
}

void freeQuadTreeNodes(quadtreenode_t root) {
	if (root == NULL) return;

	for (int i = 0; i < 4; i++) {
    	freeQuadTreeNodes(root->children[i]);
		free(root->children[i]);
	}
}

void freeQuadTree(quadtree_t quadtree, int freeImage) {
	freeQuadTreeNodes(quadtree->root);
	free(quadtree->root);
	if (freeImage) {
		free(quadtree->image->data);
		free(quadtree->image);
	}
	free(quadtree);
}

void divideQuadTree(quadtree_t quadtree, quadtreenode_t root) {
	if (root->sub_image.size == 1) return;
	if (calculateMean(quadtree->image, root->sub_image.x, root->sub_image.y,
                    root->sub_image.size) <= quadtree->factor)
    return;

	// divide size by 2 for children nodes
	int size = root->sub_image.size / 2;
	for (int i = 0; i < 4; i++) {
    	int x = root->sub_image.x, y = root->sub_image.y;

    	// calculate window top-left corner coordinates in inverse
    	// trigonometric order
    	if (i == 1 || i == 2) y += size;
    	if (i == 2 || i == 3) x += size;

    	root->children[i] = initQuadTreeNode(x, y, size);
		divideQuadTree(quadtree, root->children[i]);
	}
}

int getQuadTreeDepth(quadtreenode_t root) {
	if (root == NULL) return 0;

	int max_depth = 0;
	for (int i = 0; i < 4; i++) {
    	int depth = getQuadTreeDepth(root->children[i]);
    	if (depth > max_depth) max_depth = depth;
	}
	return max_depth + 1;
}

int getQuadTreeLeavesNR(quadtreenode_t root) {
	if (root == NULL) return 0;

	if (root->children[0] == NULL) return 1;

	int count = 0;
	for (int i = 0; i < 4; i++) {
		count += getQuadTreeLeavesNR(root->children[i]);
	}
	return count;
}

int getQuadTreeMaxUndivided(quadtreenode_t root) {
	if (root == NULL) return 0;
	if (root->children[0] == NULL) return root->sub_image.size;

	int max_size = 0;
	for (int i = 0; i < 4; i++) {
		int size = getQuadTreeMaxUndivided(root->children[i]);
		if (size > max_size) max_size = size;
	}
	return max_size;
}

void compressQuadTree(quadtree_t quadtree, FILE* output) {
	if (quadtree->root == NULL) return;
	Queue* queue = createQueue();
	enqueue(queue, quadtree->root);
	while (!isQueueEmpty(queue)) {
		quadtreenode_t curr = front(queue);
		dequeue(queue);

		unsigned char type;
		// if leaf node
		if (curr->children[0] == NULL) {
			type = 1;

			// calculate red, green and blue color mean in node window
			unsigned int red_mean = 0, green_mean = 0, blue_mean = 0;
			int x = curr->sub_image.x;
			int y = curr->sub_image.y;
			int size = curr->sub_image.size;
			for (int i = x; i < x + size; i++) {
				for (int j = y; j < y + size; j++) {
					red_mean +=
			quadtree->image->data[3 * (i * quadtree->image->width + j) + 0];
					green_mean +=
			quadtree->image->data[3 * (i * quadtree->image->width + j) + 1];
					blue_mean +=
			quadtree->image->data[3 * (i * quadtree->image->width + j) + 2];
				}
			}
			red_mean /= size * size;
			green_mean /= size * size;
			blue_mean /= size * size;
			unsigned char red = red_mean, green = green_mean, blue = blue_mean;

			fwrite(&type, sizeof(unsigned char), 1, output);
			fwrite(&red, sizeof(unsigned char), 1, output);
			fwrite(&green, sizeof(unsigned char), 1, output);
			fwrite(&blue, sizeof(unsigned char), 1, output);
		} else {
			type = 0;
			for (int i = 0; i < 4; i++) 
				enqueue(queue, curr->children[i]);
			fwrite(&type, sizeof(unsigned char), 1, output);
		}
	}
  	destroyQueue(queue);
}

image_t decompressQuadTree(FILE* input) {
	int size;
	fread(&size, sizeof(int), 1, input);
	image_t image = malloc(sizeof(struct image));
	image->height = image->width = size;
	image->data = malloc(sizeof(unsigned char) * size * size * 3);

	quadtree_t quadtree = initQuadTree(image, -1);

	Queue* queue = createQueue();
	enqueue(queue, quadtree->root);

	while (!isQueueEmpty(queue)) {
    	quadtreenode_t curr = front(queue);
		dequeue(queue);
		unsigned char type;
		fread(&type, sizeof(unsigned char), 1, input);
		if (type == 0) {
			int size = curr->sub_image.size / 2;
			for (int i = 0; i < 4; i++) {
			int x = curr->sub_image.x, y = curr->sub_image.y;
			if (i == 1 || i == 2) y += size;
			if (i == 2 || i == 3) x += size;
			curr->children[i] = initQuadTreeNode(x, y, size);

			enqueue(queue, curr->children[i]);
			}
		} else {
			unsigned char red, green, blue;
			fread(&red, sizeof(unsigned char), 1, input);
			fread(&green, sizeof(unsigned char), 1, input);
			fread(&blue, sizeof(unsigned char), 1, input);

			int x = curr->sub_image.x;
			int y = curr->sub_image.y;
			int size = curr->sub_image.size;

			for (int i = x; i < x + size; i++) {
				for (int j = y; j < y + size; j++) {
				image->data[3 * (i * quadtree->image->width + j) + 0] = red;
				image->data[3 * (i * quadtree->image->width + j) + 1] = green;
				image->data[3 * (i * quadtree->image->width + j) + 2] = blue;
				}
			}
		}
	}
	destroyQueue(queue);
	freeQuadTree(quadtree, 0);
	return image;
}

int main(int argc, char** argv) {
	if (argc <= 3) {
		printf("%s", "NOT ENOUGH ARGUMENTS!\n");
		return ERROR;
	}

	FILE *input, *output;

	if (!strcmp(argv[1], "-c1") || !strcmp(argv[1], "-c2")) {
		if (argc <= 4) {
			printf("%s", "NOT ENOUGH ARGUMENTS!\n");
			return ERROR;
		}

		input = fopen(argv[3], "rb");
		if (input == NULL) {
			printf("%s", "CANNOT OPEN INPUT FILE!\n");
			return ERROR;
		}
		if (!strcmp(argv[1], "-c1"))
			output = fopen(argv[4], "w");
		else
			output = fopen(argv[4], "wb");
		if (output == NULL) {
			printf("%s", "CANNOT OPEN OUTPUT FILE!\n");
			return ERROR;
		}

		image_t image = readPPM(input);
		if (image == NULL) {
			printf("%s", "ERROR READING PPM FORMAT\n");
			return ERROR;
		}
		fclose(input);

		int factor = atoi(argv[2]);
		quadtree_t quadtree = initQuadTree(image, factor);
		if (quadtree == NULL) {
			free(image->data);
			free(image);
			return ERROR;
		}
		
		// create compressed quadtree
		divideQuadTree(quadtree, quadtree->root);
    	if (!strcmp(argv[1], "-c1")) {
			int depth = getQuadTreeDepth(quadtree->root);
			int noLeaves = getQuadTreeLeavesNR(quadtree->root);
			int maxUndivided = getQuadTreeMaxUndivided(quadtree->root);
			fprintf(output, "%d\n%d\n%d\n", depth, noLeaves, maxUndivided);
		} else {
			fwrite(&quadtree->image->height, sizeof(int), 1, output);
			compressQuadTree(quadtree, output);
		}
		fclose(output);

		freeQuadTree(quadtree, 1);

	} else if (!strcmp(argv[1], "-d")) {
		input = fopen(argv[2], "rb");
		if (input == NULL) {
			printf("%s", "CANNOT OPEN INPUT FILE!\n");
			return ERROR;
		}
		output = fopen(argv[3], "wb");
		if (output == NULL) {
			printf("%s", "CANNOT OPEN OUTPUT FILE!\n");
			return ERROR;
		}
		image_t image = decompressQuadTree(input);
		writePPM(image, output);
		free(image->data);
		free(image);

		fclose(input);
		fclose(output);

	} else {
		printf("%s", "INVALID FIRST ARGUMENT!\n");
	}

	return 0;
}
