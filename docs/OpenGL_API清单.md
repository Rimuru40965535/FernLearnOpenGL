# FernLearnOpenGL — OpenGL API 调用清单

配套：`Fern3DGameEngine_Rebuild`（软件光栅化管线 / 第一个里程碑）

**用途**：把第一个里程碑的功能在 GPU 上重新实现一遍。目标不是"抄一份能跑的代码"，而是**清楚每一次调用的作用、参数含义、以及调用顺序为什么不能换**。

**对照关系**：你的软件管线已经用手写代码实现了下面每一项。这份清单的作用是告诉你"GPU 上这件事由哪个 API 负责"。

---

## 0. 全局速查表：里程碑功能 → API

| 第一个里程碑的功能 | 软件管线里你怎么做的 | GPU 上由谁负责 |
|---|---|---|
| 创建窗口 / 逐像素绘制 | EasyX `initgraph` + `putpixel` | GLFW 建窗口 + `glClear` + GPU 光栅化 |
| 清屏 | 手写双层循环填色 | `glClearColor` + `glClear` |
| 点 / 方向的定义 | `Vector3D` | 顶点着色器 `in vec3 aPos` |
| 矩阵变换（模型/观察/投影） | `Matrix4x4` + 自己写的乘法 | 你算好 MVP，`glUniformMatrix4fv` 上传 |
| 齐次坐标与透视除法 | 你手写 `w` 除法 | **硬件固定管线自动做**（你只管给 `w`） |
| 网格 Mesh（顶点+棱） | `Mesh` 类 | VBO 存顶点 + VAO 记属性格式 |
| 最小可用渲染管线 | 你的 `Pipe03.cpp` 主循环 | GLFW 事件循环 + `glDrawArrays` |
| 深度缓冲 | `DepthBuff` 手写二维数组 | **`glEnable(GL_DEPTH_TEST)` + 深度缓冲由驱动管** |
| 深度测试 | 你手写比较 | 硬件按 `glDepthFunc` 比较 |
| 逐像素写入 | `putpixel` | 片元着色器 `out vec4 FragColor` |
| 背面剔除（待做） | 你计划用屏幕空间有向面积* | `glEnable(GL_CULL_FACE)` + 绕序 |

> **一句话**：GPU 把"光栅化 + 插值 + 深度测试"这三件事变成了固定功能，所以你要写的代码**不到软件管线的十分之一**。剩下的都是"把数据喂给 GPU"。

> 更正：在主仓库中，我计划利用每个平面的法向量属性实现背面剔除，而无需判断顶点绕序。
---

## 1. 窗口与上下文（GLFW）

### 1.1 初始化

```cpp
if (!glfwInit()) { /* 失败处理 */ }
```
- 返回 `int`，非 0 表示成功
- **必须在所有 GLFW 调用之前**

### 1.2 指定上下文版本（关键！）

```cpp
glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
```
- **不设这三行会拿到 OpenGL 1.1 兼容上下文**，现代着色器全部不可用
- `GLFW_OPENGL_CORE_PROFILE` = Core Profile：没有废弃的固定管线 API（`glBegin`/`glVertex` 全部不可用）
- 可选：`glfwWindowHint(GLFW_SAMPLES, 4);` 开 4x MSAA

### 1.3 创建窗口

```cpp
GLFWwindow* window = glfwCreateWindow(1280, 720, "标题", nullptr, nullptr);
```
- 参数：宽、高、标题、监视器（`nullptr`=窗口模式）、share（`nullptr`）
- 返回 `GLFWwindow*`，失败返回 `nullptr`

### 1.4 把上下文设为当前

```cpp
glfwMakeContextCurrent(window);
glfwSwapInterval(1);     // 1 = 开启垂直同步
```
- **必须在加载 GL 函数之前调用**，否则 `gladLoadGLLoader` 拿不到函数
- `glfwSwapInterval(1)` 让 `glfwSwapBuffers` 阻塞到垂直回扫，避免撕裂 + 跑满 CPU

### 1.5 事件与关闭

```cpp
glfwWindowShouldClose(window)      // 返回 0 表示继续循环
glfwPollEvents()                   // 处理键盘鼠标事件（必须每帧调用）
glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS
glfwSetWindowShouldClose(window, GLFW_TRUE)
glfwGetTime()                      // 秒，double；用它做时间驱动动画
```

### 1.6 清理

```cpp
glfwDestroyWindow(window);
glfwTerminate();
```

---

## 2. 加载 OpenGL 函数（GLAD）

```cpp
if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { /* 失败 */ }
```

**为什么必须有这一步**（面试常问）：

