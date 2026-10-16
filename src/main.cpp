/**
* @file src/main.cpp
* @brief FernOpenGL 引导程序：GLFW + GLAD 最小可运行渲染管线
* @details
* 目标：验证 GLFW + GLAD 工具链打通，并跑通一条 GPU 渲染管线。
*
* 本文件刻意保持"零外部依赖"（除 GLFW/GLAD 外）：
*   下面的 Mat4 是本文件的私有工具，不是 Fern 引擎的数学库。
*   Fern::Matrix4x4 / Fern::Vector3D 的移植是下一步的独立任务。
*
* 这个源文件是我要求DeepSeek为确认我的库安装正确生成的文件，所以不应该署我的名字。
* 
* @author DeepSeek
* @date 2026/10/17
*/

#include <glad/glad.h>      // 必须在 GLFW 之前：它负责加载 OpenGL 函数指针
#include <GLFW/glfw3.h>

#include <cmath>
#include <cstdio>
#include <cstddef>
#include <iostream>

// ============================================================================
// 1. 本文件私有数学工具（临时；后续替换为 Fern::Matrix4x4 / Fern::Vector3D）
// ============================================================================

namespace {

/// 列主序 4x4 矩阵，与 OpenGL 的存储约定一致
struct Mat4 {
    float m[16];    // m[col * 4 + row]

    static Mat4 Identity() {
        Mat4 r{};
        for (int i = 0; i < 16; ++i) r.m[i] = 0.0f;
        r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
        return r;
    }

    static Mat4 Translation(float tx, float ty, float tz) {
        Mat4 r = Identity();
        r.m[12] = tx; r.m[13] = ty; r.m[14] = tz;
        return r;
    }

    static Mat4 RotationX(float rad) {
        Mat4 r = Identity();
        float c = std::cos(rad), s = std::sin(rad);
        r.m[5] = c;  r.m[6] = s;
        r.m[9] = -s; r.m[10] = c;
        return r;
    }

    static Mat4 RotationY(float rad) {
        Mat4 r = Identity();
        float c = std::cos(rad), s = std::sin(rad);
        r.m[0] = c;  r.m[2] = -s;
        r.m[8] = s;  r.m[10] = c;
        return r;
    }

    /// 透视投影。相机空间 +Z 朝前（右手系），NDC 的 z 落在 [0, 1]。
    ///
    /// 与 Fern::GeneratePerspectiveMatrix 的关系（重要）：
    ///   Fern 的软件管线采用「行向量 × 矩阵」(v' = v * M)，
    ///   而 GLSL 采用「矩阵 × 列向量」(v' = M * v)，两者互为转置。
    ///   同一个投影矩阵在两个体系里的摆放方式不同，因此这里的
    ///   第 3 行（z 行）与第 4 行（w 行）需要取相反符号：
    ///
    ///     Fern（行向量）:  m[2][2] = +zFar/(zFar-zNear),  m[3][2] = +1
    ///     OpenGL（列向量）: m[10]  = -zFar/(zFar-zNear),  m[11]  = -1
    ///
    ///   取 m[11] = -1 使 w = -z_view；相机前方的几何 z_view < 0，
    ///   于是 w > 0，不会被视锥体裁剪。已验证：
    ///     z_view = -zNear -> ndc.z = 0，z_view = -zFar -> ndc.z = 1
    static Mat4 Perspective(float fovY_rad, float aspect, float zNear, float zFar) {
        Mat4 r{};
        for (int i = 0; i < 16; ++i) r.m[i] = 0.0f;
        float f = 1.0f / std::tan(fovY_rad / 2.0f);
        r.m[0]  = f / aspect;
        r.m[5]  = f;
        r.m[10] = -zFar / (zFar - zNear);                   // z 行取反
        r.m[11] = -1.0f;                                    // w 行取反
        r.m[14] = -zNear * zFar / (zFar - zNear);
        return r;
    }
};

Mat4 Multiply(const Mat4& a, const Mat4& b) {
    Mat4 r{};
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) {
                sum += a.m[k * 4 + row] * b.m[col * 4 + k];
            }
            r.m[col * 4 + row] = sum;
        }
    }
    return r;
}

