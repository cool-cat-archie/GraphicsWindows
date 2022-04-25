#define GL_SILENCE_DEPRECATION
#include <GL/glew.h>
#include <GL/glut.h>
#include "cyTriMesh.h"
#include "Source.h"
#include "cyMatrix.h"
#include "cyAlphaDistribution.h"
#include "cyGl.h"
#include "lodepng.h"

constexpr auto PI = 3.141592653589793238462643383279502884197169;
#define DEG2RAD(deg) ((deg) * PI / 180)
#define GET_LOC(i) ((i) * 4 + 3)
#define DISTRIBUTED_TEX_UNIT 3
#define UNDISTRIBUTED_TEX_UNIT 0

using namespace cy;

cy::GLTextureCubeMap envmap;


GLuint object_buffer;
GLuint plane_buffer;
GLuint enviro_buffer;
GLuint object_vao;
GLuint plane_vao;
GLuint enviro_vao;
TriMesh tm;

GLuint program;
GLSLProgram object_prog;
GLSLProgram plane_prog;
GLSLProgram enviro_prog;

int totalVerts;
int cubeVerts;
Vec3f omega = Vec3f(-1.0f, 1.0f, 1.0f);
cy::GLRenderTexture2D renderBuffer;

cyGLTexture2D diffuse_tex;
cyGLTexture2D spec_tex;
cyGLTexture2D tree_tex;
cyGLTexture2D undistributed;

Matrix4f object_mvp;
Matrix4f object_mv;
Matrix3f object_inverseTranspose;
float object_Ns;
//Vec3f texture_inKa;
//Vec3f texture_inKs;
//Vec3f texture_inKd;
Matrix4f plane_mvp;

int textureWidth;
int textureHeight;

enum class Approach {
	DISTRIBUTED,	
	STOCHASTIC,
	HASHED,
	NORMAL
};

Approach approach = Approach::DISTRIBUTED;

void myDisplay() {

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	//both approaches apply to the same program
	if (approach == Approach::NORMAL || approach == Approach::DISTRIBUTED) {
		//draw the plane
		plane_prog.Bind();
		program = plane_prog.GetID();

		if (approach == Approach::NORMAL) {
			undistributed.Bind(UNDISTRIBUTED_TEX_UNIT);
			plane_prog["tree"] = UNDISTRIBUTED_TEX_UNIT;
		}
		if (approach == Approach::DISTRIBUTED) {
			tree_tex.Bind(DISTRIBUTED_TEX_UNIT);
			plane_prog["tree"] = DISTRIBUTED_TEX_UNIT;
		}

		glBindVertexArray(plane_vao);
		glBindBuffer(GL_ARRAY_BUFFER, plane_buffer);

		glDrawArrays(GL_TRIANGLES,
			0,
			6);
	}
	

	//draw enviro
	glDepthMask(GL_FALSE);
	glBindVertexArray(enviro_vao);
	glBindBuffer(GL_ARRAY_BUFFER, enviro_buffer);
	enviro_prog.Bind();
	program = enviro_prog.GetID();
	envmap.Bind(0);


	glDrawArrays(GL_TRIANGLES,
		0,
		cubeVerts
	);
	glDepthMask(GL_TRUE);



	glutSwapBuffers();
}


/**
 Exits when the escape key is called
 */
void myKeyboard(unsigned char key, int x, int y) {
	if (key == 27)
		exit(0);
	else if (key == 49) {
		std::cout << "1 key pressed: using traditional Alpha Testing" << std::endl;
		approach = Approach::NORMAL;
	}
	else if (key == 50) {
		std::cout << "2 key pressed: using Alpha Distribution" << std::endl;
		approach = Approach::DISTRIBUTED;
	}
	else if (key == 51) {
		std::cout << "3 key pressed: using Stochastic Alpha Testing" << std::endl;
		approach = Approach::STOCHASTIC;
	}
	else if (key == 52) {
		std::cout << "4 key pressed: using Hashed Alpha Testing" << std::endl;
		approach = Approach::HASHED;
	}
	glutPostRedisplay();
}

void myIdle() {
}

int mouseButton;
void myMouse(
	int button, int state,
	int x, int y)
{
	mouseButton = button;

}



