#version 330 core
layout(location = 0) out vec4 color;
uniform samplerCube tex;
uniform sampler2D rendered;
uniform sampler2D tree;
in vec4 v;
uniform mat4 rot; //transform the view by the rotation

in vec2 texCoord;


void main() {
	/*vec3 normal = vec3(0,1,0);
	vec3 omega_r =	reflect(-(v.xyz), normal);
	vec3 rotated = mat3(rot) * omega_r;
	color = texture(rendered, texCoord);
	if(color.a == 0){
		color = texture( tex, rotated);
	}*/

	color = texture(tree, texCoord);
	if(color.a <= .5)
        discard;

	//color = vec4(1,0, 0, 1);
}