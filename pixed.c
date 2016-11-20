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

#define TOOL_IDLE 0
#define TOOL_PAN  1
#define TOOL_MAX  (TOOL_PAN + 1)

/*
 * Forward declarations
 */
typedef struct pixed_document pixed_document;

typedef struct {
  GLuint document_vao;
  GLuint document_vbo;
  GLuint pixel_shader;
} graphics_context;

typedef struct _key_event {
  int key;
  int scancode;
  int action;
  int mode;

  struct _key_event *next;
} key_event;

typedef struct {
  key_event *head;
  key_event *last;
} key_event_buffer;

typedef enum {
  MOUSE_DOWN,
  MOUSE_UP,
  MOUSE_MOVE,
  MOUSE_SCROLL
} mouse_event_action;

typedef struct _mouse_event {
  mouse_event_action action;

  int button;
  int mods;
  int x;
  int y;

  struct _mouse_event *next;
} mouse_event;

typedef struct {
  mouse_event *head;
  mouse_event *last;
} mouse_event_buffer;

typedef struct _tool {
  int   id;
  void *state;
  bool  wants_destroy;

  bool  (*initialize)(struct _tool *);
  bool  (*on_key_down)(struct _tool *, key_event *);
  bool  (*on_key_up)(struct _tool *, key_event *);
  bool  (*on_key_repeat)(struct _tool *, key_event *);
  bool  (*on_mouse_down)(struct _tool *, mouse_event *);
  bool  (*on_mouse_up)(struct _tool *, mouse_event *);
  bool  (*on_mouse_move)(struct _tool *, mouse_event *);
  bool  (*destroy)(struct _tool *);
} tool;

typedef struct {
  pixed_document   *document;
  graphics_context *graphics;
  tool             *active_tool;  
  float             zoom;     // Size of a pixel in pixels
  float             pan_x;
  float             pan_y;
} pixed_editor;

typedef struct {
  float start_x;
  float start_y;
  float editor_start_x;
  float editor_start_y;
  bool  mouse_dragging;
} tool_pan_state;

pixed_editor   *pixed_editor_new(void);
void            pixed_editor_free(void);
void            pixed_editor_set_document(pixed_document *);
void            pixed_editor_dispatch_tool(void);

void            input_key_event_push(int key, int scancode, int action, int mode);
key_event      *input_key_event_peek(void);
void            input_key_event_consume(void);
void            input_keyboard_buffer_free(void);

void            input_mouse_event_push(mouse_event_action action, int x, int y, int button, int mods);
mouse_event    *input_mouse_event_peek(void);
void            input_mouse_event_consume(void);
void            input_mouse_buffer_free(void);

bool            tool_pan_initialize(tool *);
bool            tool_pan_on_key_up(tool *, key_event *);
bool            tool_pan_on_mouse_down(tool *, mouse_event *);
bool            tool_pan_on_mouse_move(tool *, mouse_event *);
bool            tool_pan_on_mouse_up(tool *, mouse_event *);  
bool            tool_pan_destroy(tool *);

void            graphics_init(void);
void            graphics_render(void);
void            graphics_center_document(void);
void            graphics_log_cb(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar*, const void*);

GLFWwindow     *window_create(int, int);
void            window_reshape_cb(GLFWwindow*, int, int);
void            window_key_cb(GLFWwindow*, int, int, int, int);
void            window_mouse_move_cb(GLFWwindow *, double, double);
void            window_mouse_btn_cb(GLFWwindow *, int, int, int);

/*
 * Globals
 */
GLFWwindow *window;
pixed_editor *editor;
key_event_buffer *keyboard_buffer;
mouse_event_buffer *mouse_buffer;

/* id, state, wants_destroy, initialize, on_key_down, on_key_up, on_key_repeat, on_mouse_down, on_mouse_up, on_mouse_move, destroy */
static tool tool_lookup[TOOL_MAX] = {
  { TOOL_IDLE, 0, false, 0, 0, 0, 0, 0, 0, 0, 0 },
  { TOOL_PAN, 0, false, tool_pan_initialize, 0, tool_pan_on_key_up, 0, tool_pan_on_mouse_down, tool_pan_on_mouse_up, tool_pan_on_mouse_move, tool_pan_destroy }
};