float fovText = 45;
float fovPlane = 45;
double rotationX = 0;
double rotationY = 0;
void myMouseMotion(int x, int y) {

	static int oldX;
	static int oldY;
	static int oldVertPlane;

	if (mouseButton == GLUT_LEFT_BUTTON) {
		int mod = glutGetModifiers();
		if (mod == GLUT_ACTIVE_CTRL) {
			int diffVert = y - oldVertPlane;
			fovPlane -= /*(45.0 / float(glutGet(GLUT_SCREEN_HEIGHT))) */ diffVert;
			if (fovPlane < 1)
				fovPlane = 1;
			/*if (fovPlane > 45)
				fovPlane = 45;*/
				/*cy::Matrix4f rotMatrix =
					cy::Matrix4f::RotationX(rotationX) * cy::Matrix4f::RotationY(rotationY);*/

			cy::Matrix4f view = cy::Matrix4f::View(Vec3f(0.0f, 1.0f, 3.0f),
				Vec3f(0.0f, 0.0f, 0.0f),
				Vec3f(0.0f, 1.0f, 0.0f));
			cy::Matrix4f projMatrix =
				cy::Matrix4f::Perspective(
					DEG2RAD(fovPlane),
					float(glutGet(GLUT_SCREEN_WIDTH)) / float(glutGet(GLUT_SCREEN_HEIGHT)),
					0.1f,
					1000.0f);
			plane_mvp = projMatrix * view;//* rotMatrix;
			plane_prog["mvp"] = plane_mvp;
			oldVertPlane = y;

		}
		int diffX = x - oldX;
		int diffY = y - oldY;
		rotationX += ((2 * PI) / glutGet(GLUT_SCREEN_HEIGHT)) * diffY;
		rotationY -= ((2 * PI) / glutGet(GLUT_SCREEN_WIDTH)) * diffX;
		cy::Matrix4f rotMatrix =
			cy::Matrix4f::RotationX(rotationX) * cy::Matrix4f::RotationY(rotationY);


		if (mod == GLUT_ACTIVE_ALT) {
			cy::Matrix4f view = cy::Matrix4f::View(Vec3f(0.0f, 1.0f, 3.0f),
				Vec3f(0.0f, 0.0f, 0.0f),
				Vec3f(0.0f, 1.0f, 0.0f));
			cy::Matrix4f projMatrix =
				cy::Matrix4f::Perspective(
					DEG2RAD(fovPlane),
					float(glutGet(GLUT_SCREEN_WIDTH)) / float(glutGet(GLUT_SCREEN_HEIGHT)),
					0.1f,
					1000.0f);
			plane_mvp = projMatrix * view * rotMatrix;
			plane_prog["mvp"] = plane_mvp;
			plane_prog["mv"] = view * rotMatrix;

			Matrix3f invTrans = Matrix3f(view * rotMatrix);
			invTrans.Invert();
			invTrans.Transpose();
			plane_prog["inverseTranspose"] = invTrans;

		}
		else if (mod != GLUT_ACTIVE_ALT && mod != GLUT_ACTIVE_CTRL) {
			cy::Matrix4f view = cy::Matrix4f::View(Vec3f(0.0f, 1.0f, 3.0f),
				Vec3f(0.0f, 0.0f, 0.0f),
				Vec3f(0.0f, 1.0f, 0.0f));
			cy::Matrix4f projMatrix =
				cy::Matrix4f::Perspective(
					DEG2RAD(fovPlane),
					float(glutGet(GLUT_SCREEN_WIDTH)) / float(glutGet(GLUT_SCREEN_HEIGHT)),
					0.1f,
					1000.0f);
			/* plane_mvp = projMatrix * view * rotMatrix;
			 plane_prog["mvp"] = plane_mvp;
			 plane_prog["mv"] = view * rotMatrix;*/

			 //same for environment map
			enviro_prog["mvp"] = projMatrix * view;
			enviro_prog["rot"] = rotMatrix;


			/*Matrix3f invTrans = Matrix3f(view * rotMatrix);
			invTrans.Invert();
			invTrans.Transpose();
			plane_prog["inverseTranspose"] = invTrans;*/
		}
	}
	else if (mouseButton == GLUT_RIGHT_BUTTON) {
		int mod = glutGetModifiers();
		if (mod == GLUT_ACTIVE_ALT) {
			int diffVert = y - oldVertPlane;
			fovPlane -= (45.0 / float(glutGet(GLUT_SCREEN_HEIGHT))) * diffVert;
			if (fovPlane < 1)
				fovPlane = 1;
			if (fovPlane > 45)
				fovPlane = 45;
			cy::Matrix4f rotMatrix =
				cy::Matrix4f::RotationY(rotationY) * cy::Matrix4f::RotationX(rotationX);

			cy::Matrix4f view = cy::Matrix4f::View(Vec3f(0.0f, 1.0f, 3.0f),
				Vec3f(0.0f, 0.0f, 0.0f),
				Vec3f(0.0f, 1.0f, 0.0f));
			cy::Matrix4f projMatrix =
				cy::Matrix4f::Perspective(
					DEG2RAD(fovPlane),
					float(glutGet(GLUT_SCREEN_WIDTH)) / float(glutGet(GLUT_SCREEN_HEIGHT)),
					0.1f,
					1000.0f);
			plane_mvp = projMatrix * view * rotMatrix;
			plane_prog["mvp"] = plane_mvp;
			oldVertPlane = y;

		}
		else {
			/* static int oldVertText;
			 int diffVert = y - oldVertText;
			 fovText -= (45.0 / float(glutGet(GLUT_SCREEN_HEIGHT))) * diffVert;
			 if (fovText < 1)
				 fovText = 1;
			 if (fovText > 45)
				 fovText = 45;
			 cy::Matrix4f rotMatrix =
				 cy::Matrix4f::RotationY(rotationY) * cy::Matrix4f::RotationX(rotationX);

			 cy::Matrix4f view = cy::Matrix4f::View(Vec3f(0.0f, 1.0f, 3.0f),
				 Vec3f(0.0f, 0.0f, 0.0f),
				 Vec3f(0.0f, 1.0f, 0.0f));
			 cy::Matrix4f projMatrix =
				 cy::Matrix4f::Perspective(
					 DEG2RAD(fovText),
					 float(glutGet(GLUT_SCREEN_WIDTH)) / float(glutGet(GLUT_SCREEN_HEIGHT)),
					 0.1f,
					 1000.0f);
			 tm.ComputeBoundingBox();
			 assert(tm.IsBoundBoxReady());
			 Vec3f min = tm.GetBoundMin();
			 Vec3f max = tm.GetBoundMax();
			 Vec3f mid = -(min + max) / 2.0f;
			 cy::Matrix4f trans = cy::Matrix4f::Translation(mid * 0.03);
			 object_mvp = projMatrix * view * rotMatrix * trans;
			 object_prog["mvp"] = object_mvp;

			 oldVertText = y;

			 Matrix3f inverseTranspose = Matrix3f(view * rotMatrix * trans);
			 inverseTranspose.Invert();
			 inverseTranspose.Transpose();
			 object_prog["inverseTranspose"] = inverseTranspose;*/
		}
	}
	oldX = x;
	oldY = y;
	glutPostRedisplay();
}

