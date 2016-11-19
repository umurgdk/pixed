#include <stdio.h>

#include "libglutil.h"

void _check_shader_link(GLuint);

inline
GLuint
glutil_shader_compile(const char *source, GLenum shader_type)
{
  GLuint shader = glCreateShader(shader_type);
  glShaderSource(shader, 1, &source, 0);
  glCompileShader(shader);

  GLint success;
  GLchar infoLog[512];
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

  if (!success) {
    glGetShaderInfoLog(shader, 512, 0, infoLog);
    printf("libglutil::shader_compile::error %s\n", infoLog);
  }

  return shader;
}

GLuint
glutil_shader_compile_prog2(GLuint vertex_shader, GLuint fragment_shader)
{
  GLuint program = glCreateProgram();

  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);

  _check_shader_link(program);

  return program;
}

GLuint
glutil_shader_compile_prog3(GLuint vertex_shader, GLuint fragment_shader, GLuint geometry_shader)
{
  GLuint program = glCreateProgram();

  glAttachShader(program, vertex_shader);
  glAttachShader(program, geometry_shader);
  glAttachShader(program, fragment_shader);

  glLinkProgram(program);

  _check_shader_link(program);

  return program;
}

inline
void
glutil_shader_uniform1f(GLuint shader, const char *name, GLfloat x)
{
  GLuint location = glGetUniformLocation(shader, name);
  glUniform1f(location, x);
}

inline
void
glutil_shader_uniform2f(GLuint shader, const char *name, GLfloat x, GLfloat y)
{
  GLuint location = glGetUniformLocation(shader, name);
  glUniform2f(location, x, y);
}

static void APIENTRY glutil_debug_cl(unsigned int id, unsigned int category, unsigned int severity, unsigned int length, int _s, const char* message, const void* userParam) {
  GLenum source = category;
  GLenum type = category;
  char * msg = message;
  char * sourceString;
  char * typeString;
  char * severityString;

  // The AMD variant of this extension provides a less detailed classification of the error,
  // which is why some arguments might be "Unknown".
  switch (source) {
      case GL_DEBUG_CATEGORY_API_ERROR_AMD:
      case GL_DEBUG_SOURCE_API: {
          sourceString = "API";
          break;
      }
      case GL_DEBUG_CATEGORY_APPLICATION_AMD:
      case GL_DEBUG_SOURCE_APPLICATION: {
          sourceString = "Application";
          break;
      }
      case GL_DEBUG_CATEGORY_WINDOW_SYSTEM_AMD:
      case GL_DEBUG_SOURCE_WINDOW_SYSTEM: {
          sourceString = "Window System";
          break;
      }
      case GL_DEBUG_CATEGORY_SHADER_COMPILER_AMD:
      case GL_DEBUG_SOURCE_SHADER_COMPILER: {
          sourceString = "Shader Compiler";
          break;
      }
      case GL_DEBUG_SOURCE_THIRD_PARTY: {
          sourceString = "Third Party";
          break;
      }
      case GL_DEBUG_CATEGORY_OTHER_AMD:
      case GL_DEBUG_SOURCE_OTHER: {
          sourceString = "Other";
          break;
      }
      default: {
          sourceString = "Unknown";
          break;
      }
  }

  switch (type) {
      case GL_DEBUG_TYPE_ERROR: {
          typeString = "Error";
          break;
      }
      case GL_DEBUG_CATEGORY_DEPRECATION_AMD:
      case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: {
          typeString = "Deprecated Behavior";
          break;
      }
      case GL_DEBUG_CATEGORY_UNDEFINED_BEHAVIOR_AMD:
      case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: {
          typeString = "Undefined Behavior";
          break;
      }
      case GL_DEBUG_TYPE_PORTABILITY_ARB: {
          typeString = "Portability";
          break;
      }
      case GL_DEBUG_CATEGORY_PERFORMANCE_AMD:
      case GL_DEBUG_TYPE_PERFORMANCE: {
          typeString = "Performance";
          break;
      }
      case GL_DEBUG_CATEGORY_OTHER_AMD:
      case GL_DEBUG_TYPE_OTHER: {
          typeString = "Other";
          break;
      }
      default: {
          typeString = "Unknown";
          break;
      }
  }

  switch (severity) {
      case GL_DEBUG_SEVERITY_HIGH: {
          severityString = "High";
          break;
      }
      case GL_DEBUG_SEVERITY_MEDIUM: {
          severityString = "Medium";
          break;
      }
      case GL_DEBUG_SEVERITY_LOW: {
          severityString = "Low";
          break;
      }
      default: {
          severityString = "Unknown";
          break;
      }
  }

  printf("OpenGL Error: %s [Source = %s, Type = %s, Severity = %s, ID = %ud]\n",
      msg, sourceString, typeString, severityString, id);

  fflush(stdout);
}

inline
void 
_check_shader_link(GLuint program)
{
  GLint success;
  GLchar infoLog[512];

  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(program, 512, 0, infoLog);
    printf("libglutil::shader_link::error %s\n", infoLog);
  }
}
