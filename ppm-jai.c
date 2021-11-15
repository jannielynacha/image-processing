#include <stdio.h>

#define PIXEL_BUFF_SIZE 3
#define TAB				9
#define SPACE			32
#define CR 				13
#define LF 				10

typedef unsigned char   uchar;

typedef enum {
	NO_ERR,
	ERR_FILE_NOT_FOUND,
	ERR_IMG_PROC_FAIL,
	ERR_EOF,
	ERR_INVALID_VAL,
	ERR_MEM_ALLOC,
	ERR_MAGIC_NUM
}ERROR;

typedef enum {
	RED_IDX,
	GREEN_IDX,
	BLUE_IDX
}RGB_INDEX;

typedef struct
{
	uchar  red;
	uchar  green;
	uchar  blue;
}PIXEL;

typedef struct
{
	int  height;
	int  width;
	int  max_val;
	PIXEL **img_buff;
}IMAGE;

void skip_whitespaces_comments(FILE *fp) {
	uchar   c = 0;
	int 	cmnt_flag = 0;

	while(c = fgetc(fp)) {
		if(c == '#') {
			cmnt_flag = 1;
			continue;
		}
		if(c == CR || c == LF) {
			cmnt_flag = 0;
			continue;
		}
		if(c == SPACE || c == TAB || cmnt_flag) {
			continue;
		}
		else {
			break;
		}
		
	}
	fseek( fp, -1, SEEK_CUR );
}

int process_image(FILE *fp_in, FILE *fp_out) {
	IMAGE 	*img;
	PIXEL 	*pixel;
	uchar 	magic_num[3];
	uchar 	pixel_buff[PIXEL_BUFF_SIZE];
	int 	i 					= 0;
	int 	j 					= 0;
	int 	val 				= 0;
	int 	pixel_cnt 			= 0;
	int 	init_height 		= 0;
	int 	init_width 			= 0;
	int 	max_color_val 		= 0;
	int 	size 				= 0;
	int 	gray 				= 0;
	int 	err 	 			= 0;

	skip_whitespaces_comments(fp_in);

	/* Get magic number, height, width, and maximum color value */
	fscanf(fp_in, "%s", magic_num);
	magic_num[size-1] = '\0';
	if(!(magic_num[0] == 'P' && magic_num[1] == '6')) {
		return ERR_MAGIC_NUM;
	}
	skip_whitespaces_comments(fp_in);

	fscanf(fp_in, "%d", &init_width);
	skip_whitespaces_comments(fp_in);

	fscanf(fp_in, "%d", &init_height);
	skip_whitespaces_comments(fp_in);

	fscanf(fp_in, "%d", &max_color_val);
	skip_whitespaces_comments(fp_in);

	pixel = (PIXEL *)malloc(sizeof(PIXEL));
	if(pixel == NULL) return ERR_MEM_ALLOC;

	size = init_width * init_height;

	printf("Ftell: %d\n", ftell(fp_in));
	printf("Size: %d\n", size);
	printf("magic number: %s\n", magic_num);
	printf("initial height: %d\n", init_height);
	printf("initial width: %d\n", init_width);
	printf("Max color value: %d\n", max_color_val);

	if(max_color_val > 255 || max_color_val < 1) {
		return ERR_INVALID_VAL;
	}

	/* Allocate memory for image */
	img = (IMAGE *)malloc(sizeof(IMAGE));
	if(img == NULL)
		return ERR_MEM_ALLOC;

	/* New height and width for rotated image */
	img->height = init_width;
	img->width = init_height;
	img->max_val = max_color_val;

	/* Allocate memory for image buff (according to dimensions of rotated image) */
	img->img_buff = (PIXEL **)malloc(img->height * sizeof(PIXEL *));
	if(img->img_buff == NULL) return ERR_MEM_ALLOC;

	for(i=0 ; i < img->height ; i++) {
		img->img_buff[i] = (PIXEL *)malloc(img->width * sizeof(PIXEL));
		if(img->img_buff[i] == NULL) return ERR_MEM_ALLOC;
	}

	/* Print height, width and max_val of output image */
	fprintf(fp_out, "%s\n%d\n%d\n%d\n", magic_num, img->width, img->height, img->max_val);

	i = 0;
	j = img->width-1;

	/* Rotate image */
	while(val = fread(pixel_buff, sizeof(unsigned char), PIXEL_BUFF_SIZE, fp_in)) {

		/* Assign rgb of the pixel */
		pixel->red = pixel_buff[RED_IDX];
		pixel->green = pixel_buff[GREEN_IDX];
		pixel->blue = pixel_buff[BLUE_IDX];

		pixel_cnt++;

		/* Store in buff (already in rotated positions) */
		img->img_buff[i][j] = *pixel;
		i++;
		if(i == img->height) {
			i = 0;
			j--;
		}
		if(j < 0) break;
	}

	printf("pixel count: %d\n", pixel_cnt);

	if(pixel_cnt < size) {
		return ERR_EOF;
	}

	/* Convert to Grayscale */
	for (i=0; i < img->height; i++) {
		for(j=0; j < img->width; j++) {
			gray = ((3 * img->img_buff[i][j].red)/10) + ((6 * img->img_buff[i][j].green)/10) + (img->img_buff[i][j].blue/10);
			img->img_buff[i][j].red = gray;
			img->img_buff[i][j].green = gray;
			img->img_buff[i][j].blue = gray;
		}
	}

	/* Write raster data to file */
	for (i=0; i < img->height; i++) {
		for(j=0; j < img->width; j++) {
			fwrite(&img->img_buff[i][j], sizeof(uchar), PIXEL_BUFF_SIZE, fp_out);
		}
	}

	/* De-allocate memory */
	free(pixel);
	for(i=0 ; i < img->height ; i++)
		free(img->img_buff[i]);
	free(img->img_buff);
	free(img);

	return NO_ERR;
}

int main(int argc, char *argv[])
{
	int 	err = 0;
	char	fname_out[256];
	FILE 	*fp_in;
	FILE 	*fp_out;

	if(argc < 2) {
		printf("Usage: ppm-jai [filename]\n");
		return 1;
	}

	sprintf(fname_out, "%s.out", argv[1]);

	fp_in  = fopen(argv[1], "rb");
	fp_out = fopen(fname_out, "wb");

	if(!fp_in || !fp_out) {
		err = ERR_FILE_NOT_FOUND;
	}
	else {
		err = process_image(fp_in, fp_out);
	}

	if(!err) {
		printf("Success!\n");
	}
	else if(err == ERR_FILE_NOT_FOUND) {
		printf("ERROR: File not found!\n");
	}
	else if(err == ERR_IMG_PROC_FAIL) {
		printf("ERROR: Image processing failed!\n");
	}
	else if(err == ERR_EOF) {
		printf("ERROR: Data was truncated!\n");
	}
	else if(err == ERR_INVALID_VAL) {
		printf("ERROR: Invalid maximum color value!\n");
	}
	else if(err == ERR_MEM_ALLOC) {
		printf("ERROR: Memory Allocation Failed!\n");
	}
	else if(err == ERR_MAGIC_NUM) {
		printf("ERROR: Magic number is not P6.\n");
	}

	fclose(fp_in);
	fclose(fp_out);

	return 0;
}