std::pair<int, int> sendTextureToFragmentShader(cyGLTexture2D* tex, std::string filename, std::string varName, int texUnit) {
	//texture stuff
	tex->Initialize();
	std::vector<unsigned char> image; //the raw pixels
	unsigned width, height;

	//decode
	unsigned error = lodepng::decode(image, width, height, filename);
	//if there's an error, display it
	if (error) std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;

	tex->SetImage(image.data(), 4, width, height);
	tex->BuildMipmaps();
	//tex->Bind(texUnit);
	//plane_prog[varName.data()] = texUnit;
	return { width, height };
}

void createPlane() {
	std::vector<Vec3f> vertexes;
	vertexes.push_back(Vec3f(-1, 1, 0)); //first triangle
	vertexes.push_back(Vec3f(1, 1, 0));
	vertexes.push_back(Vec3f(-1, -1, 0));

	vertexes.push_back(Vec3f(1, 1, 0)); //second triangle
	vertexes.push_back(Vec3f(-1, -1, 0));
	vertexes.push_back(Vec3f(1, -1, 0));

	//vertexes.push_back(Vec3f(-2, -.6, 2));
	//vertexes.push_back(Vec3f(2, -.6, 2));
	//vertexes.push_back(Vec3f(-2, -.6, -2));

	//vertexes.push_back(Vec3f(2, -.6, 2));
	//vertexes.push_back(Vec3f(-2, -.6, -2));
	//vertexes.push_back(Vec3f(2, -.6, -2));

	Vec3f* planeVerts = vertexes.data();

	glGenVertexArrays(1, &plane_vao);
	glBindVertexArray(plane_vao);

	glGenBuffers(1, &plane_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, plane_buffer);
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(Vec3f) * 6, /*there are 6 vertexes*/
		planeVerts,
		GL_STATIC_DRAW);

	program = plane_prog.GetID();
	GLuint pos = glGetAttribLocation(
		program, "pos");
	glEnableVertexAttribArray(pos);
	glVertexAttribPointer(
		pos, 3, GL_FLOAT,
		GL_FALSE, sizeof(Vec3f), (GLvoid*)0);

	cy::Matrix4f view = cy::Matrix4f::View(Vec3f(0.0f, 0.0f, 3.0f),
		Vec3f(0.0f, 0.0f, 0.0f),
		Vec3f(0.0f, 1.0f, 0.0f));
	cy::Matrix4f projMatrix =
		cy::Matrix4f::Perspective(
			DEG2RAD(100),
			float(glutGet(GLUT_SCREEN_WIDTH)) / float(glutGet(GLUT_SCREEN_HEIGHT)),
			0.1f,
			1000.0f);

	plane_mvp = projMatrix * view;
	plane_prog["mvp"] = plane_mvp;



}

