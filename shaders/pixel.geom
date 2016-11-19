#version 330 core

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

in vec4 vColor[];
in vec2 pixelSize[];

out vec4 gColor;

void main()
{
  gColor = vColor[0];

  float width = pixelSize[0].x;
  float height = pixelSize[0].y;

  gl_Position = gl_in[0].gl_Position + vec4(width, 0, 0, 0);
  EmitVertex();

  gl_Position = gl_in[0].gl_Position;
  EmitVertex();

  gl_Position = gl_in[0].gl_Position + vec4(width, -height, 0.0f, 0.0f);
  EmitVertex();

  gl_Position = gl_in[0].gl_Position + vec4(0, -height, 0.0f, 0.0f);
  EmitVertex();

  EndPrimitive();
}