> Windows 的 `opengl32.dll` **只导出 OpenGL 1.1** 的函数。1.1 之后的所有函数（`glGenBuffers`、`glCreateShader` …）都必须在**运行时**通过 `wglGetProcAddress` 从显卡驱动里动态取。GLAD 就是替你做这件事。

**头文件顺序（硬性要求）**：

```cpp
#include <glad/glad.h>      // 必须在 GLFW 之前
#include <GLFW/glfw3.h>
```
顺序反了，GLFW 会去包含系统 `GL/gl.h`，和 GLAD 的函数指针声明冲突。

---

## 3. 缓冲区对象（VBO / VAO）

### 3.1 生成并绑定

```cpp
GLuint vao = 0, vbo = 0;
glGenVertexArrays(1, &vao);      // 参数：数量、存放 ID 的数组
glGenBuffers(1, &vbo);

glBindVertexArray(vao);                    // 先绑 VAO
glBindBuffer(GL_ARRAY_BUFFER, vbo);        // 再绑 VBO
```

**VAO 是干什么的**（Core Profile 里**必须**有）：记录"顶点属性怎么从 buffer 里取"的全部状态（每个 attribute 的 location、分量数、类型、stride、offset、启用状态）。
- 画的时候只需 `glBindVertexArray(vao)`，不用重复设置属性
- **不绑 VAO 直接 `glDrawArrays` 在 Core Profile 下不画任何东西，而且不报错** ← 这是最常见的"黑屏且无错误"

### 3.2 上传顶点数据

```cpp
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
```
| 参数 | 含义 |
|---|---|
| target | `GL_ARRAY_BUFFER` |
| size | 字节数（**用 `sizeof`，别填顶点个数**） |
| data | 数据指针 |
| usage | `GL_STATIC_DRAW` 不改 / `GL_DYNAMIC_DRAW` 常改 |

### 3.3 描述顶点属性（最容易写错的一步）

```cpp
// 每顶点 6 个 float：3 位置 + 3 颜色
const GLsizei stride = 6 * sizeof(float);

// location 0：位置
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
glEnableVertexAttribArray(0);

// location 1：颜色（偏移 3 个 float = 12 字节）
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
glEnableVertexAttribArray(1);
```
| 参数 | 含义 |
|---|---|
| 第 1 个 | **attribute location**，必须和着色器里 `layout(location = N)` 对上 |
| 第 2 个 | 该属性几个分量（位置 3、颜色 3） |
| 第 3 个 | 分量类型 `GL_FLOAT` |
| 第 4 个 | 是否归一化（float 数据填 `GL_FALSE`） |
| 第 5 个 | **stride**：相邻两个顶点隔多少字节 |
| 第 6 个 | **offset**：该属性在一个顶点内的字节偏移（用 `(void*)` 强转） |

**三个坑**：
1. `glVertexAttribPointer` 记录的是**当前绑定的 VBO**，所以必须先 `glBindBuffer`
2. 别忘了 `glEnableVertexAttribArray`，**默认是关闭的**
3. 第 6 参数类型是 `const void*`，要传字节偏移就要强转 `(void*)(n * sizeof(float))`

---

## 4. 着色器（Shader）

### 4.1 编译

```cpp
GLuint vs = glCreateShader(GL_VERTEX_SHADER);      // 或 GL_FRAGMENT_SHADER
glShaderSource(vs, 1, &src, nullptr);              // 参数：着色器、字符串数、字符串数组、长度数组
glCompileShader(vs);
```

### 4.2 检查编译结果（**必做**）

```cpp
GLint ok = GL_FALSE;
glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);
if (!ok) {
    char log[1024];
    glGetShaderInfoLog(vs, sizeof(log), nullptr, log);
    // 打印 log：里面会有出错的行号，非常有用
}
```

### 4.3 链接成程序

```cpp
GLuint prog = glCreateProgram();
glAttachShader(prog, vs);
glAttachShader(prog, fs);
glLinkProgram(prog);

GLint ok = GL_FALSE;
glGetProgramiv(prog, GL_LINK_STATUS, &ok);
if (!ok) { char log[1024]; glGetProgramInfoLog(prog, sizeof(log), nullptr, log); }

glDeleteShader(vs);      // 链接完就可以删掉着色器对象，程序已持有编译结果
glDeleteShader(fs);
```

### 4.4 使用与销毁

```cpp
glUseProgram(prog);
glDeleteProgram(prog);
```

### 4.5 着色器源码要点

