#version 330 core

layout(location=0) in vec3 pos;

uniform mat4 mvp;
uniform mat4 rot;
out vec3 dir;

//vertex shader
void main() {
	gl_Position = mvp*vec4(pos,1);
	dir = (rot*vec4(pos,1)).xyz;
}