void initEnviroMap() {
	enviro_prog.BuildFiles("enviroVert.vert", "enviroFrag.frag");

	envmap.Initialize();
	std::string files[6] = { "cubemap_posx.png",
	"cubemap_negx.png",  "cubemap_posy.png" , "cubemap_negy.png",
	"cubemap_posz.png", "cubemap_negz.png" };

	/* std::string files[6] = { "posx.png",
	 "negx.png",  "posy.png" , "negy.png",
	 "posz.png", "negz.png" };*/

	for (int i = 0; i < 6; ++i) {
		// load image from file
		std::vector<unsigned char> image; //the raw pixels
		unsigned width, height;

		//decode
		unsigned error = lodepng::decode(image, width, height, files[i]);
		//if there's an error, display it
		if (error) std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;

		// set image data
		envmap.SetImageRGBA(
			(cy::GLTextureCubeMap::Side)i,
			image.data(), width, height);
	}
	envmap.BuildMipmaps();
	envmap.SetSeamless();
	enviro_prog["env"] = 0;

	TriMesh trimesh;
	trimesh.LoadFromFileObj("cube.obj");

	std::vector<Vec3f> objData;
	for (int i = 0; i < trimesh.NF(); i++) {
		cy::TriMesh::TriFace  face = trimesh.F(i);

		//face.v[?] gives the index of some vertex
		objData.push_back(trimesh.V(face.v[0]));
		objData.push_back(trimesh.V(face.v[1]));
		objData.push_back(trimesh.V(face.v[2]));
		cubeVerts += 3;
	}


	Vec3f* vertexes = objData.data();

	glGenVertexArrays(1, &enviro_vao);
	glBindVertexArray(enviro_vao);

	glGenBuffers(1, &enviro_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, enviro_buffer);
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(Vec3f) * cubeVerts,
		vertexes,
		GL_STATIC_DRAW);

	program = enviro_prog.GetID();
	GLuint pos = glGetAttribLocation(
		program, "pos");
	glEnableVertexAttribArray(pos);
	glVertexAttribPointer(
		pos, 3, GL_FLOAT,
		GL_FALSE, sizeof(Vec3f), (GLvoid*)0);

	cy::Matrix4f view = cy::Matrix4f::View(Vec3f(0.0f, 0.0f, 3.0f),
		Vec3f(0.0f, 0.0f, 0.0f),
		Vec3f(0.0f, 1.0f, 0.0f));
	cy::Matrix4f projMatrix =
		cy::Matrix4f::Perspective(
			DEG2RAD(40),
			float(glutGet(GLUT_SCREEN_WIDTH)) / float(glutGet(GLUT_SCREEN_HEIGHT)),
			0.1f,
			1000.0f);

	Matrix4f envro_mvp = projMatrix * view;
	enviro_prog["mvp"] = envro_mvp;
	enviro_prog["rot"] = view.Identity();

}

