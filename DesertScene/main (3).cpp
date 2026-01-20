// ===================== INCLUDES =====================
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#define GLEW_STATIC

#define LOG(x) do { if (gVerbose) { std::cout << x << std::endl; } } while(0)

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <algorithm>
#include <vector>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include "Model3D.hpp"

gps::Model3D treeModel;
gps::Model3D ruinsModel;
gps::Model3D architectureModel;
gps::Model3D torchModel;
gps::Model3D cactusModel;
gps::Model3D cactus2Model;
gps::Model3D firstPlantModel;
gps::Model3D camelModel;
gps::Model3D snakeModel;
gps::Model3D dryPlantModel;
gps::Model3D tree2Model;
gps::Model3D tumbleweedModel;


gps::Shader myShader;
gps::Shader torchShader;



// ===================== WINDOW =====================
const unsigned int SCR_WIDTH = 1000;
const unsigned int SCR_HEIGHT = 800;

// ===================== CAMERA =====================
glm::vec3 cameraPos(0.0f, 1.5f, 5.0f);
glm::vec3 cameraFront(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);

float yaw = -90.0f;
float pitch = 0.0f;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

bool gVerbose = false;

// ===================== DUNE DATA =====================
const int GRID_SIZE = 500;
const float GRID_SCALE = 1.0f;

std::vector<float> duneVertices;
std::vector<unsigned int> duneIndices;

GLuint duneVAO, duneVBO, duneEBO;
GLuint duneShader;
GLuint sandTexture;


glm::vec3 lightDir = glm::normalize(glm::vec3(-0.8f, -0.55f, -0.6f));// jos, din lateral
glm::vec3 lightColor = glm::vec3(1.0f, 0.85f, 0.65f);// portocaliu (sunset)
glm::vec3 ambientColor = glm::vec3(0.06f, 0.05f, 0.045f); // ambient cald, mai slab


constexpr int TORCH_COUNT = 2;

glm::vec3 gTorchPos[TORCH_COUNT];
glm::vec3 gTorchColor[TORCH_COUNT] = {
    glm::vec3(1.0f, 0.45f, 0.18f),
    glm::vec3(1.0f, 0.45f, 0.18f)
};

float gTorchRadius[TORCH_COUNT] = { 7.5f, 7.5f };
float gTorchIntensity[TORCH_COUNT] = { 2.2f, 2.2f };


struct FlattenZone { float cx, cz, radius, strength; };
float ruinsRadius = 10.0f;   // raza platformei

glm::vec3 ruinsPos(14.0f, 0.0f, -5.5f);
float ruinsScale = 1.0f;
float ruinsSink = 0.65f;   // dacă vrei să le “îngropi” puțin în nisip
float ruinsYawDeg = 0.0f;  // rotație pe Y (opțional)

glm::vec3 treePos(4.5f, 0.0f, -5.0f);
float treeScale = 4.5f;
float treeSink = 0.0f;   // dacă vrei să-l mai bagi în nisip

glm::vec3 cactusPos(8.0f, 0.0f, -10.0f);
float cactusScale = 3.0f;
float cactusSink = 0.0f;   // îl bagi puțin în nisip dacă vrei

glm::vec3 cactus2Pos(15.0f, 0.0f, -1.0f);
float cactus2Scale = 3.0f;
float cactus2Sink = 0.0f;
float cactus2BaseOffset = 0.0f;

glm::vec3 firstPlantPos(14.0f, 0.0f, 0.8f);
float firstPlantScale = 0.3f;
float firstPlantSink = 0.0f;

glm::vec3 camelPos(6.5f, 0.0f, 0.1f);
float camelScale = 1.5f;
float camelSink = 0.0f;

glm::vec3 snakePos(15.0f, 0.0f, -9.5f);
float snakeScale = 0.02f;
float snakeSink = 0.0f;

glm::vec3 dryPlantPos(18.0f, 0.0f, -8.5f);
float dryPlantScale = 1.5f;
float dryPlantSink = 0.0f;

glm::vec3 tree2Pos(-65.5f, 0.0f, -65.5f);
float tree2Scale = 4.5f;
float tree2Sink = 0.0f;

glm::vec3 tumbleweedPos = glm::vec3(-10.5f, 0.0f, -5.1f);
float tumbleweedSpeed = 2.0f;     // unitati pe secunda
float tumbleweedScale = 0.15f;     // ajustezi dupa model
float tumbleweedAngle = 0.0f;     // optional rotatie
float deltaTime = 0.0f;
float lastFrame = 0.0f;



// zone plate unde vrei sa pui obiecte
std::vector<FlattenZone> flattenZones = {
    { ruinsPos.x,  ruinsPos.z,  ruinsRadius, 1.0f }, // ruine
    { treePos.x,   treePos.z,   4.0f,        1.0f }, // copac
    { cactusPos.x, cactusPos.z, 3.0f,        1.0f },
    { cactus2Pos.x, cactus2Pos.z, 3.0f, 1.0f }, // cactus
    { -45.0f, -110.0f, 14.0f, 1.0f },               // architecture
    {camelPos.x, camelPos.z, 3.0f, 1.0f},
    {snakePos.x, snakePos.z, 3.0f, 1.0f},
    {dryPlantPos.x, dryPlantPos.z, 3.0f, 1.0f},

};



static float clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
static float smooth01(float t) { return t * t * (3.0f - 2.0f * t); }

// Poziția templului (folosește exact aceleași valori ca în renderArchitecture)
glm::vec3 archPos(-45.0f, 0.0f, -110.0f);
float archScale = 1.0f;
float archSink = 0.85f;


// float archYawDeg = 0.0f; // dacă vrei rotație la templu

glm::mat4 computeArchitectureModelMatrix();
glm::mat4 computeLeftTorchModelMatrix();
glm::mat4 computeRightTorchModelMatrix();

// === CAMEL ANIMATION ===
float camelAnimOffset = 0.0f;
float camelSpeed = 0.8f;        // unități pe secundă
float camelMaxDist = 4.0f;      // distanța maximă
int camelDir = 1;               // 1 = înainte, -1 = înapoi

enum RenderMode {
    RENDER_SOLID = 0,
    RENDER_WIREFRAME,
    RENDER_POINT
};

RenderMode gRenderMode = RENDER_SOLID;
bool gSmoothEnabled = true;

// ===================== SHADOW MAP =====================
const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;

GLuint shadowMapFBO;
GLuint depthMapTexture;

gps::Shader depthMapShader;

float ruinsBaseY = -0.4f;  
// ===================== MOUSE =====================
void mouse_callback(GLFWwindow*, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float sensitivity = 0.1f;
    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;

    lastX = (float)xpos;
    lastY = (float)ypos;

    yaw += xoffset * sensitivity;
    pitch += yoffset * sensitivity;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 dir;
    dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    dir.y = sin(glm::radians(pitch));
    dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(dir);
}

