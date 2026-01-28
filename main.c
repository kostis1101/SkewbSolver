
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <time.h>


#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


#define F  0
#define F_ 1
#define U  2
#define U_ 3
#define R  4
#define R_ 5
#define L  6
#define L_ 7


typedef uint8_t perm6_t[6];
typedef uint8_t perm4_t[4];


long long factorial(int x) {
	int result = 1;
	for (int a = 2; a <= x; a++)
		result *= a;
	return result;
}


/* returns a unique encoding with the maximum possible value being optimal for each even permutation */
/* the perm is a array of ints that stores the permutation. It holds the element at each index */
/* size is the total number of elements */
long long aux_encode_even_permutation(uint8_t *perm, int size) {
	if (size <= 2)
		return 0;

	for (int i = 1; i < size; i++) {
		if (perm[i] > perm[0]) {
			perm[i]--;
		}
	}

	return perm[0] * factorial(size - 1) / 2 + aux_encode_even_permutation(perm + 1, size - 1);
}


long long encode_even_permutation(uint8_t *perm, int size) {
	static uint8_t *copy = NULL;

	if (!copy)
		copy = malloc(size * sizeof(int));

	// copy = realloc(copy, size * sizeof(int));
	memcpy(copy, perm, size * sizeof(int));

	return aux_encode_even_permutation(copy, size);
}


#define perm6_id { 0, 1, 2, 3, 4, 5 }
#define perm4_id { 0, 1, 2, 3 }

#define WHITE  { 255, 255, 255 }
#define YELLOW { 255, 255,   0 }
#define RED    { 255,   0,   0 }
#define ORANGE { 255, 127,   0 } 
#define GREEN  {   0, 255,   0 }
#define BLUE   {   0,   0, 255 }


struct colour {
	uint8_t r, g, b;
};

void print_colour(struct colour c) {
	printf("C[ %d %d %d ]\n", c.r, c.g, c.b);
}

struct colour colours[8] = {
	WHITE, YELLOW, RED, ORANGE, GREEN, BLUE
};


// first three numbers are the indecies of the faces that each corner is at
// last three numbers are the indicies of the positions at each face
int corners_sides[8][6] = {
	{ 0, 2, 4, 3, 2, 1 },
	{ 0, 4, 3, 2, 0, 3 },
	{ 0, 3, 5, 0, 1, 2 },
	{ 0, 5, 2, 1, 3, 0 },

	{ 1, 2, 5, 0, 1, 1 },
	{ 1, 4, 2, 2, 3, 3 },
	{ 1, 3, 4, 3, 2, 2 },
	{ 1, 5, 3, 1, 0, 0 }
};


typedef struct skewb {
	/* number of twists we need to brint the white/yellow side up/down.
	   Indexed by position (not pieces) */
	uint8_t corners_orient[8]; 

	/* permunation of centers */
	perm6_t centers;

	/* positions of the free corners */
	perm4_t corner_pos; 
} skewb_pos;


/* a smaller representation of a skewb position equivalent to the above */
typedef struct skewb_red {
	/* only need to store the positions of 4 centers which requites 12 bits */
	uint16_t centers;

	/* only need to store the orientation of 7 corners, which requires 14 bits */
	uint16_t corners_orient;

	/* you just need one free corner position and the orientation of the fixed
	   corners to determin the position of all other free corners, which requires 3 bits */
	uint8_t corner_pos; 
} skewb_red_pos;


skewb_pos skewb_id = {
	.centers = perm6_id,
	.corners_orient = { 0 },
	.corner_pos = perm4_id
};


skewb_red_pos skewb_compress(skewb_pos *skb) {
	return (skewb_red_pos) {
		.centers = ((uint16_t)skb->centers[0] << 9) | ((uint16_t)skb->centers[1] << 6) |
				   ((uint16_t)skb->centers[2] << 3) | ((uint16_t)skb->centers[3]),
		.corners_orient =
				((uint16_t)skb->corners_orient[0] << 12) | ((uint16_t)skb->corners_orient[1] << 10) |
				((uint16_t)skb->corners_orient[2] <<  8) | ((uint16_t)skb->corners_orient[3] <<  6) |
				((uint16_t)skb->corners_orient[4] <<  4) | ((uint16_t)skb->corners_orient[5] << 2) |
				((uint16_t)skb->corners_orient[6]),
		.corner_pos = skb->corner_pos[0]
	};
}



