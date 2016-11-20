#include <GL/glew.h>

GLuint glutil_shader_compile(const char *, GLenum);

GLuint glutil_shader_compile_prog2(GLuint, GLuint);
GLuint glutil_shader_compile_prog3(GLuint, GLuint, GLuint);

void   glutil_shader_uniform1f(GLuint, const char *, GLfloat);
void   glutil_shader_uniform2f(GLuint, const char *, GLfloat, GLfloat);

void   glutil_debug_cl(unsigned int id, unsigned int category, unsigned int severity, unsigned int length, int _s, const char* message, const void* userParam); 
