# OpenGL 学习问答

> 这份文档记录我在 `FernLearnOpenGL` 中的**真实提问与求索过程**。
>
> 它不是抄来的 API 手册（那类内容见 [`OpenGL_API清单.md`](./OpenGL_API清单.md)），
> 而是**每一条都对应我在写代码时真正卡住的地方**——写注释时留下的疑问、实验做出来的反直觉结果、以及踩坑后的复盘。
>
> 组织方式是**按大目标分板块**，与开发日志里的「目标 N-N」一一对应。
> 凡是引用了官方文档结论的地方，都注明了依据（本地 `include/GLFW/glfw3.h` 的 Doxygen 注释，GLFW 3.5.1）。
>
> 我询问大肥鱼的每一个问题，它都做到了先从官方源码及注释里面翻找答案再回答。
> 在这个项目里，大肥鱼变成了我的老师。

---

## 目录

- [大目标一：让一个窗口出现](#大目标一让一个窗口出现)
  - [Q1 `glfwInit()` 为什么必须先调用？](#q1-glfwinit-为什么必须先调用)
  - [Q2 什么是"上下文"？为什么没有它就现代着色器不可用？](#q2-什么是上下文为什么没有它就现代着色器不可用)
  - [Q3 为什么 `glad.h` 必须在 `glfw3.h` 之前？为什么 GL 函数要运行时加载？](#q3-为什么-gladh-必须在-glfw3h-之前为什么-gl-函数要运行时加载)
  - [Q4 MSAA 是什么？原理是什么？](#q4-msaa-是什么原理是什么)
- [大目标二：让窗口留住并可交互](#大目标二让窗口留住并可交互)
  - [Q5 `glfwPollEvents()` 为什么叫 poll？每帧都要调吗？](#q5-glfwpollevents-为什么叫-poll每帧都要调吗)
  - [Q6 `glfwSwapInterval(1)` 的"垂直同步"是什么？](#q6-glfwswapinterval1-的垂直同步是什么)
  - [Q7 `glfwGetKey` 为什么能传 `'A'`？为什么只能大写？](#q7-glfwgetkey-为什么能传-a为什么只能大写)
  - [Q8 `glfwMakeContextCurrent` 为什么要设"当前"？](#q8-glfwmakecontextcurrent-为什么要设当前)
  - [Q9 `glfwGetFramebufferSize` 为什么用两个指针参数输出？](#q9-glfwgetframebuffersize-为什么用两个指针参数输出)
  - [Q10 `glfwGetTime()` 返回 0 是什么意思？](#q10-glfwgettime-返回-0-是什么意思)
  - [Q11 `glfwTerminate()` 是不是还要掌管资源释放？](#q11-glfwterminate-是不是还要掌管资源释放)
- [大目标三：让画面内容由我决定](#大目标三让画面内容由我决定)
  - [Q12 `glViewport` 是干嘛的？四个参数是什么意思？](#q12-glviewport-是干嘛的四个参数是什么意思)
  - [Q13 `glClearColor` 是什么？参数和返回值？](#q13-glclearcolor-是什么参数和返回值)
  - [Q14 `glClear` 的参数和返回值？](#q14-glclear-的参数和返回值)
  - [Q15 为什么深度缓冲必须每帧一起清？](#q15-为什么深度缓冲必须每帧一起清)
  - [Q16 `glClear` 的位掩码设计能不能借鉴到我自己的引擎？](#q16-glclear-的位掩码设计能不能借鉴到我自己的引擎)
- [附录一：踩坑记录（现象 → 排查 → 根因 → 修法）](#附录一踩坑记录现象--排查--根因--修法)
- [附录二：我自己答对的问题](#附录二我自己答对的问题)

---

# 大目标一：让一个窗口出现

对应提交：`目标1-1 黑框框：一闪而过的黑框框`

这个阶段要解决的是：**GLFW 与 OpenGL 这两套东西怎么被"启动"起来。**

---

## Q1 `glfwInit()` 为什么必须先调用？

**我的原始提问**（源自 `Stark.cpp` 早期注释）：

> 初始化（为什么呢？有什么说法吗？）

### 短答

`glfwInit()` 负责把 GLFW 库从"什么都没准备"变成"可以创建窗口"。不调用它，后面所有 GLFW 函数都会返回 `GLFW_NOT_INITIALIZED`。

### 详解

GLFW 是 **C 库**，没有构造函数、没有 RAII、没有全局初始化器。  
~~网上资料显示，C 是面向过程语言，第一个 C 解释编译器是用汇编写的，而 C++ 在 C 的基础上扩展了面向对象与泛型编程机制，并增加了更多关键字、语法特性（如类、继承、多态、函数重载、默认参数）等，是面向对象语言。~~  
**所以它采用 C 库最经典的 init / terminate 成对模式**：

```
glfwInit()  →  ... 使用 ...  →  glfwTerminate()
```

`glfwInit()` 内部要做的事情，远比"设个标志位"重得多：

| 它做的事 | 说明 |
|---|---|
| 选择并加载**平台后端** | Windows 上是 `win32_init.c`；Linux 是 X11/Wayland；macOS 是 Cocoa |
| 注册窗口类 | Win32 上要 `RegisterClassExW`，否则窗口创建不出来 |
| 初始化**高精度计时器** | 这就是后来 `glfwGetTime()` 的数据来源 |
| 建立**键码映射表** | 把物理扫描码映射成 `GLFW_KEY_*`（Q7 会展开） |
| 初始化线程/互斥等基础设施 | 供 `glfwWaitEvents` 等跨线程机制使用 |

### 官方依据

本地 `include/GLFW/glfw3.h` 中 `glfwInit` 的文档注释明确写了两条约束：

```c
*  @thread_safety This function must only be called from the main thread.
```

> **只能在主线程调用。** 因为 Win32 的窗口类注册和消息队列是线程绑定的。

### 与我项目的关联

我的代码：

```cpp
if (!glfwInit()) { return 1; }
```

这里 `return 1` 而不只是打个日志，是刻意的：**后续任何 GLFW 调用都没有意义，继续跑只会得到一连串误导性的错误。** 早期失败要尽早退出。

---

## Q2 什么是"上下文"？为什么没有它就现代着色器不可用？

**我的原始提问**：

> 指定上下文版本（上下文是什么，我只在 AI 相关的地方听说过？为什么拿不到这个上下文会导致现代着色器不可用？）

### 短答

**上下文（Context）是 GPU 驱动维护的一份"会话状态"**：它包含所有 OpenGL 状态变量、所有 GL 对象（buffer / texture / shader）、以及**这个会话支持哪一版 OpenGL 的能力集**。

没有上下文 = 没有会话 = 任何 `gl*` 函数都无处执行。

### 详解一：上下文里装了什么

| 类别 | 具体内容 |
|---|---|
| **状态变量** | 当前绑定的 VAO / VBO / program、深度测试开关、清屏颜色、视口矩形…… |
| **对象表** | 你 `glGenBuffers` / `glGenTextures` / `glCreateProgram` 创建的每一份资源 |
| **能力集** | 这版 GL 有哪些函数、GLSL 编译器支持到哪个版本 |

**关键认识**：这些状态**不在你的代码里**，在驱动里。你的 `glEnable(GL_DEPTH_TEST)` 是往那份状态里写一个位。

### 详解二：为什么"拿不到上下文，现代着色器就不可用"

因为**上下文决定 GLSL 编译器版本**。

```
请求 4.6 Core  →  GLSL 编译器接受 #version 460 core
不请求        →  GLFW 给 1.0/1.1 兼容上下文  →  GLSL 只到 1.10，且固定管线为主
```

如果在 1.1 上下文里写 `#version 460 core`：

- 结果不是"旧版报个语法错"，而是**整个着色器机制都不存在**——`glCreateShader` 根本不是你拿得到的入口点（见 Q3）
- 现代 API（VAO、`glGenBuffers`、`glUniformMatrix4fv`）在 1.1 上下文中要么不存在，要么被模拟

### 详解三：为什么"只在 AI 相关的地方听说过"

**这是两个完全不同的概念，只是中文都叫"上下文"：**

| | 大语言模型的 "context" | OpenGL 的 "context" |
|---|---|---|
| 含义 | 上下文窗口，模型一次能看的 token 范围 | GPU 会话状态集合 |
| 谁维护 | 推理框架 | 图形驱动 |
| 是否相关 | **完全无关** |

这个歧义值得记一笔——**技术讨论里同名不同义是常见的沟通陷阱**。

### 详解四：Windows 上版本是怎么"要"出来的

1. `wglCreateContext` 创建的**永远是 1.1 上下文**（因为 `opengl32.dll` 只到 1.1）
2. 想要更高版本，必须先用 1.1 上下文拿到 `wglCreateContextAttribsARB`（它属于 `WGL_ARB_create_context` 扩展），再用它创建高版本上下文
3. **GLFW 内部替你做了这一整套操作**

所以你在 `Stark.cpp` 里写的三行 hint：

```cpp
glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
```

**必须在 `glfwCreateWindow` 之前**——因为 hint 是"下单前的备注"，窗口一创建，上下文就按当时的备注造好了，之后改 hint 无效。

### 详解五：Core Profile 意味着什么

`GLFW_OPENGL_CORE_PROFILE` 不是"4.6 的附加选项"，而是一次**契约声明**：

| | Compatibility Profile | Core Profile |
|---|---|---|
| 固定管线（`glBegin`/`glVertex`/`glMatrixMode`） | 保留 | **全部移除** |
| VAO | 可选 | **必需**（不绑 VAO 画不出东西） |
| 即时模式绘图 | 可用 | 不可用 |
| 目的 | 向后兼容 | 逼迫使用现代管线 |

这就是为什么后面必须写 VAO，而不是"教程这么写所以我也写"。

---

## Q3 为什么 `glad.h` 必须在 `glfw3.h` 之前？为什么 GL 函数要运行时加载？

**我的原始提问**（两处，合在一起）：

> `#include <glad/glad.h>      // 必须在 GLFW 之前` —— 为什么必须在前面？
>
> `// OpenGL的函数是在运行时从显卡里动态取的。` —— 为什么可以/必须这么取？

### 短答

**Windows 的 `opengl32.dll` 只导出 OpenGL 1.1 的函数。** 1.1 之后的所有函数（`glGenBuffers`、`glCreateShader`、`glUniformMatrix4fv`……）都必须在**运行时**用 `wglGetProcAddress` 从驱动里取函数指针。GLAD 就是替你做这件事的代码生成器。

而头文件顺序，是因为**两个头都对"OpenGL 类型"有主张，必须让 GLAD 先说话**。

### 详解一：为什么 1.1 之后就没有链接期符号

正常 C++ 库的函数是**链接期符号**——你在头里声明，链接器在 `.lib` / `.dll` 导入库里找到地址，编译期"地址"就固定了。

OpenGL 不是这样：

| 年代 | 机制 |
|---|---|
| OpenGL 1.1 及以前 | 真的是 `opengl32.lib` 里的导出符号，正常链接 |
| OpenGL 1.2 及以后 | **不是符号**，是驱动在运行时提供的函数指针，用 `wglGetProcAddress("glGenBuffers")` 取 |

原因：显卡驱动独立于操作系统更新。`opengl32.dll` 是 1996 年左右的固定接口，新版 GL 函数由驱动在运行时注入。

### 详解二：GLAD 生成的到底是什么

GLAD 生成的 `glad.h` 里，每个 GL 函数都是一根**函数指针变量**：

```c
// 简化示意
extern PFNGLGENBUFFERSPROC glad_glGenBuffers;
#define glGenBuffers glad_glGenBuffers     // 你的调用被宏替换成函数指针调用
```

`gladLoadGLLoader` 做的事，就是逐个调用 `wglGetProcAddress`，把这些指针填上：

```
gladLoadGLLoader(glfwGetProcAddress)
        │              └─ GLFW 提供的"取函数地址"的适配器
        └─ 遍历 GLAD 内部登记的所有函数名，逐个填指针
```

### 详解三：这正是那次崩溃的根因

我在开发日志里记录过：

> 删去打印版本号之后返回值从 `0xcfffffff` 变成 0。
> Bug 根因定位：缺少在打印版本号函数调用之前必需的动态取函数函数调用。

现在可以精确解释：

```cpp
// 错误顺序
std::cout << glGetString(GL_VERSION);          // ← 此时 glad_glGetString 还是 nullptr
gladLoadGLLoader(...);                          // ← 才把它填上

// 正确顺序
gladLoadGLLoader(...);
std::cout << glGetString(GL_VERSION);           // ← 指针已有效
```

**调用一个空函数指针 → 跳到地址 0 → 访问违例。** 那串 `0xcfffffff` 就是 Windows 的访问违例退出码。

**这条经验的价值**：任何"You must call X before Y"的文档警告，都应该当成硬约束而不是建议——因为违反它的后果是崩溃而不是报错。

### 详解四：头文件顺序的真实原因

`glfw3.h` 默认会做一件事：

```c
#include <GL/gl.h>        // 老的 1.1 头，除非定义了 GLFW_INCLUDE_NONE
```

而 `glad.h` **自己也定义了全部 GL 类型和函数声明**。两者同时生效 → 类型/宏重复定义。

GLAD 的策略是在自己头里设置保护宏（使 `GL/gl.h` 的 include guard 失效），所以：

- **`glad.h` 在前** → GLAD 先定义类型、并让 `glfw3.h` 跳过 `GL/gl.h` → 干净
- **`glfw3.h` 在前** → 系统 `GL/gl.h` 先进来，GLAD 再进来 → 冲突

这不是"约定俗成"，是**必须**。

---

## Q4 MSAA 是什么？原理是什么？

**我的原始提问**：

> 我记得 MSAA 好像是某种抗锯齿的算法的名称。使用 `glfwWindowHint(GLFW_SAMPLES, 4);` 开 4xMSAA。所以 MSAA 是啥？原理是什么？

### 短答

**MSAA = Multi-Sample Anti-Aliasing，多重采样抗锯齿。**

核心思路：普通光栅化对每个像素**只在中心取一个点**判断覆盖；MSAA 在每个像素内取 **N 个采样点**（4x = 4 个），按"被三角形覆盖了几个采样点"算出覆盖率，最后按比例混合颜色。

### 详解一：锯齿是怎么来的

一个像素要么"整个属于三角形"，要么"完全不属于"——这是**二值判断**。所以斜边会呈现台阶状（jaggies）。

真实的几何覆盖是连续的部分覆盖（比如覆盖 37%），但光栅化把它量化成 0% 或 100%。

> 我主项目现有的光栅化算法，只有一个采样点，就是像素中心。这使我的立方体有很明显的锯齿状边缘。

### 详解二：MSAA 具体怎么做

以 4x 为例：

```
一个像素内放 4 个采样点（常见布局：旋转网格）

1. 覆盖判断：逐个采样点判断是否在三角形内 → 得到 coverage mask（如 0b1011）
2. 深度测试：逐采样点比较深度，各自写入
3. 片元着色：⚠️ 只在像素中心着色一次，得到 1 个颜色
4. Resolve：把覆盖到的采样点的颜色按比例混合 → 输出该像素最终颜色
```

**第 3 步是 MSAA 便宜的关键**——着色只做一次，不是四次。

### 详解三：和 SSAA 的区别（面试常考）

| | SSAA（超采样） | MSAA（多重采样） |
|---|---|---|
| 每个子采样点都跑片元着色器 | **是** | **否（只算一次）** |
| 显存开销 | N 倍 | N 倍（只有采样缓冲） |
| 着色开销 | **N 倍** | **1 倍** |
| 画质 | 更好（抗着色锯齿） | 只抗几何边缘 |

**所以 MSAA 的局限**：它只解决**几何边缘**的锯齿。对以下无效：

- 高频纹理（摩尔纹）
- 高光点（specular aliasing）
- 着色器内部计算产生的锯齿
- 透明物体的排序/边缘

这类问题要用 FXAA/TAA/后处理抗锯齿。

### 详解四：在 OpenGL 里怎么开

```cpp
glfwWindowHint(GLFW_SAMPLES, 4);        // 必须在 glfwCreateWindow 之前
```

**官方定义**（本地 `glfw3.h`）：

```c
/*! @brief Framebuffer MSAA samples hint. */
#define GLFW_SAMPLES  0x0002100D
```

**两个重要限制**：

1. **它只作用于"默认帧缓冲"**（也就是直接显示到窗口的那个）。如果你渲染到自建 FBO，要自己建 multisample renderbuffer（`glRenderbufferStorageMultisample`）+ 再 blit resolve，麻烦得多。
2. 它是 **hint（请求）不是命令**——驱动可能给你少于 4 个采样，甚至 0。

### 与我项目的关联

我在 `Stark.cpp` L38 写了这条注释，但**代码里注释掉了**（没启用）。这其实是个合理的选择：**先跑通管线，再谈画质**。等有立方体了再开 MSAA 对比效果，那时能直观看到锯齿差异，理解会深一层。

---

# 大目标二：让窗口留住并可交互

对应提交：`目标 1-2 ： 把一闪而过的框框留住`

这个阶段要解决的是：**窗口为什么需要"事件"和"呈现"两个动作，以及为什么它们必须每帧都做。**

---

## Q5 `glfwPollEvents()` 为什么叫 poll？每帧都要调吗？

**我的原始提问**：

> 处理键鼠事件 —— `glfwPollEvents();`

### 短答

`poll` = **轮询**：主动去问操作系统"有新事件吗？"，**不管有没有都立刻返回**。每帧都必须调用，否则窗口会"假死"。

> 我想起主项目这个函数签名被我误写为 PullEvent() 了。

### 详解一：为什么叫 poll

这是操作系统 API 的通用术语（`select` / `poll` / `epoll`）。含义是"**问一次，立即返回**"，与"阻塞等待"相对。

GLFW 提供了一对：

| 函数 | 行为 | 何时用 |
|---|---|---|
| `glfwPollEvents()` | 立即返回，有多少事件处理多少 | 实时渲染（每帧都画） |
| `glfwWaitEvents()` | **阻塞**，直到有新事件才返回 | 静态/省电场景（最小化、无动画） |

### 详解二：为什么必须每帧调

Windows 用**消息队列**模型。你的窗口能拖动、能响应关闭按钮、能重绘，靠的是程序不断地从队列里**取消息并分发**（`PeekMessage` + `DispatchMessage`）。

`glfwPollEvents()` 内部就在做这件事。

**不调的后果**：消息在队列里积压 → 窗口不响应鼠标点击、拖动卡住、标题栏变白 → Windows 弹出**"此程序无响应"**。

这不是"卡了几下"，是操作系统层面的假死。

### 详解三：`glfwPollEvents` 和 `glfwSwapBuffers` 的关系

两者**没有依赖关系**，但职责不同：

```
glfwSwapBuffers(window)  → 把后缓冲呈现到屏幕（输出）
glfwPollEvents()         → 处理系统消息（输入）
```

**顺序不严格要求**，但常见写法是 `SwapBuffers` 在前、`PollEvents` 在后，或反过来，都行。我的代码里用的是：

```cpp
glfwSwapBuffers(window);
currentSecond = glfwGetTime();
```
循环开头 `glfwPollEvents()`。

### 详解四：一个容易忽略的官方警告

本地 `glfw3.h` 里 `glfwPollEvents` 的注释有这一条：

```c
*  Do not assume that callbacks you set will _only_ be called in response to
*  event processing functions like this one.
```

意思是：**你注册的回调（比如 `glfwSetFramebufferSizeCallback`）不一定只在 `glfwPollEvents` 里被调用。** 某些窗口系统会在别的 GLFW 函数调用期间顺手派发事件。

**实践含义**：回调里不要做重活儿（不要在里面创建/销毁 GL 资源），因为它的**调用时机不完全可预测**。

### 详解五：最小化时的优化（我的 0×0 分支）

我在代码里加了：

```cpp
if (fbw == 0 || fbh == 0) { continue; }
```

但 `continue` 只是跳过了绘制，**循环仍在全速空转**（每轮还是调 `glfwPollEvents`）。窗口最小化放几小时会白烧 CPU。

更标准的写法：

```cpp
if (fbw == 0 || fbh == 0) {
    glfwWaitEvents();    // 阻塞到有事件（比如还原窗口）再继续
    continue;
}
```

**这一对函数（poll vs wait）放在一起理解，比单独记一个效果好得多。**

---

## Q6 `glfwSwapInterval(1)` 的"垂直同步"是什么？

**我的原始提问**（早期注释里留的）：

> `glfwSwapInterval(1);` —— 垂直同步是什么意思？

### 短答

**垂直同步（vsync）** = 让 `glfwSwapBuffers` **等到显示器完成一次垂直回扫再交换缓冲**，从而避免"撕裂"，并把帧率上限钉在刷新率。

`1` 表示"至少等待 1 次屏幕刷新"。

### 详解一：撕裂是怎么产生的

显示器不是瞬时更新整屏，而是**从左到右、从上到下一行行扫描**，60Hz 就是每 16.67ms 扫完一屏。

如果程序不考虑扫描进度，直接交换缓冲：

```
屏幕上 60% 显示旧帧，下 40% 显示新帧  →  画面中间一条水平错位线  =  tearing（撕裂）
```

### 详解二：`swapInterval(1)` 做了什么

```
渲染完成 → 请求 swap → 【等待下一次垂直回扫间隙】→ 真正交换
```

结果：
- **无撕裂**（交换总发生在扫描间隙）
- **帧率上限 = 刷新率**（60Hz 屏最多 60fps）

### 详解三：`interval` 的取值

| 值 | 含义 | 效果 |
|---|---|---|
| `0` | 不等待 | 帧率无上限，**有撕裂** |
| `1` | 等 1 次刷新 | 锁刷新率，无撕裂（最常见） |
| `n` | 等 n 次刷新 | 锁到 刷新率/n（如 2 → 30fps） |
| `-1` | 自适应 | 帧率低于刷新率时撕裂，高于时同步（减少卡顿感） |

### 详解四：两个必须知道的"不可靠"

**① 它是 hint（请求），驱动可以忽略**

本地 `glfw3.h` 原文：

```c
*  @remark Some GPU drivers do not honor the requested swap interval, either
*  because of a user setting that overrides the application's request or due to
*  bugs in the driver.
```

**在 NVIDIA 控制面板里强制"关闭垂直同步"，你的代码就不生效。** 所以调试帧率问题时，别假设 vsync 一定开着。

**② 它不会导致"帧时间平滑"，反而会"量化"帧时间**

如果一帧要 18ms，而刷新间隔是 16.67ms：

```
理想：18ms/帧 → 55.5 fps
实际：等 2 个回扫 = 33.3ms/帧 → 30 fps
```

**这就是"开启 vsync 后帧率突然砍半"的原因**——不是性能变差，是帧时间被量化了。

### 详解五：为什么放在 `MakeContextCurrent` 之后

`glfwSwapInterval` 操作的是**当前上下文的交换行为**。没有 current 上下文，它无从设置（会报 `GLFW_NO_CURRENT_CONTEXT`）。

所以顺序必须是：

```
glfwMakeContextCurrent(window);   // 先有 current
glfwSwapInterval(1);              // 再设它的 swap 行为
```

---

## Q7 `glfwGetKey` 为什么能传 `'A'`？为什么只能大写？

**我的原始提问 + 我的实验结论**：

> 等等，我记得 EasyX 的空格的键值也是 32 来着？是不是说明这里可以传一个字符作为键值？
>
> 的确可以。但是作为键值的字母只能是大写，只能接受 65 作为 A 而不能接受 97。

### 短答

因为 **GLFW 的键码常量数值上等于 ASCII 大写字母**。我从本地 `glfw3.h` 读出的真实值：

| 常量 | 值 | 对应 |
|---|---|---|
| `GLFW_KEY_SPACE` | **32** | ASCII 空格 |
| `GLFW_KEY_A` | **65** | ASCII `'A'` |
| `GLFW_KEY_ESCAPE` | **256** | 超出 ASCII 范围（非打印键） |
| `GLFW_KEY_UNKNOWN` | **-1** | 未知键 |

所以 `glfwGetKey(window, 'A')` 和 `glfwGetKey(window, GLFW_KEY_A)` **是同一个调用**。

**我的 EasyX 直觉是正确的**，不是巧合。

### 详解一：为什么只能大写（97 不行）

因为 GLFW 的键码是**物理按键标识**，不是字符编码。

GLFW 的约定是：**按 US 键盘布局，把这个物理位置对应的（未按 Shift 时的）大写字母**作为键码。

- `GLFW_KEY_A = 65` → "A 键所在的那个物理位置"
- `97`（`'a'`）**不在键码表里** → `glfwGetKey` 返回 `GLFW_KEY_UNKNOWN`(-1)，永远不会等于 `GLFW_PRESS`

### 详解二：这是"键码 ≠ 字符"的经典体现

| 你想知道 | 该用什么 | 得到什么 |
|---|---|---|
| **某个键现在是否被按住** | `glfwGetKey` | 状态（`GLFW_PRESS` / `GLFW_RELEASE`） |
| **用户实际输入了什么字符** | `glfwSetCharCallback` | Unicode 码点（区分大小写、考虑 Shift、输入法、IME） |

**举个例子**：

```
用户按住 Shift 再按 A 键
  → glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS   （物理键被按下）
  → 字符回调收到 U+0041 'A'                        （实际输入的字符）
  → 如果没按 Shift，字符回调会收到 U+0061 'a'
```

**如果一个游戏用 `glfwGetKey` 去实现文字输入框，它将无法区分大小写、无法输入中文。** 这是很多初学者的坑。

### 详解三：键码范围的设计

| 范围 | 含义 | 例子 |
|---|---|---|
| 32 ~ 96 | 可打印 ASCII 对应的键 | `GLFW_KEY_SPACE`(32)、`GLFW_KEY_A`(65)、`GLFW_KEY_0`(48) |
| 161 ~ 348 | 非 ASCII 键（功能键、方向键、修饰键） | `GLFW_KEY_ESCAPE`(256)、`GLFW_KEY_F1`(290)、`GLFW_KEY_LEFT_SHIFT`(340) |

所以 `glfwGetKey` 的第二个参数是 `int`，能同时容纳这两段。

---

## Q8 `glfwMakeContextCurrent` 为什么要设"当前"？

**我的原始提问**：

> 设置上下文为当前（为什么，是不是和多窗口相关？）

### 短答

**因为 OpenGL 的 API 没有"在哪个上下文上执行"这个参数。**

看任何一个 `gl*` 函数的签名——`glGenBuffers(1, &id)`、`glClear(mask)`、`glDrawArrays(...)`——**没有一个接受上下文句柄**。所以 GL 必须依赖一个**隐式的"当前上下文"**。

### 详解一：我的直觉对了一半

我说"是不是和多窗口相关"——**对了一半**：

- ✅ 多窗口共享 GL 资源时确实要切换上下文
- ❌ 但根本原因不是"多窗口"，而是**GL 的 API 设计**

即使你只有一个窗口、一个上下文，`glfwMakeContextCurrent` 也**必须调用**——否则上下文虽然创建了，但"没有任何线程在使用它"，所有 `gl*` 调用都无的放矢。

### 详解二：当前上下文是 **per-thread**（按线程）的

| 事实 | 含义 |
|---|---|
| 一个线程**同一时刻只能有一个** current 上下文 | 切换 = 解绑旧的 + 绑定新的 |
| 不同线程可以各自有**不同的** current 上下文 | 这是多线程渲染的基础 |
| 上下文之间可以**共享资源**（用 `glfwCreateWindow` 的 share 参数） | 共享纹理/buffer，但**不共享状态** |

**`glfw3.h` 原文**：

```c
*  @thread_safety This function may be called from any thread.
```

对比 `glfwInit` 和 `glfwTerminate` 都是 "must only be called from the main thread"——`MakeContextCurrent` 则允许任意线程，正是因为它的用途就是**在不同线程上绑定各自的上下文**。

### 详解三：`window` 传 `NULL` 是合法的

```c
*  @param[in] window The window whose context to make current, or `NULL` to
*  detach the current context.
```

传 `NULL` = **解绑当前上下文**。用途：

- 线程退出前解绑，避免资源释放时序问题
- 把上下文彻底交给另一个线程之前，先解绑

### 详解四：多窗口的正确姿势

```cpp
// 窗口 A 渲染
glfwMakeContextCurrent(windowA);
glClear(...); glDrawArrays(...);
glfwSwapBuffers(windowA);

// 窗口 B 渲染
glfwMakeContextCurrent(windowB);
glClear(...); glDrawArrays(...);
glfwSwapBuffers(windowB);
```

**代价**：每次都可能有同步开销（驱动要 flush 上一个上下文的命令队列）。所以性能敏感的场景会尽量用"单上下文 + 多 FBO"而不是"多上下文"。

### 与我项目的关联

`Stark.cpp` 的顺序完全正确：

```cpp
GLFWwindow* window = glfwCreateWindow(...);
glfwMakeContextCurrent(window);                    // ① 先绑定
glfwSwapInterval(1);                               // ② 设这个上下文的 swap 行为
if (!gladLoadGLLoader(...)) { return 2; }          // ③ 再加载函数（需要 current 上下文）
```

**顺序不能换**：`gladLoadGLLoader` 内部会调 `wglGetProcAddress`，而它**需要一个 current 上下文**才能工作。

---

## Q9 `glfwGetFramebufferSize` 为什么用两个指针参数输出？

**我的原始提问**：

> 首先获取一下屏幕大小。看英语提示，两个引用参数应该是用来存储函数本来应该返回的值的。

> 所以循环体外部会有 `fbw` 和 `fbh` 的定义。

### 短答

**因为 C 语言没有多返回值。**

GLFW 是 C 库，要保持 C ABI。要返回两个整数，只能用 **out-parameter（输出参数）**：函数返回值留给"成功/失败"或干脆是 `void`，实际结果通过指针写回。

> 实际上，C++ 语言程序设计的课程内容已经包含了这个问题的一部分：函数只允许一个返回值，想要一个函数处理多个数据并获取多个结果，最好的方法就是引用传参，并附有课上案例分析。

### 详解一：C 的 out-param 惯例

```c
void glfwGetFramebufferSize(GLFWwindow* window, int* width, int* height);
```

| 参数 | 方向 | 说明 |
|---|---|---|
| `window` | in | 输入：查哪个窗口 |
| `width` | **out** | 输出：写回宽度 |
| `height` | **out** | 输出：写回高度 |
| 返回值 | —— | `void`，没有成功/失败信息 |

**注意这里的一个设计取舍**：因为是 `void`，你**无法知道调用是否失败**。如果 `window` 是无效句柄，`width/height` 可能不被写入——这正是为什么要**先初始化**：

```cpp
int fbw = 0, fbh = 0;      // ← 这一句是防御，不是形式主义
glfwGetFramebufferSize(window, &fbw, &fbh);
```

如果忘记初始化又恰好调用失败，你会读到**栈上的垃圾值**，然后传给 `glViewport` —— 一个完全随机的视口。

### 详解二：为什么不用结构体返回

C 其实**可以**返回结构体：

```c
typedef struct { int width, height; } Size;
Size glfwGetFramebufferSize(GLFWwindow*);   // 语法上合法
```

GLFW 不这么做，原因有三：

1. **保持 C89 兼容**：早期 C 不支持在返回值里用复杂结构体，ABI 约定也不统一（不同编译器/调用约定对 small struct 的返回方式不同）
2. **风格一致性**：GLFW 的**所有**查询函数都用 out-param
   - `glfwGetWindowSize(w, &width, &height)`
   - `glfwGetCursorPos(w, &xpos, &ypos)`
   - `glfwGetMonitorPos(mon, &x, &y)`
   - `glfwGetVersion(&major, &minor, &rev)`
3. **允许复用已有变量**：out-param 可以写进任何变量，不需要额外分配

### 详解三：一个 C++ 程序员应该有的敏感度

在 C++ 里我们**会**返回结构体（`std::pair`、`struct`、`std::tuple`），因为有移动语义和统一 ABI。

但**和 C 库交互时，必须接受 C 的惯例**。这个"翻译层"的敏感度很重要——**你写的项目里如果要做 C++/C 混编，也要在边界上遵守这个约定，不能只顾自己写起来舒服。**

---

## Q10 `glfwGetTime()` 返回 0 是什么意思？

**我的原始笔记**：

> 根据英语提示，这个函数获取一个 GLFW 秒数并返回 double 秒数，而且是线程安全的。
> GLFW 秒数指从 GLFW 被初始化或者执行 `glfwSetTime()` 之后到调用这个函数之前的秒数。
> 如果它返回 0，说明有 GLFW 未初始化错误。

### 核实结果：**我的笔记与官方文档一致**

本地 `glfw3.h` 原文：

```c
*  @return The current time, in seconds, or zero if an
*  [error](@ref error_handling) occurred.
*
*  @errors Possible errors include @ref GLFW_NOT_INITIALIZED.
```

三条笔记全部对得上：

| 我的笔记 | 官方文档 | 判定 |
|---|---|---|
| 返回 double 秒数 | `@return The current time, in seconds` | ✅ |
| 从 `glfwInit`/`glfwSetTime` 起算 | `measures time elapsed since GLFW was initialized`（或 set 之后） | ✅ |
| 返回 0 表示未初始化错误 | `or zero if an error occurred` / `GLFW_NOT_INITIALIZED` | ✅ |

### 但要补一个精度：`0` 是**有歧义**的

因为计时器测的是"**自 `glfwInit()` 起的经过时间**"，所以：

**刚初始化完的那一瞬间，`glfwGetTime()` 本来就会返回接近 0 的值。**

于是 `0.000` 可能是：

| 可能 | 判断依据 |
|---|---|
| 正常：刚刚初始化，时间还没走 | —— |
| 错误：`GLFW_NOT_INITIALIZED` | 需要用 `glfwGetError()` 查错误码 |

**只看返回值无法区分两者。**

不过我的代码**不需要处理这个歧义**——我在 L32 已经检查了 `glfwInit()` 的成功与否，走到 L115 必然初始化成功，所以这里的 0 只可能是"刚启动"。

### 一处需要修正的表述：线程安全

我写的是"而且是线程安全的"。官方原文更精确：

```c
*  @thread_safety This function may be called from any thread.  Reading and
*  writing of the internal base time is not atomic, so it needs to be
*  externally synchronized with calls to @ref glfwSetTime.
```

翻译：

- ✅ **可以从任意线程调用**
- ⚠️ 但内部基准时间的读写**不是原子的**
- ⚠️ 如果你同时在别的线程调 `glfwSetTime`，**需要你自己加同步**

**所以是"部分线程安全"，不是无条件线程安全。** 这个差别在文档里写得很清楚，是我当时的表述过度简化了。

### 实现的底层

文档还提到：

```c
*  This function and @ref glfwSetTime are helper functions on top of @ref
*  glfwGetTimerFrequency and @ref glfwGetTimerValue.
```

即：GLFW 用一对 **原始单调计数器**（频率 + 计数值）实现秒数换算。**"单调"（monotonic）很关键**——它不会因为系统时间被修改（NTP 校正、用户改时钟）而跳变。

**如果你自己用 `std::chrono::system_clock` 做游戏计时，就会撞到这个坑**：系统对时一次，你的游戏时间会突然跳 3 秒。正确用 `steady_clock`。

---

## Q11 `glfwTerminate()` 是不是还要掌管资源释放？

**我的原始提问**：

> 这个函数是不是还要掌管资源的释放？

### 短答

**管，但只管 GLFW 自己的东西，不管 OpenGL 的对象。**

### 详解一：`glfwTerminate` 到底释放什么

本地 `glfw3.h` 原文：

```c
*  This function destroys all remaining windows and cursors, restores any
*  modified gamma ramps and frees any other allocated resources.  Once this
*  function is called, you must again call glfwInit successfully before you
*  will be able to use most GLFW functions.
```

| 它做 | 说明 |
|---|---|
| 销毁**所有剩余窗口** | 包括它们的 OpenGL 上下文 |
| 恢复被修改的**伽马设置** | 如果你用 `glfwSetGamma` 改过显示器 |
| 释放 GLFW 分配的**所有内存** | 内部结构、键码表等 |
| 关闭**平台后端** | 注销 Win32 窗口类 |

**之后必须重新 `glfwInit()` 才能再用 GLFW。**

### 详解二：关键区分——三层资源，三个归属

这是最容易混的地方：

| 层 | 例子 | 谁负责释放 |
|---|---|---|
| **GL 对象** | VBO、VAO、Program、Texture、Framebuffer | **你**：`glDeleteBuffers` / `glDeleteProgram` …必须在**上下文销毁之前** |
| **GL 上下文 + 窗口** | `GLFWwindow*` | `glfwDestroyWindow`（显式）或 `glfwTerminate`（兜底） |
| **GLFW 库自身** | 内部全局状态 | `glfwTerminate` |

**所以 `glfwTerminate` 不会帮你删 VBO。** 它销毁上下文时，那些 GL 对象会随上下文一起被驱动回收——但那是**驱动兜底**，不是你"正确释放"。

**正确顺序**：

```cpp
// ① 先删 GL 对象（此时上下文还活着）
glDeleteProgram(program);
glDeleteBuffers(1, &vbo);
glDeleteVertexArrays(1, &vao);

// ② 再销毁窗口（释放上下文）
glfwDestroyWindow(window);

// ③ 最后终止 GLFW
glfwTerminate();
```

### 详解三：那为什么我代码里两个都调了

```cpp
glfwDestroyWindow(window);
glfwTerminate();
```

`glfwTerminate` 的文档说它会"销毁所有剩余窗口"——所以**理论上只调 Terminate 也行**。

但显式 `glfwDestroyWindow` 有三个好处：

1. **释放时机可控**：你在"这一刻"明确知道上下文没了，而不是等到程序末尾
2. **多窗口时更清晰**：可以逐个销毁，也可以先销毁部分
3. **符合"谁创建谁释放"原则**：`glfwCreateWindow` 和 `glfwDestroyWindow` 成对

**唯一需要注意的**：`MakeContextCurrent` 之后如果没解绑，销毁窗口时要确保那个上下文不在别的线程上 current。文档警告：

```c
*  @warning The contexts of any remaining windows must not be current on any
*  other thread when this function is called.
```

### 详解四：什么时候**不用**调 `glfwTerminate`

文档：

```c
*  If initialization fails, there is no need to call this function, as it is
*  called by @ref glfwInit before it returns failure.
```

**`glfwInit()` 失败时，它内部已经调用了 `glfwTerminate`。** 所以：

```cpp
if (!glfwInit()) {
    return 1;          // ← 不需要再调 glfwTerminate
}
```

这个细节我原来的代码就是对的（直接 return），但当时并不知道**为什么**对。

---

# 大目标三：让画面内容由我决定

对应提交：`目标1-3 ： 召唤黑框框`、`目标 1-4 ： 完善帧循环`

这个阶段要解决的是：**从"屏幕上有个窗口"到"我精确控制每个像素的颜色"。**

---

## Q12 `glViewport` 是干嘛的？四个参数是什么意思？

**我的原始提问**：

> `glViewport` 是干嘛的？四个参数都是什么意思？

### 短答

`glViewport` 定义 **"归一化设备坐标（NDC）怎么映射到窗口的像素坐标"**——它是渲染管线里**最后一个坐标变换**，把 `[-1, 1]` 的 NDC 变成实际像素位置，同时**限定只在这个矩形区域内光栅化**。

### 参数

```cpp
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
```

| 参数 | 含义 | 注意 |
|---|---|---|
| `x` | 视口左下角像素 X | **原点是左下角**，不是左上角 |
| `y` | 视口左下角像素 Y | 同上 |
| `width` | 视口宽度（像素） | |
| `height` | 视口高度（像素） | |

`glViewport(0, 0, 1280, 720)` = 铺满整个窗口。

### 精确公式

对 NDC 坐标 `(x_ndc, y_ndc)`：

```
x_win = x + (x_ndc + 1) * width  / 2
y_win = y + (y_ndc + 1) * height / 2
```

传入 `glViewport(0, 0, W, H)` 化简：

```
x_win = (W/2) * x_ndc + W/2
y_win = (H/2) * y_ndc + H/2
```

### 关键：这个函数我自己实现过

`Fern3DGameEngine_Rebuild` 里的 `FernPack/render/Window.h` L56–60：

```cpp
windowMatrix.values[0][0] = -(double)width / 2;
windowMatrix.values[0][3] = (double)width / 2 + 0.5;   // 带有0.5的偏移可以使点被映射到像素中央
windowMatrix.values[1][1] = -(double)height / 2;
windowMatrix.values[1][3] = (double)height / 2 + 0.5;
```

这是**行向量**约定（`v' = v · M`），展开：

```
x_screen = -(W/2) * x_ndc + (W/2 + 0.5)
y_screen = -(H/2) * y_ndc + (H/2 + 0.5)
```

**逐项对照**：

| | 我的 `windowMatrix` | `glViewport(0,0,W,H)` | 差异原因 |
|---|---|---|---|
| x 斜率 | **−W/2** | **+W/2** | 我的 NDC **+X 朝左**；GL 的 NDC **+X 朝右** |
| y 斜率 | **−H/2** | **+H/2** | 我的屏幕原点**左上、y 向下**；GL 帧缓冲原点**左下、y 向上** |
| 平移 | W/2 **+ 0.5** | W/2 | 我手动补 +0.5 对齐**像素中心**；GL 光栅化器内部就用像素中心 |
| 执行者 | 我手写矩阵乘法 | 硬件固定功能 | |

**所以：我在软件管线里手写的"视窗变换"，在 GPU 上就是一行 `glViewport`。** 我不是在学新概念，是在学"同一件事换谁来做"。

> 注：`Perspective.h` 头部写明「相机空间定义：**+X 左**，+Y 上，+Z 前（右手系）」——我的 x 取负是**故意的、自洽的**，不是 bug。

### 三个必踩的坑

**① 原点是左下角**

和 GDI / EasyX 相反。要画在窗口右上角，参数不是 `(W-w, 0, w, h)`，而是 `(W-w, H-h, w, h)`。

**② 用 `glfwGetFramebufferSize`，不要用 `glfwGetWindowSize`**

高 DPI 缩放下两者不等：

```
窗口尺寸    1280 × 720        ← glfwGetWindowSize
帧缓冲尺寸  1920 × 1080       ← glfwGetFramebufferSize（真正要用的）
```

传错 → 画面只渲染在左下角一小块。

**③ 窗口缩放时不会自动更新**

`glViewport` 的默认值在**上下文创建时**设为当时的帧缓冲大小，之后**再也不变**。

两种处理：

```cpp
// A. 每帧设（简单，初学推荐）
glfwGetFramebufferSize(window, &fbw, &fbh);
glViewport(0, 0, fbw, fbh);

// B. 注册回调（高效）
glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
});
```

### 与 aspect 的联动（马上会遇到）

投影矩阵的 `aspect` 必须和 **viewport 的宽高比**一致：

```cpp
float aspect = (float)fbw / (float)fbh;
Mat4 proj = Perspective(fov, aspect, 0.1f, 100.0f);
```

**不一致 → 立方体被压扁/拉长。** 这是"透视看着怪"最常见的原因，比投影矩阵算错更常见。

### `glViewport` **不是**什么

| 函数 | 作用 | 区别 |
|---|---|---|
| `glViewport` | **映射** NDC → 像素，并限定光栅化区域 | 改变坐标映射本身 |
| `glScissor` | 像素级**裁剪测试** | 不改变映射，只丢弃矩形外的片元 |
| `glDepthRange` | NDC 的 z 映射到深度缓冲范围 | 只影响深度 |

### 我的决定

我在 `Stark.cpp` L85–88 记下了这个决定：

> 需要注意我的 NDC 和窗口的坐标定义其实和 OpenGL 不同。如果还按照我自己的习惯开发代码，会导致窗口里面没有我想渲染的东西。
> 在接下来的开发中我希望用 OpenGL 自有的标准，对接开发。

**理由**：软件管线是自洽的，但 GPU 管线的标准是 +X 右。为了两边一致而翻转 GPU 的惯例，以后每接一个第三方工具（RenderDoc、模型导入、光照公式）都要多记一条例外。**少数派约定在自己家里没问题，一旦要和生态对接就会不断付利息。**

---

## Q13 `glClearColor` 是什么？参数和返回值？

**我的原始提问**：

> `glClearColor(0.08f, 0.09f, 0.11f, 1.0f);` 这个函数是啥？参数返回值都是啥？

### 短答

**它只在设置一个状态变量**——"等下 `glClear` 清屏时，用什么颜色填"。

**它自己不清理任何东西。**

### 参数

```cpp
void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
```

| 参数 | 类型 | 范围 | 含义 |
|---|---|---|---|
| `red` | `GLfloat` | **0.0 ~ 1.0** | 红分量 |
| `green` | `GLfloat` | 0.0 ~ 1.0 | 绿分量 |
| `blue` | `GLfloat` | 0.0 ~ 1.0 | 蓝分量 |
| `alpha` | `GLfloat` | 0.0 ~ 1.0 | 透明度 |

**返回值：`void`。**

⚠️ **是 `0.0~1.0` 的浮点，不是 `0~255` 的整数。** 超出范围会被钳到 `[0,1]`。

### 我做过的实验（这是本文档里最有价值的一段）

我在 `Stark.cpp` L93–99 记录了完整的假设—验证—修正过程：

```
假设：四个分量是 0~1，如果传 0~255 会被钳成 1 → 变成纯白

验证：glClearColor(207, 239, 252, 255);
结果：窗口变为纯白 —— 与猜想一致

修正：glClearColor(207/255.0, 239/255.0, 252/255.0, 255/255.0);
结果：窗口变为利姆露蓝

二次验证：我电脑的主题色就是利姆露蓝，标题栏是准确的利姆露蓝。
          现在窗口和标题栏无法分辨 → 成功
```

**三个值得复用的方法论**：

1. **先提出可证伪的假设，再跑实验** —— 不是"试试看会怎样"
2. **用已知正确的参照物验证** —— 拿系统主题色的标题栏当色卡，比对着屏幕猜专业
3. **确认钳位方向** —— 不只是"变了"，而是"变成了纯白，说明 207→1.0 而不是 207→0.81"

### 关键：它和 `glClear` 是两个函数，必须配对

```cpp
glClearColor(0.08f, 0.09f, 0.11f, 1.0f);   // ① 设定："清成什么颜色"
glClear(GL_COLOR_BUFFER_BIT);              // ② 执行："真的清"
```

**只调 ① 不调 ② → 什么都不清。**

### 对照我自己写过的代码

`Fern3DGameEngine_Rebuild` 的 `Window::Clear()`：

```cpp
void Clear() {
    cleardevice();        // 清成 setbkcolor() 设定的背景色
}
```

完整流程对照：

| | 设定状态 | 执行动作 | 呈现到屏幕 |
|---|---|---|---|
| **EasyX 管线** | `setbkcolor(color)` | `cleardevice()` | `FlushBatchDraw()` |
| **OpenGL** | `glClearColor(r,g,b,a)` | `glClear(GL_COLOR_BUFFER_BIT)` | `glfwSwapBuffers()` |

**一一对应。** 我不是在学新东西，是在学"同一套状态/动作分离，换谁来实现"。

### 第四个参数 `alpha` 管什么

**在默认窗口上基本没用。** 它只是往颜色缓冲的 alpha 通道写一个值，屏幕显示通常忽略这个通道。

真正有意义的时候：

- 渲染到 **FBO**（离屏帧缓冲），之后当纹理采样
- 开了**混合**（`glEnable(GL_BLEND)`）且 alpha 参与运算

**实践：填 `1.0f`。** 后面写 PBR、阴影贴图时会真正用到。

### 三个补充

**① 它是"状态"，会一直保持** —— 不需要每帧调

```cpp
glClearColor(0.08f, 0.09f, 0.11f, 1.0f);   // 循环外设一次
while (...) {
    glClear(GL_COLOR_BUFFER_BIT);           // 每帧清
}
```

放在循环里也无害，而且要做"背景色渐变"就必须放循环里。

**② 默认值是纯透明黑 `(0, 0, 0, 0)`** —— 不调 `glClearColor` 直接 `glClear`，清出来是黑色。这就是"默认清屏就是黑"这个误解的来源。

**③ `glClear` 不受 `glViewport` 限制** —— 见 Q14。

---

## Q14 `glClear` 的参数和返回值？

**我的原始提问**：

> `glClear` 的参数和返回值有吗？

### 函数签名

```cpp
void glClear(GLbitfield mask);
```

**返回值：`void`。**

**参数：只有一个 `mask`**——**位掩码**，不是枚举。

### 参数详解

`GLbitfield` 在 `glad.h` 中定义为 `typedef unsigned int`。它能接受的位（数值从本地 `glad.h` 读取）：

| 常量 | 实际值 | 清哪个缓冲 | 清屏值来自 |
|---|---|---|---|
| `GL_COLOR_BUFFER_BIT` | `0x00004000` | 颜色缓冲 | `glClearColor(r,g,b,a)` |
| `GL_DEPTH_BUFFER_BIT` | `0x00000100` | 深度缓冲 | `glClearDepth(d)` |
| `GL_STENCIL_BUFFER_BIT` | `0x00000400` | 模板缓冲 | `glClearStencil(s)` |

因为是位掩码，可以按位或：

```cpp
glClear(GL_COLOR_BUFFER_BIT);                              // 只清颜色
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);        // 颜色 + 深度（最常用）
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);  // 全清
glClear(0);                                                // 合法，但什么都不做
```

### 为什么设计成位掩码，而不是三个函数？

**① 一次函数调用 = 一次硬件批量操作**

驱动可以把"清颜色 + 清深度"合并成硬件的一次快清。`glClear` 在很多 GPU 上是**专用快速通路**（接近 `memset` 的速度），不是"画一个全屏四边形"。分成三次调用就丢失了合并机会。

**② `glClear` 没有颜色参数，是刻意的**

因为 OpenGL 是状态机：

```
glClearColor(...)   → 改状态（"清成什么颜色"）
glClearDepth(...)   → 改状态（"清成什么深度"）
glClear(mask)       → 读状态 + 执行动作
```

`Q13` 里那个"状态 vs 动作分离"，在这里体现得最彻底。

### 四个容易踩的点

**① `glClear` 不认 `glViewport`，只认 `glScissor`**

```cpp
glViewport(0, 0, 640, 720);      // 只在左半边绘制
glClear(GL_COLOR_BUFFER_BIT);    // ← 还是清整个屏幕！
```

想"左半边清蓝、右半边清红"必须用 scissor：

```cpp
glEnable(GL_SCISSOR_TEST);
glScissor(0, 0, 640, 720);
glClearColor(0, 0, 1, 1);  glClear(GL_COLOR_BUFFER_BIT);
glScissor(640, 0, 640, 720);
glClearColor(1, 0, 0, 1);  glClear(GL_COLOR_BUFFER_BIT);
```

**② 缓冲不存在时，静默忽略**

默认窗口帧缓冲通常**没有模板缓冲**，`glClear(GL_STENCIL_BUFFER_BIT)` 不报错也不清任何东西。"我清了但没效果"时，先确认那个缓冲真的存在。

**③ 忘清深度 = 3D 渲染经典 bug** —— 见 Q15

**④ 清屏值只在调过设置函数后才是你要的**

默认：颜色透明黑、深度 `1.0`、模板 `0`。

### 一次干净的设置

```cpp
// 循环外（状态设一次就够）
glEnable(GL_DEPTH_TEST);
glClearColor(0.81f, 0.94f, 0.99f, 1.0f);   // 利姆露蓝，归一化后
glClearDepth(1.0);                          // 默认就是 1.0，可不写

// 循环内
glfwGetFramebufferSize(window, &fbw, &fbh);
glViewport(0, 0, fbw, fbh);
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
// ... 绘制 ...
glfwSwapBuffers(window);
```

**为什么清屏色故意不用纯黑**：纯黑可能是"没清屏"，也可能是"清成了黑色"，两者无法区分。**用一个偏离纯黑的可辨识颜色（比如利姆露蓝），能直接证明清屏真的生效了。** 这是我自己在实验里发现的技巧，后来成了调试习惯。

---

## Q15 为什么深度缓冲必须每帧一起清？

**我的原始笔记**：

> 文档提示我需要一并重设深度缓冲，而且是着重强调。完整管线实现后，我想可以故意踩一下这个坑。

### 短答

深度缓冲保存的是 **每一个屏幕像素对应离屏幕最近的那个点，离屏幕有多远**  
如果只在初始化时清一次，后续每帧的新几何都会和**上一帧残留的深度值**比较——上一帧的近处物体会"挡住"这一帧的远处物体。

### 详解：具体会看到什么

假设第 1 帧画了一个近处的小球（深度 0.2），第 2 帧把小球移到远处（深度 0.8）：

```
第 2 帧深度测试：0.8 < 0.2 ？ → 否 → 该像素被丢弃
结果：小球有一部分"消失"了，留下上一帧形状的残影
```

**视觉表现**：拖影、残像、遮挡关系错乱、"物体在某些角度莫名其妙消失"。

### 为什么这个 bug 特别危险

**因为在静态场景下可能完全看不出来。**

| 场景 | 能否暴露问题 |
|---|---|
| 静态立方体、相机不动 | ❌ 深度值每帧相同，测试结果一致 → **看不出异常** |
| 旋转立方体 / 相机移动 | ✅ 立刻拖影 |

**这正是它危险的地方**：很多人用静态场景测试，得出"不清深度也没事"的错误结论，一加动画就崩。

### 我说"想故意踩一下这个坑"——这是个好主意，但要设计成实验

**实验设计**：

```
对照实验：把 glClear 改成只清颜色（去掉 GL_DEPTH_BUFFER_BIT）

实验组 A：静态立方体
  → 预期：可能完全看不出问题

实验组 B：旋转立方体
  → 预期：拖影 / 残像 / 遮挡错乱

结论：深度缓冲的"脏"只有在深度值变化的帧之间才暴露
```

**为什么值得做**：实验 A 会让你**误以为没问题**——这个"误以为"才是最有价值的发现。**能主动构造出"看起来正常但实际上是错的"场景，说明理解已经不是停留在"文档让我清"。**

### 为什么清屏值深度默认是 1.0

深度比较默认是 `GL_LESS`：**新深度 < 旧深度 才通过**（更近的才能覆盖）。

所以清屏要把深度缓冲填成**"最远"**，这样任何真实几何都比它近，都能通过。

> 我的主项目里面，清除深度缓冲的时候用的是 1.5 ，而我的NDC空间定义了z的范围是[0,1]。  
> 我认为这样可以很方便地判断深度缓冲的值有没有被重设，有没有被刷新。  
> 被重设的深度缓冲会是 1.5 ，在远平面上的点会把深度缓冲刷新为最远的 1.0 。  

用 NDC 的 `[0, 1]` 约定（近=0，远=1），最远就是 `1.0`。这就是 `glClearDepth(1.0)` 的默认值。

**如果清成 0.0**，那所有几何的深度都比 0 大，全部通不过 → **屏幕上什么都不显示**。

---

## Q16 `glClear` 的位掩码设计能不能借鉴到我自己的引擎？

**我的原始笔记**：

> 我觉得我可以借鉴这个函数的设计，优化时可以考虑将缓冲清除函数实现为一个。

### 短答

**能借鉴，但要看清楚它成立的前提。现在（只有一个颜色缓冲）不该重构。**

### 详解一：这个设计成立需要三个前提

| 前提 | OpenGL 满足吗 | 我的引擎满足吗 |
|---|---|---|
| 被清的缓冲**互不重叠**，可合并/并行 | ✅ 颜色、深度、模板是独立内存区 | 要看实现 |
| 清屏值的**类型各不相同**（颜色 4 float、深度 1 float、模板 1 int） | ✅ 所以必须拆成三个**状态设置**函数 | 我只有颜色 → 不需要拆 |
| 清屏是**热路径**，值得省一次调用 | ✅ | ❓ 软件管线里 `cleardevice()` 一次清完，本来就一次 |

**关键洞察**：OpenGL 把"设置"和"执行"拆开、执行又用一个位掩码，**是因为它要同时管三种类型不同的缓冲**。

**我目前只有颜色缓冲一个**——硬套这个设计会让代码更绕（多两个参数、多一个掩码判断），换来的只有一个调用点。

### 详解二：什么时候该借鉴

**等到真的出现"清屏代码被复制粘贴三次"的时候。** 具体触发条件：

- 同时有颜色 + 深度 + 模板缓冲
- 且发现有 3 处以上代码在做"清其中一部分"
- 且这些调用点的组合各不相同

**现在先不重构**——主仓库刚做完一轮工程化，正处在"能跑就别动"的阶段。

### 详解三：但有一个手法值得现在就学

**位掩码本身**这个技巧。

它在软件管线的**另一个场景**更有价值：如果 `DrawWithDepthTest` 以后要支持"只更新颜色不更新深度"或"只写深度不写颜色"，一个：

```cpp
enum class BufferMask : unsigned {
    Color   = 1u << 0,
    Depth   = 1u << 1,
    All     = Color | Depth,
};
// 配合 operator| 重载
```

会比三个 `bool` 参数（`drawWithDepthTest(x, y, depth, color, true, false, true)`）清晰得多。

**判断标准**：当布尔参数的个数超过 2 个，就该考虑位掩码或强类型枚举了——因为你已经无法从调用点看出每个 `true` 是什么含义。

### 一个诚实的自我提醒

**"看到好设计就想搬"是很自然的冲动，但工程判断力体现在"判断它在这个上下文里是否成立"。**

OpenGL 的 `glClear` 是为"多缓冲、多类型、热路径"设计的。**我的引擎目前一个条件都不满足。** 这时候正确的做法是：**记下来，等条件成熟**，而不是为了"学了新东西"而重构。

---

# 附录一：踩坑记录（现象 → 排查 → 根因 → 修法）

> 这一节记录的是**真实的失败过程**。比上面任何一条问答都更接近工程实践。

## 坑 1：程序崩溃，退出码 `0xcfffffff`

| 项 | 内容 |
|---|---|
| **现象** | `std::cout << glGetString(GL_VERSION);` 一执行就崩，退出码 `0xcfffffff` |
| **排查** | 删掉这一行 → 返回值变成 0，程序正常结束 |
| **根因** | `gladLoadGLLoader` 还没调用，`glGetString` 的函数指针是 `nullptr`。调用空指针 → 跳到地址 0 → 访问违例 |
| **修法** | 把 `gladLoadGLLoader` 移到所有 `gl*` 调用之前 |
| **教训** | 文档里"You must call X before Y"是**硬约束**，违反的后果是崩溃而不是报错 |

**为什么这个坑值得记**：那串十六进制退出码看起来像编译器/链接器的内部错误，实际是**运行时的访问违例**。识别它需要知道"空函数指针调用"的典型表现。

## 坑 2：白框框（大晚上闪瞎眼睛）

| 项 | 内容 |
|---|---|
| **现象** | 窗口是纯白，不是预期的黑 |
| **根因** | ① 没有 `glClear`（没有清屏）② 没有 `glfwSwapBuffers`（没有呈现） |
| **修法** | 补上 `glfwSwapBuffers` → 变黑（但见坑 3）；再补 `glClearColor` + `glClear` → 可控 |
| **教训** | "白"和"黑"都不是你画出来的，是**显存碰巧的内容**。没有清屏就没有确定性 |

## 坑 3：黑框框其实是巧合

| 项 | 内容 |
|---|---|
| **现象** | 加了 `glfwSwapBuffers` 后窗口变黑了，以为完成了 |
| **根因** | **没有 `glClear`**。变黑是因为翻出来的后缓冲**碰巧是零**，不是因为我清了 |
| **修法** | 补 `glClearColor` + `glClear` |
| **教训** | **"看起来对"和"做对了"是两件事。** 判据应该是"我能精确控制颜色"，而不是"它现在是黑的" |

**这个坑让我形成了调试习惯**：清屏色故意用一个**偏离纯黑的可辨识颜色**（利姆露蓝），这样"没清屏"和"清成了黑色"就能区分开。

## 坑 4：`glClearColor` 传 0~255 → 纯白

| 项 | 内容 |
|---|---|
| **现象** | 传 `(207, 239, 252, 255)` 得到纯白 |
| **根因** | 参数是 `0.0~1.0` 浮点，传入值被钳到 `[0,1]` → `(1,1,1,1)` = 纯白 |
| **修法** | `(207/255.0, 239/255.0, 252/255.0, 255/255.0)` |
| **教训** | 见 Q13 的"假设—验证—修正"过程 |

## 坑 5：守卫条件写成了 `&&`

| 项 | 内容 |
|---|---|
| **现象** | 防止帧缓冲为 0×0 的守卫：`if (fbw == 0 && fbh == 0) continue;` |
| **根因** | `&&` 只在**两个都为 0** 时拦截。若只有一维为 0，会继续执行 → `aspect = 0/x = 0` → 投影矩阵 `f/0 = +inf` → NaN 污染 |
| **修法** | 改成 `\|\|`：任一维非法就跳过 |
| **教训** | **守卫条件要匹配"不变量"。** 这里的意图是"两个维度都必须有效"，所以任一失效就该拦截。写 `&&` 是"意图翻译错误" |

**为什么当时没暴露**：还没算 aspect，且 Windows 最小化时确实两个都变 0。**典型的"恰好能跑"。**

## 坑 6（预防性）：最小化时忙等

| 项 | 内容 |
|---|---|
| **现象** | 最小化时 `continue` 跳过绘制，但循环仍全速空转 |
| **修法** | 在守卫分支里调 `glfwWaitEvents()` 阻塞等待 |
| **教训** | `glfwPollEvents`（立即返回）和 `glfwWaitEvents`（阻塞）是**一对**，按场景选 |

> 实际在此前的提交中，我没有实现这个内容。

---

# 附录二：我自己答对的问题

> 这几条**我没有提问**，是自己推导/实验出来的。列在这里是因为它们同样是学习过程的一部分。

| # | 内容 | 验证方式 |
|---|---|---|
| 1 | `glfwCreateWindow` 第 4 参是**监视器**指针，传 `nullptr` = 窗口化，传监视器 = 全屏 | 读参数名 + 官方文档 |
| 2 | 第 5 参是**共享上下文** | 同上 |
| 3 | 返回值非空 = 窗口句柄，空 = 失败 | 同上 |
| 4 | `GLFW_KEY_SPACE` 的值是 **32**（和 EasyX 空格键值相同） | 从 `glfw3.h` 读到 `#define GLFW_KEY_SPACE 32` |
| 5 | 这个"相同"不是巧合——GLFW 键码在可打印区**就是 ASCII 大写字母** | 实测 `'A'`(65) 有效、`'a'`(97) 无效 |
| 6 | 传 0~255 给 `glClearColor` 会被钳成 `(1,1,1,1)` 变纯白 | **先假设，再跑实验验证** |
| 7 | 可以用系统主题色（利姆露蓝标题栏）作为色卡验证渲染结果 | 实际比对成功 |

---

## 结语

这份文档记录的**不是"我学会了多少 API"，而是"我卡在哪里、怎么想通的"**。

回头看，真正有价值的都不是"某个函数有几个参数"（那些查文档就有），而是：

- **为什么** `glad.h` 必须在 `glfw3.h` 之前（类型主张的冲突）
- **为什么** 键码不等于字符（物理按键 vs 输入字符）
- **为什么** 我的 `windowMatrix` 和 `glViewport` 差一个负号（坐标约定不同）
- **为什么** "黑框框"不等于"我清屏了"（碰巧 vs 可控）

**这些"为什么"才是能带走的东西。** API 会换（OpenGL → Vulkan → 下一个），但"坐标系约定""状态与动作分离""out-param 惯例""静默失败"这些概念是跨 API 的。

下一个大目标是**阶段 3：VBO / VAO / 着色器 → 画出第一个三角形**。等撞到新问题，这份文档会继续长。