void clampedError(int index, int error, unsigned char* image) {
	image[index] += error;
	if (image[index] < 0) {
		image[index] = 0;
	}
	else if (image[index] > 255) {
		image[index] = 255;
	}
}

void setWidth(int level, int* width) {
	glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_WIDTH, width);
}
void setHeight(int level, int* height) {
	glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_WIDTH, height);
}

void distributeError(int iw, int width, int i, int portion0, unsigned char* image, int ih, int height, int portion1, int portion2, int portion3) {
	if (iw < width - 1)
		clampedError(GET_LOC(i + 1), portion0, image);
	if (ih < height - 1) {
		if (iw > 0)
			clampedError(GET_LOC(width + i - 1), portion1, image);

		clampedError(GET_LOC(width + i), portion2, image);

		if (iw < width - 1)
			clampedError(GET_LOC(width + i + 1), portion3, image);
	}
}

//Error Distribution implementation
void AlphaDistribute() {
	int mipMapLevel = 0;
	int height = 0;
	int width = 0;
	int textureId = tree_tex.GetID();
	tree_tex.Bind(DISTRIBUTED_TEX_UNIT);
	setWidth(mipMapLevel, &width);
	setHeight(mipMapLevel, &height);
	for (mipMapLevel = 0; width >= 1 && height >= 1; mipMapLevel++,
		//every iteration change the width and height considered
		setWidth(mipMapLevel, &width),
		setHeight(mipMapLevel, &height)) {
		std::vector<unsigned char> imageV(width * height * 4);
		unsigned char* image = imageV.data();
		glGetTexImage(GL_TEXTURE_2D, mipMapLevel, GL_RGBA, GL_UNSIGNED_BYTE, imageV.data());
		for (int i = 0, ih = 0; ih < height; ih++) {
			for (int iw = 0; iw < width; iw++, i++) {
				int originalAlpha = image[GET_LOC(i)];
				int quantizedAlpha = 0;
				//compare to the threshold
				if (originalAlpha >= 128)
					quantizedAlpha = 255;

				image[GET_LOC(i)] = quantizedAlpha;
				int quantizationError = originalAlpha - quantizedAlpha;

				//calculate each portion of the error to distribute
				int portion0 = 7 * quantizationError / 16;
				int portion1 = 3 * quantizationError / 16;
				int portion2 = 5 * quantizationError / 16;
				int portion3 = quantizationError / 16;

				int summedPortions = portion0 + portion1 + portion2 + portion3;
				int difference = quantizationError - summedPortions;
				portion0 += difference;

				distributeError(iw, width, i, portion0, image, ih, height, portion1, portion2, portion3);

			}
		}
		if (width * height == 1) {
			image[3] = 255;
			return;
		}

		glTexImage2D(GL_TEXTURE_2D, mipMapLevel, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageV.data());
	}


}