#define SKEWB_CENTERS_COMB	360
#define SKEWB_ORIENT_COMB	2187
#define SKEWB_CORN_POS_COMB	4
#define SKEWB_TOTAL_COMB	(SKEWB_CENTERS_COMB * SKEWB_ORIENT_COMB * SKEWB_CORN_POS_COMB)

long long skewb_encode_centers(skewb_pos *skb) {
	return encode_even_permutation(skb->centers, 6);
}

long long skewb_encode_corners_orient(skewb_pos *skb) {
	uint8_t *cor = skb->corners_orient;
	return cor[0] + cor[1] * 3 + cor[2] * 9 + cor[3] * 27 +
		   cor[4] * 81 + cor[5] * 243 + cor[6] * 729;
}


/* "The orientations of the fixed corners and the position of one of the free corners will determine the positions of the other three (3)"
 * https://www.jaapsch.net/puzzles/skewb.htm */
long long skewb_encode_corner_pos(skewb_pos *skb) {
	return skb->corner_pos[0];
/*	if (skb->corner_pos[0] < skb->corner_pos[1])
		return skb->corner_pos[0] * 3 + (corner_pos[1] - 1);
	else
		return skb->corner_pos[0] * 3 + corner_pos[1]; */
}


/* returns a unique encoding for each skewb position optimally */
long long skewb_encode(skewb_pos *skb) {
	return skewb_encode_centers(skb) * SKEWB_ORIENT_COMB * SKEWB_CORN_POS_COMB +
		   skewb_encode_corners_orient(skb) * SKEWB_CORN_POS_COMB +
		   skewb_encode_corner_pos(skb);
}


void perm_cycle3(uint8_t *perm, int i1, int i2, int i3, int way) {
	uint8_t temp = perm[i1];
	if (way) {
		perm[i1] = perm[i3];
		perm[i3] = perm[i2];
		perm[i2] = temp;
	}
	else {
		perm[i1] = perm[i2];
		perm[i2] = perm[i3];
		perm[i3] = temp;
	}
}


/* indecies are the indecies of corner positions */
void rotate_free_corners(skewb_pos *skb, int c1, int c2, int c3, int way) {
	perm_cycle3(skb->corner_pos, c1 / 2, c2 / 2, c3 / 2, way);
	skb->corners_orient[c1] = (skb->corners_orient[c1] + 2 - way) % 3;
	skb->corners_orient[c2] = (skb->corners_orient[c2] + 2 - way) % 3;
	skb->corners_orient[c3] = (skb->corners_orient[c3] + 2 - way) % 3;
	perm_cycle3(skb->corners_orient, c1, c2, c3, way);
}


void move(skewb_pos *skb, int move) {
	
	int way = move & 1;
	uint8_t *cor = skb->corners_orient;
	
	int fixed_corner = (move >> 1) * 2;
	cor[fixed_corner] = (cor[fixed_corner] + 1 + way) % 3;

	switch (move >> 1) {
		case 0: // F
			perm_cycle3(skb->centers, 0, 4, 2, way);
			rotate_free_corners(skb, 1, 5, 3, way);
			break;
		case 1: // U
			perm_cycle3(skb->centers, 0, 5, 3, way);
			rotate_free_corners(skb, 1, 3, 7, way);
			break;
		case 2: // R
			perm_cycle3(skb->centers, 1, 5, 2, way);
			rotate_free_corners(skb, 3, 5, 7, way);
			break;
		case 3: // L
			perm_cycle3(skb->centers, 1, 4, 3, way);
			rotate_free_corners(skb, 1, 7, 5, way);
			break;
	}
}


