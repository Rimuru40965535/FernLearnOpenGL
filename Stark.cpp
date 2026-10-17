/**
* @file Stark.cpp
* @brief 从零开始学习OpenGL API，目标是还原我的里程碑成果
* @details
* 目标：验证 GLFW + GLAD 工具链打通，并跑通一条 GPU 渲染管线。
*
* 目标3 ： 渲染出一个正方体
* 
* 
* @author 半人马座beta星
* 测试数据使用AI生成
* @date 2026/10/17
*/

#include <iostream>
#include <sstream>

// 完全从主项目复制的代码，包括Matrix4x4的未测试修复。

// vcxproj 里 AdditionalIncludeDirectories 已经是 $(ProjectDir)include 。基于此，调整 include 路径：
#include "FernPack/basic_funcion/MathUtils.h"
#include "FernPack/math/Vector3D.h"
#include "FernPack/math/Matrix4x4.h"
#include "FernPack/math/Perspective.h"

// 加载OpenGL函数

#include <glad/glad.h>      // 必须在 GLFW 之前
#include <GLFW/glfw3.h>

// 点着色器源码。看使用要求应该要写成字符串数组。
// 第一行 #version 是干啥的？
// 第三行 看起来不像C++代码。它是拿来干啥的？更深入的，编译是什么过程？这段代码片会被编译成什么？怎么运行？
// Python可以做到把一串字符串的引号去掉以后把结果当作程序代码运行。这里也有类似的机制吗？
const GLchar* vertice_src = "#version 460 core\n\nlayout(location=0) in vec3 aPos;// 从 VBO 的 location 0 读入顶点位置\n\nvoid main(){\n\tgl_Position = vec4(aPos,1.0); // 必须写！直接当作裁剪空间坐标\n}";

// 片元着色器源码。
const GLchar* plate_src = "#version 460 core\n\nout vec4 FragColor;// 输出到颜色缓冲\n\nvoid main() {\n\tFragColor = vec4(1.0, 0.5, 0.2, 1.0);// 橙色，RGBA 都是 0~1\n}";