// ===================== INPUT =====================
void processInput(GLFWwindow* window)
{
    float speed = 5.0f * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
    static bool key1Pressed = false;
    static bool key2Pressed = false;
    static bool key3Pressed = false;
    static bool key4Pressed = false;

    // SOLID
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS && !key1Pressed) {
        gRenderMode = RENDER_SOLID;
        key1Pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_RELEASE)
        key1Pressed = false;

    // WIREFRAME
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS && !key2Pressed) {
        gRenderMode = RENDER_WIREFRAME;
        key2Pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_RELEASE)
        key2Pressed = false;

    // POINT / POLIGONAL
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS && !key3Pressed) {
        gRenderMode = RENDER_POINT;
        key3Pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_RELEASE)
        key3Pressed = false;

    // SMOOTH ON / OFF
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS && !key4Pressed) {
        gSmoothEnabled = !gSmoothEnabled;
        key4Pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_RELEASE)
        key4Pressed = false;
    float twMove = tumbleweedSpeed * deltaTime;

// SAGETI = TUMBLEWEED
if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
    tumbleweedPos.z -= twMove;

if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
    tumbleweedPos.z += twMove;

if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
    tumbleweedPos.x -= twMove;

if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
    tumbleweedPos.x += twMove;

// rotatie cand se misca
if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS ||
    glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS ||
    glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS ||
    glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
{
    tumbleweedAngle += 180.0f * deltaTime;
}

}

// ===================== DUNE GENERATION =====================
float getHeightRaw(float x, float z)
{
    float h1 = 4.0f * sin(0.02f * x) * cos(0.02f * z);
    float h2 = 1.5f * sin(0.06f * x + 1.3f) * cos(0.06f * z);
    float h3 = 0.2f * sin(0.18f * x) * cos(0.18f * z);
    return h1 + h2 + h3;
}

float getHeight(float x, float z)
{
    float h = getHeightRaw(x, z);

    for (const auto& zone : flattenZones)
    {
        float dx = x - zone.cx;
        float dz = z - zone.cz;
        float d = std::sqrt(dx * dx + dz * dz);

        float w = clamp01(1.0f - d / zone.radius);
        w = smooth01(w) * zone.strength;        // 0..1

        float h0 = getHeightRaw(zone.cx, zone.cz);  // “inaltimea platformei”
        h = h * (1.0f - w) + h0 * w;               // mix spre plat
    }

    return h;
}



void generateDunes()
{
    duneVertices.clear();
    duneIndices.clear();

    for (int z = 0; z < GRID_SIZE; z++)
    {
        for (int x = 0; x < GRID_SIZE; x++)
        {
            float xpos = (x - GRID_SIZE / 2) * GRID_SCALE;
            float zpos = (z - GRID_SIZE / 2) * GRID_SCALE;

            float height = getHeight(xpos, zpos);

            // derivari numerice
            float eps = GRID_SCALE;
            float hL = getHeight(xpos - eps, zpos);
            float hR = getHeight(xpos + eps, zpos);
            float hD = getHeight(xpos, zpos - eps);
            float hU = getHeight(xpos, zpos + eps);

            glm::vec3 normal = glm::normalize(glm::vec3(
                hL - hR,
                2.0f,
                hD - hU
            ));

            // pozitie
            duneVertices.push_back(xpos);
            duneVertices.push_back(height);
            duneVertices.push_back(zpos);

            // normala REALA
            duneVertices.push_back(normal.x);
            duneVertices.push_back(normal.y);
            duneVertices.push_back(normal.z);
        }
    }

    for (int z = 0; z < GRID_SIZE - 1; z++)
    {
        for (int x = 0; x < GRID_SIZE - 1; x++)
        {
            int i = z * GRID_SIZE + x;

            duneIndices.push_back(i);
            duneIndices.push_back(i + GRID_SIZE);
            duneIndices.push_back(i + 1);

            duneIndices.push_back(i + 1);
            duneIndices.push_back(i + GRID_SIZE);
            duneIndices.push_back(i + GRID_SIZE + 1);
        }
    }
}


// ===================== BUFFERS =====================
void initDuneBuffers()
{
    glGenVertexArrays(1, &duneVAO);
    glGenBuffers(1, &duneVBO);
    glGenBuffers(1, &duneEBO);

    glBindVertexArray(duneVAO);

    glBindBuffer(GL_ARRAY_BUFFER, duneVBO);
    glBufferData(GL_ARRAY_BUFFER,
        duneVertices.size() * sizeof(float),
        duneVertices.data(),
        GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, duneEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        duneIndices.size() * sizeof(unsigned int),
        duneIndices.data(),
        GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
        (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);


    glBindVertexArray(0);
}

void initShadowMap()
{
    glGenFramebuffers(1, &shadowMapFBO);

    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT,
        0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D,
        depthMapTexture,
        0
    );

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeRuinsModelMatrix(float yOffset = 0.0f)
{
    float y = getHeight(ruinsPos.x, ruinsPos.z) - ruinsSink + yOffset;
    glm::mat4 M(1.0f);
    M = glm::translate(M, glm::vec3(ruinsPos.x, y, ruinsPos.z));
    M = glm::rotate(M, glm::radians(ruinsYawDeg), glm::vec3(0, 1, 0));
    M = glm::scale(M, glm::vec3(ruinsScale));
    return M;
}


glm::mat4 computeLightSpaceTrMatrix()
{
    // centru aproximativ al scenei tale
    glm::vec3 sceneCenter(
        cameraPos.x,
        0.0f,
        cameraPos.z
    );


    glm::mat4 lightView = glm::lookAt(
        sceneCenter - lightDir * 60.0f,
        sceneCenter,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    glm::mat4 lightProjection = glm::ortho(
        -80.0f, 80.0f,
        -80.0f, 80.0f,
        0.1f, 200.0f
    );

    return lightProjection * lightView;
}



GLuint loadTexture(const char* path)
{
    GLuint textureID;
    glGenTextures(1, &textureID);

    // Debug: verifica dacă fisierul exista (mai ales la cai relative)
    std::ifstream f(path, std::ios::binary);
    if (!f.good())
    {
        std::cout << "FAILED TO OPEN TEXTURE FILE: " << path << "\n";
        return 0;
    }
    f.close();

    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);

    unsigned char* data = stbi_load(path, &width, &height, &channels, 0);
    if (!data)
    {
        std::cout << "FAILED TO LOAD TEXTURE: " << path << "\n";
        std::cout << "stb_image reason: " << stbi_failure_reason() << "\n";
        return 0;
    }

    GLenum format = GL_RGB;
    if (channels == 1) format = GL_RED;
    else if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format,
        width, height, 0,
        format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return textureID;
}



GLuint loadShader(const char* vsPath, const char* fsPath)
{
    auto loadFile = [](const char* path) {
        std::ifstream file(path);
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
        };

    std::string vsCode = loadFile(vsPath);
    std::string fsCode = loadFile(fsPath);

    const char* vsSrc = vsCode.c_str();
    const char* fsSrc = fsCode.c_str();

    GLuint v = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v, 1, &vsSrc, nullptr);
    glCompileShader(v);

    GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f, 1, &fsSrc, nullptr);
    glCompileShader(f);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, v);
    glAttachShader(prog, f);
    glLinkProgram(prog);

    glDeleteShader(v);
    glDeleteShader(f);
    return prog;
}