void draw_center(uint8_t *image, int width, int x, int y, int s, struct colour c) {
	for (int xx = x; xx < x + s; xx++) {
		int h = xx - x;
		if (h > s / 2)
			h = s - h;
		for (int yy = y + s / 2 - h + 1; yy < y + s / 2 + h; yy++) {
			image[3 * (xx + yy * width) + 0] = c.r;
			image[3 * (xx + yy * width) + 1] = c.g;
			image[3 * (xx + yy * width) + 2] = c.b;
		}
	}
}



void draw_corner(uint8_t *image, int width, int sx, int sy, int s, int corn, struct colour c) {
	for (int xx = 0; xx < s/2; xx++) {
		int xpos = xx;
		if (corn & 1)
			xpos = s - xx;
		xpos += sx;
		for (int yy = 0; yy < s/2 - xx; yy++) {
			int ypos = yy;
			if (corn & 2)
				ypos = s - yy;
			ypos += sy;

			image[3 * (xpos + ypos * width) + 0] = c.r;
			image[3 * (xpos + ypos * width) + 1] = c.g;
			image[3 * (xpos + ypos * width) + 2] = c.b;
		}
	}
}

void do_moves(skewb_pos *skb, const char *moves) {
	int i = 0;

	/* skip spaces */
	for (; moves[i] && isspace(moves[i]); i++);

	while (moves[i]) {
		int is_prime = (moves[i + 1] == '\'');
		
		switch (moves[i]) {
			case 'F':
				move(skb, F + is_prime);
				break;
			case 'U':
				move(skb, U + is_prime);
				break;
			case 'R':
				move(skb, R + is_prime);
				break;
			case 'L':
				move(skb, L + is_prime);
				break;
			default:
				printf("Unknown move %c\n", moves[i]);
				exit(1);
		}

		i += is_prime + 1;

		for (; moves[i] && isspace(moves[i]); i++);
	}
}


long number_of_checks = 0;

/* TODO: make it fasterrr */
int is_solved(skewb_pos *skb) {

#define AND_SYMBOL &

	/* onliner equiavlent to the code below. For some reason a bit faster with bitwise and */
	return (skb->centers[0] == 0) AND_SYMBOL (skb->centers[1] == 1) AND_SYMBOL
		   (skb->centers[2] == 2) AND_SYMBOL (skb->centers[3] == 3) AND_SYMBOL
		   \
		   (!skb->corners_orient[0]) AND_SYMBOL (!skb->corners_orient[1]) AND_SYMBOL 
		   (!skb->corners_orient[2]) AND_SYMBOL (!skb->corners_orient[3]) AND_SYMBOL 
		   (!skb->corners_orient[4]) AND_SYMBOL (!skb->corners_orient[5]) AND_SYMBOL 
		   (!skb->corners_orient[6]) AND_SYMBOL
		   \
		   (skb->corner_pos[0] == 0) AND_SYMBOL (skb->corner_pos[1] == 1);
#undef AND_SYMBOL

	// number_of_checks++;
	
	/* no need to check of centers due to the permutation being even */
	for (int c = 0; c < 4; c++) {
		if (skb->centers[c] != (uint8_t)c)
			return 0;
	}

	/* no need to check all corners due to parity. No need to check the last corner */
	for (int c = 0; c < 7; c++) {
		if (skb->corners_orient[c] != 0)
			return 0;
	}

	/* only need to check for the position of two corners */
	for (int c = 0; c < 2; c++) {
		if (skb->corner_pos[c] != c)
			return 0;
	}

	return 1;
}



/* return the number of moves to solution
 * if no solution was found, returns -1 */
int aux_solve(skewb_pos *skb, int prev_move_type, int *moves_buffer, int max_moves) {
	if (is_solved(skb)) {
		*moves_buffer = -1;
		return 0;
	}
	else if (max_moves == 0)
		return -1;
	
	for (int m = 0; m < 8; m++) {
		if ((m >> 1) == prev_move_type) {
			m++;
			continue;
		}

		int res;

		move(skb, m);
		res = aux_solve(skb, m >> 1, moves_buffer + 1, max_moves - 1);
		if (res != -1) {
			*moves_buffer = m;
			return res + 1;
		}

		move(skb, m);
		res = aux_solve(skb, m >> 1, moves_buffer + 1, max_moves - 1);
		if (res != -1) {
			*moves_buffer = m + 1;
			return res + 1;
		}

		move(skb, m);
	}

	*moves_buffer = -1;

	return -1;
}


