#version 330 core

layout(location=0) in vec3 pos;

uniform mat4 mvp;
uniform mat4 mv;

out vec2 texCoord;
out vec4 v; //view

//vertex shader
void main() {
	gl_Position = mvp * vec4(.75*pos,1);
	v = (-(mv * vec4(0.75*pos,1)));

	texCoord = ((-pos + 1)/2).xy;

}