```glsl
#version 460 core
layout (location = 0) in vec3 aPos;      // in = 顶点属性，location 与 glVertexAttribPointer 对应
layout (location = 1) in vec3 aColor;
uniform mat4 uMVP;                        // uniform = 每次绘制前由 CPU 设置，所有顶点共享
out vec3 vColor;                          // out → 下一阶段的 in

void main() {
    gl_Position = uMVP * vec4(aPos, 1.0); // 必须写 gl_Position，别自己算 w 除法
    vColor = aColor;
}
```

```glsl
#version 460 core
in vec3 vColor;             // 从顶点着色器插值而来（已按重心坐标插好）
out vec4 FragColor;         // 输出到帧缓冲

void main() {
    FragColor = vec4(vColor, 1.0);
}
```
- **透视除法与视口变换由硬件做**，你只需给出裁剪空间坐标（含 `w`）
- **光栅化时的属性插值由硬件做**，你只需在顶点着色器把颜色 `out` 出去

---

## 5. Uniform 上传

```cpp
GLint loc = glGetUniformLocation(prog, "uMVP");   // 返回 -1 表示没找到
glUniformMatrix4fv(loc, 1, GL_FALSE, matrixPtr);
```
| 参数 | 含义 |
|---|---|
| 1 | uniform 位置 |
| 2 | 上传几个矩阵（通常 1） |
| 3 | 是否转置。**`GL_FALSE`**，因为 GLSL 约定矩阵是列主序 |
| 4 | 指向 16 个 float 的指针，**列主序** |

**两个坑**：
1. 若 uniform 在着色器里声明了但**从未使用**，编译器会优化掉它，`glGetUniformLocation` 返回 **-1**，此时 `glUniform*` 调用是**静默空操作，不报错**
2. 第 3 参数必须在 CPU 侧把矩阵排成列主序；若你的库是行主序（如 `Fern::Matrix4x4` 的 `values[row][col]`），就要在这里做转置

### 5.1 行向量 ↔ 列向量的转换（你的项目会撞上）

`Fern::Matrix4x4` 用 `values[row][col]`（行主序）且语义是 `v' = v · M`（行向量）；
GLSL 是 `v' = M · v`（列向量）。**两者互为转置**，所以上传时桥接一次：

```cpp
float glMat[16];
for (int row = 0; row < 4; ++row)
    for (int col = 0; col < 4; ++col)
        glMat[col * 4 + row] = (float)m.values[row][col];   // 转置
glUniformMatrix4fv(loc, 1, GL_FALSE, glMat);
```

**症状记忆**：只改一半（比如只翻 `w` 行不翻 `z` 行），结果是**整个物体消失、只剩清屏色、没有任何 GL 错误**。

---

## 6. 绘制

```cpp
glDrawArrays(GL_TRIANGLES, 0, 36);
```
| 参数 | 含义 |
|---|---|
| 1 | 图元类型：`GL_TRIANGLES` / `GL_LINES` / `GL_POINTS` |
| 2 | 起始顶点索引 |
| 3 | **顶点个数**（不是三角形个数、不是字节数） |

配套：
- `glBindVertexArray(vao)` — **画之前必须绑定**
- `glUseProgram(prog)` — 必须绑定程序
- 有索引的话用 `glDrawElements(mode, count, GL_UNSIGNED_INT, 0)`，配合 EBO

---

## 7. 帧循环与清屏

```cpp
while (!glfwWindowShouldClose(window)) {
    glfwGetFramebufferSize(window, &fbw, &fbh);
    glViewport(0, 0, fbw, fbh);                              // 每帧设，窗口缩放后必须更新
    glClearColor(0.08f, 0.09f, 0.11f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);      // 深度也要清！
    // ... 设置矩阵、uniform、draw ...
    glfwSwapBuffers(window);                                 // 交换前后缓冲（双缓冲）
    glfwPollEvents();
}
```

**三个坑**：
1. **`glClear` 必须带 `GL_DEPTH_BUFFER_BIT`**。忘了清深度，第二帧之后的深度测试全错
2. 忘了 `glViewport` → 画到错误区域；窗口缩放后不更新 → 画面被拉伸
3. 只在改窗口大小时设 viewport 是常见优化，但**初学阶段每帧设最安全**

---

## 8. 深度测试与面剔除

```cpp
glEnable(GL_DEPTH_TEST);          // 开启深度测试（默认是关闭的！）
glDepthFunc(GL_LESS);             // 默认就是 GL_LESS：新深度更小（更近）才通过
glClearDepth(1.0);                // 清深度用的值，默认 1.0

glEnable(GL_CULL_FACE);           // 开启面剔除
glCullFace(GL_BACK);              // 剔除背面（默认）
glFrontFace(GL_CCW);              // 逆时针绕序 = 正面（默认）
```