int aux_solve_least_moves(skewb_pos *skb, int prev_move_type, int *moves_buffer, int max_moves) {

	number_of_checks++;

	if (is_solved(skb)) {
		*moves_buffer = -1;
		return 0;
	}
	else if (max_moves == 0)
		return 1;

	int least_moves = max_moves;
	int best_move = -1;

	for (int m = 0; m < 4; m++) {
		if (m == prev_move_type)
			continue;

		int res;

		move(skb, 2 * m);
		res = aux_solve_least_moves(skb, m, moves_buffer + 1, least_moves - 1);
		if (res + 1 < least_moves) {
			least_moves = res + 1;
			best_move = 2 * m;
		}

		move(skb, 2 * m);
		res = aux_solve_least_moves(skb, m, moves_buffer + 1, least_moves - 1);
		if (res + 1 < least_moves) {
			least_moves = res + 1;
			best_move = 2 * m + 1;
		}

		move(skb, 2 * m);
	}

	if (best_move != -1) {
		*moves_buffer = best_move;
		return least_moves;
	}

	return max_moves + 1;
}


int cache_hits = 0;

/* cache should be empty */
int aux_solve_least_moves_cached(skewb_pos *skb, int prev_move_type, int *moves_buffer, int max_moves, int depth, int8_t *cache) {


	long long hash = skewb_encode(skb);

	// already encountered this state in the tree above. no need to check anything further
	if (cache[hash] && cache[hash] < depth) {
		cache_hits++;
		return max_moves + 1;
	}
	cache[hash] = depth;
	number_of_checks++;

	if (is_solved(skb)) {
		*moves_buffer = -1;
		return 0;
	}
	else if (max_moves == 0)
		return 1;

	int least_moves = max_moves;
	int best_move = -1;

	for (int m = 0; m < 4; m++) {
		if (m == prev_move_type)
			continue;

		int res;

		move(skb, 2 * m);
		res = aux_solve_least_moves_cached(skb, m, moves_buffer + 1, least_moves - 1, depth + 1, cache);
		if (res + 1 < least_moves) {
			least_moves = res + 1;
			best_move = 2 * m;
		}

		move(skb, 2 * m);
		res = aux_solve_least_moves_cached(skb, m, moves_buffer + 1, least_moves - 1, depth + 1, cache);
		if (res + 1 < least_moves) {
			least_moves = res + 1;
			best_move = 2 * m + 1;
		}

		move(skb, 2 * m);
	}

	if (best_move != -1) {
		*moves_buffer = best_move;
		return least_moves;
	}

	return max_moves + 1;
}



int aux_solve_least_moves_property(skewb_pos *skb, int prev_move_type, int *moves_buffer, int max_moves, int (*pred)(skewb_pos*)) {
	if (pred(skb)) {
		*moves_buffer = -1;
		return 0;
	}
	else if (max_moves == 0)
		return 1;

	int least_moves = max_moves;
	int best_move = -1;

	for (int m = 0; m < 4; m++) {
		if (m == prev_move_type)
			continue;

		int res;

		move(skb, 2 * m);
		res = aux_solve_least_moves_property(skb, m, moves_buffer + 1, least_moves - 1, pred);
		if (res + 1 < least_moves) {
			least_moves = res + 1;
			best_move = 2 * m;
		}

		move(skb, 2 * m);
		res = aux_solve_least_moves_property(skb, m, moves_buffer + 1, least_moves - 1, pred);
		if (res + 1 < least_moves) {
			least_moves = res + 1;
			best_move = 2 * m + 1;
		}

		move(skb, 2 * m);
	}

	if (best_move != -1) {
		*moves_buffer = best_move;
		return least_moves;
	}

	return max_moves + 1;
}

const char *moves_str[] = {
	"F", "F'", "U", "U'", "R", "R'", "L", "L'"
};