/*
 * Function implementations
 */
pixed_editor *
pixed_editor_new()
{
  pixed_editor * editor = (pixed_editor *)malloc(sizeof(pixed_editor));
  editor->document = 0;
  editor->active_tool = &tool_lookup[TOOL_IDLE];
  editor->graphics = (graphics_context *)malloc(sizeof(graphics_context));
  editor->zoom = 10.0f;
  editor->pan_x = 0;
  editor->pan_y = 0;

  return editor;
}

void
pixed_editor_free()
{
  pixed_document_free(editor->document);
  free(editor);
}

void
pixed_editor_set_document(pixed_document *document)
{
  // TODO: Check for already opened document and ask if there are unsaved changes!
  editor->document = document;
}

void
pixed_editor_dispatch_tool()
{
  key_event *key_e = input_key_event_peek();
  mouse_event *mouse_e = input_mouse_event_peek();
  tool *active_tool = editor->active_tool;
  tool *new_tool = 0;

  // Handle key event
  if (key_e) {
    bool consume_input = false;

    switch (key_e->action) {
    case GLFW_PRESS:
      if (active_tool->on_key_down)
        consume_input = active_tool->on_key_down(active_tool, key_e);
      break;

    case GLFW_RELEASE:
      if (active_tool->on_key_up)
        consume_input = active_tool->on_key_up(active_tool, key_e);
      break;

    case GLFW_REPEAT:
      if (active_tool->on_key_repeat)
        consume_input = active_tool->on_key_repeat(active_tool, key_e);
      break;

    default:
      break;
    }

    if (consume_input){
      input_key_event_consume();
    } else if (active_tool == &tool_lookup[TOOL_IDLE] && key_e->action == GLFW_PRESS) {
      // Key presses in idle tool may activate other tools
      switch (key_e->key) {
      // Pan tool
      case GLFW_KEY_SPACE:
        new_tool = &tool_lookup[TOOL_PAN];
        input_key_event_consume();
        break;

      default:
        input_key_event_consume();
        break;
      }

      if (new_tool) {
        if (active_tool->destroy)
          active_tool->destroy(active_tool);

        if (new_tool->initialize) {
          if (!new_tool->initialize(new_tool))
            editor->active_tool = &tool_lookup[TOOL_IDLE];
          else
            editor->active_tool = new_tool;
        } else {
          editor->active_tool = new_tool;
        }
      }      
    } else {
      input_key_event_consume();
    }
  }

  if (new_tool == 0 && mouse_e != 0) {
    switch (mouse_e->action) {
    case MOUSE_MOVE:
      if (active_tool->on_mouse_move) 
        active_tool->on_mouse_move(active_tool, mouse_e);
      break;

    case MOUSE_DOWN:
      if (active_tool->on_mouse_down)
        active_tool->on_mouse_down(active_tool, mouse_e);
      break;

    case MOUSE_UP:
      if (active_tool->on_mouse_up)
        active_tool->on_mouse_up(active_tool, mouse_e);
      break;

    case MOUSE_SCROLL:
    default:
      break;
    }
    
    input_mouse_event_consume();
  }

  // Check if tool needs to be destroyed
  if (new_tool == 0 && active_tool != &tool_lookup[TOOL_IDLE] && active_tool->wants_destroy) {
    if (active_tool->destroy)
      active_tool->destroy(active_tool);

    editor->active_tool = &tool_lookup[TOOL_IDLE];
  }
}

void
input_key_event_push(int key, int scancode, int action, int mode)
{
  key_event *key_e = (key_event *)malloc(sizeof(key_event));
  if (!key_e) {
    perror("ERROR: Allocating key event failed");
    return;
  }

  key_e->key = key;
  key_e->scancode = scancode;
  key_e->action = action;
  key_e->mode = mode;
  key_e->next = 0;

  if (keyboard_buffer->head == 0) {
    keyboard_buffer->head = key_e;
  } else {
    keyboard_buffer->last->next = key_e;
  }

  keyboard_buffer->last = key_e;
}

