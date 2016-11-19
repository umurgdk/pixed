#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "libcol.h"
#include "libpixed.h"
#include "libglutil.h"
#include "shaders.h"

/*
 * Definitions
 */
#define FLOAT_PER_PIXEL_VERTEX 6

#define PIXEL_UNIFORM_PAN      "pan"
#define PIXEL_UNIFORM_ZOOM     "zoom"
#define PIXEL_UNIFORM_VIEWPORT "viewport"

/*
 * Forward declarations
 */
typedef struct {
  GLuint document_vao;
  GLuint document_vbo;
  GLuint pixel_shader;
} graphics_context;

typedef struct {
  struct pixed_document *document;
  graphics_context      *graphics;
  float                  zoom;     // Size of a pixel in pixels
  float                  pan_x;
  float                  pan_y;
} pixed_editor;

pixed_editor *pixed_editor_new();
void          pixed_editor_free(pixed_editor *);

void          graphics_init();
void          graphics_render();
void          graphics_log_cb(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar*, const void*);

GLFWwindow   *window_create(int, int);
void          window_reshape_cb(GLFWwindow*, int, int);
void          window_key_cb(GLFWwindow*, int, int, int, int);

/*
 * Globals
 */
pixed_editor *editor;

/*
 * Function implementations
 */
pixed_editor *
pixed_editor_new()
{
  pixed_editor * editor = (pixed_editor *)malloc(sizeof(pixed_editor));
  editor->document = pixed_document_new("New Document", 800, 800);
  editor->graphics = (graphics_context *)malloc(sizeof(graphics_context));
  editor->zoom = 1.0f;
  editor->pan_x = 0;
  editor->pan_y = 0;

  return editor;
}

void
pixed_editor_free(pixed_editor *editor)
{
  pixed_document_free(editor->document);
  free(editor);
}

void
graphics_init()
{
  GLuint pixel_vert = glutil_shader_compile(shader_pixel_vert, GL_VERTEX_SHADER);
  GLuint pixel_geom = glutil_shader_compile(shader_pixel_geom, GL_GEOMETRY_SHADER);
  GLuint pixel_frag = glutil_shader_compile(shader_pixel_frag, GL_FRAGMENT_SHADER);

  graphics_context *ctx = editor->graphics;
  struct pixed_document *document = editor->document;

  ctx->pixel_shader = glutil_shader_compile_prog3(pixel_vert, pixel_frag, pixel_geom);

  glUseProgram(ctx->pixel_shader);
  glutil_shader_uniform1f(ctx->pixel_shader, PIXEL_UNIFORM_ZOOM, editor->zoom);
  glutil_shader_uniform2f(ctx->pixel_shader, PIXEL_UNIFORM_PAN, editor->pan_x, editor->pan_y);
  glutil_shader_uniform2f(ctx->pixel_shader, PIXEL_UNIFORM_VIEWPORT, 800.0f, 800.0f);
  glUseProgram(0);

  glGenVertexArrays(1, &ctx->document_vao);
  glGenBuffers(1, &ctx->document_vbo);

  glBindVertexArray(ctx->document_vao);
  {
    /* TODO: Make this dynamic allocation to keep it in graphics context */
    size_t buffer_len = (document->width * document->height) * FLOAT_PER_PIXEL_VERTEX;
    GLfloat *buffer = (GLfloat *)malloc(sizeof(GLfloat) * buffer_len);

    uint32_t pixel_color = 0;
    int row = 0, col = 0, offset = 0;
    for (row = 0; row < document->height; row++) {
      for (col = 0; col < document->width; col++) {
        int offset = ((row * document->width) + col) * 6;

        /* Position data */
        buffer[offset + 0] = (float)col;
        buffer[offset + 1] = (float)row;

        /* Color data */
        pixel_color = pixed_document_get_pixel(document, col, row);
        float red = ((float)pixed_color_r(pixel_color) / 255.0f);
        float green = ((float)pixed_color_g(pixel_color) / 255.0f);
        float blue = ((float)pixed_color_b(pixel_color) / 255.0f);
        float alpha = ((float)pixed_color_a(pixel_color) / 255.0f);

        buffer[offset + 2] = red;
        buffer[offset + 3] = green;
        buffer[offset + 4] = blue;
        buffer[offset + 5] = alpha;
      }
    }

    glBindBuffer(GL_ARRAY_BUFFER, ctx->document_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * buffer_len, buffer, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(sizeof(GLfloat) * 2));
  }
  glBindVertexArray(0);
}

void
graphics_render()
{
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glUseProgram(editor->graphics->pixel_shader);
  glBindVertexArray(editor->graphics->document_vao);
  glDrawArrays(GL_POINTS, 0, editor->document->width *editor->document->height);
  glBindVertexArray(0);
  glUseProgram(0);
}

void
graphics_log_cb(GLenum source, GLenum type, GLuint id, GLenum severity,
                GLsizei length, const GLchar* message, const void* userParam)
{
  printf("Got log\n");
}

GLFWwindow *
window_create(int width, int height)
{
	GLFWwindow *window;

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

	window = glfwCreateWindow(width, height, "Snake", NULL, NULL);
	if (window == NULL)
	{
		fprintf(stderr, "Failed to create GLFW window!\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);

	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		fprintf(stderr, "Failed to initialize GLEW!\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSwapInterval(1);

	// Set OpenGL settings
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnable(GL_PROGRAM_POINT_SIZE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

  // Set GLFW callbacks
  glfwSetFramebufferSizeCallback(window, window_reshape_cb);
  glfwSetKeyCallback(window, window_key_cb);

  // Set debug callback
  if(glDebugMessageCallback){
    GLuint unusedIds = 0;

    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(glutil_debug_cl, 0);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, &unusedIds, true);
  } else {
    printf("glDebugMessageCallback not available\n");
  }


  int _width, _height;
  glfwGetFramebufferSize(window, &_width, &_height);
    
  glViewport(0, 0, _width, _height);

	return window;
}

void window_reshape_cb(GLFWwindow* window, int width, int height)
{
  glViewport(0, 0, width / 2, height / 2);
}

void window_key_cb(GLFWwindow* window, int key, int scancode, int action, int mode)
{
  // TODO: check for press
  if (key == GLFW_KEY_LEFT) {
    editor->pan_x -= editor->zoom;
  } else if (key == GLFW_KEY_RIGHT) {
    editor->pan_x += editor->zoom;
  }

  glUseProgram(editor->graphics->pixel_shader);
  glutil_shader_uniform2f(editor->graphics->pixel_shader, PIXEL_UNIFORM_PAN, editor->pan_x, editor->pan_y);
  glUseProgram(0);
}

static void APIENTRY DebugCallback(unsigned int id, unsigned int category, unsigned int severity, unsigned int length, int _s, const char* message, const void* userParam);


int main(int argvc, char **argv)
{
  GLFWwindow *window = window_create(800, 800);

  editor = pixed_editor_new();

  pixed_document_set_pixel(editor->document, 0, 0, 0xff0000ff);
  pixed_document_set_pixel(editor->document, 799, 799, 0xffffffff);
  graphics_init(editor);

  while(!glfwWindowShouldClose(window))
  {
    glfwPollEvents();

    graphics_render(editor);

    glfwSwapBuffers(window);
  }

  pixed_editor_free(editor);

  glfwTerminate();
  return 0;
}