/// 用 MVP 把点变换到 NDC，用于诊断
bool ProjectToNdc(const Mat4& mvp, float x, float y, float z,
                  float* ndcX, float* ndcY, float* ndcZ, float* wOut) {
    float cx = mvp.m[0] * x + mvp.m[4] * y + mvp.m[8]  * z + mvp.m[12];
    float cy = mvp.m[1] * x + mvp.m[5] * y + mvp.m[9]  * z + mvp.m[13];
    float cz = mvp.m[2] * x + mvp.m[6] * y + mvp.m[10] * z + mvp.m[14];
    float cw = mvp.m[3] * x + mvp.m[7] * y + mvp.m[11] * z + mvp.m[15];
    *wOut = cw;
    if (std::fabs(cw) < 1e-6f) return false;
    *ndcX = cx / cw; *ndcY = cy / cw; *ndcZ = cz / cw;
    return true;
}

// ============================================================================
// 2. 着色器源码
// ============================================================================

const char* kVertexShader = R"(
#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 uMVP;

out vec3 vColor;

void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vColor = aColor;
}
)";

const char* kFragmentShader = R"(
#version 460 core
in vec3 vColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(vColor, 1.0);
}
)";

// ============================================================================
// 3. 工具与诊断函数
// ============================================================================

GLuint CompileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << "[shader compile failed] " << log << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint LinkProgram(const char* vsSrc, const char* fsSrc) {
    GLuint vs = CompileShader(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fsSrc);
    if (!vs || !fs) return 0;

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint ok = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        std::cerr << "[program link failed] " << log << std::endl;
        glDeleteProgram(prog);
        prog = 0;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

const char* GlErrorName(GLenum e) {
    switch (e) {
        case GL_NO_ERROR:          return "GL_NO_ERROR";
        case GL_INVALID_ENUM:      return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE:     return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
        case GL_OUT_OF_MEMORY:     return "GL_OUT_OF_MEMORY";
        case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
        default:                   return "GL_UNKNOWN_ERROR";
    }
}

int CheckGlError(const char* tag) {
    int count = 0;
    for (GLenum e = glGetError(); e != GL_NO_ERROR; e = glGetError()) {
        std::cerr << "[GL ERROR] at " << tag << " -> " << GlErrorName(e)
                  << " (0x" << std::hex << e << std::dec << ")" << std::endl;
        if (++count > 8) break;
    }
    return count;
}

void CheckFramebuffer(const char* tag) {
    GLenum st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (st != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[FBO INCOMPLETE] at " << tag << " -> 0x"
                  << std::hex << st << std::dec << std::endl;
    } else {
        std::cout << "FBO     : complete" << std::endl;
    }
}

void WaitSeconds(double sec) {
    double t0 = glfwGetTime();
    while (glfwGetTime() - t0 < sec) {
        glfwPollEvents();
    }
}

/// 诊断：打印立方体 8 个角点的 NDC 落点
void DiagnoseMvp(const Mat4& mvp) {
    const float kProbe[8][3] = {
        { 1, -1, -1}, { 1,  1, -1}, { 1,  1,  1}, { 1, -1,  1},
        {-1, -1, -1}, {-1,  1, -1}, {-1,  1,  1}, {-1, -1,  1},
    };
    std::cout << "MVP 顶点落点（NDC，应落在 [-1,1] 内）:" << std::endl;
    for (int i = 0; i < 8; ++i) {
        float nx, ny, nz, w;
        if (!ProjectToNdc(mvp, kProbe[i][0], kProbe[i][1], kProbe[i][2], &nx, &ny, &nz, &w)) {
            std::cout << "  顶点" << i << " w~0，不可投影" << std::endl;
            continue;
        }
        bool inside = (nx >= -1.0f && nx <= 1.0f && ny >= -1.0f && ny <= 1.0f
                       && nz >= -1.0f && nz <= 1.0f);
        std::printf("  顶点%d  ndc=(%6.3f, %6.3f, %6.3f)  w=%5.2f  %s\n",
                    i, nx, ny, nz, w, inside ? "在裁剪体内" : "*** 在裁剪体外 ***");
    }
}

} // namespace

// ============================================================================
// 4. 入口
// ============================================================================

