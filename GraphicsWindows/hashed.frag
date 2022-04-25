#version 330 core
layout(location = 0) out vec4 color;
uniform samplerCube tex;
uniform sampler2D rendered;
uniform sampler2D tree;
in vec4 v;
uniform mat4 rot; //transform the view by the rotation

in vec2 texCoord;

float hash( vec2 coord ) {
	return fract( 1.0e4 * sin( 17.0*coord.x + 0.1*coord.y ) *
	( 0.1 + abs( sin( 13.0*coord.y + coord.x ))));
}


void main() {

	vec4 texel = texture(tree, texCoord);
    float hashed = hash(texCoord);
	hashed = clamp(hashed, .000001, 1.0);

	if(texel.a < hashed)
        discard;

	color = texel;
}