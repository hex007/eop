#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/stat.h>
#include <stdbool.h>

#include <jpeglib.h>

#include "bcm_host.h"
#include "image.h"


bool loadJPG(const char *f_name, Image *image) {
	int rc, i, j;

	// ------------------------------------------------------------------- SETUP

	// Variables for the source jpg
	struct stat file_info;
	unsigned long jpg_size;
	unsigned char *jpg_buffer;

	// Variables for the decompressor itself
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	// Variables for the output buffer, and how long each row is
	unsigned long bmp_size;
	unsigned char *bmp_buffer;
	int row_stride, width, height, pixel_size;

	rc = stat(f_name, &file_info);
	if (rc) {
		// syslog(LOG_ERR, "FAILED to stat source jpg");
		return false;
	}
	jpg_size = file_info.st_size;
	jpg_buffer = (unsigned char*) malloc(jpg_size + 100);

	int fd = open(f_name, O_RDONLY);
	i = 0;
	while (i < jpg_size) {
		rc = read(fd, jpg_buffer + i, jpg_size - i);
		// syslog(LOG_INFO, "Input: Read %d/%lu bytes", rc, jpg_size-i);
		i += rc;
	}
	close(fd);

	// ------------------------------------------------------------------- START

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	jpeg_mem_src(&cinfo, jpg_buffer, jpg_size);

	rc = jpeg_read_header(&cinfo, TRUE);

	if (rc != 1) {
		// syslog(LOG_ERR, "File does not seem to be a normal JPEG");
		return false;
	}

	jpeg_start_decompress(&cinfo);

	width = cinfo.output_width;
	height = cinfo.output_height;
	pixel_size = cinfo.output_components;

	bmp_size = width * height * pixel_size;
	bmp_buffer = (unsigned char*) malloc(bmp_size);

	row_stride = width * pixel_size;

	while (cinfo.output_scanline < cinfo.output_height) {
		unsigned char *buffer_array[1];
		buffer_array[0] = bmp_buffer + (cinfo.output_scanline) * row_stride;

		jpeg_read_scanlines(&cinfo, buffer_array, 1);

	}
	// syslog(LOG_INFO, "Proc: Done reading scanlines");

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	// And free the input buffer
	free(jpg_buffer);

	// -------------------------------------------------------------------- DONE

	image->bpp = 3;
	image->width = width;
	image->height = height;
	image->buffer = bmp_buffer;
	image->type = VC_IMAGE_RGB888;
	image->pitch = 3 * width;

	return true;
}