char *str_moves(int *moves) {	
	int chars_amount = 0;
	int number_of_moves = 0;
	for (number_of_moves = 0; moves[number_of_moves] != -1 ;
		number_of_moves++, chars_amount += 2 + (moves[number_of_moves] & 1));

	char *result = malloc(chars_amount);

	for (int i = 0; i < number_of_moves; i++) {
		strcat(result, moves_str[moves[i]]);
		strcat(result, " ");
	}

	return result;
}



int *solve(skewb_pos *skb) {
	int *moves_buffer = malloc(12 * sizeof(int));
	for (int i = 0; i < 12; i++) {
		moves_buffer[i] = -1;
	}

	const int max_moves = 11;
	int number_of_moves = aux_solve(skb, -1, moves_buffer, max_moves);
	if (number_of_moves == -1) {
		free(moves_buffer);
		return NULL;
	}
	
	return moves_buffer;
}


int *solve_fastest(skewb_pos skb) {
	int *moves_buffer = malloc(12 * sizeof(int));
	for (int i = 0; i < 12; i++) {
		moves_buffer[i] = -1;
	}

	// uint8_t *cache = calloc(SKEWB_TOTAL_COMB, 1);

	skewb_pos copy = skb;
	const int max_moves = 11;
	int number_of_moves = aux_solve_least_moves(&copy, -1, moves_buffer, max_moves);
	// free(cache);
	if (number_of_moves == -1) {
		free(moves_buffer);
		return NULL;
	}
	
	return moves_buffer;
}



int *solve_fastest_for_property(skewb_pos skb, int (*pred)(skewb_pos*)) {
	int *moves_buffer = malloc(12 * sizeof(int));
	for (int i = 0; i < 12; i++) {
		moves_buffer[i] = -1;
	}

	skewb_pos copy = skb;
	const int max_moves = 11;
	int number_of_moves = aux_solve_least_moves_property(&copy, -1, moves_buffer, max_moves, pred);
	if (number_of_moves == -1) {
		free(moves_buffer);
		return NULL;
	}

	return moves_buffer;
}


/* Some auxilary functions not currently used. Planned to using them in the future */

int is_white_solved(skewb_pos *skb) {
	if (skb->centers[0] != 0)
		return 0;
	
	for (int c = 0; c < 4; c++) {
		if (skb->corners_orient[c] != 0)
			return 0;
	}

	return skb->corner_pos[0] == 0 && skb->corner_pos[1] == 1;
}


int is_yellow_solved(skewb_pos *skb) {
	if (skb->centers[1] != 1)
		return 0;
	
	/* check if the bottom corners are oriented correctly */
	for (int c = 4; c < 8; c++) {
		if (skb->corners_orient[c] != 0)
			return 0;
	}

	return skb->corner_pos[2] == 2 && skb->corner_pos[3] == 3;
}


int is_white_yellow_solved(skewb_pos *skb) {
	return is_white_solved(skb) && is_yellow_solved(skb);
}


int are_corn_pos_correct(skewb_pos *skb) {
	return skb->corner_pos[0] == 0 && skb->corner_pos[1] == 1
		&& skb->corner_pos[2] == 2 && skb->corner_pos[3] == 3;
}


int are_centers_correct(skewb_pos *skb) {
	return skb->centers[0] == 0 && skb->centers[1] == 1
		&& skb->centers[2] == 2 && skb->centers[3] == 3
		&& skb->centers[4] == 4 && skb->centers[5] == 5;
}

int are_corners_corr_pos_and_centers_corr(skewb_pos *skb) {
	return are_corn_pos_correct(skb) && are_centers_correct(skb);
}


struct image {
	int width, height;
	uint8_t *data;
};


/* takes the side size */
struct image create_skewb_image(skewb_pos *skb, int s) {
	static const int face_coord[6][2] = {
		{ 1, 1 },
		{ 3, 1 },
		{ 2, 1 },
		{ 0, 1 },
		{ 1, 2 },
		{ 1, 0 },
	};