int main() {

	//初始化（为什么呢？有什么说法吗？）
	if (!glfwInit()) { return 1; }

	//指定上下文版本（上下文是什么，我只在AI相关的地方听说过？为什么拿不到这个上下文会导致现代着色器不可用？）
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//我记得MSAA好像是某种抗锯齿的算法的名称。使用glfwWindowHint(GLFW_SAMPLES, 4); 开4xMSAA。所以MSAA是啥？原理是什么？）

	//创建窗口
	//根据英文提示，前两个参数应该是窗口宽高，第三个参数是窗口标题，第四个参数是指向某一个窗口的指针，填入窗口指针指定一个窗口开启全屏显示，如果填空指针就是不开启全屏，最后一个参数是和哪一个上下文共享资源。
	GLFWwindow* window = glfwCreateWindow(1280, 720, "2.绘制三角形", nullptr, nullptr);
	//返回值如果非空，就是窗口句柄指针，如果是空是报错。

	//设置上下文为当前（为什么，是不是和多窗口相关？）
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);     // 1 = 开启垂直同步

	//OpenGL的函数是在运行时从显卡里动态取的。
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { return 2; }
	//过往提交中把这个语句删掉可以解决程序莫名其妙崩溃问题。原因已经查明。
	//我没有调用一个必须的前置调用。
	std::cout << glGetString(GL_VERSION) << std::endl;

	int fbw = 0, fbh = 0;
	double currentSecond = 0;

	// 三角形顶点数据。
	// 这里填写的已经是 NDC 空间的数据。
	// OpenGL 的 NDC 空间定义，xy两个轴都应该是[-1,1]。 -> 我需要确认一下NDC 的定义以验证我的猜想。这里的 NDC 定义和我的 NDC 定义不一样。我的技术选择是迁就已有定义和代码风格。
	const float NDC_vertices[] = {
		0.0f,  0.5f, 0.0f,
		-0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
	};

	// 验收代码，预期代码行输出36（预期的9个4字节的float大小）
	std::cout << "NDC_vertices 数组大小：" << sizeof(NDC_vertices) << std::endl;
	// 实际控制台输出：
	/**
	* 4.6.0 - Build 31.0.101.2137
	* NDC_vertices 数组大小：36
	*/
	// 验收合格。

	// 为了三角形的清楚显示，我把背景颜色换一下，并且这一行代码放到循环外，以节省每循环一次的状态设置函数调用（而且循环内实际调用了也是啥也没改）
	glClearColor(0.11f, 0.45f, 0.14f, 1.0f);

	// 接下来的代码片段是 AI 生成的测试数据片段。
	// 由于我没有预先计算一组可以复用的数据，所以我需要使用 AI 生成一份。
	// ================= P0 步骤1：CPU 侧矩阵自检 =================
	glfwGetFramebufferSize(window, &fbw, &fbh);      // 循环外先取一次尺寸

	Fern::Matrix4x4 model = Fern::Matrix4x4::Identity();
	Fern::Matrix4x4 view = Fern::Matrix4x4::Translation(0, 0, 4);   // +4，见下方说明
	Fern::Matrix4x4 proj = Fern::GeneratePerspectiveMatrix(
		fbw, fbh,
		60.0 * 3.14159265358979 / 180.0,
		0.1, 100.0);
	Fern::Matrix4x4 MVP = model * view * proj;      // ★ 你的顺序（书写=应用顺序）

	const double corners[8][3] = {
		{ 1, 1, 1}, { 1, 1,-1}, { 1,-1, 1}, { 1,-1,-1},
		{-1, 1, 1}, {-1, 1,-1}, {-1,-1, 1}, {-1,-1,-1},
	};
	std::cout << "---- MVP 自检（8 个角点的 NDC）----" << std::endl;
	for (int i = 0; i < 8; ++i) {
		Fern::Vector3D ndc = MVP * Fern::Vector3D(corners[i][0], corners[i][1], corners[i][2]);
		std::cout << "  角点" << i << "  ndc = ("
			<< ndc.x << ", " << ndc.y << ", " << ndc.z << ")" << std::endl;
	}
	// ===========================================================

	// VAO 和 VBO 
	// 这好像是OpenGL区别于我的引擎的一个特性 -> VAO 和 VBO 分别是什么？有什么用处？
	// 这块大量采用宏封装，在IDE里面没有浮动显示的英语提示，只有解内联。
	// API 清单里面关于这一块为什么要这样的解释有点难懂。我希望更深入地了解。
	GLuint vao = 0, vbo = 0;
	glGenVertexArrays(1, &vao);     // -> 这个函数的参数列表和返回值？API 文档里面说这个函数是拿来申请几个存名字的地址的，为什么要这么干？
	glGenBuffers(1, &vbo);			// -> 这个函数的参数列表和返回值？为什么这里又变成 VBO 了？

	glBindVertexArray(vao);                        // ① 先绑 VAO   -> 这个函数的参数列表和返回值？
	glBindBuffer(GL_ARRAY_BUFFER, vbo);            // ② 再绑 VBO	-> 这个函数的参数列表和返回值？ -> 为什么要这样干？

	glBufferData(GL_ARRAY_BUFFER,               //    上传 -> 这个函数的参数列表和返回值？
		sizeof(NDC_vertices),					//    字节数（用 sizeof！）
		NDC_vertices,							//    数据指针
		GL_STATIC_DRAW);						//    用法：上传一次不改（因为现在我在做的是静态渲染，所以数据使用方法是上传一次不改已经足够。
	// 验收：vao 和 vbo 是非零整数，且glGetError() 返回 GL_NO_ERROR
	std::cout << "vao= " << vao << std::endl << "vbo= " << vbo << std::endl;
	std::cout << ((glGetError() == GL_NO_ERROR) ? "NO_ERROR通过" : "NO_ERROR不通过") << std::endl;
	// 实际控制台输出：
	/**
	* 4.6.0 - Build 31.0.101.2137
	* NDC_vertices 数组大小：36
	* vao= 1
	* vbo= 1
	* NO_ERROR通过
	*/
	// 验收合格。


	// 这一块在文档里面是描述顶点属性。-> 这个代码块存在的意义是什么？我想深入探究
	//
	glVertexAttribPointer(0, // ① 属性位置，必须和着色器 layout(location=N) 对上 -> 看不见英文提示。我想知道这个函数参数都是啥，返回值是啥，深入探究这个函数主要执行了什么功能，为什么这些参数是必要的，运作原理是什么。
		3,                   // ② 这个属性占几个分量（位置 = x,y,z = 3）
		GL_FLOAT,            // ③ 分量类型
		GL_FALSE,            // ④ 是否归一化（float 数据填 GL_FALSE）
		3 * sizeof(float),   // ⑤ stride：相邻两个顶点隔多少字节
		(void*)0);           // ⑥ offset：这个属性在顶点内的字节偏移 -> void* 是什么东西？

	glEnableVertexAttribArray(0);              // -> 看不见英文提示。我想知道这个函数参数都是啥，返回值是啥，深入探究这个函数主要执行了什么功能，为什么这些参数是必要的，运作原理是什么。
	// 验收：文档里面说，没有返回值，等到画出来才知道对错。因此跳过验收阶段。
	
	// 这一步在 API 文档里面的编译着色器的步骤。
	GLuint vertice_vs = glCreateShader(GL_VERTEX_SHADER);   // 片元着色器用 GL_FRAGMENT_SHADER -> Shader 是什么？（我看文档之后的推断是着色器。）有什么功能？
	// 在这里好像就应该预定义一个src数组存储源码了。
	glShaderSource(vertice_vs, 1, &vertice_src, nullptr);           // ①着色器 ②源码串数 ③源码数组 ④长度数组(nullptr=以\0结尾) -> 这个看代码的话是要传入一个 shader，传入一串以字符串形式存储的代码，以及这段代码有多长。
	glCompileShader(vertice_vs);	// 这个函数就是编译着色器的函数吗？

	// 这一段应该不属于编译，应该属于检查结果。
	GLint ok = GL_FALSE;
	glGetShaderiv(vertice_vs, GL_COMPILE_STATUS, &ok);      // 查是否编译成功
	if (!ok) {
		char log[1024];
		glGetShaderInfoLog(vertice_vs, sizeof(log), nullptr, log);   // 取出错误日志
		// 打印 log —— 里面有出错的行号，是唯一的线索
		std::cerr << "[shader compile failed]\n" << log << std::endl;
		return 3; //编译失败就不往下走了，尽早退出。
	}
	else {
		std::cout << "点着色器编译通过" << std::endl;
	}

	// 片元着色器
	GLuint plate_fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(plate_fs, 1, &plate_src, nullptr);           
	glCompileShader(plate_fs);

	ok = GL_FALSE;
	glGetShaderiv(plate_fs, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		char log[1024];
		glGetShaderInfoLog(plate_fs, sizeof(log), nullptr, log);   // 取出错误日志
		// 打印 log —— 里面有出错的行号，是唯一的线索
		std::cerr << "[shader compile failed]\n" << log << std::endl;
		return 3; //编译失败就不往下走了，尽早退出。
	}
	else {
		std::cout << "片元着色器编译通过" << std::endl;
	}

	// 链接程序
	GLuint prog = glCreateProgram();
	glAttachShader(prog, vertice_vs);
	glAttachShader(prog, plate_fs);
	glLinkProgram(prog);

	ok = GL_FALSE;
	glGetProgramiv(prog, GL_LINK_STATUS, &ok);
	if (!ok) {
		char log[1024];
		glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
	}

	// 链接完了就可以删程序 -> 编译结果是怎么被持有的？
	glDeleteShader(vertice_vs);
	glDeleteShader(plate_fs);


	//循环，和我的引擎有相似之处
	while(!glfwWindowShouldClose(window)){
		
		//处理键鼠事件
		glfwPollEvents();

		//看起来这个API应该是判别一个键是否被按下。
		//API清单给的键应该是ESC，我改空格试试。
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}
		//等等，我记得EazyX的空格的键值也是32来着？是不是说明这里可以传一个字符作为键值？
		if (glfwGetKey(window, 'A') == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}
		//的确可以。但是作为键值的字母只能是大写，只能接受65作为A而不能接受97。

		// 每一帧都要重设深度缓冲。
		// 大小需要每帧核验。
		// 首先获取一下屏幕大小。看英语提示，两个引用参数应该是用来存储函数本来应该返回的值的。
		// 所以循环体外部会有 fbw 和 fbh 的定义。
		// 窗口尺寸变化，深度缓冲的尺寸应当跟随变化。所以需要首先核验窗口大小。
		glfwGetFramebufferSize(window, &fbw, &fbh);		

		// 对最小化尺寸做一些防护,文档里有说需要计算宽高比，防止出现除零错误。
		// 这里我把守卫的逻辑想错了，我以为只有两个方向上都是0（也就是窗口最小化的情形）需要被守卫
		// 实际上需要守卫的情况还包括某一个维度变成0的情况。所以这两个条件应该“任一满足即满足”而不是“任一不满足即不满足”。
		if (fbw == 0 || fbh == 0) { continue; }

		// 问了一下AI才知道这个就是我已经实现的视口变换。
		// 在这里，这个函数的作用是：定义"归一化设备坐标（NDC）怎么映射到窗口的像素坐标"——它是渲染管线里最后一个坐标变换，把 [-1, 1] 的 NDC 变成实际的像素位置，同时限定只在这个矩形区域内光栅化
		// 需要注意我的NDC和窗口的坐标定义其实和OpenGL不同。
		// 如果还按照我自己的习惯开发代码，会导致窗口里面没有我想渲染的东西。
		// 参考实现踩过这个坑。在接下来的开发中我希望用OpenGL自有的标准，对接开发。
		glViewport(0, 0, fbw, fbh);                             

		// 设置清屏使用的背景色
		// 四个分量分别是RGBA分量。
		// 我明白我为啥没和我自己的实现对上号了，奶奶滴这里还藏了一个陷阱：
		// 四个分量都是 0~1之间的浮点数，而不是0~255之间的整数。所以如果我写 glClearColor(207,239,252,255) 会因为数值超限，被规约成(1,1,1,1)，从而画面变成纯白。跑一下验证一下我的猜想。
		// glClearColor(207, 239, 252, 255);
		// 结果与猜想一致，窗口变为纯白。
		// 现在试试这个值能否把窗口填充色变为利姆露蓝。（207，239，252）
		//glClearColor(207 / 255.0, 239 / 255.0, 252 / 255.0, 255 / 255.0);
		// 验证结果：我电脑设置的主题色就是利姆露蓝，也就导致我的标题栏的颜色是准确的利姆露蓝。现在窗口和标题栏无法分辨，证明成功显示背景色。


		// 重设画面缓冲状态的执行函数
		// 位掩码参数的设计是对函数的一个优化，它可以支持按位或（想清谁就清谁，彼此互不干扰）
		// 函数会执行选定的清除操作，这可比实现两个清除函数、调用两个清除函数好多了
		// 除了这两个预定义常量，还有一个预定义常量 GL_STENCIL_BUFFER_BIT ，指定它可以清除模板缓冲
		// 我觉得我可以借鉴这个函数的设计，优化时可以考虑将缓冲清除函数实现为一个。
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);      
		// 文档提示我需要一并重设深度缓冲，而且是着重强调。完整管线实现后，我想可以故意踩一下这个坑。

		// 根据已有开发经验，绘制代码集中在清屏与交换缓冲区之间。


		glUseProgram(prog);
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, 3);





		//OpenGL使用双缓冲。这个函数的作用就是交换缓冲区。
		glfwSwapBuffers(window);

		// 根据浮动显示的英语提示，这个函数获取一个GLFW秒数并返回 double 秒数，线程安全不是无条件的。所以我拿一个 double 变量存储返回值。
		// GLFW 秒数指从 GLFW 被初始化或者执行 glfwSetTime() 之后到调用这个函数之前的秒数。
		// 如果它返回0，说明有GLFW未初始化错误。
		currentSecond = glfwGetTime();
	}


	//关闭窗口
	glfwDestroyWindow(window);

	// 这个函数是不是还要掌管资源的释放？
	glfwTerminate();

	return 0;
}
