#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "libpixed.h"

int read_uint32_big_endian(FILE *, uint32_t *);
int write_uint32_big_endian(FILE *, uint32_t);

PixedDocument *
pixed_document_new(const char *name, uint32_t width, uint32_t height)
{
	PixedDocument *document = malloc(sizeof(PixedDocument));
	if (!document)
		return 0;

	document->name = malloc(sizeof(char) * strlen(name));
	if (!document->name) {
		free(document);
		return 0;
	}

	strncpy(document->name, name, sizeof(char) * strlen(name));

	document->canvas = malloc(sizeof(uint32_t) * (width * height));
	if (!document->canvas) {
		free(document->name);
		free(document);
		return 0;
	}

	memset(document->canvas, 0, sizeof(uint32_t) * (width * height));

	document->width = width;
	document->height = height;

	return document;
}

void
pixed_document_free(PixedDocument *document)
{
	free(document->name);
	free(document->canvas);
	free(document);
}

PixedDocument *
pixed_document_read_file(const char *file_name)
{
	if (strlen(file_name) <= 0)
		return 0;

	FILE *file = fopen(file_name, "rb");
	PixedDocument *document = 0;

	if (!file) {
		return 0;
	}

	// Read PiXd magic header
	char magic[4];
	fread(&magic, 1, 4, file);

	// Wrong file format
	if (strncmp(magic, PIXED_HEADER_MAGIC, 4) != 0) {
		fclose(file);
		return 0;
	}

	// Read width and height in big endian order
	uint32_t width = 0, height = 0;
	if (read_uint32_big_endian(file, &width) != 0) {
		fclose(file);
		return 0;
	}

	if (read_uint32_big_endian(file, &height) != 0) {
		fclose(file);
		return 0;
	}

	// Document dimensions can't be equal or less than 0
	if (width <= 0 || height <= 0) {
		fclose(file);
		return 0;
	}

	document = pixed_document_new(file_name, width, height);

	uint32_t pixels_length = width * height;
	document->canvas = malloc(sizeof(uint32_t) * pixels_length);
	if (!document->canvas) {
		pixed_document_free(document);
		fclose(file);
		return 0;
	}

	size_t read_pixels = fread(document->canvas, sizeof(uint32_t), pixels_length, file);
	if (read_pixels < pixels_length) {
		pixed_document_free(document);
		fclose(file);
		return 0;
	}

	fclose(file);
	return document;
}

int
pixed_document_write_file(PixedDocument *document, char *file_name)
{
	if (!document)
		return -1;

	FILE *file = fopen(file_name, "wb");
	if (!file)
		return -1;

	if (fwrite(PIXED_HEADER_MAGIC, sizeof(char), 4, file) < 1) {
		fclose(file);
		return -1;
	}

	if (write_uint32_big_endian(file, document->width) < 1) {
		fclose(file);
		return -1;
	}

	if (write_uint32_big_endian(file, document->height) < 1) {
		fclose(file);
		return -1;
	}

	uint32_t pixels_length = document->width * document->height;
	
	int i = 0;
	for (; i < pixels_length; i++) {
		if (write_uint32_big_endian(file, document->canvas[i]) < 1) {
			fclose(file);
			return -1;
		}
	}

	fclose(file);
	return 0;
}

int
pixed_document_resize(PixedDocument *document, int width, int height)
{
	if (!document)
		return -1;

	// Skip unnecessary process
	if (document->width == width && document->height == height)
		return 0;

	return -1;
}

int read_uint32_big_endian(FILE *file, uint32_t *value)
{
	char buf[4];

	if (fread(buf, 1, 4, file) < 4) {
		return -1;
	}

	*value = (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3];
	return 0;
}

int write_uint32_big_endian(FILE * file, uint32_t number)
{
	char bytes[4];

	bytes[0] = number >> 24;
	bytes[1] = number >> 16;
	bytes[2] = number >> 8;
	bytes[3] = number;

	return fwrite(bytes, sizeof(char), 4, file);
}
