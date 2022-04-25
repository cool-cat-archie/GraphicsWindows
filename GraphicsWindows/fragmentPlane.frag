#version 330 core
layout(location = 0) out vec4 color;
uniform sampler2D tree;
in vec4 v;
uniform mat4 rot; //transform the view by the rotation

in vec2 texCoord;


void main() {

	vec4 texel = texture(tree, texCoord);
	if(texel.a < .5)
        discard;

	color = texel;
}