static std::string trim(std::string s)
{
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.erase(s.begin());
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n')) s.pop_back();

    // dacă e între ghilimele
    if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\'')))
        s = s.substr(1, s.size() - 2);

    return s;
}



glm::mat4 computeRightTorchModelMatrix()
{
    float baseY = getHeight(archPos.x, archPos.z) - archSink;

    glm::mat4 M(1.0f);
    M = glm::translate(M, glm::vec3(-40.7f, baseY + 0.5f, -55.5f));
    M = glm::scale(M, glm::vec3(9.0f));
    return M;
}



glm::mat4 computeLeftTorchModelMatrix()
{
    float baseY = getHeight(archPos.x, archPos.z) - archSink;

    glm::mat4 M(1.0f);
    M = glm::translate(M, glm::vec3(-49.1f, baseY + 0.5f, -55.5f));
    M = glm::scale(M, glm::vec3(9.0f));
    return M;
}



glm::mat4 computeArchitectureModelMatrix()
{
    float groundY = getHeight(archPos.x, archPos.z);
    float baseY = groundY - archSink;

    glm::mat4 M(1.0f);
    M = glm::translate(M, glm::vec3(archPos.x, baseY, archPos.z));
    M = glm::scale(M, glm::vec3(archScale));
    return M;
}

void updateCamelAnimation(float deltaTime)
{
    camelAnimOffset += camelDir * camelSpeed * deltaTime;

    if (camelAnimOffset > camelMaxDist)
    {
        camelAnimOffset = camelMaxDist;
        camelDir = -1;
    }
    else if (camelAnimOffset < -camelMaxDist)
    {
        camelAnimOffset = -camelMaxDist;
        camelDir = 1;
    }
}