	int width = s * 4;
	int height = s * 3;
	uint8_t *image  = calloc(width * height * 3, 1);

	
	/* draw centers */
	for (int c = 0; c < 6; c++) {
		draw_center(image, width, 
			face_coord[c][0] * s, face_coord[c][1] * s, s, colours[skb->centers[c]]);
	}


	/* draw corners */
	for (int c = 0; c < 8; c++) {
		int corner_piece = c;
		if (c & 1) // if the corner is a free corner
			corner_piece = 2 * skb->corner_pos[c >> 1] + 1;

		int orient = skb->corners_orient[c];

		for (int sidx = 0; sidx < 3; sidx ++) {
			// the position of the corner at the side
			int side = corners_sides[c][sidx];
			int side_corner = corners_sides[c][sidx + 3];

			struct colour col = colours[corners_sides[corner_piece][(sidx + 2 * orient) % 3]];

			draw_corner(image, width,
				face_coord[side][0] * s, face_coord[side][1] * s, s, side_corner, col);
		}
	}


	return (struct image) {
		.width = width,
		.height = height,
		.data = image
	};
}


void image_save(const char *name, struct image img) {

	int len = strlen(name);

	if ((len >= 4 && strcmp(name + len - 4, ".jpg") == 0) || (len >= 5 && strcmp(name + len - 5, ".jpeg") == 0))
		stbi_write_jpg(name, img.width, img.height, 3, img.data, 70);
	else
		stbi_write_png(name, img.width, img.height, 3, img.data, 3);

	/* ppm stuff
	FILE *file = fopen(name, "wb");

	fprintf(file, "P6\n");
	fprintf(file, "%d %d\n", img.width, img.height);
	fprintf(file, "255\n");

	fwrite(img.data, img.width * img.height, 3, file);

	fclose(file); */
}


/* the first number of each pair stores the optimal first move
 * the second number stores the number of total moves */
uint8_t optimal_first_move[SKEWB_TOTAL_COMB][2];

void print_help() {
	printf(
		"\nUsage:\n"
		" gskewb scramble [Options]\n\n"
		"Scramble:\n"
		" Uses F, R, L, U notation\n"
		" Cube position: top white, left green\n"
		" White-Green-Red corner remains stationary (can only rotate)\n\n"
		"Options:\n"
		" -s          Find fastest solution\n"
		" -i filename Output scramble into an image\n"
		" -h          Print this message\n\n"
	);
}


int main(int argc, const char **argv) {

	if (argc == 1) {
		print_help();
		return 0;
	}
	else if (argc == 2) {
		if (strcmp(argv[1], "-h")) {
			print_help();
			return 0;
		}
		printf("Nothing to do. Use -h for help\n");
		return 0;
	}


	int solve_cube = 0;
	const char *img_name = NULL;

	int i = 2;
	while (i < argc) {
		if (argv[i][0] != '-')
			continue;
		switch (argv[i][1]) {
			case 's':
				solve_cube = 1;
				break;
			case 'i':
				i++;
				if (i == argc) {
					printf("-i option requires an file name. Use -h for help\n");
					return 0;
				}
				img_name = argv[i + 1];
				break;
			case 'h':
				print_help();
				return 0;
			default:
				printf("Unknown option %s\n", argv[i]);
				print_help();
				return 0;
		}
		i++;
	}


	skewb_pos skb = skewb_id;
	do_moves(&skb, argv[1]);

	if (img_name) {
		struct image img = create_skewb_image(&skb, 100);
		image_save(img_name, img);
		free(img.data);
	}

	if (solve_cube) {
		clock_t begin = clock();
		int *solve = solve_fastest(skb);
		clock_t end = clock();

		char *moves = str_moves(solve);

		int number_of_moves = 0;
		for (; solve[number_of_moves] != -1; number_of_moves++);

		printf("Solved in %d moves!\n", number_of_moves);
		printf("Checked %ld possibilities\n\n", number_of_checks);

		printf("Solution: %s\n", moves);
		printf("Finished in %.2lf secs\n", (double)(end - begin) / CLOCKS_PER_SEC);
	}

	if (!img_name && !solve_cube) {
		printf("Nothing to do. Use -h for help\n");
	}

	return 0;
}