inline
key_event *
input_key_event_peek()
{
  return keyboard_buffer->head; 
}

void
input_key_event_consume()
{
  if (keyboard_buffer->head == 0) {
    printf("WARNING: Trying to consume empty keyboard buffer!\n");
    return;
  }

  key_event *current = keyboard_buffer->head;
  if (keyboard_buffer->head && keyboard_buffer->head->next){
    keyboard_buffer->head = keyboard_buffer->head->next;
  } else {
    keyboard_buffer->head = 0;
    keyboard_buffer->last = 0;
  }

  free(current);
}

void
input_keyboard_buffer_free()
{
  key_event *temp = 0;
  key_event *key_e = keyboard_buffer->head;
  
  while (key_e != 0) {
    temp = key_e->next;
    free(key_e);
    key_e = temp;
  }

  keyboard_buffer->last = 0;
}

void
input_mouse_event_push(mouse_event_action action, int x, int y, int button, int mods)
{
  mouse_event *mouse_e = (mouse_event *)malloc(sizeof(mouse_event));
  if (!mouse_e) {
    perror("ERROR: Allocating key event failed");
    return;
  }

  mouse_e->action = action;
  mouse_e->x = x;
  mouse_e->y = y;
  mouse_e->button = button;
  mouse_e->mods = mods;
  mouse_e->next = 0;

  if (mouse_buffer->head == 0) {
    mouse_buffer->head = mouse_e;
  } else {
    mouse_buffer->last->next = mouse_e;
  }

  mouse_buffer->last = mouse_e;
}

inline
mouse_event *
input_mouse_event_peek()
{
  return mouse_buffer->head;
}

void
input_mouse_event_consume()
{
  if (mouse_buffer->head == 0) {
    printf("WARNING: Trying to consume empty mouse buffer!\n");
    return;
  }

  mouse_event *current = mouse_buffer->head;
  if (mouse_buffer->head && mouse_buffer->head->next) {
    mouse_buffer->head = mouse_buffer->head->next;
  } else {
    mouse_buffer->head = 0;
    mouse_buffer->last = 0;
  }

  free(current);
}

void
input_mouse_buffer_free()
{
  mouse_event *temp = 0;
  mouse_event *mouse_e = mouse_buffer->head;

  while (mouse_e != 0) {
    temp = mouse_e->next;
    free(mouse_e);
    mouse_e = temp;
  }

  mouse_buffer->last = 0;
}

bool
tool_pan_initialize(tool *pan)
{
  tool_pan_state *state = (tool_pan_state *)malloc(sizeof(tool_pan_state));
  if (!state) {
    perror("ERROR: Pan tool initialization failed");
    return false;
  }

  GLFWcursor *cursor = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
  glfwSetCursor(window, cursor);

  state->start_x = 0;
  state->start_y = 0;
  state->editor_start_x = editor->pan_x;
  state->editor_start_y = editor->pan_y;
  state->mouse_dragging = false;

  pan->state = state;
  pan->wants_destroy = false;

  return true;
}

bool
tool_pan_on_key_up(tool *pan, key_event *key_e)
{
  if (key_e->key == GLFW_KEY_SPACE) {
    pan->wants_destroy = true;
    return true;
  }

  return false;
}

bool
tool_pan_on_mouse_down(tool *pan, mouse_event *mouse_e)
{
  /* Pan tool only checks for left mouse button */
  if (mouse_e->button != GLFW_MOUSE_BUTTON_LEFT)
    return false;

  tool_pan_state *state = (tool_pan_state *)pan->state;
  if (!state) {
    printf("ERROR: Pan tool expecting state but got null!\n");
    return false;
  }

  state->start_x = mouse_e->x;
  state->start_y = mouse_e->y;
  state->editor_start_x = editor->pan_x;
  state->editor_start_y = editor->pan_y;
  state->mouse_dragging = true;

  return true;
}

