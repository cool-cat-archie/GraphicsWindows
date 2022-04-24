#define GL_SILENCE_DEPRECATION
#include <GL/glew.h>
#include <GL/glut.h>
#include "cyTriMesh.h"
#include "Source.h"
#include "cyMatrix.h"
#include "cyGl.h"
#include "lodepng.h"

constexpr auto PI = 3.141592653589793238462643383279502884197169;
#define DEG2RAD(deg) ((deg) * PI / 180)

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

void myDisplay() {
    
    renderBuffer.Bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //draw object
    object_prog.Bind();
    program = object_prog.GetID();
    envmap.Bind(0);
    
    object_prog["target"] = 1; //means rendering texture

    object_prog["env"] = 0;
    glBindVertexArray(object_vao);
    glBindBuffer(GL_ARRAY_BUFFER, object_buffer);

    glEnable(GL_DEPTH_TEST);

    glDrawArrays(GL_TRIANGLES,
        0,
        totalVerts);


    renderBuffer.Unbind();
    renderBuffer.BuildTextureMipmaps();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    object_prog["target"] = 0;

    /*glDrawArrays(GL_TRIANGLES,
        0,
        totalVerts);*/


    //also draw the plane
    plane_prog.Bind();
    program = plane_prog.GetID();
    renderBuffer.BindTexture(2);
    plane_prog["rendered"] = 2;

    //tree_tex.Bind(3);
    plane_prog["tree"] = 3;


    glBindVertexArray(plane_vao);
    glBindBuffer(GL_ARRAY_BUFFER, plane_buffer);

    glDrawArrays(GL_TRIANGLES,
        0,
        6);

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

    /*
    renderBuffer.Bind();

    glClearColor(.3, .3, .3, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    object_prog.Bind();
    program = object_prog.GetID();
    envmap.Bind(0);
    object_prog["env"] = 0;
    glBindVertexArray(object_vao);
    glBindBuffer(GL_ARRAY_BUFFER, object_buffer);


    glEnable(GL_DEPTH_TEST);

    glDrawArrays(GL_TRIANGLES,
        0,
        totalVerts);

    renderBuffer.Unbind();
    renderBuffer.BuildTextureMipmaps();


    plane_prog.Bind();
    program = plane_prog.GetID();

    glBindVertexArray(plane_vao);
    glBindBuffer(GL_ARRAY_BUFFER, plane_buffer);
    renderBuffer.BindTexture(2);

    plane_prog["rendered"] = 2;
    glClearColor(0, 0, 0, 1);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES,
        0,
        6);*/

    glutSwapBuffers();
}


/**
 Exits when the escape key is called
 */
void myKeyboard(unsigned char key, int x, int y) {
    if (key == 27)
        exit(0);
    else if (key == GLUT_KEY_F6) {
        std::cout << "recompiling shaders" << std::endl;

        //compile the shaders
        object_prog.BuildFiles("shader.vert",
            "shader.frag");
        object_prog.Bind();
        program = object_prog.GetID();
    }

}

void myIdle() {
    //glutPostRedisplay();
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
        else if(mod != GLUT_ACTIVE_ALT && mod != GLUT_ACTIVE_CTRL){
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

std::pair<int,int> sendTextureToFragmentShader(cyGLTexture2D* tex, std::string filename, std::string varName, int texUnit) {
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
    //object_prog[varName.data()] = texUnit;
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
            DEG2RAD(45),
            float(glutGet(GLUT_SCREEN_WIDTH)) / float(glutGet(GLUT_SCREEN_HEIGHT)),
            0.1f,
            1000.0f);

    plane_mvp = projMatrix * view;
    plane_prog["mvp"] = plane_mvp;

    //will be bound to tecture unit 0
    plane_prog["tex"] = 0;


}

void initEnviroMap() {
    enviro_prog.BuildFiles("enviroVert.vert", "enviroFrag.frag");

    envmap.Initialize();
    std::string files[6] = {"cubemap_posx.png",
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
        "fragmentPlane.frag");
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
        GL_FALSE, sizeof(Vec3f) * 3, (GLvoid*)(2*sizeof(Vec3f)));

    /*int matIndex = tm.GetMaterialIndex(0);
    cy::TriMesh::Mtl mtl = tm.M(matIndex);*/

    //std::string filename = mtl.map_Ka.data; 
    std::pair<int,int> dimensions = sendTextureToFragmentShader(&diffuse_tex, "brick.png", "tex", 0);
    textureWidth = dimensions.first;
    textureHeight = dimensions.second;

    /*filename = mtl.map_Ks;
    sendTextureToFragmentShader(&spec_tex, filename, "specTex", 1);*/


    //texture_inKa = Vec3f(mtl.Ka);
    //texture_inKd = Vec3f(mtl.Kd);
    //texture_inKs = Vec3f(mtl.Ks);
    //texture_Ns = mtl.Ns;


    //texture_prog["inKa"] = texture_inKa;
    //texture_prog["inKd"] = texture_inKd;
    //texture_prog["inKs"] = texture_inKs;
    object_prog["Ns"] = 20;
    object_prog["omega"] = omega;

    
    renderBuffer.Initialize(
        true, // create depth buffer
        4, // RGBA
        textureWidth, // width
        textureHeight // height
    );
    renderBuffer.SetTextureFilteringMode(GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
    renderBuffer.SetTextureMaxAnisotropy();

    dimensions = sendTextureToFragmentShader(&tree_tex, "LushPine.png", "tex", 0);
    tree_tex.Bind(3);
    textureWidth = dimensions.first;
    textureHeight = dimensions.second;

    createPlane();
    glutMainLoop();

    return 0;
}