# FernLearnOpenGL — OpenGL API 调用清单

配套：`Fern3DGameEngine_Rebuild`（软件光栅化管线 / 第一个里程碑）

**用途**：把第一个里程碑的功能在 GPU 上重新实现一遍。目标不是"抄一份能跑的代码"，而是**清楚每一次调用的作用、参数含义、以及调用顺序为什么不能换**。

**对照关系**：你的软件管线已经用手写代码实现了下面每一项。这份清单的作用是告诉你"GPU 上这件事由哪个 API 负责"。

> **现在要动手画第一个三角形？** 直接跳到 [§13 阶段 3 实操](#13-阶段-3-实操从零到第一个三角形)。

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

## 13. 阶段 3 实操：从零到第一个三角形

> **这一节是"动手做"的路线；前面的 §3/§4/§6 是"查细节"的参考。**
>
> 目标：在已经跑通「窗口 + 视口 + 清屏」的 `Stark.cpp` 上，画出第一个几何体。
>
> **本阶段刻意不引入**：uniform、矩阵、深度测试、背面剔除——那些属于立方体阶段。
> 顶点直接写在 NDC 的 `[-1, 1]` 里，**不需要任何矩阵**就能出画面。

### 13.1 本阶段只需要 18 个 API

| # | API | 一句话作用 | 调几次 | 放哪 |
|---|---|---|---|---|
| 1 | `glGenVertexArrays` | 申请 1 个 VAO 名字 | 1 | 循环前 |
| 2 | `glBindVertexArray` | 绑定 VAO（**之后所有属性设置都记入它**） | ≥1 | 初始化 + 每帧 |
| 3 | `glGenBuffers` | 申请 1 个 VBO 名字 | 1 | 循环前 |
| 4 | `glBindBuffer` | 绑定 VBO | 1 | 循环前 |
| 5 | `glBufferData` | 把顶点数据上传到显存 | 1 | 循环前 |
| 6 | `glVertexAttribPointer` | 描述"顶点属性怎么从缓冲里取" | 每条属性 1 次 | 循环前 |
| 7 | `glEnableVertexAttribArray` | 启用某条属性（**默认是关的**） | 每条属性 1 次 | 循环前 |
| 8 | `glCreateShader` | 创建着色器对象 | 2 | 循环前 |
| 9 | `glShaderSource` | 把源码交给着色器对象 | 2 | 循环前 |
| 10 | `glCompileShader` | 编译 | 2 | 循环前 |
| 11 | `glGetShaderiv` | 查编译是否成功 | 2 | 循环前 |
| 12 | `glGetShaderInfoLog` | 取编译错误日志（**失败时必看**） | 0~2 | 循环前 |
| 13 | `glCreateProgram` | 创建程序对象 | 1 | 循环前 |
| 14 | `glAttachShader` | 把着色器挂到程序上 | 2 | 循环前 |
| 15 | `glLinkProgram` | 链接成可执行程序 | 1 | 循环前 |
| 16 | `glGetProgramiv` / `glGetProgramInfoLog` | 查链接状态 / 日志 | 1 | 循环前 |
| 17 | `glUseProgram` | 启用程序 | 1 | 每帧 |
| 18 | `glDrawArrays` | 真正发起绘制 | 1 | 每帧 |

收尾还要 4 个：`glDeleteShader` ×2、`glDeleteProgram`、`glDeleteBuffers`、`glDeleteVertexArrays`。

**总计 22 个调用，其中真正难理解的只有 3 个**：`glVertexAttribPointer`（参数最多）、`glShaderSource`（多级指针）、VAO 的作用（不可见但必须）。

---

### 13.2 步骤 A：准备顶点数据

三角形只要 3 个顶点。**位置直接写 NDC 坐标**（范围 `[-1, 1]`，原点在屏幕中心）：

```
( 0.0 ,  0.5 )      上
(-0.5 , -0.5 )      左下
( 0.5 , -0.5 )      右下
```

存成一个 `float` 数组，**每 3 个 float 一个顶点**（只要位置，不要颜色）：

```cpp
const float vertices[] = {
     0.0f,  0.5f, 0.0f,
    -0.5f, -0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
};
```

| 项 | 值 |
|---|---|
| 顶点数 | 3 |
| float 个数 | **9** |
| 字节数 | 9 × 4 = **36** |
| z 分量 | 全填 `0.0f`（2D 测试不需要深度） |

> **验收**：不用跑程序，数一下 `sizeof(vertices)` 应该是 36。

---

### 13.3 步骤 B：VAO + VBO

```cpp
GLuint vao = 0, vbo = 0;
glGenVertexArrays(1, &vao);      // ① 申请几个   ② 存名字的地址
glGenBuffers(1, &vbo);

glBindVertexArray(vao);                        // ① 先绑 VAO
glBindBuffer(GL_ARRAY_BUFFER, vbo);            // ② 再绑 VBO
glBufferData(GL_ARRAY_BUFFER,                  // ③ 上传
             sizeof(vertices),                 //    字节数（用 sizeof！）
             vertices,                         //    数据指针
             GL_STATIC_DRAW);                  //    用法：上传一次不改
```

**为什么 `glGen*` 只给一个"名字"**：OpenGL 用不透明的整数 ID 代表对象，`glGen*` 只负责注册一个 ID，真正分配资源发生在后续的 `glBind*` / `glBufferData`。

**VAO 到底记录了什么**（最容易误解的一点）：

> VAO **不存顶点数据**。它记录的是"**哪条属性用哪个缓冲、什么格式、从哪个偏移开始**"这一整套状态。
>
> 所以 `glVertexAttribPointer` 的**副作用**是写进"当前绑定的 VAO"。这就是为什么必须先 `glBindVertexArray`。

| 坑 | 后果 |
|---|---|
| `glBufferData` 的 size 填成顶点个数（3）而不是字节数（36） | 只传了 3 字节 → 画不出东西或乱画 |
| 忘记 `glBindVertexArray` 就调 `glVertexAttribPointer` | 属性设置丢失（没有 VAO 接收） |
| `glGenVertexArrays` 之后忘记 `glBindVertexArray` 就直接画 | **Core Profile 下什么都不画，且不报错** |

> **验收**：`vao`、`vbo` 是非零整数；`glGetError()` 返回 `GL_NO_ERROR`。

---

### 13.4 步骤 C：描述顶点属性（参数最多的一步）

```cpp
glVertexAttribPointer(0,                   // ① 属性位置，必须和着色器 layout(location=N) 对上
                      3,                   // ② 这个属性占几个分量（位置 = x,y,z = 3）
                      GL_FLOAT,            // ③ 分量类型
                      GL_FALSE,            // ④ 是否归一化（float 数据填 GL_FALSE）
                      3 * sizeof(float),   // ⑤ stride：相邻两个顶点隔多少字节
                      (void*)0);           // ⑥ offset：这个属性在顶点内的字节偏移

glEnableVertexAttribArray(0);              // 别忘了！默认是关闭的
```

| 参数 | 三角形这里填什么 | 立方体阶段会变成 |
|---|---|---|
| 属性位置 | `0` | 位置 `0`、颜色 `1` |
| 分量数 | `3` | `3` / `3` |
| stride | `3 * sizeof(float)` = 12 | `6 * sizeof(float)` = 24 |
| offset | `0` | 位置 `0`、颜色 `(void*)(3*sizeof(float))` = 12 |

第 ⑥ 参数类型是 `const void*`，**但实际传的是字节偏移**，所以要强转。这是 OpenGL 早期设计留下的怪癖。

> **验收**：这一步没有返回值，看不出对错。**只能靠"最后画出东西"来验证**——所以它最容易错、也最难定位。

---

### 13.5 步骤 D：编译两个着色器

顺序固定：**创建 → 喂源码 → 编译 → 查状态**。

```cpp
GLuint vs = glCreateShader(GL_VERTEX_SHADER);   // 片段用 GL_FRAGMENT_SHADER
glShaderSource(vs, 1, &src, nullptr);           // ①着色器 ②源码串数 ③源码数组 ④长度数组(nullptr=以\0结尾)
glCompileShader(vs);

GLint ok = GL_FALSE;
glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);      // 查是否编译成功
if (!ok) {
    char log[1024];
    glGetShaderInfoLog(vs, sizeof(log), nullptr, log);   // 取出错误日志
    // 打印 log —— 里面有出错的行号，是唯一的线索
}
```

**注意 `glShaderSource` 的第三参**：它是 `const GLchar**`（字符串指针的地址），**所以传 `&src` 而不是 `src`**——这是多级指针的常见坑。

| 坑 | 后果 |
|---|---|
| `glShaderSource(vs, 1, src, nullptr)` 少了个 `&` | 编译期类型不匹配 / 运行期崩 |
| 不查 `glGetShaderiv` | 编译失败也继续走，最后画不出东西但**不知道为什么** |
| 不打印 `glGetShaderInfoLog` | 丢掉了唯一能定位行号的信息 |

> **验收**：两个 `ok` 都是 `GL_TRUE`；失败时日志里有 `ERROR: 0:行号: ...`。

---

### 13.6 步骤 E：链接成程序

```cpp
GLuint prog = glCreateProgram();
glAttachShader(prog, vs);
glAttachShader(prog, fs);
glLinkProgram(prog);

GLint ok = GL_FALSE;
glGetProgramiv(prog, GL_LINK_STATUS, &ok);
if (!ok) {
    char log[1024];
    glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
}

glDeleteShader(vs);       // 链接完就可以删，程序已持有编译结果
glDeleteShader(fs);
```

> **验收**：`GL_LINK_STATUS` 为 `GL_TRUE`。链接失败最常见的原因是**顶点着色器没有写 `gl_Position`**。

---

### 13.7 步骤 F：着色器源码（最小可用版）

> GLSL 语法猜不出来，所以给一份**带逐行解释**的最小版本。**建议照着敲一遍而不是复制**——敲的过程会记住结构。

**顶点着色器**（算每个顶点的位置）：

```glsl
#version 460 core

layout (location = 0) in vec3 aPos;   // 从 VBO 的 location 0 读入顶点位置

void main() {
    gl_Position = vec4(aPos, 1.0);    // 必须写！直接当作裁剪空间坐标
}
```

| 行 | 作用 | 说明 |
|---|---|---|
| `#version 460 core` | 声明 GLSL 版本 | **必须是文件第一行**（前面不能有空行或注释） |
| `layout (location = 0) in vec3 aPos` | 声明输入属性 | `location = 0` 必须和 `glVertexAttribPointer(0, ...)` 的第一个参数一致 |
| `gl_Position = ...` | 输出裁剪空间坐标 | **不写它是未定义行为**（常见表现：什么都不显示） |
| `vec4(aPos, 1.0)` | 补成齐次坐标 | 第 4 分量 `w = 1.0` 表示"这是一个点"；透视除法由硬件自动做 |

**片元着色器**（给每个像素上色）：

```glsl
#version 460 core

out vec4 FragColor;      // 输出到颜色缓冲

void main() {
    FragColor = vec4(1.0, 0.5, 0.2, 1.0);   // 橙色，RGBA 都是 0~1
}
```

| 要点 | 说明 |
|---|---|
| 输出变量 | Core Profile 里**必须自己声明 `out`**（老版本的 `gl_FragColor` 已被移除） |
| 颜色范围 | 同样是 `0.0 ~ 1.0`（和 `glClearColor` 同一个约定） |
| 片元着色器不需要输入 | 三角形阶段颜色写死就够；立方体阶段再通过 `in` 接顶点颜色 |

---

### 13.8 步骤 G：帧循环内的三个调用

在你现有的 `Stark.cpp` 循环里，**`glClear` 之后、`glfwSwapBuffers` 之前**插入：

```cpp
glUseProgram(prog);          // ① 启用这个程序
glBindVertexArray(vao);      // ② 绑定 VAO（每帧都要，别假设它还绑着）
glDrawArrays(GL_TRIANGLES,   // ③ 图元类型：三角形
             0,              //    起始顶点索引
             3);             //    顶点个数（不是三角形个数！）
```

**为什么每帧都要 `glBindVertexArray`**：OpenGL 是状态机，任何中间调用（尤其是后面加 FBO、加多个物体时）都可能改变当前 VAO。**养成"画之前先绑"的习惯**，比记住"我上一次绑的是哪个"可靠。

| 坑 | 后果 |
|---|---|
| `glDrawArrays` 第 3 参填 1（以为"画 1 个三角形"） | 只送 1 个顶点 → 画不出三角形 |
| 忘记 `glBindVertexArray` | **黑屏且不报错**（Core Profile 的经典静默失败） |
| 忘记 `glUseProgram` | 用默认程序（无着色器）→ 画不出东西 |

---

### 13.9 三个"几乎一定会先撞上"的错误

| 症状 | 最可能的原因 | 怎么排查 |
|---|---|---|
| **窗口是清屏色，什么都没有，控制台无报错** | ① 没绑 VAO ② 着色器编译失败但没检查 ③ `gl_Position` 没赋值 | 依次打印 `ok` 状态；在 `glDrawArrays` 后加 `glGetError()` |
| **三角形只露出一个角 / 缺一块** | 顶点绕序和 `glFrontFace` 不一致（如果已开 `GL_CULL_FACE`） | 阶段 3 **先不要开背面剔除**，画出来再加 |
| **三角形位置/大小不对** | 没调 `glViewport`，或顶点值超出 `[-1, 1]` | 检查 viewport；把顶点改成 `±0.5` 看是否出现 |

**排查纪律**（和主仓库的 bug 记录风格一致）：

> 每加一个功能点就运行一次，**一次只加一个变量**。这样出错时你知道是"刚加的那一步"错了。
> 一次加三步然后黑屏，就只能靠猜。

---

### 13.10 验收标准

画出三角形并满足以下全部，才算阶段 3 完成：

1. **画面**：屏幕中央一个橙色三角形，尖朝上
2. **控制台**：两个着色器编译成功、程序链接成功、`glGetError()` 全为 `GL_NO_ERROR`
3. **能讲清**（比画出三角形更重要）：
   - VAO 记录的是什么？为什么 `glVertexAttribPointer` 必须先绑 VAO？
   - `glDrawArrays` 第 3 个参数是顶点数还是三角形数？
   - 顶点着色器里 `gl_Position` 的第 4 个分量 `w` 是什么含义？（→ 齐次坐标，透视除法在硬件里做）
   - 为什么片元着色器要自己声明 `out` 变量？
4. **能对比**：这个三角形在我软件管线里对应哪几行？**我少写了什么？**
   → 少写的是：光栅化、属性插值、深度测试、视口变换——这四件事硬件做了。

---

### 13.11 阶段 3 之后立刻要做的（预告，先不用看细节）

| 阶段 | 内容 | 新增 API |
|---|---|---|
| 3.5 | 顶点数组加颜色（每顶点 6 个 float） | 无（只改 stride / offset） |
| 4 | 立方体 + 自己的 `Matrix4x4` 算 MVP | `glGetUniformLocation`、`glUniformMatrix4fv` |
| 5 | 深度测试 | `glEnable(GL_DEPTH_TEST)` |
| 6 | 背面剔除 | `glEnable(GL_CULL_FACE)` + `glFrontFace` |
| 7 | 时间驱动旋转 | 无（用 `glfwGetTime()`） |

---

## 附：本文件对应的第三方依赖版本

| 组件 | 版本 | 说明 |
|---|---|---|
| GLFW | 3.5.1 | 预编译包 `lib-vc2026\glfw3.lib` |
| GLAD | glad2 生成 | GL 4.6 Core，已勾选 loader |
| 工具集 | VS2026 (v145) | C++20，x64 |
| 显卡 | Intel UHD 630 / Quadro P1000 | 均支持 GL 4.6 |
