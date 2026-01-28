
//@HR
//this is Rouibah hanine project
//perspective future= ? not defined !
//last update : 2025-12-12 6:32PM
//vertex : pour gerer les transfotrmations géométrique
//fragment pour le calcule de la lumiere ( & couleurs)
//lets start !
#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#include "stb_image.h"
#include "tiny_obj_loader.h"
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <vector>
#include <iostream>
#include <windows.h>   // required for PlaySound()
#include <mmsystem.h>
#include <fstream>
#include <sstream>
#include <cmath>
#include <ctime>
#include <map>
// --------------------- Globals (kept from original) ---------------------
// Mouse control for free cam
bool mouseCaptured = false;        // Tracks if we're capturing mouse
int lastMouseX = -1, lastMouseY = -1;
float mouseSensitivity = 0.15f;    // Adjust feel (lower = slower)
int winW = 1280, winH = 720;
// Safe camera limits for ORBIT mode
const float MAX_SAFE_ORBIT_RADIUS = 3.6f;   // Never go beyond this
const float MIN_SAFE_ORBIT_RADIUS = 1.5f;   // Don't get too close to center (optional)
bool showHelp = true;
float sunlight = 0.75f;
bool lampOn = true;
bool ceilingLightOn = true;
bool screenOn=true;
bool windowOpen = false;
bool wardrobeOpen = false;
bool doorOpen = false;
// camera
enum CamMode { CAM_AUTO, CAM_FREE };
CamMode camMode = CAM_AUTO;
bool animPaused = false;
float orbitAngle = 20.0f, orbitRadius = 3.6f, orbitHeight = 2.0f;
float cx = 0.0f, cy = 1.3f, cz = 0.0f;
float yaw = -90.0f, pitch = -10.0f;
float moveSpeed = 0.15f, turnSpeed = 2.5f;
// model state preserved
float winAngleLeft = 0.0f, winAngleRight = 0.0f;
float wardrobeDoorAngleLeft = 0.0f, wardrobeDoorAngleRight = 0.0f;
float doorAngle = 0.0f, targetDoorAngle = 0.0f;
float globalIntensity = 1.0f;
bool doorLightOn = true;           // new toggle
float doorLightIntensity = 1.0f;   // optional multiplier
bool isFullscreen = false;

GLuint screentextId=0;
std::string vertexSrc ;

std:: string  fragSrc ;
// textures
std::map<std::string, GLuint> texMap;
GLuint texNoise = 0;

// tinyobj meshes (cpu-side)
struct Mesh
{
    std::vector<float> vertices; // x,y,z
    std::vector<float> normals;  // x,y,z
    std::vector<float> texcoords; // u,v
    std::vector<unsigned int> indices;
    // GPU
    GLuint vao = 0, vbo = 0, ebo = 0;
};
std::map<std::string, Mesh> meshes;

// --------------------- Utility: read file ---------------------
std::string readFile(const std::string &path)
{
    std::ifstream f(path);
    if(!f) return std::string();
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// --------------------- Init loads textures + objects ---------------------
//std::string getExecutableDir(); // original helper if you need Tex(...) — omitted here for clarity
//----------------------------------------------
std::string getExecutableDir()
{
    std::string path;
#ifdef _WIN32
    char buffer[MAX_PATH];
    if (GetModuleFileNameA(NULL, buffer, MAX_PATH) != 0)
    {
        path = std::string(buffer);
        size_t pos = path.find_last_of("\\/");
        if (pos != std::string::npos)
        {
            path = path.substr(0, pos);
        }
    }
#else
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1)
    {
        buffer[len] = '\0';
        path = std::string(buffer);
        size_t pos = path.find_last_of("/");
        if (pos != std::string::npos)
        {
            path = path.substr(0, pos);
        }
    }
#endif

    // Go up two levels: from /bin/Debug → /Project_Official
    for (int i = 0; i < 2; ++i)
    {
        size_t pos = path.find_last_of("\\/");
        if (pos != std::string::npos)
        {
            path = path.substr(0, pos);
        }
    }
    return path;
}

std::string Tex(const std::string& filename)
{
    return getExecutableDir() + "/Ressources/" + filename;
}

// --------------------- Shader helpers ---------------------
GLuint compileShader(GLenum type, const char *src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if(!ok)
    {
        char log[4096];
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        std::cerr<<"Shader compile error: "<<log<<"\n";
    }
    return s;
}
GLuint linkProgram(GLuint vs, GLuint fs)
{
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if(!ok)
    {
        char log[4096];
        glGetProgramInfoLog(p, sizeof(log), nullptr, log);
        std::cerr<<"Program link error: "<<log<<"\n";
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return p;
}

//-----------------------------------------------

void drawTextScreen(const char *txt, int x, int y)
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, winW, 0, winH);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();


    glDisable(GL_TEXTURE_2D);
    glRasterPos2i(x, y);
    for(const char *c = txt; *c; ++c)
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);


    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();                // restore previous MODELVIEW

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();                // restore previous PROJECTION (perspective)

    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_TEXTURE_2D);
}

//-------------------------------------
void toggleFullscreen()
{
    if (isFullscreen)
    {
        glutLeaveFullScreen();
        // Optionally restore original window size
        glutReshapeWindow(1280, 720);
        glutPositionWindow(50, 50);  // optional: move window back
    }
    else
    {
        glutFullScreen();
    }
    isFullscreen = !isFullscreen;
}
// --------------------- Globals for modern path ---------------------
GLuint program = 0;
GLint uni_model= -1, uni_view=-1, uni_proj=-1, uni_viewPos=-1;
GLint uni_useTexture=-1, uni_tex0=-1, uni_objectColor=-1, uni_emission=-1, uni_alpha=-1;
GLint uni_uSun_dir=-1, uni_uSun_on=-1; // we'll use struct locations later via name queries

// cube and plane VAOs
GLuint cubeVAO = 0, cubeVBO = 0;
GLuint planeVAO = 0, planeVBO = 0;

// simple screen quad for posters, overlays
GLuint quadVAO = 0, quadVBO = 0;

// --------------------- Matrix Stack compatibility layer ---------------------
struct MatrixStack {
    std::vector<glm::mat4> stack;
    //glm::vec3 color = glm::vec3(1.0f); // not used by GL state, but we emulate glColor*
    MatrixStack(){ stack.push_back(glm::mat4(1.0f)); }
    void push(){ stack.push_back(stack.back()); }
    void pop(){ if(stack.size()>1) stack.pop_back(); }
    void loadIdentity(){ stack.back() = glm::mat4(1.0f); }
    void translate(float x,float y,float z){ stack.back() = glm::translate(stack.back(), glm::vec3(x,y,z)); }
    void scale(float x,float y,float z){ stack.back() = glm::scale(stack.back(), glm::vec3(x,y,z)); }
    void rotate(float angleDeg, float x,float y,float z){ stack.back() = glm::rotate(stack.back(), glm::radians(angleDeg), glm::vec3(x,y,z)); }
    glm::mat4 top() const { return stack.back(); }
    glm::vec3 curColor = glm::vec3(1.0f);
} M; // single global stack named M

// Map common legacy calls to our stack — this lets many old draw functions work unchanged:
#define glPushmatrix()    M.push()
#define glPopMatrix()     M.pop()
#define glLoadIdentity()  M.loadIdentity()
#define glTranslatef(x,y,z) M.translate((x),(y),(z))
#define glScalef(x,y,z)      M.scale((x),(y),(z))
#define glRotatef(angle,x,y,z) M.rotate((angle),(x),(y),(z))
//#define glColor3f(r,g,b)     (M.curColor = glm::vec3((r),(g),(b)))