void renderTumbleweed()
{
    myShader.useShaderProgram();
    GLuint p = myShader.shaderProgram;

    // === SHADOW MAP ===
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(p, "shadowMap"), 1);

    glUniformMatrix4fv(
        glGetUniformLocation(p, "lightSpaceTrMatrix"),
        1, GL_FALSE,
        glm::value_ptr(computeLightSpaceTrMatrix())
    );

    // === LIGHTING ===
    glUniform3fv(glGetUniformLocation(p, "lightDir"), 1, glm::value_ptr(lightDir));
    glUniform3fv(glGetUniformLocation(p, "lightColor"), 1, glm::value_ptr(lightColor));
    glUniform3fv(glGetUniformLocation(p, "ambientColor"), 1, glm::value_ptr(ambientColor));
    glUniform3fv(glGetUniformLocation(p, "viewPos"), 1, glm::value_ptr(cameraPos));

    // === MATERIAL (simplu) ===
    glUniform3f(glGetUniformLocation(p, "materialTint"), 1.0f, 1.0f, 1.0f);
    glUniform1f(glGetUniformLocation(p, "specStrength"), 0.05f);
    glUniform1f(glGetUniformLocation(p, "shininess"), 8.0f);

    // === FOG ===
    glUniform3f(glGetUniformLocation(p, "fogColor"), 0.75f, 0.70f, 0.60f);
    glUniform1f(glGetUniformLocation(p, "fogDensity"), 0.006f);

    // === TEXTURE ===
    glUniform1i(glGetUniformLocation(p, "diffuseTexture"), 0);

    // === MATRICES ===
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::mat4 projection = glm::perspective(
        glm::radians(60.0f),
        (float)SCR_WIDTH / SCR_HEIGHT,
        0.1f,
        1000.0f
    );

    float y = getHeight(tumbleweedPos.x, tumbleweedPos.z);

    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(tumbleweedPos.x, y, tumbleweedPos.z));
    model = glm::rotate(model, glm::radians(tumbleweedAngle), glm::vec3(0, 1, 0));
    model = glm::scale(model, glm::vec3(tumbleweedScale));

    glUniformMatrix4fv(glGetUniformLocation(p, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(p, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(p, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    tumbleweedModel.Draw(myShader);
}


void renderTree2()
{
    myShader.useShaderProgram();
    GLuint p = myShader.shaderProgram;

    // === LIGHTING ===
    glUniform3fv(glGetUniformLocation(p, "lightDir"), 1, glm::value_ptr(lightDir));
    glUniform3fv(glGetUniformLocation(p, "lightColor"), 1, glm::value_ptr(lightColor));
    glUniform3fv(glGetUniformLocation(p, "ambientColor"), 1, glm::value_ptr(ambientColor));
    glUniform3fv(glGetUniformLocation(p, "viewPos"), 1, glm::value_ptr(cameraPos));

    // === MATERIAL ===
    glUniform3f(glGetUniformLocation(p, "materialTint"), 1.0f, 1.0f, 1.0f);
    glUniform1f(glGetUniformLocation(p, "specStrength"), 0.08f);
    glUniform1f(glGetUniformLocation(p, "shininess"), 16.0f);

    // === EMISSIVE (OFF) ===
    glUniform1f(glGetUniformLocation(p, "uEmissive"), 0.0f);
    glUniform3f(glGetUniformLocation(p, "uEmissiveColor"), 1.0f, 0.5f, 0.2f);

    // === FOG ===
    glUniform3f(glGetUniformLocation(p, "fogColor"), 0.75f, 0.70f, 0.60f);
    glUniform1f(glGetUniformLocation(p, "fogDensity"), 0.006f);

    // === TEXTURE ===
    glUniform1i(glGetUniformLocation(p, "diffuseTexture"), 0);

    // === MATRICES ===
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::mat4 projection = glm::perspective(
        glm::radians(60.0f),
        (float)SCR_WIDTH / SCR_HEIGHT,
        0.1f,
        1000.0f
    );

    float y = getHeight(tree2Pos.x, tree2Pos.z) - treeSink;

    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(tree2Pos.x, y, tree2Pos.z));
    model = glm::scale(model, glm::vec3(tree2Scale));

    glUniformMatrix4fv(glGetUniformLocation(p, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(p, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(p, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // === DRAW ===
    tree2Model.Draw(myShader);
}

void renderDryPlant()
{
    myShader.useShaderProgram();
    GLuint p = myShader.shaderProgram;
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(p, "shadowMap"), 1);

    glUniformMatrix4fv(
        glGetUniformLocation(p, "lightSpaceTrMatrix"),
        1, GL_FALSE,
        glm::value_ptr(computeLightSpaceTrMatrix())
    );


    // === LIGHTING ===
    glUniform3fv(glGetUniformLocation(p, "lightDir"), 1, glm::value_ptr(lightDir));
    glUniform3fv(glGetUniformLocation(p, "lightColor"), 1, glm::value_ptr(lightColor));
    glUniform3fv(glGetUniformLocation(p, "ambientColor"), 1, glm::value_ptr(ambientColor));
    glUniform3fv(glGetUniformLocation(p, "viewPos"), 1, glm::value_ptr(cameraPos));

    // === MATERIAL ===
    glUniform3f(glGetUniformLocation(p, "materialTint"), 1.0f, 1.0f, 1.0f);
    glUniform1f(glGetUniformLocation(p, "specStrength"), 0.05f);
    glUniform1f(glGetUniformLocation(p, "shininess"), 8.0f);

    // === EMISSIVE OFF ===
    glUniform1f(glGetUniformLocation(p, "uEmissive"), 0.0f);

    // === FOG ===
    glUniform3f(glGetUniformLocation(p, "fogColor"), 0.75f, 0.70f, 0.60f);
    glUniform1f(glGetUniformLocation(p, "fogDensity"), 0.006f);

    // === TEXTURE ===
    glUniform1i(glGetUniformLocation(p, "diffuseTexture"), 0);

    // === MATRICES ===
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::mat4 projection = glm::perspective(
        glm::radians(60.0f),
        (float)SCR_WIDTH / SCR_HEIGHT,
        0.1f,
        1000.0f
    );

    float y = getHeight(dryPlantPos.x, dryPlantPos.z) - dryPlantSink;

    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(dryPlantPos.x, y, dryPlantPos.z));
    model = glm::scale(model, glm::vec3(dryPlantScale));

    glUniformMatrix4fv(glGetUniformLocation(p, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(p, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(p, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    dryPlantModel.Draw(myShader);
}

void renderSnake()
{
    myShader.useShaderProgram();
    GLuint p = myShader.shaderProgram;
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(p, "shadowMap"), 1);

    glUniformMatrix4fv(
        glGetUniformLocation(p, "lightSpaceTrMatrix"),
        1, GL_FALSE,
        glm::value_ptr(computeLightSpaceTrMatrix())
    );


    // === LIGHTING ===
    glUniform3fv(glGetUniformLocation(p, "lightDir"), 1, glm::value_ptr(lightDir));
    glUniform3fv(glGetUniformLocation(p, "lightColor"), 1, glm::value_ptr(lightColor));
    glUniform3fv(glGetUniformLocation(p, "ambientColor"), 1, glm::value_ptr(ambientColor));
    glUniform3fv(glGetUniformLocation(p, "viewPos"), 1, glm::value_ptr(cameraPos));

    // === MATERIAL ===
    glUniform3f(glGetUniformLocation(p, "materialTint"), 1.0f, 1.0f, 1.0f);
    glUniform1f(glGetUniformLocation(p, "specStrength"), 0.05f);
    glUniform1f(glGetUniformLocation(p, "shininess"), 8.0f);

    // === EMISSIVE OFF ===
    glUniform1f(glGetUniformLocation(p, "uEmissive"), 0.0f);

    // === FOG ===
    glUniform3f(glGetUniformLocation(p, "fogColor"), 0.75f, 0.70f, 0.60f);
    glUniform1f(glGetUniformLocation(p, "fogDensity"), 0.006f);

    // === TEXTURE ===
    glUniform1i(glGetUniformLocation(p, "diffuseTexture"), 0);

    // === MATRICES ===
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::mat4 projection = glm::perspective(
        glm::radians(60.0f),
        (float)SCR_WIDTH / SCR_HEIGHT,
        0.1f,
        1000.0f
    );
    float t = (float)glfwGetTime();
    float forwardBack = sin(t * 1.0f) * 0.4f;
    float idleRot = sin(t * 0.7f) * glm::radians(5.0f);

    float y = getHeight(snakePos.x, snakePos.z + forwardBack) - snakeSink;

    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(snakePos.x + forwardBack, y, snakePos.z));

    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, idleRot, glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(snakeScale));

    glUniformMatrix4fv(glGetUniformLocation(p, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(p, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(p, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    snakeModel.Draw(myShader);
}

void renderCamel()
{
    myShader.useShaderProgram();
    GLuint p = myShader.shaderProgram;
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(p, "shadowMap"), 1);

    glUniformMatrix4fv(
        glGetUniformLocation(p, "lightSpaceTrMatrix"),
        1, GL_FALSE,
        glm::value_ptr(computeLightSpaceTrMatrix())
    );


    // === LIGHTING ===
    glUniform3fv(glGetUniformLocation(p, "lightDir"), 1, glm::value_ptr(lightDir));
    glUniform3fv(glGetUniformLocation(p, "lightColor"), 1, glm::value_ptr(lightColor));
    glUniform3fv(glGetUniformLocation(p, "ambientColor"), 1, glm::value_ptr(ambientColor));
    glUniform3fv(glGetUniformLocation(p, "viewPos"), 1, glm::value_ptr(cameraPos));

    // === MATERIAL ===
    glUniform3f(glGetUniformLocation(p, "materialTint"), 1.0f, 1.0f, 1.0f);
    glUniform1f(glGetUniformLocation(p, "specStrength"), 0.05f);
    glUniform1f(glGetUniformLocation(p, "shininess"), 8.0f);

    // === EMISSIVE OFF ===
    glUniform1f(glGetUniformLocation(p, "uEmissive"), 0.0f);

    // === FOG ===
    glUniform3f(glGetUniformLocation(p, "fogColor"), 0.75f, 0.70f, 0.60f);
    glUniform1f(glGetUniformLocation(p, "fogDensity"), 0.006f);

    // === TEXTURE ===
    glUniform1i(glGetUniformLocation(p, "diffuseTexture"), 0);

    // === MATRICES ===
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::mat4 projection = glm::perspective(
        glm::radians(60.0f),
        (float)SCR_WIDTH / SCR_HEIGHT,
        0.1f,
        1000.0f
    );

    /*float y = getHeight(camelPos.x, camelPos.z) - camelSink;

    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(camelPos.x, y, camelPos.z));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(camelScale));*/
    float t = (float)glfwGetTime();

    float idleY = sin(t * 1.2f) * 0.03f;              // sus–jos
    float idleRot = sin(t * 0.7f) * glm::radians(2.0f); // balans

    float y = getHeight(camelPos.x, camelPos.z) - camelSink;

    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(camelPos.x, y + idleY, camelPos.z));

    // rotație FIXĂ la 180° (față de orientarea inițială)
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0, 1, 0));

    // balans idle
    model = glm::rotate(model, idleRot, glm::vec3(0, 0, 1));

    // scale
    model = glm::scale(model, glm::vec3(camelScale));


    glUniformMatrix4fv(glGetUniformLocation(p, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(p, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(p, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    camelModel.Draw(myShader);
}

void renderFirstPlant()
{
    myShader.useShaderProgram();
    GLuint p = myShader.shaderProgram;
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(p, "shadowMap"), 1);

    glUniformMatrix4fv(
        glGetUniformLocation(p, "lightSpaceTrMatrix"),
        1, GL_FALSE,
        glm::value_ptr(computeLightSpaceTrMatrix())
    );


    // === LIGHTING ===
    glUniform3fv(glGetUniformLocation(p, "lightDir"), 1, glm::value_ptr(lightDir));
    glUniform3fv(glGetUniformLocation(p, "lightColor"), 1, glm::value_ptr(lightColor));
    glUniform3fv(glGetUniformLocation(p, "ambientColor"), 1, glm::value_ptr(ambientColor));
    glUniform3fv(glGetUniformLocation(p, "viewPos"), 1, glm::value_ptr(cameraPos));

    // === MATERIAL ===
    glUniform3f(glGetUniformLocation(p, "materialTint"), 1.0f, 1.0f, 1.0f);
    glUniform1f(glGetUniformLocation(p, "specStrength"), 0.05f);
    glUniform1f(glGetUniformLocation(p, "shininess"), 8.0f);

    // === EMISSIVE OFF ===
    glUniform1f(glGetUniformLocation(p, "uEmissive"), 0.0f);

    // === FOG ===
    glUniform3f(glGetUniformLocation(p, "fogColor"), 0.75f, 0.70f, 0.60f);
    glUniform1f(glGetUniformLocation(p, "fogDensity"), 0.006f);

    // === TEXTURE ===
    glUniform1i(glGetUniformLocation(p, "diffuseTexture"), 0);

    // === MATRICES ===
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::mat4 projection = glm::perspective(
        glm::radians(60.0f),
        (float)SCR_WIDTH / SCR_HEIGHT,
        0.1f,
        1000.0f
    );

    float y = getHeight(firstPlantPos.x, firstPlantPos.z) - firstPlantSink;

    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(firstPlantPos.x, y, firstPlantPos.z));
    model = glm::scale(model, glm::vec3(firstPlantScale));

    glUniformMatrix4fv(glGetUniformLocation(p, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(p, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(p, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    firstPlantModel.Draw(myShader);
}

void renderSecondCactus()
{
    myShader.useShaderProgram();
    GLuint p = myShader.shaderProgram;
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(p, "shadowMap"), 1);

    glUniformMatrix4fv(
        glGetUniformLocation(p, "lightSpaceTrMatrix"),
        1, GL_FALSE,
        glm::value_ptr(computeLightSpaceTrMatrix())
    );


    glUniform3fv(glGetUniformLocation(p, "lightDir"), 1, glm::value_ptr(lightDir));
    glUniform3fv(glGetUniformLocation(p, "lightColor"), 1, glm::value_ptr(lightColor));
    glUniform3fv(glGetUniformLocation(p, "ambientColor"), 1, glm::value_ptr(ambientColor));
    glUniform3fv(glGetUniformLocation(p, "viewPos"), 1, glm::value_ptr(cameraPos));

    glUniform3f(glGetUniformLocation(p, "materialTint"), 1.0f, 1.0f, 1.0f);
    glUniform1f(glGetUniformLocation(p, "specStrength"), 0.05f);
    glUniform1f(glGetUniformLocation(p, "shininess"), 8.0f);

    glUniform1f(glGetUniformLocation(p, "uEmissive"), 0.0f);

    glUniform3f(glGetUniformLocation(p, "fogColor"), 0.75f, 0.70f, 0.60f);
    glUniform1f(glGetUniformLocation(p, "fogDensity"), 0.006f);

    glUniform1i(glGetUniformLocation(p, "diffuseTexture"), 0);

    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::mat4 projection = glm::perspective(
        glm::radians(60.0f),
        (float)SCR_WIDTH / SCR_HEIGHT,
        0.1f, 1000.0f
    );

    float y = getHeight(cactus2Pos.x, cactus2Pos.z)
        + cactus2BaseOffset
        - cactus2Sink;

    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(cactus2Pos.x, y, cactus2Pos.z));
    model = glm::scale(model, glm::vec3(cactus2Scale));

    glUniformMatrix4fv(glGetUniformLocation(p, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(p, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(p, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    cactus2Model.Draw(myShader);
}


void renderCactus()
{
    myShader.useShaderProgram();
    GLuint p = myShader.shaderProgram;
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(p, "shadowMap"), 1);

    glUniformMatrix4fv(
        glGetUniformLocation(p, "lightSpaceTrMatrix"),
        1, GL_FALSE,
        glm::value_ptr(computeLightSpaceTrMatrix())
    );


    // === LIGHTING ===
    glUniform3fv(glGetUniformLocation(p, "lightDir"), 1, glm::value_ptr(lightDir));
    glUniform3fv(glGetUniformLocation(p, "lightColor"), 1, glm::value_ptr(lightColor));
    glUniform3fv(glGetUniformLocation(p, "ambientColor"), 1, glm::value_ptr(ambientColor));
    glUniform3fv(glGetUniformLocation(p, "viewPos"), 1, glm::value_ptr(cameraPos));

    // === MATERIAL ===
    glUniform3f(glGetUniformLocation(p, "materialTint"), 1.0f, 1.0f, 1.0f);
    glUniform1f(glGetUniformLocation(p, "specStrength"), 0.05f);
    glUniform1f(glGetUniformLocation(p, "shininess"), 8.0f);

    // === EMISSIVE OFF ===
    glUniform1f(glGetUniformLocation(p, "uEmissive"), 0.0f);

    // === FOG ===
    glUniform3f(glGetUniformLocation(p, "fogColor"), 0.75f, 0.70f, 0.60f);
    glUniform1f(glGetUniformLocation(p, "fogDensity"), 0.006f);

    // === TEXTURE ===
    glUniform1i(glGetUniformLocation(p, "diffuseTexture"), 0);

    // === MATRICES ===
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::mat4 projection = glm::perspective(
        glm::radians(60.0f),
        (float)SCR_WIDTH / SCR_HEIGHT,
        0.1f,
        1000.0f
    );

    float y = getHeight(cactusPos.x, cactusPos.z) - cactusSink;

    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(cactusPos.x, y, cactusPos.z));
    model = glm::scale(model, glm::vec3(cactusScale));

    glUniformMatrix4fv(glGetUniformLocation(p, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(p, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(p, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    cactusModel.Draw(myShader);
}


void renderLeftTorchOnTemple()
{
    torchShader.useShaderProgram();



    GLuint p = torchShader.shaderProgram;
    

    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(p, "diffuseTexture"), 0);

    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::mat4 projection = glm::perspective(
        glm::radians(60.0f),
        (float)SCR_WIDTH / SCR_HEIGHT,
        0.1f, 1000.0f
    );

    glm::mat4 model = computeLeftTorchModelMatrix();

    glUniformMatrix4fv(glGetUniformLocation(p, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(p, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(p, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glUniform3fv(glGetUniformLocation(p, "viewPos"), 1, glm::value_ptr(cameraPos));

    glUniform3f(glGetUniformLocation(p, "ambientColor"), 0.08f, 0.06f, 0.04f);
    glUniform3f(glGetUniformLocation(p, "emissiveColor"), 1.0f, 0.45f, 0.18f);
    glUniform1f(glGetUniformLocation(p, "emissiveStrength"), 0.8f);


    torchModel.Draw(torchShader);
}


void renderRightTorchOnTemple()
{
    torchShader.useShaderProgram();

    GLuint p = torchShader.shaderProgram;
    

    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(p, "diffuseTexture"), 0);


    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::mat4 projection = glm::perspective(
        glm::radians(60.0f),
        (float)SCR_WIDTH / SCR_HEIGHT,
        0.1f, 1000.0f
    );

    glm::mat4 model = computeRightTorchModelMatrix();

    glUniformMatrix4fv(glGetUniformLocation(p, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(p, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(p, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glUniform3fv(glGetUniformLocation(p, "viewPos"), 1, glm::value_ptr(cameraPos));

    glUniform3f(glGetUniformLocation(p, "ambientColor"), 0.08f, 0.06f, 0.04f);
    glUniform3f(glGetUniformLocation(p, "emissiveColor"), 1.0f, 0.45f, 0.18f);
    glUniform1f(glGetUniformLocation(p, "emissiveStrength"), 0.8f);



    torchModel.Draw(torchShader);
}





void renderArchitecture()
{
    myShader.useShaderProgram();
    GLuint p = myShader.shaderProgram;
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(p, "shadowMap"), 1);

    glUniformMatrix4fv(
        glGetUniformLocation(p, "lightSpaceTrMatrix"),
        1, GL_FALSE,
        glm::value_ptr(computeLightSpaceTrMatrix())
    );


    glUniform3fv(glGetUniformLocation(p, "lightDir"), 1, glm::value_ptr(lightDir));
    glUniform3fv(glGetUniformLocation(p, "lightColor"), 1, glm::value_ptr(lightColor));
    glUniform3fv(glGetUniformLocation(p, "ambientColor"), 1, glm::value_ptr(ambientColor));
    glUniform3fv(glGetUniformLocation(p, "viewPos"), 1, glm::value_ptr(cameraPos));

    // material
    glUniform3f(glGetUniformLocation(p, "materialTint"), 1.0f, 1.0f, 1.0f);
    glUniform1f(glGetUniformLocation(p, "specStrength"), 0.08f);
    glUniform1f(glGetUniformLocation(p, "shininess"), 16.0f);

    // emissive (default OFF)
    glUniform1f(glGetUniformLocation(p, "uEmissive"), 0.0f);
    glUniform3f(glGetUniformLocation(p, "uEmissiveColor"), 1.0f, 0.5f, 0.2f);

    // fog
    glUniform3f(glGetUniformLocation(p, "fogColor"), 0.75f, 0.70f, 0.60f);
    glUniform1f(glGetUniformLocation(p, "fogDensity"), 0.006f);

    // texture unit
    glUniform1i(glGetUniformLocation(p, "diffuseTexture"), 0);


    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::mat4 projection = glm::perspective(glm::radians(60.0f),
        (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 1000.0f);

    glm::mat4 model = computeArchitectureModelMatrix();

    glUniformMatrix4fv(glGetUniformLocation(myShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(myShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(myShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    architectureModel.Draw(myShader);
}


void renderRuins()
{
    myShader.useShaderProgram();
    GLuint p = myShader.shaderProgram;

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(p, "shadowMap"), 1);

    glUniformMatrix4fv(
        glGetUniformLocation(p, "lightSpaceTrMatrix"),
        1, GL_FALSE,
        glm::value_ptr(computeLightSpaceTrMatrix())
    );

    // lighting
    glUniform3fv(glGetUniformLocation(p, "lightDir"), 1, glm::value_ptr(lightDir));
    glUniform3fv(glGetUniformLocation(p, "lightColor"), 1, glm::value_ptr(lightColor));
    glUniform3fv(glGetUniformLocation(p, "ambientColor"), 1, glm::value_ptr(ambientColor));
    glUniform3fv(glGetUniformLocation(p, "viewPos"), 1, glm::value_ptr(cameraPos));

    // material
    glUniform3f(glGetUniformLocation(p, "materialTint"), 1.0f, 1.0f, 1.0f);
    glUniform1f(glGetUniformLocation(p, "specStrength"), 0.08f);
    glUniform1f(glGetUniformLocation(p, "shininess"), 16.0f);

    // fog
    glUniform3f(glGetUniformLocation(p, "fogColor"), 0.75f, 0.70f, 0.60f);
    glUniform1f(glGetUniformLocation(p, "fogDensity"), 0.006f);

    // matrices
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::mat4 projection = glm::perspective(
        glm::radians(60.0f),
        (float)SCR_WIDTH / SCR_HEIGHT,
        0.1f, 1000.0f
    );

    glm::mat4 model = computeRuinsModelMatrix(0.0f); // ✅ DOAR ASTA

    glUniformMatrix4fv(glGetUniformLocation(p, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(p, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(p, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    ruinsModel.Draw(myShader);
}





// ===================== SHADER =====================

void renderTree()
{
    myShader.useShaderProgram();
    GLuint p = myShader.shaderProgram;
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(p, "shadowMap"), 1);

    glUniformMatrix4fv(
        glGetUniformLocation(p, "lightSpaceTrMatrix"),
        1, GL_FALSE,
        glm::value_ptr(computeLightSpaceTrMatrix())
    );


    // === LIGHTING ===
    glUniform3fv(glGetUniformLocation(p, "lightDir"), 1, glm::value_ptr(lightDir));
    glUniform3fv(glGetUniformLocation(p, "lightColor"), 1, glm::value_ptr(lightColor));
    glUniform3fv(glGetUniformLocation(p, "ambientColor"), 1, glm::value_ptr(ambientColor));
    glUniform3fv(glGetUniformLocation(p, "viewPos"), 1, glm::value_ptr(cameraPos));

    // === MATERIAL ===
    glUniform3f(glGetUniformLocation(p, "materialTint"), 1.0f, 1.0f, 1.0f);
    glUniform1f(glGetUniformLocation(p, "specStrength"), 0.08f);
    glUniform1f(glGetUniformLocation(p, "shininess"), 16.0f);

    // === EMISSIVE (OFF) ===
    glUniform1f(glGetUniformLocation(p, "uEmissive"), 0.0f);
    glUniform3f(glGetUniformLocation(p, "uEmissiveColor"), 1.0f, 0.5f, 0.2f);

    // === FOG ===
    glUniform3f(glGetUniformLocation(p, "fogColor"), 0.75f, 0.70f, 0.60f);
    glUniform1f(glGetUniformLocation(p, "fogDensity"), 0.006f);

    // === TEXTURE ===
    glUniform1i(glGetUniformLocation(p, "diffuseTexture"), 0);

    // === MATRICES ===
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::mat4 projection = glm::perspective(
        glm::radians(60.0f),
        (float)SCR_WIDTH / SCR_HEIGHT,
        0.1f,
        1000.0f
    );

    float y = getHeight(treePos.x, treePos.z) - treeSink;

    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(treePos.x, y, treePos.z));
    model = glm::scale(model, glm::vec3(treeScale));

    glUniformMatrix4fv(glGetUniformLocation(p, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(p, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(p, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // === DRAW ===
    treeModel.Draw(myShader);
}



void renderDunes()
{
    glUseProgram(duneShader);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(duneShader, "shadowMap"), 1);

    glUniformMatrix4fv(
        glGetUniformLocation(duneShader, "lightSpaceTrMatrix"),
        1, GL_FALSE,
        glm::value_ptr(computeLightSpaceTrMatrix())
    );

    glUniform3f(glGetUniformLocation(duneShader, "fogColor"), 0.75f, 0.70f, 0.60f);
    glUniform1f(glGetUniformLocation(duneShader, "fogDensity"), 0.006f); // începe cu 0.004..0.008

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::mat4 projection = glm::perspective(
        glm::radians(60.0f),
        (float)SCR_WIDTH / SCR_HEIGHT,
        0.1f,
        1000.0f
    );

    glUniformMatrix4fv(glGetUniformLocation(duneShader, "model"),
        1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(duneShader, "view"),
        1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(duneShader, "projection"),
        1, GL_FALSE, glm::value_ptr(projection));

    glUniform3fv(glGetUniformLocation(duneShader, "lightDir"),
        1, glm::value_ptr(lightDir));
    glUniform3fv(glGetUniformLocation(duneShader, "lightColor"),
        1, glm::value_ptr(lightColor));
    glUniform3fv(glGetUniformLocation(duneShader, "viewPos"),
        1, glm::value_ptr(cameraPos));
    glUniform3fv(glGetUniformLocation(duneShader, "ambientColor"), 1, glm::value_ptr(ambientColor));
    //glUniform1f(glGetUniformLocation(duneShader, "fogDensity"), 0.0f);


    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sandTexture);
    glUniform1i(glGetUniformLocation(duneShader, "sandTexture"), 0);

    glBindVertexArray(duneVAO);
    glDrawElements(GL_TRIANGLES,
        (GLsizei)duneIndices.size(),
        GL_UNSIGNED_INT,
        0);
}

static float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }

static float flicker(float t, float phase)
{
    float f = 0.85f
        + 0.15f * sinf(t * 7.0f + phase)
        + 0.08f * sinf(t * 13.0f + phase * 2.3f);
    return clampf(f, 0.65f, 1.20f);
}

void updateTorchLights(float timeSec)
{
    glm::mat4 ML = computeLeftTorchModelMatrix();
    glm::mat4 MR = computeRightTorchModelMatrix();

    // poziția flăcării în modelul torței (local space)
    glm::vec3 flameLocalOffset(0.0f, 0.36f, 0.14f);

    gTorchPos[0] = glm::vec3(ML * glm::vec4(flameLocalOffset, 1.0f));
    gTorchPos[1] = glm::vec3(MR * glm::vec4(flameLocalOffset, 1.0f));

    float base = 3.2f;
    gTorchIntensity[0] = base * flicker(timeSec, 0.0f);
    gTorchIntensity[1] = base * flicker(timeSec, 1.7f);
}




void uploadTorchUniforms(GLuint program)
{
    GLint locCount = glGetUniformLocation(program, "uTorchCount");
    if (locCount < 0) return; // shaderul nu are torțe (încă)

    glUniform1i(locCount, TORCH_COUNT);

    GLint locPos = glGetUniformLocation(program, "uTorchPos[0]");
    GLint locCol = glGetUniformLocation(program, "uTorchColor[0]");
    GLint locRad = glGetUniformLocation(program, "uTorchRadius[0]");
    GLint locInt = glGetUniformLocation(program, "uTorchIntensity[0]");

    if (locPos >= 0) glUniform3fv(locPos, TORCH_COUNT, &gTorchPos[0].x);
    if (locCol >= 0) glUniform3fv(locCol, TORCH_COUNT, &gTorchColor[0].x);
    if (locRad >= 0) glUniform1fv(locRad, TORCH_COUNT, &gTorchRadius[0]);
    if (locInt >= 0) glUniform1fv(locInt, TORCH_COUNT, &gTorchIntensity[0]);
}

void renderDepthPass(gps::Shader& shader)
{
    shader.useShaderProgram();

    glm::mat4 lightSpace = computeLightSpaceTrMatrix();
    glUniformMatrix4fv(
        glGetUniformLocation(shader.shaderProgram, "lightSpaceTrMatrix"),
        1, GL_FALSE, glm::value_ptr(lightSpace)
    );

    glm::mat4 model;  // ✅ DECLARAT O SINGURĂ DATĂ

    // ================= DUNE =================
    model = glm::mat4(1.0f);
    glUniformMatrix4fv(
        glGetUniformLocation(shader.shaderProgram, "model"),
        1, GL_FALSE, glm::value_ptr(model)
    );
    glBindVertexArray(duneVAO);
    glDrawElements(GL_TRIANGLES, (GLsizei)duneIndices.size(), GL_UNSIGNED_INT, 0);

    // ================= TREE =================
    model = glm::translate(glm::mat4(1.0f),
        glm::vec3(treePos.x, getHeight(treePos.x, treePos.z), treePos.z));
    model = glm::scale(model, glm::vec3(treeScale));
    glUniformMatrix4fv(
        glGetUniformLocation(shader.shaderProgram, "model"),
        1, GL_FALSE, glm::value_ptr(model)
    );
    treeModel.Draw(shader);

    // ================= RUINS =================
    model = computeRuinsModelMatrix(0.02f);  // ✅ DOAR ASIGNARE

    glUniformMatrix4fv(
        glGetUniformLocation(shader.shaderProgram, "model"),
        1, GL_FALSE, glm::value_ptr(model)
    );
    ruinsModel.Draw(shader);

    // ================= SNAKE =================
    float t = (float)glfwGetTime();
    float forwardBack = sin(t * 1.0f) * 0.4f;
    float idleRot = sin(t * 0.7f) * glm::radians(5.0f);

    float y = getHeight(snakePos.x, snakePos.z + forwardBack) - snakeSink;

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(
        snakePos.x + forwardBack,
        y,
        snakePos.z
    ));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0, 1, 0));
    model = glm::rotate(model, idleRot, glm::vec3(0, 0, 1));
    model = glm::scale(model, glm::vec3(snakeScale));

    glUniformMatrix4fv(
        glGetUniformLocation(shader.shaderProgram, "model"),
        1, GL_FALSE, glm::value_ptr(model)
    );
    snakeModel.Draw(shader);

    // ================= CACTUS 1 =================
    model = glm::mat4(1.0f);
    model = glm::translate(model,
        glm::vec3(
            cactusPos.x,
            getHeight(cactusPos.x, cactusPos.z) - cactusSink + 0.02f,
            cactusPos.z
        )
    );
    model = glm::scale(model, glm::vec3(cactusScale));

    glUniformMatrix4fv(
        glGetUniformLocation(shader.shaderProgram, "model"),
        1, GL_FALSE, glm::value_ptr(model)
    );
    cactusModel.Draw(shader);

    // ================= CACTUS 2 =================
    model = glm::mat4(1.0f);
    model = glm::translate(model,
        glm::vec3(
            cactus2Pos.x,
            getHeight(cactus2Pos.x, cactus2Pos.z) - cactus2Sink + 0.02f,
            cactus2Pos.z
        )
    );
    model = glm::scale(model, glm::vec3(cactus2Scale));

    glUniformMatrix4fv(
        glGetUniformLocation(shader.shaderProgram, "model"),
        1, GL_FALSE, glm::value_ptr(model)
    );
    cactus2Model.Draw(shader);

    // ================= CAMEL =================
    model = glm::mat4(1.0f);
    model = glm::translate(model,
        glm::vec3(
            camelPos.x,
            getHeight(camelPos.x, camelPos.z) - camelSink + 0.02f,
            camelPos.z
        )
    );
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0, 1, 0));
    model = glm::scale(model, glm::vec3(camelScale));

    glUniformMatrix4fv(
        glGetUniformLocation(shader.shaderProgram, "model"),
        1, GL_FALSE, glm::value_ptr(model)
    );
    camelModel.Draw(shader);


    // ================= ARCHITECTURE (TEMPLE) =================
    model = computeArchitectureModelMatrix();  // NU mai adaugi translate/scale manual

    glUniformMatrix4fv(
        glGetUniformLocation(shader.shaderProgram, "model"),
        1, GL_FALSE, glm::value_ptr(model)
    );
    architectureModel.Draw(shader);

    // ================= TREE 2 =================
    model = glm::mat4(1.0f);
    model = glm::translate(model,
        glm::vec3(
            tree2Pos.x,
            getHeight(tree2Pos.x, tree2Pos.z) - tree2Sink + 0.02f,
            tree2Pos.z
        )
    );
    model = glm::scale(model, glm::vec3(tree2Scale));

    glUniformMatrix4fv(
        glGetUniformLocation(shader.shaderProgram, "model"),
        1, GL_FALSE, glm::value_ptr(model)
    );
    tree2Model.Draw(shader);

    // ================= TUMBLEWEED =================
    y = getHeight(tumbleweedPos.x, tumbleweedPos.z);

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(tumbleweedPos.x, y, tumbleweedPos.z));
    model = glm::rotate(model, glm::radians(tumbleweedAngle), glm::vec3(0, 1, 0));
    model = glm::scale(model, glm::vec3(tumbleweedScale));

    glUniformMatrix4fv(
        glGetUniformLocation(shader.shaderProgram, "model"),
        1, GL_FALSE, glm::value_ptr(model)
    );
    tumbleweedModel.Draw(shader);


}




// ===================== RENDER =====================
void renderScene(GLFWwindow* window)
{
    float now = (float)glfwGetTime();
    updateTorchLights(now);
    glDisable(GL_CULL_FACE);


    // ===================== 1) DEPTH PASS =====================
    depthMapShader.useShaderProgram();

    glm::mat4 lightSpaceMatrix = computeLightSpaceTrMatrix();
    glUniformMatrix4fv(
        glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
        1, GL_FALSE, glm::value_ptr(lightSpaceMatrix)
    );

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    renderDepthPass(depthMapShader);
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    myShader.useShaderProgram();
    uploadTorchUniforms(myShader.shaderProgram);


    if (duneShader)
    {
        glUseProgram(duneShader);
        uploadTorchUniforms(duneShader);
    }

    processInput(window);
    float groundY = getHeight(cameraPos.x, cameraPos.z);
    cameraPos.y = groundY + 1.7f; // înălțimea ochilor

    // cer
    glClearColor(0.75f, 0.70f, 0.60f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // === RENDER MODE ===
    switch (gRenderMode)
    {
    case RENDER_SOLID:
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        break;

    case RENDER_WIREFRAME:
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        break;

    case RENDER_POINT:
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        break;
    }

    // === SMOOTH / FLAT ===
    if (gSmoothEnabled)
    {
        glEnable(GL_LINE_SMOOTH);
        glEnable(GL_POINT_SMOOTH);
    }
    else
    {
        glDisable(GL_LINE_SMOOTH);
        glDisable(GL_POINT_SMOOTH);
    }


    // === DUNE ===
    renderDunes();
    renderTree();
    renderRuins();
    renderArchitecture();
    renderLeftTorchOnTemple();
    renderRightTorchOnTemple();
    renderCactus();
    renderSecondCactus();
    renderFirstPlant();
    renderCamel();
    updateCamelAnimation(deltaTime);
    renderSnake();
    renderDryPlant();
    renderTree2();
    renderTumbleweed();
}

// ===================== MAIN =====================
int main()
{
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Desert Dunes", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    glewInit();
    glEnable(GL_DEPTH_TEST);
    initShadowMap();

    /*depthMapShader.loadShader(
        "shaders/shaderDepth.vert",
        "shaders/shaderDepth.frag"
    );*/


    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    myShader.loadShader(
        "shaders/shaderModel.vert",
        "shaders/shaderModel.frag"
    );
    torchShader.loadShader(
        "shaders/shaderTorch.vert",
        "shaders/shaderTorch.frag"
    );
    depthMapShader.loadShader(
        "shaders/shaderDepth.vert",
        "shaders/shaderDepth.frag"
    );



    // IMPORTANT: baseDir trebuie să fie folderul în care e .mtl (și față de care sunt căile din map_Kd)
    treeModel.LoadModel("models/sahara_tree/sahara_tree.obj", "models/sahara_tree/");
    ruinsModel.LoadModel("models/desert_ruins/desert_ruins.obj", "models/desert_ruins/");
    architectureModel.LoadModel(
        "models/desert_architecture_building/desert_architecture.obj",
        "models/desert_architecture_building/"
    );
    torchModel.LoadModel("models/torch/torch.obj", "models/torch/");
    cactusModel.LoadModel(
        "models/plants/firstcactus.obj",
        "models/plants/"
    );
    cactus2Model.LoadModel(
        "models/plants/secondcactus.obj",
        "models/plants/"
    );
    firstPlantModel.LoadModel("models/plants/plant.obj", "models/plants/");
    camelModel.LoadModel("models/camel/camel.obj", "models/camel/");
    snakeModel.LoadModel("models/snake/snake.obj", "models/snake/");
    dryPlantModel.LoadModel("models/plants/dryplant1.obj", "models/plants/");
    tree2Model.LoadModel("models/sahara_tree/saharatree2.obj", "models/sahara_tree/");
    tumbleweedModel.LoadModel("models/tumbleweed/tumbleweed.obj");


    // dacă frunzele au alpha (PNG), activează blending (opțional dar util)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    generateDunes();

    initDuneBuffers();
    sandTexture = loadTexture("textures/sand.jpg");
    duneShader = loadShader(
        "shaders/shaderDune.vert",
        "shaders/shaderDune.frag"
    );
    while (!glfwWindowShouldClose(window))
    {
        // === deltaTime (O SINGURA DATA) ===
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // === input / movement ===
        processInput(window);

        // === render ===
        renderScene(window);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

 
    glfwTerminate();
    return 0;
}