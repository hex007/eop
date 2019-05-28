#include <assert.h>
#include <ctype.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "bcm_host.h"
#include "image.h"

extern bool loadJPG(const char* f_name, Image *image);
extern bool loadPNG(const char* f_name, Image *image);

static void signalHandler(int signalNumber)
{
}

void usage(const char *program)
{
	fprintf(stderr, "Usage: %s ", program);
	fprintf(stderr, "<file> l [x y w h]\n");
	fprintf(stderr, "	file     png or jpeg file to display\n");
	fprintf(stderr, "	l        layer, if not specified, layer will be 1.\n");
	fprintf(stderr, "	x        horizontal offset\n");
	fprintf(stderr, "	y        vertical offset\n");
	fprintf(stderr, "	w        width to use [0 for height constrained]\n");
	fprintf(stderr, "	h        height to use [0 for width constrained]\n");
	fprintf(stderr, "	           note: both height and width can be zero for fullscreen\n");
	fprintf(stderr, "\n");

	exit(EXIT_FAILURE);
}


bool loadImage(char* f_name, Image *img)
{
	if (endsWith(f_name, ".png"))
	{
		return loadPNG(f_name, img);
	}
	else if (endsWith(f_name, ".jpg") || endsWith(f_name, ".jpeg"))
	{
		return loadJPG(f_name, img);
	}

	return false;
}

int endsWith(const char *str, const char *suffix)
{
	if (!str || !suffix)
		return 0;
	size_t lenstr = strlen(str);
	size_t lensuffix = strlen(suffix);
	if (lensuffix >  lenstr)
		return 0;
	return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}


int main(int argc, char *argv[])
{
	int layer = 1;
	uint32_t displayNumber = 0;
	int xOffset = 0;
	int yOffset = 0;
	int width = 0;
	int height = 0;
	char *f_name = NULL;
	int result = 0;
	//---------------------------------------------------------------------

	// Parse args
	const char *program = (char*) basename(argv[0]);
	switch (argc) {
	case 2:
		f_name = argv[1];
		break;
	case 3:
		f_name = argv[1];
		layer = atoi(argv[2]);
		break;
	case 6:
	    f_name = argv[1];
		xOffset = atoi(argv[2]);
		yOffset = atoi(argv[3]);
		width = atoi(argv[4]);
		height = atoi(argv[5]);
		break;
	case 7:
		f_name = argv[1];
		layer = atoi(argv[2]);	
		xOffset = atoi(argv[3]);
		yOffset = atoi(argv[4]);
		width = atoi(argv[5]);
		height = atoi(argv[6]);
		break;
	default:
		usage(program);
		return 1;
	}

	// Signal handling
	if (signal(SIGINT, signalHandler) == SIG_ERR)
	{
		perror("installing SIGINT signal handler");
		exit(EXIT_FAILURE);
	}
	if (signal(SIGTERM, signalHandler) == SIG_ERR)
	{
		perror("installing SIGTERM signal handler");
		exit(EXIT_FAILURE);
	}
	//---------------------------------------------------------------------

	// Load image file to structure Image
	Image image = { 0 };
	if (loadImage(f_name, &image) == false || image.buffer == NULL
		|| image.width == 0 || image.height == 0)
	{
		fprintf(stderr, "Unable to load %s\n", f_name);
		return 1;
	}

	// Init BCM
	bcm_host_init();

	DISPMANX_DISPLAY_HANDLE_T display
		= vc_dispmanx_display_open(displayNumber);
	assert(display != 0);

	DISPMANX_MODEINFO_T info;
	result = vc_dispmanx_display_get_info(display, &info);
	assert(result == 0);

	// Calculate linear scaling maintaining aspect ratio
	if (width == 0 && height != 0) {
		width = (height * image.width) / image.height;
	} else if (width != 0 && height == 0) {
		height = (width * image.height) / image.width;
	}

	// Create a resource and copy bitmap to resource
	uint32_t vc_image_ptr;
	DISPMANX_RESOURCE_HANDLE_T resource = vc_dispmanx_resource_create(
		image.type, image.width, image.height, &vc_image_ptr);

	assert(resource != 0);

	// Set dimentions of the bitmap to be copied
	VC_RECT_T bmpRect;
	vc_dispmanx_rect_set(&bmpRect, 0, 0, image.width, image.height);

	// Copy bitmap data to vc
	result = vc_dispmanx_resource_write_data(
		resource, image.type, image.pitch, image.buffer, &bmpRect);

	assert(result == 0);

	// Free bitmap data
	free(image.buffer);
	//---------------------------------------------------------------------

	// Notify vc that an update is takng place
	DISPMANX_UPDATE_HANDLE_T update = vc_dispmanx_update_start(0);
	assert(update != 0);

	// Calculate source and destination rect values
	VC_RECT_T srcRect, dstRect;
	vc_dispmanx_rect_set(&srcRect, 0, 0, image.width << 16, image.height << 16);
	vc_dispmanx_rect_set(&dstRect, xOffset, yOffset, width, height);

	// Add element to vc
	VC_DISPMANX_ALPHA_T alpha = { DISPMANX_FLAGS_ALPHA_FROM_SOURCE, 255, 0 };
	DISPMANX_ELEMENT_HANDLE_T element = vc_dispmanx_element_add(
		update, display, layer, &dstRect, resource, &srcRect,
		DISPMANX_PROTECTION_NONE, &alpha, NULL, DISPMANX_NO_ROTATE);

	assert(element != 0);

	// Notify vc that update is complete
	result = vc_dispmanx_update_submit_sync(update);
	assert(result == 0);
	//---------------------------------------------------------------------

	// Wait till a signal is received
	pause();
	//---------------------------------------------------------------------

	// Delete layer and free memory
	update = vc_dispmanx_update_start(0);
	assert(update != 0);
	result = vc_dispmanx_element_remove(update, element);
	assert(result == 0);
	result = vc_dispmanx_update_submit_sync(update);
	assert(result == 0);
	result = vc_dispmanx_resource_delete(resource);
	assert(result == 0);
	result = vc_dispmanx_display_close(display);
	assert(result == 0);

	return 0;
}
