#include "bcm_host.h"

typedef struct
{
	uint8_t *buffer;
	int width;
	int height;
	int bpp;
	int pitch;
	VC_IMAGE_TYPE_T type;
} Image;
