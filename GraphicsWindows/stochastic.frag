#version 330 core
layout(location = 0) out vec4 color;
uniform samplerCube tex;
uniform sampler2D rendered;
uniform sampler2D tree;
in vec4 v;
uniform mat4 rot; //transform the view by the rotation

in vec2 texCoord;

//random function from https://thebookofshaders.com/10/
float random (vec2 st) {
    return fract(sin(dot(st.xy,
                         vec2(12.9898,78.233)))*
        43758.5453123);
}

void main() {

	vec4 texel = texture(tree, texCoord);
    float rand_num = random(texCoord);
    rand_num = clamp(rand_num, .000001, 1.0);
	if(texel.a < rand_num)
        discard;

	color = texel;
}