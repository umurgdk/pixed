#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

static const char shader_template[] = "const char *$NAME = \"$CODE\";";

static char *valid_shader_extensions[] = {
  "vert",
  "geom",
  "frag"
};

int is_valid_extension(char *extension)
{
  int i = 0;
  for (; i < 3; i++) {
    if (strcmp(extension, valid_shader_extensions[i]) == 0)
      return 1;
  }

  return 0;
}

char *read_file(const char *file_name)
{
  FILE *file = fopen(file_name, "r");
  if (!file)
    return 0;

  fseek(file, 0, SEEK_END);
  int len = ftell(file);
  fseek(file, SEEK_SET, 0);

  char *buffer = (char *)malloc(sizeof(char) * (len + 1));
  fread(buffer, len + 1, 1, file);
  buffer[len] = '\0';

  fclose(file);

  return buffer;
}

void str_replace(char **str_ptr, const char *search, const char *replace)
{
  char *str = *str_ptr;
  size_t str_size = strlen(str);
  size_t search_size = strlen(search);
  size_t replace_size = strlen(replace);

  size_t found = 0;

  char *pos = str;
  char *rest_pos = str;
  char *last_pos = str + str_size;

  while ((pos = strstr(rest_pos, search)) != 0) {
    found++;
    rest_pos = pos + search_size;
  }

  size_t diff = replace_size - search_size;
  size_t new_str_size = str_size + (found * diff);

  char *new_str = (char *)malloc(sizeof(char) * new_str_size + 1);
  new_str[0] = '\0';

  pos = str;
  rest_pos = str;

  while ((pos = strstr(rest_pos, search)) != 0) {
    strncat(new_str, rest_pos, pos - rest_pos);
    strcat(new_str, replace);
    rest_pos = pos + search_size;

    if (rest_pos > last_pos)
      break;
  }

  if (rest_pos < last_pos) {
    strcat(new_str, rest_pos);
  }

  new_str[new_str_size] = '\0';

  free(str);
  *str_ptr = new_str;
}

void remove_new_lines(char *shader_code)
{
  char *pos = shader_code;
  while ((pos = strchr(pos, '\n')) != 0) {
    *pos = ' ';
  }
}

char *compile_template(const char *shader_name, const char *shader_code)
{
  size_t template_size = strlen(shader_template);
  size_t name_size = strlen(shader_name);
  size_t code_size = strlen(shader_code);
  size_t compiled_size = template_size - 10 + (name_size + code_size);

  char *compiled = (char *)malloc(sizeof(char) * compiled_size);
  compiled[0] = '\0';

  char *name_pos = strstr(shader_template, "$NAME");
  char *name_after_pos = name_pos + 5;
  char *code_pos = strstr(shader_template, "$CODE");
  char *code_after_pos = code_pos + 5;

  strncat(compiled, shader_template, name_pos - shader_template);
  strcat(compiled, shader_name);
  strncat(compiled, name_after_pos, code_pos - name_after_pos);
  strcat(compiled, shader_code);
  strncat(compiled, code_after_pos, shader_template + template_size - code_after_pos);

  return compiled;
}

char *generate_c_file(char **compiled_templates, int length)
{
  char *c_file_content = 0;
  size_t total_bytes = 0;

  int i = 0;
  for (; i < length; i++)
    total_bytes += strlen(compiled_templates[i]);

  total_bytes += length; // new line characters

  c_file_content = (char *)malloc(sizeof(char) * total_bytes);
  c_file_content[0] = '\0';
  if (!c_file_content) {
    perror("c_file_content malloc failed");
    return 0;
  }

  for (i = 0; i < length; i++) {
    strcat(c_file_content, compiled_templates[i]);
    strcat(c_file_content, "\n");
  }

  return c_file_content;
}

int
main(int argc, char **argv)
{
  char *shader_compiles[20] = {0};
  int compiled_shaders = 0;

  const char shader_dir_path[] = "./shaders";
  DIR *shader_dir = opendir(shader_dir_path);
  if (!shader_dir) {
    fprintf(stderr, "ERR: ./shaders directory does not exist\n!");
    return 1;
  }

  struct dirent *entry;
  while ((entry = readdir(shader_dir)) != 0) {
    size_t name_len = strlen(entry->d_name);
    size_t ext_pos = name_len - 4;
    char *ext = entry->d_name + ext_pos;

    if (!(ext_pos > 0 && is_valid_extension(ext))) {
      continue;
    }

    char *shader_name = (char *)malloc(sizeof(char) * name_len + 7);
    strcpy(shader_name, "shader_");
    strcpy(shader_name + 7, entry->d_name);

    *(shader_name + 7 + (ext_pos - 1)) = '_';

    char *shader_path = (char *)malloc(sizeof(char) * (strlen(shader_dir_path) + name_len + 1));
    strcpy(shader_path, shader_dir_path);
    strcat(shader_path, "/");
    strcat(shader_path, entry->d_name);

    char *shader_code = read_file(shader_path);
    str_replace(&shader_code, "\n", "\\n");

    char *template = compile_template(shader_name, shader_code);
    shader_compiles[compiled_shaders++] = template;


    free(shader_code);
    free(shader_name);
    free(shader_path);
  }

  char *c_file_content = generate_c_file(shader_compiles, compiled_shaders);
  printf("%s", c_file_content);

  for (int i = 0; i < compiled_shaders; i++)
    free(shader_compiles[i]);

  free(c_file_content);
  closedir(shader_dir);
}
