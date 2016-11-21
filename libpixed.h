#include <stdint.h>

/* PiXD */
#define PIXED_HEADER_MAGIC "PiXd" 

typedef struct
{
	char *name;
	uint32_t width, height;
	uint32_t *canvas; // 8-bit rgba
} PixedDocument;

PixedDocument * pixed_document_new(const char *, uint32_t, uint32_t);
void            pixed_document_free(PixedDocument *);
PixedDocument * pixed_document_read_file(const char *);
int             pixed_document_write_file(PixedDocument *, char *);
int             pixed_document_resize(PixedDocument *, int, int);

#define         pixed_document_get_pixel(document, X, Y) ((document->canvas[((Y) * document->width) + X]))
#define         pixed_document_set_pixel(document, X, Y, COLOR) (((document)->canvas[((Y) * (document)->width) + X]) = COLOR);
#define         pixed_color_rgba(R, G, B, A) ((R << 24) + (G << 16) + (B << 8) + A)
#define         pixed_color_rgb(R, G, B) (pixed_color_rgba(R, G, B, 0xFF)
#define         pixed_color_r(COLOR) ((COLOR) >> 24)
#define         pixed_color_g(COLOR) (((COLOR) >> 16) & 0x000000ff)
#define         pixed_color_b(COLOR) ((COLOR >> 8) & 0x000000ff)
#define         pixed_color_a(COLOR) ((COLOR) & 0x000000ff)
