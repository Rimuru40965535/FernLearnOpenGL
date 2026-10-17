/**
* @file Stark.cpp
* @brief 从零开始学习OpenGL API，目标是还原我的里程碑成果
* @details
* 目标：验证 GLFW + GLAD 工具链打通，并跑通一条 GPU 渲染管线。
*
* 第4个小目标：完善帧循环，实现“我让窗口是啥颜色窗口就得是啥颜色”
* 
* @author 半人马座beta星
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
	GLFWwindow* window = glfwCreateWindow(1280, 720, "1、创建窗口", nullptr, nullptr);
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
		glClearColor(207 / 255.0, 239 / 255.0, 252 / 255.0, 255 / 255.0);
		// 验证结果：我电脑设置的主题色就是利姆露蓝，也就导致我的标题栏的颜色是准确的利姆露蓝。现在窗口和标题栏无法分辨，证明成功显示背景色。

		// 重设画面缓冲状态的执行函数
		// 位掩码参数的设计是对函数的一个优化，它可以支持按位或（想清谁就清谁，彼此互不干扰）
		// 函数会执行选定的清除操作，这可比实现两个清除函数、调用两个清除函数好多了
		// 除了这两个预定义常量，还有一个预定义常量 GL_STENCIL_BUFFER_BIT ，指定它可以清除模板缓冲
		// 我觉得我可以借鉴这个函数的设计，优化时可以考虑将缓冲清除函数实现为一个。
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);      
		// 文档提示我需要一并重设深度缓冲，而且是着重强调。完整管线实现后，我想可以故意踩一下这个坑。

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