bool
tool_pan_on_mouse_move(tool *pan, mouse_event *mouse_e)
{
  tool_pan_state *state = (tool_pan_state *)pan->state;
  if (!state) {
    printf("ERROR: Pan tool expecting state but got null!\n");
    return false;
  }

  /* If mouse button is not down do nothing */
  if (state->mouse_dragging == false)
    return false;

  float pan_x, pan_y;
  pan_x = state->editor_start_x + (mouse_e->x - state->start_x);
  pan_y = state->editor_start_y + (mouse_e->y - state->start_y);

  editor->pan_x = pan_x;
  editor->pan_y = pan_y;

  glUseProgram(editor->graphics->pixel_shader);
  glutil_shader_uniform2f(editor->graphics->pixel_shader, PIXEL_UNIFORM_PAN, pan_x, pan_y);
  glUseProgram(0);

  return true;
}

bool
tool_pan_on_mouse_up(tool *pan, mouse_event *mouse_e)
{
  if (mouse_e->button != GLFW_MOUSE_BUTTON_LEFT)
    return false;

  tool_pan_state *state = (tool_pan_state *)pan->state;
  if (!state) {
    printf("ERROR: Pan tool expecting state but got null!\n");
    return false;
  }

  state->mouse_dragging = false;
  return true;
}


bool
tool_pan_destroy(tool *pan)
{
  tool_pan_state *state = (tool_pan_state *)pan->state;
  if (!state) {
    printf("ERROR: Pan tool destroy failed because state is null!\n");
    return false;
  }

  free(state);
  pan->state = 0;

  glfwSetCursor(window, NULL);

  return true;
}

void
graphics_init()
{
  GLuint pixel_vert = glutil_shader_compile(shader_pixel_vert, GL_VERTEX_SHADER);
  GLuint pixel_geom = glutil_shader_compile(shader_pixel_geom, GL_GEOMETRY_SHADER);
  GLuint pixel_frag = glutil_shader_compile(shader_pixel_frag, GL_FRAGMENT_SHADER);

  graphics_context *ctx = editor->graphics;
  pixed_document *document = editor->document;

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
         offset = ((row * document->width) + col) * 6;

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
graphics_center_document()
{
  float viewport_w = 800;
  float viewport_h = 800;

  editor->pan_x = (viewport_w / 2.0f) - ((editor->document->width * editor->zoom) / 2.0f);
  editor->pan_y = (viewport_h / 2.0f) - ((editor->document->height * editor->zoom) / 2.0f);

  glUseProgram(editor->graphics->pixel_shader);
  glutil_shader_uniform2f(editor->graphics->pixel_shader, PIXEL_UNIFORM_PAN, editor->pan_x, editor->pan_y);
  glUseProgram(0);
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
  glfwSetMouseButtonCallback(window, window_mouse_btn_cb);
  glfwSetCursorPosCallback(window, window_mouse_move_cb);

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
  input_key_event_push(key, scancode, action, mode);
}

void
window_mouse_move_cb(GLFWwindow *window, double x, double y)
{
  input_mouse_event_push(MOUSE_MOVE, x, y, -1, -1);
}

void
window_mouse_btn_cb(GLFWwindow *window, int button, int action, int mods)
{
  double x, y;
  glfwGetCursorPos(window, &x, &y);
  input_mouse_event_push(action == GLFW_PRESS ? MOUSE_DOWN : MOUSE_UP, x, y, button, mods);
}

int main(int argvc, char **argv)
{
  window = window_create(800, 800);

  mouse_buffer = (mouse_event_buffer *)malloc(sizeof(mouse_event_buffer));
  keyboard_buffer = (key_event_buffer *)malloc(sizeof(key_event_buffer));

  editor = pixed_editor_new();

  pixed_document *document = pixed_document_new("Untitled", 16, 16);
  pixed_editor_set_document(document);

  pixed_document_set_pixel(editor->document, 0, 0, 0xff0000ff);

  graphics_init();
  graphics_center_document();

  while(!glfwWindowShouldClose(window))
  {
    glfwPollEvents();

    pixed_editor_dispatch_tool();
    graphics_render();

    glfwSwapBuffers(window);
  }

  pixed_editor_free();
  input_mouse_buffer_free();
  input_keyboard_buffer_free();

  glfwTerminate();
  return 0;
}