int main(int argc, char** argv) {
	char* objName = argv[1];
	/*std::string mtlName(objName);
	mtlName.replace(mtlName.length() - 4, 4, ".mtl");*/

	glutInit(&argc, argv);


	glutInitWindowSize(1080, 1080);
	glutInitWindowPosition(40, 40);
	glutCreateWindow("Project");
	glEnable(GL_DEPTH_TEST);
	glClearColor(0, 0, 0, 0);
	glViewport(0, 0, glutGet(GLUT_SCREEN_WIDTH), glutGet(GLUT_SCREEN_HEIGHT));


	glewInit();
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutDisplayFunc(myDisplay);
	glutKeyboardFunc(myKeyboard);
	glutIdleFunc(myIdle);
	glutMotionFunc(myMouseMotion);
	glutMouseFunc(myMouse);

	initEnviroMap();


	//Create VAO/VBO
	glGenVertexArrays(1, &object_vao);
	glBindVertexArray(object_vao);


	tm.LoadFromFileObj(objName);
	std::vector<Vec3f> triangleVerts;

	//iterate through the faces
	for (int i = 0; i < tm.NF(); i++) {
		cy::TriMesh::TriFace  face = tm.F(i);
		cy::TriMesh::TriFace  faceText = tm.FT(i);

		//face.v[?] gives the index of some vertex
		triangleVerts.push_back(tm.V(face.v[0]));
		triangleVerts.push_back(tm.VN(face.v[0]));
		triangleVerts.push_back(tm.VT(faceText.v[0]));

		triangleVerts.push_back(tm.V(face.v[1]));
		triangleVerts.push_back(tm.VN(face.v[1]));
		triangleVerts.push_back(tm.VT(faceText.v[1]));

		triangleVerts.push_back(tm.V(face.v[2]));
		triangleVerts.push_back(tm.VN(face.v[2]));
		triangleVerts.push_back(tm.VT(faceText.v[2]));

		totalVerts += 3;
	}
	Vec3f* verts = triangleVerts.data();

	glGenBuffers(1, &object_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, object_buffer);
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(Vec3f) * totalVerts * 3,
		verts,
		GL_STATIC_DRAW);

	//compile the shaders
	object_prog.BuildFiles("shader.vert",
		"shader.frag");
	plane_prog.BuildFiles("vertexPlane.vert",
		"fragmentPlane.frag" /*"stochastic.frag"*/ /*"hashed.frag"*/);
	object_prog.Bind();
	program = object_prog.GetID();


	cy::Matrix4f view = cy::Matrix4f::View(Vec3f(0.0f, 0.0f, 3.0f),
		Vec3f(0.0f, 0.0f, 0.0f),
		Vec3f(0.0f, 1.0f, 0.0f));
	cy::Matrix4f projMatrix =
		cy::Matrix4f::Perspective(
			DEG2RAD(fovText),
			float(glutGet(GLUT_SCREEN_WIDTH)) / float(glutGet(GLUT_SCREEN_HEIGHT)),
			0.1f,
			1000.0f);
	tm.ComputeBoundingBox();
	assert(tm.IsBoundBoxReady());
	Vec3f min = tm.GetBoundMin();
	Vec3f max = tm.GetBoundMax();
	Vec3f mid = -(min + max) / 2.0f;
	cy::Matrix4f trans = cy::Matrix4f::Translation(mid * 0.05);

	object_mvp = projMatrix * view * trans;
	object_prog["mvp"] = object_mvp;
	object_mv = view * trans;
	object_prog["mv"] = object_mv;

	//set mv for plane as well
	plane_prog["mv"] = view;

	object_prog["rot"] = object_mvp.Identity();
	plane_prog["rot"] = object_mvp.Identity();


	object_inverseTranspose = Matrix3f(view);
	object_inverseTranspose.Invert();
	object_inverseTranspose.Transpose();
	object_prog["inverseTranspose"] = object_inverseTranspose;

	GLuint pos = glGetAttribLocation(
		program, "pos");
	glEnableVertexAttribArray(pos);
	glVertexAttribPointer(
		pos, 3, GL_FLOAT,
		GL_FALSE, sizeof(Vec3f) * 3, (GLvoid*)0);
	GLuint clr = glGetAttribLocation(
		program, "clr");
	glEnableVertexAttribArray(clr);
	glVertexAttribPointer(
		clr, 3, GL_FLOAT,
		GL_FALSE, sizeof(Vec3f) * 3, (GLvoid*)(sizeof(Vec3f)));
	GLuint txc = glGetAttribLocation(
		program, "txc");
	glEnableVertexAttribArray(txc);
	glVertexAttribPointer(
		txc, 3, GL_FLOAT,
		GL_FALSE, sizeof(Vec3f) * 3, (GLvoid*)(2 * sizeof(Vec3f)));

	object_prog["Ns"] = 20;
	object_prog["omega"] = omega;


	createPlane();
	std::pair<int, int> dimensions = sendTextureToFragmentShader(&tree_tex, "LushPine.png", "tree", DISTRIBUTED_TEX_UNIT);
	AlphaDistribute();

	sendTextureToFragmentShader(&undistributed, "LushPine.png", "tree", UNDISTRIBUTED_TEX_UNIT);


	textureWidth = dimensions.first;
	textureHeight = dimensions.second;

	glutMainLoop();

	return 0;
}