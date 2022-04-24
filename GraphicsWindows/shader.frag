#version 330 core
layout(location = 0) out vec4 color;
//uniform sampler2D tex;
//uniform sampler2D specTex;
//uniform vec3 inKa;
//uniform vec3 inKs;
//uniform vec3 inKd;

uniform samplerCube env;
uniform float Ns;
uniform vec3 omega;

in vec2 texCoord;

uniform mat4 rot; //transform the view by the rotation

in vec3 vcolor;
in vec4 v;

void main() {
	vec4 Kr = vec4(.2,.2,.2,1);

	vec3 Kd = vec3(.2,.2,.2); //texture( tex, texCoord ).rgb;
	vec3 normal = vcolor;
	vec3 I = vec3(1,1,1);


	//compute omega_r
	vec3 omega_r =	reflect(-(v.xyz), normal);
	vec3 rotated = mat3(rot) * omega_r;
	vec4 reflective_color = Kr * texture( env, rotated );

	
	//computing ambient light
	vec3 Ia = vec3(.2,.2,.2);
	//vec3 Ka = vec3(1,1,0); //texture( tex, texCoord ).rgb;
	vec3 ambient = Ia /* Ka*/;

	//compute diffuse
	float costheta = dot(normal, omega);
	vec3 diffuse = (I * costheta) * Kd;

	//compute specular
	float alpha = Ns;
	vec3 Ks = vec3(.2,.2,.2); //texture( specTex, texCoord ).rgb;
	vec3 h = normalize(omega + v.xyz);
	float cosphi = dot(normal, h);
	vec3 specular = I * Ks*(pow(cosphi, alpha));

	vec3 blinn = diffuse + specular + ambient;

	if(costheta <= 0)
		color = vec4(ambient, 1) + reflective_color;
	else
		color = vec4(blinn, 1) + reflective_color ;
	
	//color = texture( env, rotated );
	//color = vec4(1,0,0,1);
}