#version 330 core

layout(location=0) in vec3 pos;
layout(location=1) in vec3 clr;
layout(location=2) in vec3 txc;
uniform mat4 mvp;
out vec2 texCoord;


uniform mat4 mv;
uniform mat3 inverseTranspose;
out vec3 vcolor;
out vec4 v; //view


uniform int target;

//vertex shader
void main() {
	if(target == 0){
		gl_Position = mvp * vec4(0.05*pos,1);
		texCoord = vec2(txc);


		vcolor = inverseTranspose * clr;
		v = (-(mv * vec4(0.05*pos,1)));
	}
	//want texture to have a bigger teapot
	else{
		gl_Position = mvp * vec4(.062*pos,1);
		texCoord = vec2(txc);


		vcolor = inverseTranspose * clr;
		v = (-(mv * vec4(.062*pos,1)));
	}
}
