#version 330 core

layout (location = 0) in vec2 position;
layout (location = 1) in vec4 color;

uniform float zoom;
uniform vec2  pan;
uniform vec2  viewport;

out vec4 vColor;
out vec2 pixelSize;

void main()
{
  vColor = vec4(color.r, color.g, color.b, 1.0f);

  float width = zoom / (viewport.x / 2);
  float height = zoom / (viewport.y / 2);

  float pan_x = pan.x / (viewport.x / 2);
  float pan_y = pan.y / (viewport.y / 2);

  pixelSize.x = width;
  pixelSize.y = height;

  float x = -1 + (width * position.x) + pan_x;
  float y =  1 - (height * position.y) - pan_y;

  gl_Position = vec4(x, y, 0.0f, 1.0f);
}