// --------------------- Create unit cube and plane (GPU) ---------------------
void createCube() {
    if(cubeVAO) return;
    float verts[] = {
                        // positions        // normals         // uvs
                        // back face (-z)
                        -0.5f,-0.5f,-0.5f,  0,0,-1,  0.0f,0.0f,
                        0.5f,-0.5f,-0.5f,  0,0,-1,  1.0f,0.0f,
                        0.5f, 0.5f,-0.5f,  0,0,-1,  1.0f,1.0f,
                        0.5f, 0.5f,-0.5f,  0,0,-1,  1.0f,1.0f,
                        -0.5f, 0.5f,-0.5f,  0,0,-1,  0.0f,1.0f,
                        -0.5f,-0.5f,-0.5f,  0,0,-1,  0.0f,0.0f,
                        // front (+z)
                        -0.5f,-0.5f, 0.5f,  0,0,1,  0.0f,0.0f,
                        0.5f,-0.5f, 0.5f,  0,0,1,  1.0f,0.0f,
                        0.5f, 0.5f, 0.5f,  0,0,1,  1.0f,1.0f,
                        0.5f, 0.5f, 0.5f,  0,0,1,  1.0f,1.0f,
                        -0.5f, 0.5f, 0.5f,  0,0,1,  0.0f,1.0f,
                        -0.5f,-0.5f, 0.5f,  0,0,1,  0.0f,0.0f,
                        // left (-x)
                        -0.5f, 0.5f, 0.5f, -1,0,0,  1.0f,0.0f,
                        -0.5f, 0.5f,-0.5f, -1,0,0,  1.0f,1.0f,
                        -0.5f,-0.5f,-0.5f, -1,0,0,  0.0f,1.0f,
                        -0.5f,-0.5f,-0.5f, -1,0,0,  0.0f,1.0f,
                        -0.5f,-0.5f, 0.5f, -1,0,0,  0.0f,0.0f,
                        -0.5f, 0.5f, 0.5f, -1,0,0,  1.0f,0.0f,
                        // right (+x)
                        0.5f, 0.5f, 0.5f,  1,0,0,  1.0f,0.0f,
                        0.5f, 0.5f,-0.5f,  1,0,0,  1.0f,1.0f,
                        0.5f,-0.5f,-0.5f,  1,0,0,  0.0f,1.0f,
                        0.5f,-0.5f,-0.5f,  1,0,0,  0.0f,1.0f,
                        0.5f,-0.5f, 0.5f,  1,0,0,  0.0f,0.0f,
                        0.5f, 0.5f, 0.5f,  1,0,0,  1.0f,0.0f,
                        // bottom (-y)
                        -0.5f,-0.5f,-0.5f,  0,-1,0,  0.0f,1.0f,
                        0.5f,-0.5f,-0.5f,  0,-1,0,  1.0f,1.0f,
                        0.5f,-0.5f, 0.5f,  0,-1,0,  1.0f,0.0f,
                        0.5f,-0.5f, 0.5f,  0,-1,0,  1.0f,0.0f,
                        -0.5f,-0.5f, 0.5f,  0,-1,0,  0.0f,0.0f,
                        -0.5f,-0.5f,-0.5f,  0,-1,0,  0.0f,1.0f,
                        // top (+y)
                        -0.5f, 0.5f,-0.5f,  0,1,0,  0.0f,1.0f,
                        0.5f, 0.5f,-0.5f,  0,1,0,  1.0f,1.0f,
                        0.5f, 0.5f, 0.5f,  0,1,0,  1.0f,0.0f,
                        0.5f, 0.5f, 0.5f,  0,1,0,  1.0f,0.0f,
                        -0.5f, 0.5f, 0.5f,  0,1,0,  0.0f,0.0f,
                        -0.5f, 0.5f,-0.5f,  0,1,0,  0.0f,1.0f
                        };
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(3*sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(6*sizeof(float))); glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

void createPlane() {
    if(planeVAO) return;
    float plane[] = {
                        // positions       // normals    // uvs
                        -0.5f,0.0f,-0.5f,   0,1,0,        0,0,
                        0.5f,0.0f,-0.5f,   0,1,0,        1,0,
                        0.5f,0.0f, 0.5f,   0,1,0,        1,1,
                        0.5f,0.0f, 0.5f,   0,1,0,        1,1,
                        -0.5f,0.0f, 0.5f,   0,1,0,        0,1,
                        -0.5f,0.0f,-0.5f,   0,1,0,        0,0
                        };
    glGenVertexArrays(1,&planeVAO);
    glGenBuffers(1,&planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane), plane, GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(3*sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(6*sizeof(float))); glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

// quad for posters / overlays
void createQuad() {
    if(quadVAO) return;
    float q[] = {
                    // pos    // normal // uv
                    -0.5f, 0.5f, 0.0f,  0,0,1,  0,1,
                    0.5f, 0.5f, 0.0f,  0,0,1,  1,1,
                    0.5f,-0.5f, 0.0f,  0,0,1,  1,0,
                    0.5f,-0.5f, 0.0f,  0,0,1,  1,0,
                    -0.5f,-0.5f, 0.0f,  0,0,1,  0,0,
                    -0.5f, 0.5f, 0.0f,  0,0,1,  0,1
                    };
    glGenVertexArrays(1,&quadVAO);
    glGenBuffers(1,&quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(q), q, GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(3*sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(6*sizeof(float))); glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

// --------------------- Load textures ---------------------
GLuint loadTexture(const std::string &path , bool inverse=true) {
    int w,h,n;
    stbi_set_flip_vertically_on_load(inverse);
    unsigned char *data = stbi_load(path.c_str(), &w,&h,&n,0);
    if(!data){ std::cerr<<"Failed to load texture: "<<path<<"\n"; return 0; }
    GLuint id; glGenTextures(1,&id);
    glBindTexture(GL_TEXTURE_2D,id);
    GLenum format = (n==4)?GL_RGBA:GL_RGB;
    glTexImage2D(GL_TEXTURE_2D,0,format,w,h,0,format,GL_UNSIGNED_BYTE,data);
    //glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    stbi_image_free(data);
    return id;
}

// --------------------- Create GPU mesh from Mesh ---------------------
void uploadMeshToGPU(Mesh &m) {
    if(m.vao) return;
    std::vector<float> interleaved;
    size_t vcount = m.indices.size();
    for(size_t i=0;i<vcount;i++){
        unsigned int idx = m.indices[i];
        interleaved.push_back(m.vertices[3*idx+0]);
        interleaved.push_back(m.vertices[3*idx+1]);
        interleaved.push_back(m.vertices[3*idx+2]);
        interleaved.push_back(m.normals[3*idx+0]);
        interleaved.push_back(m.normals[3*idx+1]);
        interleaved.push_back(m.normals[3*idx+2]);
        interleaved.push_back(m.texcoords[2*idx+0]);
        interleaved.push_back(m.texcoords[2*idx+1]);
    }
    glGenVertexArrays(1,&m.vao);
    glGenBuffers(1,&m.vbo);
    glBindVertexArray(m.vao);
    glBindBuffer(GL_ARRAY_BUFFER,m.vbo);
    glBufferData(GL_ARRAY_BUFFER, interleaved.size()*sizeof(float), interleaved.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(3*sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(6*sizeof(float))); glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    // store index count in indices vector size for draw
}

// --------------------- Draw helpers ---------------------
void setCommonUniforms(const glm::mat4 &view, const glm::mat4 &proj, const glm::vec3 &camPos) {
    glUseProgram(program);
    glUniformMatrix4fv(uni_view, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(uni_proj, 1, GL_FALSE, glm::value_ptr(proj));
    glUniform3fv(uni_viewPos, 1, glm::value_ptr(camPos));
}

void drawCubeWithModel(const glm::mat4 &model, GLuint tex = 0, glm::vec3 color = glm::vec3(1.0f), glm::vec3 emi = glm::vec3(0.0f), float alpha=1.0f) {
    glUseProgram(program);
    glUniformMatrix4fv(uni_model,1,GL_FALSE,glm::value_ptr(model));
    glUniform1i(uni_useTexture, tex?1:0);
    glUniform3fv(uni_objectColor, 1, glm::value_ptr(color));
    glUniform3fv(uni_emission,1, glm::value_ptr(emi));
    glUniform1f(uni_alpha, alpha);
    if(tex){
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glUniform1i(uni_tex0, 0);
    }else {
        // IMPORTANT: When no texture, bind 0 or a default white texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);  // Unbind any texture
    }
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    // === CLEANUP: Very important! ===
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);        // Unbind texture
    glActiveTexture(GL_TEXTURE0);           // Reset active unit (good practice)
    glUseProgram(0);
}

void drawPlaneWithModel(const glm::mat4 &model, GLuint tex = 0, glm::vec3 color = glm::vec3(1.0f), glm::vec3 emi = glm::vec3(0.0f), float alpha=1.0f) {
    glUseProgram(program);
    glUniformMatrix4fv(uni_model,1,GL_FALSE,glm::value_ptr(model));
    glUniform1i(uni_useTexture, tex?1:0);
    glUniform3fv(uni_objectColor, 1, glm::value_ptr(color));
    glUniform3fv(uni_emission,1, glm::value_ptr(emi));  // ← this was already there
    glUniform1f(uni_alpha, alpha);
    if(tex){
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glUniform1i(uni_tex0, 0);
    }else {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glBindVertexArray(planeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glUseProgram(0);
}

// Emulate old drawBox(sx,sy,sz, tex, draw) using the matrix stack top
void drawBox(float sx, float sy, float sz, GLuint tex = 0, bool draw = true) {
    (void)draw;
    glm::mat4 model = M.top() * glm::scale(glm::mat4(1.0f), glm::vec3(sx, sy, sz));
    drawCubeWithModel(model, tex, M.curColor, glm::vec3(0.0f), 1.0f);
}

// plane: width w, depth d, repeat UV
void drawPlane(float w, float d, GLuint tex = 0, float repeat = 4.0f) {
    // scale our unit plane (size 1x1 centered) into w x d and keep at current transform
    glm::mat4 model = M.top() * glm::scale(glm::mat4(1.0f), glm::vec3(w, 1.0f, d));
    drawPlaneWithModel(model, tex, M.curColor, glm::vec3(0.0f), 1.0f);
}

// draw a GPU-backed tinyobj mesh
void drawMeshGPU(const Mesh &m, GLuint tex = 0 , glm::vec3 color = glm::vec3(1.0f)) {
    if(!m.vao) return;
    glUseProgram(program);
    glUniformMatrix4fv(uni_model,1,GL_FALSE,glm::value_ptr(M.top()));

    // ← THIS WAS MISSING!
    glUniform1i(uni_useTexture, (tex != 0) ? 1 : 0);
    glUniform3fv(uni_objectColor, 1, glm::value_ptr(color));
    glUniform3fv(uni_emission, 1, glm::value_ptr(glm::vec3(0.0f)));
    glUniform1f(uni_alpha, 1.0f);
    //glUniform1i(uni_useTexture, tex?1:0);
    if(tex){

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glUniform1i(uni_tex0,0);
    }
    glBindVertexArray(m.vao);
    // we uploaded as sequential vertices; draw with the number of vertices = indices.size()
    GLsizei count = (GLsizei)m.indices.size();
    glDrawArrays(GL_TRIANGLES, 0, count);
    glBindVertexArray(0);
}

// --------------------- Lighting upload (replaces setupLights) ---------------------
void uploadLights() {
    glUseProgram(program);
    // SUN directional (window)
    glm::vec3 sunDir = glm::vec3(0.5f, -0.3f, -1.0f);
    // map to shader uniforms (struct)
    GLint loc;
    loc = glGetUniformLocation(program, "uSun.dir"); if(loc>=0) glUniform3fv(loc,1,glm::value_ptr(sunDir));
    loc = glGetUniformLocation(program, "uSun.ambient"); if(loc>=0) glUniform3fv(loc,1,glm::value_ptr(glm::vec3(0.01f*sunlight,0.03f*sunlight,0.05f*sunlight)));
    loc = glGetUniformLocation(program, "uSun.diffuse"); if(loc>=0) glUniform3fv(loc,1,glm::value_ptr(glm::vec3(0.01f*sunlight,0.03f*sunlight,0.06f*sunlight)));
    //loc = glGetUniformLocation(program, "uSun.spec"); if(loc>=0) glUniform3fv(loc,1,glm::value_ptr(glm::vec3(0.1f)));
    loc = glGetUniformLocation(program, "uSun.spec"); if(loc>=0) glUniform3fv(loc,1,glm::value_ptr(glm::vec3(0.1f * sunlight)));
    loc = glGetUniformLocation(program, "uSun.on"); if(loc>=0) glUniform1i(loc, 1);

    // Lamp point as in original
    loc = glGetUniformLocation(program, "uLamp.pos"); if(loc>=0) glUniform3fv(loc,1,glm::value_ptr(glm::vec3(1.9f,1.15f,-3.68f)));
    //loc = glGetUniformLocation(program, "uLamp.ambient"); if(loc>=0) glUniform3fv(loc,1,glm::value_ptr(glm::vec3(0.04f*globalIntensity,0.025f*globalIntensity,0.015f*globalIntensity)));
    glm::vec3 lampAmbient = lampOn ? glm::vec3(0.04f,0.025f,0.015f) * globalIntensity : glm::vec3(0.0f);
    loc = glGetUniformLocation(program, "uLamp.ambient"); if(loc>=0) glUniform3fv(loc,1,glm::value_ptr(lampAmbient));
    loc = glGetUniformLocation(program, "uLamp.diffuse"); if(loc>=0) glUniform3fv(loc,1,glm::value_ptr(glm::vec3(0.98f*globalIntensity,0.8f*globalIntensity,0.43f*globalIntensity)));
    loc = glGetUniformLocation(program, "uLamp.spec"); if(loc>=0) glUniform3fv(loc,1,glm::value_ptr(glm::vec3(1.0f)));
    loc = glGetUniformLocation(program, "uLamp.constant"); if(loc>=0) glUniform1f(loc, 1.0f);
    loc = glGetUniformLocation(program, "uLamp.linear");   if(loc>=0) glUniform1f(loc, 0.0f);
    loc = glGetUniformLocation(program, "uLamp.quad");     if(loc>=0) glUniform1f(loc, 0.5f);
    loc = glGetUniformLocation(program, "uLamp.on"); if(loc>=0) glUniform1i(loc, lampOn?1:0);

    // Ceiling point light
    loc = glGetUniformLocation(program, "uCeiling.pos"); if(loc>=0) glUniform3fv(loc,1,glm::value_ptr(glm::vec3(0.0f,3.0f,0.0f)));
    //loc = glGetUniformLocation(program, "uCeiling.ambient"); if(loc>=0) glUniform3fv(loc,1,glm::value_ptr(glm::vec3(0.1f,0.18f,0.25f)));
    //loc = glGetUniformLocation(program, "uCeiling.diffuse"); if(loc>=0) glUniform3fv(loc,1,glm::value_ptr(glm::vec3(0.80f,0.75f,0.72f)));
    glm::vec3 ceilingAmbient = ceilingLightOn ? glm::vec3(0.1f,0.18f,0.25f) : glm::vec3(0.0f);
    glm::vec3 ceilingDiffuse = ceilingLightOn ? glm::vec3(0.80f,0.75f,0.72f) : glm::vec3(0.0f);
    loc = glGetUniformLocation(program, "uCeiling.ambient"); if(loc>=0) glUniform3fv(loc,1,glm::value_ptr(ceilingAmbient));
    loc = glGetUniformLocation(program, "uCeiling.diffuse"); if(loc>=0) glUniform3fv(loc,1,glm::value_ptr(ceilingDiffuse));
    loc = glGetUniformLocation(program, "uCeiling.spec"); if(loc>=0) glUniform3fv(loc,1,glm::value_ptr(glm::vec3(0.2f)));
    loc = glGetUniformLocation(program, "uCeiling.constant"); if(loc>=0) glUniform1f(loc, 1.0f);
    loc = glGetUniformLocation(program, "uCeiling.linear");   if(loc>=0) glUniform1f(loc, 0.015f);
    loc = glGetUniformLocation(program, "uCeiling.quad");     if(loc>=0) glUniform1f(loc, 0.00005f);
    loc = glGetUniformLocation(program, "uCeiling.on"); if(loc>=0) glUniform1i(loc, ceilingLightOn?1:0);

   loc = glGetUniformLocation(program, "uDoorLight.pos"); if(loc>=0) glUniform3fv(loc,1,glm::value_ptr(glm::vec3(0.5f, 0.5f, -1.0f))); // just inside, above door

    loc = glGetUniformLocation(program, "uDoorLight.ambient"); if(loc>=0) glUniform3fv(loc,1,glm::value_ptr(glm::vec3(0.01f*sunlight,0.04f*sunlight,0.09f*sunlight)));
    loc = glGetUniformLocation(program, "uDoorLight.diffuse"); if(loc>=0) glUniform3fv(loc,1,glm::value_ptr(glm::vec3(0.01f*sunlight,0.04f*sunlight,0.1f*sunlight)));
    loc = glGetUniformLocation(program, "uDoorLight.spec"); if(loc>=0) glUniform3fv(loc,1,glm::value_ptr(glm::vec3(0.1f)));
    loc = glGetUniformLocation(program, "uDoorLight.on"); if(loc>=0) glUniform1i(loc, 1);

}

// --------------------- Load OBJ wrapper ---------------------
bool loadOBJtoMesh(const std::string &path, Mesh &out) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    bool ok = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str());
    if(!warn.empty()) std::cout<<"WARN: "<<warn<<"\n";
    if(!err.empty()) std::cerr<<"ERR: "<<err<<"\n";
    if(!ok) return false;
    out.vertices.clear(); out.normals.clear(); out.texcoords.clear(); out.indices.clear();
    for(const auto &shape : shapes) {
        for(const auto &idx : shape.mesh.indices){
            out.vertices.push_back(attrib.vertices[3*idx.vertex_index+0]);
            out.vertices.push_back(attrib.vertices[3*idx.vertex_index+1]);
            out.vertices.push_back(attrib.vertices[3*idx.vertex_index+2]);
            if(!attrib.normals.empty()){
                out.normals.push_back(attrib.normals[3*idx.normal_index+0]);
                out.normals.push_back(attrib.normals[3*idx.normal_index+1]);
                out.normals.push_back(attrib.normals[3*idx.normal_index+2]);
            } else {
                out.normals.push_back(0); out.normals.push_back(1); out.normals.push_back(0);
            }
            if(!attrib.texcoords.empty()){
                out.texcoords.push_back(attrib.texcoords[2*idx.texcoord_index+0]);
                out.texcoords.push_back(attrib.texcoords[2*idx.texcoord_index+1]);
            } else {
                out.texcoords.push_back(0); out.texcoords.push_back(0);
            }
            out.indices.push_back((unsigned int)out.indices.size());
        }
    }
    uploadMeshToGPU(out);
    return true;
}

// --------------------- Convenience: drawPoster (replaces glBegin quads) ---------------------
void drawPosterAt(float x, float y, float z, float sx, float sy, float angle,bool turntowall ,bool forscreen,GLuint tex) {
    M.push();
    M.translate(x,y,z);
    M.scale(1.0f,1.0f,1.1f);

    if( forscreen){
    M.scale(sx,sy,1.0);
    }
    if (turntowall){
        M.rotate(90,1,0,0);
    }

    M.rotate(angle,0,0,1);
    // Plane is in XZ plane in our unit definition; we want it facing +Z with normal +Z
    // rotate so quad normal is +Z
    // Our quad is in XY plane at z=0, but our plane/quad expects that orientation; so:
    glm::mat4 model = M.top() * glm::scale(glm::mat4(1.0f), glm::vec3(sx,sy,1.0f));
    drawPlaneWithModel(model, tex);
    M.pop();
}


// --------------------- Converted drawRoom (example) ---------------------
void drawRoom() {
    // floor
    M.push();
    M.loadIdentity();
    M.translate(0.0f, 0.0f, 0.0f);
    drawPlane(10.0f, 8.0f, texMap["floor"], 16.0f);
    M.pop();

    // back wall
    M.push();
    M.loadIdentity();
    M.translate(0.0f, 2.0f, -4.0f);
    M.scale(10.0f, 4.0f, 0.1f);
    drawBox(1,1,1, texMap["Wall3"]);
    M.pop();

    // left wall
    M.push();
    M.loadIdentity();
    M.translate(-5.0f, 2.0f, 0.0f);
    M.scale(0.1f, 4.0f, 8.0f);
    drawBox(1,1,1, texMap["Wall3"]);
    M.pop();

    // right wall
    M.push(); M.loadIdentity();
    M.translate(5.0f,2.0f,0.0f);
    M.scale(0.1f,4.0f,8.0f);
    drawBox(1,1,1, texMap["Wall3"]);
    M.pop();

    // ceiling (plain)
    M.push(); M.loadIdentity();
    M.translate(0, 4.0f, 0);
    //M.scale(10.0f, 0.1f, 8.0f);
    M.rotate(180,1,0,0);
    //setMaterial(0.9f,0.9f,0.9f,2);
    //drawBox(1,1,1, texMap["BlueWood"]);
    drawPlane(10.0f, 8.0f, texMap["ceiling"], 6.0f);
    M.pop();

    // front wall sections, door & window simplified
    // ======================================
    // FRONT WALL (z = +4) WITH DOOR + CENTERED WINDOW
    // ======================================
    float wallW  = 10.0f;
    float wallH  = 4.0f;
    float wallZ  = 4.0f;

    // --- DOOR (left side) ---
    float doorX      = -2.0f;
    float doorW      = 1.11f;
    float doorH      = 2.2f;
    float doorBottom = 0.0f;

    // --- WINDOW (perfectly centered horizontally and vertically) ---
    float windowW    = 1.45f;
    float windowH    = 1.3f;
    float windowX    = 1.0f;                    // center X
    float windowY    = 1.5f;                    // center Y (middle of 4m wall)
    float windowBottom = windowY - windowH/2.0f; // = 1.2
    float windowTop    = windowY + windowH/2.0f; // = 2.8

    // 1. Left part (before door)
    float leftWidth = (doorX - doorW/2.0f) + wallW/2.0f;
    M.push();
    glTranslatef(leftWidth/2.0f - wallW/2.0f, wallH/2.0f, wallZ);
    glScalef(leftWidth, wallH, 0.1f);
    drawBox(1,1,1, texMap["Wall3"]);
    M.pop();

    // 2. Between door and window
    float midLeftW = (windowX - windowW/2.0f) - (doorX + doorW/2.0f);
    if (midLeftW > 0.01f)
    {
        M.push();
        glTranslatef((doorX + doorW/2.0f) + midLeftW/2.0f, wallH/2.0f, wallZ);
        glScalef(midLeftW, wallH, 0.1f);
        drawBox(1,1,1, texMap["Wall3"]);
        M.pop();
    }

    // 3. Between window and right wall
    float midRightW = wallW/2.0f - (windowX + windowW/2.0f);
    if (midRightW > 0.01f)
    {
        M.push();
        glTranslatef((windowX + windowW/2.0f) + midRightW/2.0f, wallH/2.0f, wallZ);
        glScalef(midRightW, wallH, 0.1f);
        drawBox(1,1,1, texMap["Wall3"]);
        M.pop();
    }

    // 4. Above the door
    M.push();
    glTranslatef(doorX, doorBottom + doorH + (wallH - doorH)/2.0f, wallZ);
    glScalef(doorW, wallH - doorH, 0.1f);
    drawBox(1,1,1, texMap["Wall3"]);
    M.pop();

    // 5. Above the window
    float topHeight = wallH - windowTop;
    M.push();
    glTranslatef(windowX, windowTop + topHeight/2.0f, wallZ);
    glScalef(windowW, topHeight, 0.1f);
    drawBox(1,1,1, texMap["Wall3"]);
    M.pop();

    // 6. Below the window (from floor to window bottom)
    float bottomHeight = windowBottom - doorBottom;
    if (bottomHeight > 0.01f)
    {
        M.push();
        glTranslatef(windowX, bottomHeight/2.0f, wallZ);
        glScalef(windowW, bottomHeight, 0.1f);
        drawBox(1,1,1, texMap["Wall3"]);
        M.pop();
    }
    //---------- door-----------

    // door position
    float doorXX = -2.0f;   // center
    float doorYY = 1.1f;    // center Y
    float doorZZ = 3.95f;   // slightly in front of wall

    float doorWW = 1.12f;   // full width (trim width)

    // === HINGE POSITION (right side) ===
    float hingeXX = doorXX + (doorWW / 2.0f);   // right side hinge
    float hingeYY = doorYY;
    float hingeZZ = doorZZ;

    M.push();

    // ------------------------------
    // MOVE door to hinge, rotate, move back
    // ------------------------------
    glTranslatef(hingeXX, hingeYY, hingeZZ);      // move to hinge
    glRotatef(-doorAngle, 0, 1, 0);             // rotate around hinge
    glTranslatef(-hingeXX, -hingeYY, -hingeZZ);   // move back to original

    // =================================================================
    // ORIGINAL DOOR CODE (unchanged)
    // =================================================================
    // Frame (trim)
    M.push();
    glTranslatef(doorXX, doorYY, doorZZ);
    glRotatef(180, 0, 1, 0);

    // Slightly larger than door leaf
    M.push();
    glScalef(1.12f, 2.32f, 0.08f);
    drawBox(1,1,1, texMap["door_panel_b"]);
    M.pop();

    // Decorative vertical strip
    M.push();
    glTranslatef(0.0f, -0.03f, 0.026f);
    glScalef(0.12f, 1.4f, 0.021f);
    drawBox(1,1,1, texMap["door_panel_b"]);
    M.pop();

    M.pop(); // frame
    M.pop(); // hinge transform
    //---------------
    // =========================================
    //           WINDOW WITH 2 LEAFS
    // =========================================

    float frameSize = 0.05f;      // thickness of frames
    float leafDepth = 0.08f;      // depth of window leaf
    float leafMargin = 0.01f;

    // Top frame
    M.push();
    glTranslatef(windowX, windowTop + frameSize/2.0f, wallZ + 0.01f);
    glScalef(windowW, frameSize, leafDepth);
    drawBox(1,1,1, texMap["metal"]);
    M.pop();

    // Bottom frame
    M.push();
    glTranslatef(windowX, windowBottom - frameSize/2.0f, wallZ + 0.01f);
    glScalef(windowW, frameSize, leafDepth);
    drawBox(1,1,1, texMap["metal"]);
    M.pop();

    // Left vertical frame
    M.push();
    glTranslatef(windowX - windowW/2.0f - frameSize/2.0f, windowY, wallZ + 0.01f);
    glScalef(frameSize, windowH, leafDepth);
    drawBox(1,1,1, texMap["metal"]);
    M.pop();

    // Right vertical frame
    M.push();
    glTranslatef(windowX + windowW/2.0f + frameSize/2.0f, windowY, wallZ + 0.01f);
    glScalef(frameSize, windowH, leafDepth);
    drawBox(1,1,1, texMap["metal"]);
    M.pop();


    // =========================================
    //              LEFT LEAF
    // =========================================
    float halfW = windowW / 2.0f - leafMargin;
    float halfH = windowH;

    // hinge at left side
    M.push();
    glTranslatef(windowX - halfW, windowY, wallZ - 0.06f);
    glRotatef(winAngleLeft, 0, 1, 0);
    glTranslatef(halfW / 2.0f, 0, 0);

    M.push();
    glScalef(halfW, halfH, leafDepth);
    drawBox(1,1,1, texMap["WindowTex"]);
    M.pop();

    // border inside the leaf (thin frame)
    float innerT = 0.03f;

    // Top border
    M.push();
    glTranslatef(0, halfH/2.0f - innerT/2.0f, 0);
    glScalef(halfW, innerT, leafDepth + 0.01f);
    drawBox(1,1,1, texMap["metal"]);
    M.pop();

    // Bottom border
    M.push();
    glTranslatef(0, -halfH/2.0f + innerT/2.0f, 0);
    glScalef(halfW, innerT, leafDepth + 0.01f);
    drawBox(1,1,1, texMap["metal"]);
    M.pop();

    // Left border
    M.push();
    glTranslatef(-halfW/2.0f + innerT/2.0f, 0, 0);
    glScalef(innerT, halfH, leafDepth + 0.01f);
    drawBox(1,1,1, texMap["metal"]);
    M.pop();

    // Right border
    M.push();
    glTranslatef(halfW/2.0f - innerT/2.0f, 0, 0);
    glScalef(innerT, halfH, leafDepth + 0.01f);
    drawBox(1,1,1, texMap["metal"]);
    M.pop();

    M.pop();


    // =========================================
    //              RIGHT LEAF
    // =========================================

    // hinge at right side
    M.push();
    glTranslatef(windowX + halfW, windowY, wallZ - 0.06f);
    glRotatef(winAngleRight, 0, 1, 0);
    glTranslatef(-halfW / 2.0f, 0, 0);


    M.push();
    glScalef(halfW, halfH, leafDepth);
    drawBox(1,1,1,texMap["WindowTex"]);
    M.pop();


    // Top border
    M.push();
    glTranslatef(0, halfH/2.0f - innerT/2.0f, 0);
    glScalef(halfW, innerT, leafDepth + 0.01f);
    drawBox(1,1,1, texMap["metal"]);
    M.pop();

    // Bottom border
    M.push();
    glTranslatef(0, -halfH/2.0f + innerT/2.0f, 0);
    glScalef(halfW, innerT, leafDepth + 0.01f);
    drawBox(1,1,1, texMap["metal"]);
    M.pop();

    // Left border
    M.push();
    glTranslatef(-halfW/2.0f + innerT/2.0f, 0, 0);
    glScalef(innerT, halfH, leafDepth + 0.01f);
    drawBox(1,1,1, texMap["metal"]);
    M.pop();

    // Right border
    M.push();
    glTranslatef(halfW/2.0f - innerT/2.0f, 0, 0);
    glScalef(innerT, halfH, leafDepth + 0.01f);
    drawBox(1,1,1, texMap["metal"]);
    M.pop();

    M.pop();


    // desk + monitor (example of model mesh usage if available)
    if(meshes.count("keyboard.obj")) {
        // draw keyboard (the drawMeshGPU reads M.top())
        M.push();
        M.translate(-1.3f, 0.90f, -1.23f);
        M.rotate(180,0,1,0);
        M.scale(0.15f,0.25f,0.18f);
        drawMeshGPU(meshes["keyboard.obj"], texMap["door_panel_a"]);
        M.pop();
    }


    //wardrobe !
    M.push();
    M.translate(-4.2f, 0.0f, -3.0f);
    M.rotate(-90,0,1,0);
    M.scale(1.0f,1.1f,1.0f);
    drawMeshGPU(meshes["wardrobe.obj"], texMap["wardrobe"]);
    M.pop();



     //uefa chamopns league cup!
    M.push();
    M.translate(-4.0f,2.70f, -2.85f);
    M.rotate(-45,0,1,0);
    M.scale(0.95f,0.95f,0.95f);
    drawMeshGPU(meshes["UclCup.obj"], texMap["UclCup"]);
    M.pop();



    //draw shelves
float i=1.0; int j=1;
   while(i<2.0f){
    M.push();
    M.translate(0.3f, i*1.05f, -3.85f);
    //M.rotate(0,0,1,0);
        M.push();
        M.scale(0.35f,0.2f,0.25f);
        drawMeshGPU(meshes["shelf.obj"], texMap["woood2"]);
        M.pop();

        if(j%2!=0 ){
            M.push();
            M.scale(0.1f,0.1f,0.1f);

            M.translate(0.0f,(i*1.05)+1.385f,0.0f);
            M.rotate(-90,1,0,0);

            drawMeshGPU(meshes["book.obj"], texMap["book"]);
            M.pop();


            //coffee cup (this on in shelves)
            M.push();
            M.translate(0.47f, (i*0.109), 0.135f);
            //M.rotate(-90,0,1,0);
            M.scale(0.16f,0.16f,0.16f);
            drawMeshGPU(meshes["PlasticCup.obj"], texMap["PlasticCup"]);
            M.pop();
        }

        if (j%2==0 ){
            M.push();
            M.scale(0.05f,0.045f,0.05f);

            M.translate(0.0f,1.97f,1.05f);
            //M.rotate(-90,1,0,0);

            drawMeshGPU(meshes["pots.obj"], texMap["pots"]);
            M.pop();
        }
    M.pop();

    i=i+0.8f;
    j++;
    }






    // draw lamp
    M.push();
    M.translate(1.9f, 0.7f, -3.68f);
    // M.rotate(0,0,1,0);
    M.scale(0.01f, 0.01f, 0.01);
    drawMeshGPU(meshes["lamp.obj"], texMap["lamp"]);
    M.pop();
    //draaw table (under lamp)

    //table

    M.push();
    glTranslatef(1.9f, 0.0f, -3.68f);
    glScalef(1.0f, 1.0f, 1.0f);
    drawMeshGPU(meshes["table.obj"], texMap["texBedtable"]);
    M.pop();

    //bed
    M.push();
    glTranslatef(3.60f, 0.0f, -2.75f); // adjust as needed
    glScalef(1.0f, 1.25f, 1.25f);       // scale to fit room
    drawMeshGPU(meshes["bed.obj"],texMap["texbed"]);    // you can keep blanket texture
    M.pop();


    //G chair
    M.push();

    M.translate(-3.65f,0,2.60f);
    M.scale(0.014f,0.011f,0.014f);
    M.rotate(135,0,1,0);

    drawMeshGPU(meshes["GChair.obj"], texMap["texGChair"]);

    M.pop();

    //draw Trashcan under desktop
    M.push();
    M.translate(-1.6f, 0.0f, -1.0f);

    //desktop,
    M.push();


    M.scale(0.24, 0.16f, 0.2f);
    M.rotate(180,0,1,0);
    drawMeshGPU(meshes["Desktop.obj"], texMap["woood2"]);

    M.pop();


    //screen pc
    M.push();

    M.translate(0.1f, 0.90f, 0.3f);
    M.scale(0.025, 0.0184f, 0.021f);
    M.rotate(180,0,1,0);
    drawMeshGPU(meshes["PC.obj"], texMap["door_panel_a"]);

    M.pop();

    //pc unite
    M.push();

    M.translate(-1.0f, 1.40f, 0.05f);
    M.scale(0.4, 0.45f, 0.4);
    //M.rotate(180,0,1,0);
    drawMeshGPU(meshes["PCUnite.obj"], texMap["door_panel_a"]);

    M.pop();


    //mouse
    M.push();
    M.translate(-0.1f, 0.9f, -0.25f);
    M.rotate(180,0,1,0);
    M.scale(0.009f, 0.01f, 0.009f);
    drawMeshGPU(meshes["Mouse.obj"], texMap["door_panel_a"]);
    M.pop();


    //speaker
    M.push();

    M.translate(1.05f, 0.9f, 0.3f);
    M.scale(0.18, 0.15f, 0.16);
    M.rotate(180,0,1,0);
    drawMeshGPU(meshes["PCSpeaker.obj"], texMap["door_panel_a"]);

    M.pop();

    //coffee cup (this on in table)
    M.push();
    M.translate(1.1f, 0.9f, -0.3f);
    //M.rotate(-90,0,1,0);
    M.scale(0.1f,0.1f,0.1f);
    drawMeshGPU(meshes["PlasticCup.obj"], texMap["PlasticCup"]);
    M.pop();



    //trashcan
    M.push();
    M.translate(1.72f, 0.0f, 0.1f);

    M.scale(0.065f, 0.05f, 0.055f);
    drawMeshGPU(meshes["TrashCan.obj"], texMap["BlueWood"]);

    M.pop();

    //paper inside trash can

    M.push();
    M.translate(1.72f, 0.05f, 0.1f);
    M.scale(0.06f, 0.06f, 0.06f);

    drawMeshGPU(meshes["Paper.obj"], texMap[""]);

    M.push();
    M.scale(0.25f, 0.2f, 0.2f);
    drawMeshGPU(meshes["canlowpoly.obj"], texMap[""]);
    M.pop();

    M.pop();


    M.pop();


    //----
    //ceil lamp

    M.push();
    M.translate(0.0f,2.4f,0.0f);
    M.scale(1.0f, 1.0f, 1.0f);
    drawMeshGPU(meshes["CeilLamp.obj"], texMap["ceilLamp"]);
    M.pop();

    //--bike
    M.push();
    M.translate(4.2f,0.0f,3.4f);
    M.rotate(180,0,1,0);
    M.scale(0.014f, 0.011f, 0.013f);
    drawMeshGPU(meshes["Bike.obj"], texMap["Bike"]);
    M.pop();

    //--chair
    M.push();
    M.translate(-1.5f,0.0f,-2.5f);
    M.rotate(-90,0,1,0);
    M.scale(0.055f, 0.05f, 0.05f);
    drawMeshGPU(meshes["Chair.obj"], texMap["Chair"]);
    M.pop();


    //--ctbale
    M.push();
    M.translate(-4.48f,0.0f,1.0f);
    //M.rotate(-90,0,1,0);
    M.scale(0.026f, 0.029f, 0.026f);
    drawMeshGPU(meshes["tablec.obj"], texMap["WindowTex"]);
    M.pop();

    //------------------
    // posters as quads
    drawPosterAt(-1.5f, 1.6f, -3.90f, 1.0f, 0.9f,0,true ,false,texMap["landscape"]);
    drawPosterAt(2.2f, 1.6f, -3.90f, 0.7f, 0.9f,0,true , false ,texMap["me"]);
    drawPosterAt(4.93f, 1.6f, -2.5f, 1.1f, 0.8f, 90 ,true , false ,texMap["algeria"]);

    drawPosterAt(4.93f, 1.6f, 1.0f, 1.1f, 0.8f, 90 ,true ,false , texMap["Slogan"]);


    drawPosterAt(-1.95f, 0.025f, 3.15f, 0.95f, 0.7f, 0.0f ,false,false , texMap["tapi"]);

    //drawPosterAt(-1.5f, 1.37f, -0.745f, 1.1f, 0.58f, 0.0f ,true,true, screentextId);



M.push();
M.translate(-1.5f, 1.37f, -0.745f);
M.scale(1.0f, 1.0f, 1.1f);  // your original mysterious scale
M.scale(1.1f,0.58f,1.0f);
M.rotate(90, 1, 0, 0);  // turn to wall
M.rotate(0, 0, 0, 1);
// Scale the plane
glm::mat4 model = M.top() * glm::scale(glm::mat4(1.0f), glm::vec3(1.1f, 0.58f, 1.0f));

// Determine emission based on screen state
glm::vec3 screenEmission = screenOn ? glm::vec3(1.0f) : glm::vec3(0.0f);

drawPlaneWithModel(model, screentextId, glm::vec3(1.0f), screenEmission, 1.0f);
M.pop();

}

// --------------------- Camera helpers ---------------------
glm::mat4 buildViewMatrix() {
    if(camMode == CAM_AUTO) {
        float rad = glm::radians(orbitAngle);
        float x = orbitRadius * cos(rad);
        float z = orbitRadius * sin(rad);
        glm::vec3 eye(x, orbitHeight, z);
        glm::vec3 center(0.0f, 1.0f, 0.0f);
        return glm::lookAt(eye, center, glm::vec3(0,1,0));
    } else {
        glm::vec3 pos(cx, cy, cz);
        float yaw_r = glm::radians(yaw), pitch_r = glm::radians(pitch);
        glm::vec3 dir(cos(yaw_r)*cos(pitch_r), sin(pitch_r), sin(yaw_r)*cos(pitch_r));
        return glm::lookAt(pos, pos + dir, glm::vec3(0,1,0));
    }
}

glm::vec3 cameraPos() {
    if(camMode==CAM_AUTO) {
        float rad = glm::radians(orbitAngle);
        return glm::vec3(orbitRadius * cos(rad), orbitHeight, orbitRadius * sin(rad));
    } else return glm::vec3(cx, cy, cz);
}

// --------------------- Input handlers (kept original behavior) ---------------------
void keyboard(unsigned char key, int x, int y) {
    switch(key) {
    case 27: case 'q': case 'Q': exit(0); break;
    //case 'c': case 'C': camMode = (camMode==CAM_AUTO)?CAM_FREE:CAM_AUTO; break;
    case 'c': case 'C':
    camMode = (camMode == CAM_AUTO) ? CAM_FREE : CAM_AUTO;
    if (camMode == CAM_FREE) {
        mouseCaptured = true;
        lastMouseX = -1; lastMouseY = -1;
        glutSetCursor(GLUT_CURSOR_NONE);  // Hide cursor
        // Center mouse initially
        int centerX = winW / 2;
        int centerY = winH / 2;
        glutWarpPointer(centerX, centerY);
    } else {
        mouseCaptured = false;
        glutSetCursor(GLUT_CURSOR_INHERIT);  // Show normal cursor
    }
    break;
    case 'l': case 'L': lampOn = !lampOn; break;
    case 'g': case 'G': ceilingLightOn = !ceilingLightOn; break;
    case 'p': case 'P': animPaused = !animPaused; break;
    case '+': if(lampOn){globalIntensity = glm::min(globalIntensity + 0.1f, 3.0f);}
            break;
    case '-': if(lampOn){ globalIntensity = glm::max(globalIntensity - 0.1f, 0.35f); }
            break;
    case 'w': case 'W': if(camMode==CAM_FREE){ float r = glm::radians(yaw); cx += cos(r)*moveSpeed; cz += sin(r)*moveSpeed; } break;
    case 's': case 'S': if(camMode==CAM_FREE){ float r = glm::radians(yaw); cx -= cos(r)*moveSpeed; cz -= sin(r)*moveSpeed; } break;
    case 'a': case 'A': if(camMode==CAM_FREE){ float r = glm::radians(yaw-90.0f); cx += cos(r)*moveSpeed; cz += sin(r)*moveSpeed; } break;
    case 'd': case 'D': if(camMode==CAM_FREE){ float r = glm::radians(yaw+90.0f); cx += cos(r)*moveSpeed; cz += sin(r)*moveSpeed; } break;
    case 'b' : case 'B':  mouseCaptured=!mouseCaptured; break;
    case 'n': case 'N':
        if(screenOn){
             screentextId = texMap["screen"];
        }else{
             screentextId = texMap["door_panel_a"];
        }
        screenOn=!screenOn; break;
    case 'u': case 'U':
        if(doorOpen){
                PlaySound(TEXT(Tex("sounds/door_close.wav").c_str()), NULL, SND_FILENAME | SND_ASYNC);
        }else{
            PlaySound(TEXT(Tex("sounds/door_open.wav").c_str()), NULL, SND_FILENAME | SND_ASYNC);
        }
        doorOpen = !doorOpen;
        doorLightOn = !doorLightOn;
        if(doorOpen) {targetDoorAngle = 120.0f;} else{ targetDoorAngle = 0.0f;}
         break;

    case 'h': case 'H': showHelp = !showHelp; break;
    case 'f': case 'F': windowOpen = !windowOpen; break;
    }



    // === CLAMP CAMERA INSIDE ROOM (only in free mode) ===
    if (camMode == CAM_FREE) {
        cx = glm::clamp(cx, -4.8f, 4.8f);
        cz = glm::clamp(cz, -3.8f, 3.8f);
        cy = glm::clamp(cy, 0.4f, 3.2f);  // prevent going through floor/ceiling
    }

    glutPostRedisplay();
}
void specialKeys(int key, int x, int y) {

    if (key == GLUT_KEY_INSERT)
    {
        toggleFullscreen();
        return;  // don't pass to camera controls
    }

    if(camMode==CAM_FREE) {
        switch(key){
        case GLUT_KEY_LEFT: yaw -= turnSpeed; if(yaw<-360) yaw=0; break;
        case GLUT_KEY_RIGHT: yaw += turnSpeed;if(yaw>360) yaw=0; break;
        case GLUT_KEY_UP: pitch += 1.5f; if(pitch>85) pitch=85; break;
        case GLUT_KEY_DOWN: pitch -= 1.5f; if(pitch<-85) pitch=-85; break;
        }
    } else {
        switch(key){
        case GLUT_KEY_LEFT: orbitAngle -= 3.0f; break;
        case GLUT_KEY_RIGHT: orbitAngle += 3.0f; break;
        case GLUT_KEY_UP: orbitRadius = glm::max(2.0f, orbitRadius - 0.2f); break;
        case GLUT_KEY_DOWN: orbitRadius = glm::min(12.0f, orbitRadius + 0.3f); break;
        }
    }
    glutPostRedisplay();
}



// --- FPS CALCULATION (ACCURATE & SMOOTH) ---
int frameCount = 0;
float fps = 0.0f;
float rawFps = 0.0f;
clock_t fpsStartTime = 0;
const int FPS_UPDATE_INTERVAL_MS = 800; // Update every 1000ms
float displayedFps = 0.0f;
// Calibration phase
std::vector<float> fpsSamples;
bool calibrationDone = false;
float fpsMin = 9999.0f;
float fpsMax = 0.0f;


float smoothDisplayedFps = 30.0f;
const float FPS_SMOOTHING = 0.1f; // lower = smoother
const int CALIBRATION_SAMPLES = 10;

void idle()
{
    clock_t now = clock();
    static clock_t lastTime = 0;
    if (lastTime == 0) lastTime = now;

    ++frameCount;

    // Update FPS every ~800ms
    if (now - fpsStartTime >= (CLOCKS_PER_SEC * FPS_UPDATE_INTERVAL_MS / 1000))
    {
        float deltaTimeSec = (float)(now - fpsStartTime) / CLOCKS_PER_SEC;
        rawFps = frameCount / deltaTimeSec;  // Real measured FPS

        // === Calibration phase: collect first 10 samples ===
        if (!calibrationDone)
        {
            fpsSamples.push_back(rawFps);

            fpsMin = std::min(fpsMin, rawFps);
            fpsMax = std::max(fpsMax, rawFps);

            if (fpsSamples.size() >= CALIBRATION_SAMPLES)
            {
                calibrationDone = true;

                // Safety: avoid division by zero
                if (fpsMax <= fpsMin) fpsMax = fpsMin + 1.0f;
            }
        }

        // === Normalize FPS to [0, 120] using calibration range ===
        if (calibrationDone)
        {
            // Map [fpsMin, fpsMax] → [0, 120]
            float t = (rawFps - fpsMin) / (fpsMax - fpsMin);  // t in [0, 1]
            t = std::max(0.0f, std::min(1.0f, t));           // clamp
            displayedFps = t * 120.0f;
        }
        else
        {
            // During calibration, maybe show raw or estimated value
            displayedFps = rawFps;
        }

        // Reset counters for next interval
        frameCount = 0;
        fpsStartTime = now;
    }

    // Your rendering code here...
    // Use 'displayedFps' for UI (e.g. bar height, color, etc.)

    smoothDisplayedFps += (displayedFps - smoothDisplayedFps) * FPS_SMOOTHING;
}

// --------------------- Display / main loop ---------------------
void display() {
    glClearColor(0.75f,0.8f,0.95f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // black background → magenta really pops

    glUseProgram(program);
    glUniform3fv(uni_objectColor, 1, glm::value_ptr(glm::vec3(1.0f)));
    glm::mat4 view = buildViewMatrix();
    glm::mat4 proj = glm::perspective(glm::radians(60.0f), (float)winW/(float)winH, 0.1f, 100.0f);
    glm::vec3 camPos = cameraPos();


    // upload common uniforms
    setCommonUniforms(view, proj, camPos);
    uploadLights();

    // draw scene using modern draws (uses M stack)
    drawRoom();


    if(screenOn){
             screentextId = texMap["screen"];
        }else{
             screentextId = texMap["door_panel_a"];
        }


    // === IMPORTANT: Clean shader state before switching to fixed pipeline ===
    glUseProgram(0);  // <--- Crucial: disable shader before legacy drawing
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
        // HUD text still uses legacy bitmap (we keep small overlay using fixed pipeline)


       // === 2D HUD / TEXT (legacy fixed-function) ===
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, winW, 0, winH);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
        //glColor3f(1,1,1);

 char buf[128];

        int lineHeight = 25;
    int starty = winH - 80;  // start near bottom-left (or change to winH - 110 as you had)

    glColor3f(1.0f,0.0f,1.0f);
    int first= false;
    if (camMode == CAM_AUTO) {
        snprintf(buf, sizeof(buf), "Camera: ORBIT");
        drawTextScreen(buf, 20, starty);

        snprintf(buf, sizeof(buf), "Angle  : %.1f degrees", orbitAngle);
        drawTextScreen(buf, 20, starty - lineHeight);

        snprintf(buf, sizeof(buf), "Radius : %.2f", orbitRadius);
        drawTextScreen(buf, 20, starty - lineHeight*2);

        snprintf(buf, sizeof(buf), "Height : %.2f", orbitHeight);
        drawTextScreen(buf, 20, starty - lineHeight*3);
        first=true;

    } else {
        snprintf(buf, sizeof(buf), "Camera: FREE (FPS-style)");
        drawTextScreen(buf, 20, starty);

        snprintf(buf, sizeof(buf), "Yaw    : %.1f degrees", yaw);
        drawTextScreen(buf, 20, starty - lineHeight);

        snprintf(buf, sizeof(buf), "Pitch  : %.1f degrees", pitch);
        drawTextScreen(buf, 20, starty - lineHeight*2);
    first=false;

    }

     // Always show current eye position (useful in both modes)
    snprintf(buf, sizeof(buf), "Eye    : %.3f, %.3f, %.3f", camPos.x, camPos.y, camPos.z);

    if(first){
        drawTextScreen(buf, 20, starty - lineHeight*4);
    }else{
        drawTextScreen(buf, 20, starty - lineHeight*3);
    }



        //glColor3f(0.7f, 1.0f, 0.7f);



        fps= smoothDisplayedFps;
        //glColor3f(fps > 55 ? 0.8f : 1.0f, 1.0f, fps > 55 ? 1.0f : 0.5f); // Green if >55, red if low
        if(fps>55){
            glColor3f(0.2f,1.0f,0.8f);
        }else{
            glColor3f(1.0f,0.2f,0.2f);
        }
        snprintf(buf, sizeof(buf),
                 "FPS: %.1f",fps);

        drawTextScreen(buf, (winW-250), (winH-50));

        // Compact help displayed on screen
        int startY =  50;
        int step = 26;

        // Titre
        glColor3f(1.0f, 0.8f, 0.2f);
        drawTextScreen("=== CONTROLES & ETAT ===", 20, startY);
        startY += step * 1.5f;

        drawTextScreen("H - Show/Hide Help",               20, startY += step);


        // Fonction pour afficher [ON] ou [OFF] en couleur
        auto printControl = [&](const char* text, bool state, int y)
        {
            glColor3f(1.0f, 1.0f, 1.0f);
            drawTextScreen(text, 30, y);
            glColor3f(state ? 0.0f : 1.0f, state ? 1.0f : 0.2f, 0.2f);
            drawTextScreen(state ? " [ON]" : " [OFF]", 380, y);
        };


        printControl("F11 - Plein ecran", isFullscreen, startY += step);


        if (showHelp)
        {
            printControl("C - Camera Auto/Libre", camMode == CAM_FREE, startY += step);
            printControl("L - Lampe de chevet", lampOn, startY += step);
            printControl("G - Plafonnier", ceilingLightOn, startY+= step);

            printControl("N - Ecran Pc", screenOn, startY+= step);
            printControl("U - Porte entree", doorOpen, startY += step);

            printControl("F - Fenetre", windowOpen, startY += step);
            if(camMode==CAM_FREE){
                printControl("Mouse Control",mouseCaptured,startY+=step);
            }

            // État spécial : lumière du jour selon ouverture fenêtre
            float openPercent = (fabs(winAngleLeft) + fabs(winAngleRight)) / 2.2f; // 0 à 100%
            char winState[64];
            sprintf(winState, "Fenetre: %.0f%% ouverte", openPercent);
            glColor3f(0.6f, 0.8f, 1.0f);
            drawTextScreen(winState, 30, startY += step);

            float doorOpenPercent = glm::abs(doorAngle) / 1.2f;
            char doorState[64];
            sprintf(doorState, "Porte: %.0f%% ouverte", doorOpenPercent);
            glColor3f(0.6f, 0.8f, 1.0f);
            drawTextScreen(doorState, 30, startY += step);
            char sunState[64];
            sprintf(sunState, "Lumiere du jour: %.0f%%", sunlight * 100.0f);
            glColor3f(1.0f, 0.9f, 0.6f);
            drawTextScreen(sunState, 30, startY += step);

            // Autres infos
            glColor3f(0.7f, 1.0f, 0.7f);
            drawTextScreen("+ / - : Intensite globale", 30, startY += step);
            drawTextScreen("P : Pause animation", 30, startY += step);
            drawTextScreen("Fleches : Tourner/Zoom (auto)", 30, startY += step);
            drawTextScreen("WASD : Deplacer (mode libre)", 30, startY += step);

    }

    //problem : if we set glcolor here , the color affects the color light !!!!
    glColor3f(1,1,1); // white
    drawTextScreen("Rouibah Hanine 2025!", (winW/2)-100, 50);
    glColor3f(1,1,0); //yellow
    drawTextScreen("Projet 02 - Scene 3D Realiste -- Informatique Graphique", 50, (winH-50));


    // ---- Restore 3D matrices ----
   // Restore 3D state
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    //glUseProgram(program);
    if (program) glUseProgram(program);

    glutSwapBuffers();
}

// timer/idle for animation
void timerFunc(int v)
{
    if(!animPaused && camMode==CAM_AUTO)
    {
        orbitAngle += 0.05f;
        if(orbitAngle > 360.0f) orbitAngle -= 360.0f;
    }
    // door smooth anim
    if(fabs(doorAngle - targetDoorAngle) > 0.1f)
    {
        float sp = 0.99f;
        if(doorAngle < targetDoorAngle) doorAngle += sp;
        else doorAngle -= sp;
    }
    // window animation and sunlight
    float tLeft = windowOpen ? 110.0f : 0.0f;
    float tRight = windowOpen ? -110.0f : 0.0f;
    float spd = 2.0f;
    if(winAngleLeft < tLeft)
    {
        winAngleLeft += spd;
    }
    if(winAngleLeft > tLeft)
    {
        winAngleLeft -= spd;
    }
    if(winAngleRight > tRight)
    {
        winAngleRight -= spd;
    }
    if(winAngleRight < tRight)
    {
        winAngleRight += spd;
    }
    /*float openAmount = (fabs(winAngleLeft) + fabs(winAngleRight)) / 180.0f;
    sunlight = 0.01f + 0.75f * glm::clamp(openAmount, 0.0f, 1.0f);*/

        // Light coming from WINDOW
    float windowOpenAmount = (fabs(winAngleLeft) + fabs(winAngleRight)) / 220.0f;  // 0 → 1

    // Light coming from DOOR (hallway light)
    float doorOpenAmount = glm::abs(doorAngle) / 120.0f;  // 0 when closed, 1 when fully open

    // Combine both sources (you can tweak the weights)
    float totalLightFromOutside = glm::clamp(windowOpenAmount * 0.75f + doorOpenAmount * 0.5f, 0.0f, 1.0f);

    // Final daylight intensity in the room
    sunlight = 0.0f + 0.95f * totalLightFromOutside;  // from almost dark → very bright
    glutPostRedisplay();
    glutTimerFunc(16, timerFunc, 0);

    // Add this line at the end of timerFunc()
orbitRadius = glm::clamp(orbitRadius, MIN_SAFE_ORBIT_RADIUS, MAX_SAFE_ORBIT_RADIUS);
}

// reshape
void reshape(int w, int h)
{
    winW = w;
    winH = h;
    glViewport(0,0,w,h);
}

void motion(int x, int y) {
    if (camMode != CAM_FREE || !mouseCaptured) return;

    if (lastMouseX == -1) { // First call
        lastMouseX = x;
        lastMouseY = y;
        return;
    }

    int dx = x - lastMouseX;
    int dy = y - lastMouseY;

    yaw += dx * mouseSensitivity;
    pitch -= dy * mouseSensitivity;  // Inverted Y is standard for FPS

    // Normalize yaw
    if (yaw > 360.0f) yaw -= 360.0f;
    if (yaw < 0.0f) yaw += 360.0f;

    // Clamp pitch to avoid flipping
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    lastMouseX = x;
    lastMouseY = y;

    // Warp pointer back to center to allow continuous movement
    int centerX = winW / 2;
    int centerY = winH / 2;
    if (x != centerX || y != centerY) {
        glutWarpPointer(centerX, centerY);
        lastMouseX = centerX;
        lastMouseY = centerY;
    }

    glutPostRedisplay();
}


// --------------------- Init modern GL ---------------------
void initGL() {
    glewExperimental = GL_TRUE;
    GLenum g = glewInit();
    if(g != GLEW_OK) {
        std::cerr<<"glewInit failed\n"; exit(1);
    }

    //start pipeline : //Full pipeline: load files → compile both → create program → attach → link → delete individual shaders.
    //load files
    vertexSrc = readFile(Tex("shaders/shader.vert"));
    fragSrc = readFile(Tex("shaders/shader.frag"));

    // compile shaders
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexSrc.c_str());
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragSrc.c_str());

    //create program -> attache -> link -> delete shaders !
    program = linkProgram(vs, fs);
    // uniform locations
    uni_model = glGetUniformLocation(program, "model");
    uni_view  = glGetUniformLocation(program, "view");
    uni_proj  = glGetUniformLocation(program, "proj");
    uni_viewPos = glGetUniformLocation(program, "viewPos");
    uni_useTexture = glGetUniformLocation(program, "useTexture");
    uni_tex0 = glGetUniformLocation(program, "tex0");
    uni_objectColor = glGetUniformLocation(program, "objectColor");
    uni_emission = glGetUniformLocation(program, "emission");
    uni_alpha = glGetUniformLocation(program, "alpha");

    glEnable(GL_DEPTH_TEST);

    createCube();
    createPlane();
    createQuad();

}

//---------------------------------------------------------------------------------------------------
int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(winW, winH);
    glutCreateWindow("Projet 02 - Scene 3D Realiste");

    initGL();
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_COLOR_MATERIAL);     // usually not needed, but safe
glDisable(GL_LIGHTING);
    // load a few example textures (adjust paths to your Ressources folder)
    auto tryLoad = [&](const std::string &key, const std::string &path, bool inverse=true)
    {
        GLuint id = loadTexture(path, inverse);
        if(id) texMap[key] = id;
        else std::cerr<<"Failed to load: "<<path<<"\n";
    };

    tryLoad("floor", Tex("textures/floor.jpg").c_str());
    tryLoad("Wall3", Tex("textures/Wall3.jpg").c_str());
    tryLoad("door_panel_a", Tex("textures/door_panel_a.jpg").c_str());
    tryLoad("door_glass2", Tex("textures/door_glass2.png").c_str());
    tryLoad("landscape", Tex("images/landscape.jpg").c_str(), false);
    tryLoad("me", Tex("images/me.png").c_str(), false);
    tryLoad("HR", Tex("images/HR.jpg").c_str());
    tryLoad("ceiling", Tex("textures/ceiling.jpg").c_str());
    tryLoad("BlueWood", Tex("textures/BlueWood.jpg").c_str());
    tryLoad("texbed", Tex("textures/texbed.png").c_str());
    tryLoad("texlamp2", Tex("textures/texlamp2.png").c_str());
    tryLoad("door_panel_b", Tex("textures/door_panel_b.jpg").c_str());
    tryLoad("metal", Tex("textures/metal.jpg").c_str());
    tryLoad("texbed", Tex("textures/texbed.png").c_str());
    tryLoad("texBedtable", Tex("textures/texbedtable.png").c_str());
    tryLoad("texGChair", Tex("textures/Chair.png").c_str());
    tryLoad("algeria", Tex("images/algeria.png").c_str());
    tryLoad("Slogan", Tex("images/Title.png").c_str(),false);
    tryLoad("woood2", Tex("textures/doow.jpg").c_str(),false);
    tryLoad("wardrobe", Tex("textures/wardrobe.png").c_str());
    tryLoad("book", Tex("textures/book_tex.png").c_str());
    tryLoad("pots", Tex("textures/texpot.png").c_str());
    tryLoad("tapi", Tex("images/trash3.jpg").c_str());
    tryLoad("ceil-Lamp", Tex("textures/ceil-Lamp.png").c_str());
    tryLoad("Bike", Tex("textures/texforbike.png").c_str());
    tryLoad("Chair", Tex("textures/Chair2.png").c_str());
    tryLoad("lamp", Tex("textures/texlamp2.png").c_str());

    tryLoad("UclCup", Tex("textures/UclCup.png").c_str());
    tryLoad("PlasticCup", Tex("textures/PlasticCup.png").c_str());
    tryLoad("WindowTex", Tex("textures/texwindow.jpg").c_str());
    tryLoad("screen", Tex("images/windows.jpg").c_str());



    // load sample OBJ models (non-blocking for missing files)
    Mesh Keyboard;
    if(loadOBJtoMesh(Tex("objects/keyboard.obj"), Keyboard)) meshes["keyboard.obj"] = Keyboard;

    Mesh lamp;
    if(loadOBJtoMesh(Tex("objects/lamp.obj"), lamp)) meshes["lamp.obj"] = lamp;
    Mesh mouse;
    if(loadOBJtoMesh(Tex("objects/Mouse.obj"), mouse)) meshes["Mouse.obj"] = mouse;

    Mesh Bed;
    if(loadOBJtoMesh(Tex("objects/bed.obj"), Bed)) meshes["bed.obj"] = Bed;

    Mesh Table;
    if(loadOBJtoMesh(Tex("objects/bedtable.obj"), Table)) meshes["table.obj"] = Table;

    Mesh GrandChair;
    if(loadOBJtoMesh(Tex("objects/GChair.obj"), GrandChair)) meshes["GChair.obj"] = GrandChair;

    Mesh TrashCan;
    if(loadOBJtoMesh(Tex("objects/TrashCan-2.obj"), TrashCan)) meshes["TrashCan.obj"] = TrashCan;

    Mesh Paper;
    if(loadOBJtoMesh(Tex("objects/Paper2.obj"), Paper)) meshes["Paper.obj"] = Paper;

    Mesh canlowpoly;
    if(loadOBJtoMesh(Tex("objects/canlowpoly.obj"), canlowpoly)) meshes["canlowpoly.obj"] = canlowpoly;

    Mesh Desktop;
    if(loadOBJtoMesh(Tex("objects/Desktop.obj"), Desktop)) meshes["Desktop.obj"] = Desktop;

    Mesh Wardrobe;
    if(loadOBJtoMesh(Tex("objects/wardrob.obj"), Wardrobe)) meshes["wardrobe.obj"] = Wardrobe;

    Mesh PC;
    if(loadOBJtoMesh(Tex("objects/PC.obj"), PC)) meshes["PC.obj"] = PC;

     Mesh PCUnite;
    if(loadOBJtoMesh(Tex("objects/unite.obj"), PCUnite)) meshes["PCUnite.obj"] = PCUnite;

    Mesh PCSpeaker;
    if(loadOBJtoMesh(Tex("objects/Speaker.obj"), PCSpeaker)) meshes["PCSpeaker.obj"] = PCSpeaker;

    Mesh shelf;
    if(loadOBJtoMesh(Tex("objects/shelf.obj"), shelf)) meshes["shelf.obj"] = shelf;

    Mesh Book;
    if(loadOBJtoMesh(Tex("objects/book.obj"), Book)) meshes["book.obj"] = Book;

    Mesh Pots;
    if(loadOBJtoMesh(Tex("objects/pots.obj"), Pots)) meshes["pots.obj"] = Pots;

    Mesh CeilLamp;
    if(loadOBJtoMesh(Tex("objects/ceil-Lamp.obj"), CeilLamp)) meshes["CeilLamp.obj"] = CeilLamp;

     Mesh Bike;
    if(loadOBJtoMesh(Tex("objects/bike.obj"), Bike)) meshes["Bike.obj"] = Bike;

     Mesh Chair;
    if(loadOBJtoMesh(Tex("objects/Chair.obj"), Chair)) meshes["Chair.obj"] = Chair;



    Mesh UclCup;
    if(loadOBJtoMesh(Tex("objects/UclCup.obj"), UclCup)) meshes["UclCup.obj"] = UclCup;

    Mesh PlasticCup;
    if(loadOBJtoMesh(Tex("objects/PlasticCup.obj"), PlasticCup)) meshes["PlasticCup.obj"] = PlasticCup;

    Mesh tablec;
    if(loadOBJtoMesh(Tex("objects/ctable.obj"), tablec)) meshes["tablec.obj"] = tablec;

    // setup GLUT callbacks
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutReshapeFunc(reshape);
    glutTimerFunc(16, timerFunc, 0);

    glutMotionFunc(motion);        // For when mouse button is held
    glutPassiveMotionFunc(motion); // For free movement (recommended)
    glutIdleFunc(idle);

    glutMainLoop();
    return 0;
}

//End
//c'est était Rouibah Hanine !
//2025