int main() {
    if (!glfwInit()) {
        std::cerr << "[fatal] glfwInit failed" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "FernOpenGL - L0", nullptr, nullptr);
    if (!window) {
        std::cerr << "[fatal] glfwCreateWindow failed" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "[fatal] gladLoadGLLoader failed" << std::endl;
        return -1;
    }
    std::cout << "OpenGL  : " << glGetString(GL_VERSION) << "\n"
              << "Renderer: " << glGetString(GL_RENDERER) << "\n"
              << "GLSL    : " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    // 给窗口留出真正显示出来的时间
    WaitSeconds(0.5);

    int winW = 0, winH = 0, fbW = 0, fbH = 0;
    glfwGetWindowSize(window, &winW, &winH);
    glfwGetFramebufferSize(window, &fbW, &fbH);
    std::cout << "窗口尺寸: " << winW << " x " << winH
              << "   帧缓冲: " << fbW << " x " << fbH << std::endl;

    const float kCube[] = {
        // +X 红
         1,-1,-1,  1,0,0,   1, 1,-1,  1,0,0,   1, 1, 1,  1,0,0,
         1,-1,-1,  1,0,0,   1, 1, 1,  1,0,0,   1,-1, 1,  1,0,0,
        // -X 青
        -1,-1,-1,  0,1,1,  -1,-1, 1,  0,1,1,  -1, 1, 1,  0,1,1,
        -1,-1,-1,  0,1,1,  -1, 1, 1,  0,1,1,  -1, 1,-1,  0,1,1,
        // +Y 绿
        -1, 1,-1,  0,1,0,  -1, 1, 1,  0,1,0,   1, 1, 1,  0,1,0,
        -1, 1,-1,  0,1,0,   1, 1, 1,  0,1,0,   1, 1,-1,  0,1,0,
        // -Y 品红
        -1,-1,-1,  1,0,1,   1,-1,-1,  1,0,1,   1,-1, 1,  1,0,1,
        -1,-1,-1,  1,0,1,   1,-1, 1,  1,0,1,  -1,-1, 1,  1,0,1,
        // +Z 蓝
        -1,-1, 1,  0,0,1,   1,-1, 1,  0,0,1,   1, 1, 1,  0,0,1,
        -1,-1, 1,  0,0,1,   1, 1, 1,  0,0,1,  -1, 1, 1,  0,0,1,
        // -Z 黄
        -1,-1,-1,  1,1,0,  -1, 1,-1,  1,1,0,   1, 1,-1,  1,1,0,
        -1,-1,-1,  1,1,0,   1, 1,-1,  1,1,0,   1,-1,-1,  1,1,0,
    };

    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kCube), kCube, GL_STATIC_DRAW);

    const GLsizei stride = 6 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    std::cout << "VAO=" << vao << "  VBO=" << vbo
              << "  顶点字节数=" << sizeof(kCube) << std::endl;
    CheckGlError("buffer setup");

    GLuint program = LinkProgram(kVertexShader, kFragmentShader);
    if (!program) {
        std::cerr << "[fatal] shader program build failed" << std::endl;
        WaitSeconds(10.0);
        return -1;
    }
    GLint uMVP   = glGetUniformLocation(program, "uMVP");
    std::cout << "program=" << program << "  uMVP=" << uMVP << std::endl;
    if (uMVP < 0) {
        std::cerr << "[warn] uMVP 未被找到，矩阵不会被上传" << std::endl;
    }
    CheckGlError("shader setup");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    CheckGlError("state setup");

    CheckFramebuffer("startup");

    bool diagnosed = false;
    int  frameCount = 0;

    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        int fw = 0, fh = 0;
        glfwGetFramebufferSize(window, &fw, &fh);
        glViewport(0, 0, fw, fh);
        float aspect = (fh > 0) ? (float)fw / (float)fh : 1.0f;

        glClearColor(0.08f, 0.09f, 0.11f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float t = (float)glfwGetTime();
        Mat4 model = Multiply(Mat4::RotationY(t * 0.8f), Mat4::RotationX(t * 0.35f));
        Mat4 view  = Mat4::Translation(0.0f, 0.0f, -4.0f);
        Mat4 proj  = Mat4::Perspective(60.0f * 3.14159265f / 180.0f, aspect, 0.1f, 100.0f);
        Mat4 mvp   = Multiply(Multiply(proj, view), model);

        if (!diagnosed) {
            diagnosed = true;
            DiagnoseMvp(mvp);
            std::printf("mvp[0]=%.4f mvp[5]=%.4f mvp[10]=%.4f mvp[11]=%.4f mvp[14]=%.4f\n",
                        mvp.m[0], mvp.m[5], mvp.m[10], mvp.m[11], mvp.m[14]);
        }

        glUseProgram(program);
        glUniformMatrix4fv(uMVP, 1, GL_FALSE, mvp.m);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        if (frameCount == 0) {
            CheckGlError("first draw");
            glFinish();
        }
        ++frameCount;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    std::cout << "共渲染 " << frameCount << " 帧" << std::endl;

    glDeleteProgram(program);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