**对照你的软件管线**：
| 你的做法 | GPU 做法 |
|---|---|
| `DepthBuff` 手写 `double** buffer` + 每帧重置为 1.5 | 驱动内部维护深度缓冲，`glClear` 清零 |
| 手写"新深度 < 旧深度则写入" | `glDepthFunc(GL_LESS)` 的硬件比较 |
| 手写背面剔除（计划中，用屏幕空间有向面积） | `glFrontFace` 定义绕序，硬件按绕序剔除 |

> 注意：深度值范围。你的 `DepthBuff` 约定 NDC 的 z ∈ [0,1]（DirectX 风格）。OpenGL 默认也是 [0,1]，一致。但**若沿用你的 `GeneratePerspectiveMatrix`（行向量版），上传前必须做第 5.1 节的转置**，否则 `w` 符号反了，几何会被整体裁掉。

---

## 9. 资源释放

```cpp
glDeleteProgram(prog);
glDeleteBuffers(1, &vbo);
glDeleteVertexArrays(1, &vao);
glfwDestroyWindow(window);
glfwTerminate();
```
对应 `glGen*` / `glCreate*`。**每个 `glGen` 都要有对应的 `glDelete`**（和你软件管线里 `new`/`delete` 的配对意识一样）。

---

## 10. 调用顺序总览（顺序不能乱）

```
glfwInit
  → glfwWindowHint ×3          指定 4.6 Core
  → glfwCreateWindow
  → glfwMakeContextCurrent     ← 必须在下一步之前
  → gladLoadGLLoader           ← 之后才能调 gl* 函数

【一次性初始化】
  → glGenVertexArrays / glBindVertexArray
  → glGenBuffers / glBindBuffer / glBufferData
  → glVertexAttribPointer ×N / glEnableVertexAttribArray ×N
  → glCreateShader / glShaderSource / glCompileShader ×2
  → glCreateProgram / glAttachShader ×2 / glLinkProgram
  → glGetUniformLocation
  → glEnable(GL_DEPTH_TEST) / glEnable(GL_CULL_FACE)

【每帧】
  → glfwPollEvents
  → glfwGetFramebufferSize / glViewport
  → glClearColor / glClear(COLOR | DEPTH)
  → glUseProgram
  → glUniformMatrix4fv
  → glBindVertexArray
  → glDrawArrays
  → glfwSwapBuffers

【退出】
  → glDelete* / glfwDestroyWindow / glfwTerminate
```

---

## 11. 六个必踩的坑（提前知道能省几小时）

| # | 症状 | 原因 |
|---|---|---|
| 1 | 黑屏，无任何错误 | 没绑 VAO；或 `glEnable(GL_DEPTH_TEST)` 忘了开；或没设 `GLFW_CONTEXT_VERSION` 拿到 1.1 上下文 |
| 2 | 整个物体消失，只剩清屏色，无错误 | 矩阵约定搞混（`w` 为负，几何全被裁）。见 5.1 |
| 3 | 第一帧正常，之后错乱 | `glClear` 漏了 `GL_DEPTH_BUFFER_BIT` |
| 4 | 着色器编译失败但看不出哪错 | 没读 `glGetShaderInfoLog`；它里面有行号 |
| 5 | `glGetUniformLocation` 返回 −1 | uniform 声明了但没在着色器里用，被优化掉 |
| 6 | 面看不见 / 该露的没露 | 绕序与 `glFrontFace` 不一致；或搞反了 `glCullFace` |

---

## 12. 用这套 API 复刻里程碑的验收标准

完成后你应该能说：

1. **窗口**：GLFW 建成 1280×720 的 4.6 Core 上下文
2. **矩阵**：用我自己的 `Matrix4x4` 算 MVP，通过一次转置上传给 GLSL
3. **网格**：我的立方体 8 顶点 / 12 棱数据能正常上传并渲染
4. **深度**：遮挡关系正确（等价于我软件管线的 `DepthBuff`）
5. **背面剔除**：等价于我计划中的屏幕空间有向面积剔除
6. **动画**：时间驱动的旋转，与软件版立方体视觉一致
7. **能讲清**：为什么 1.1 之后的 GL 函数要运行时加载；为什么 Core Profile 必须有 VAO；行向量/列向量转置发生在哪一步

**第 7 条才是面试真正考的**，前 6 条只是载体。

---

## 附：本文件对应的第三方依赖版本

| 组件 | 版本 | 说明 |
|---|---|---|
| GLFW | 3.5.1 | 预编译包 `lib-vc2026\glfw3.lib` |
| GLAD | glad2 生成 | GL 4.6 Core，已勾选 loader |
| 工具集 | VS2026 (v145) | C++20，x64 |
| 显卡 | Intel UHD 630 / Quadro P1000 | 均支持 GL 4.6 |
