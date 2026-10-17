/**
* @file Stark.cpp
* @brief 从零开始学习OpenGL API，目标是还原我的里程碑成果
* @details
* 目标：验证 GLFW + GLAD 工具链打通，并跑通一条 GPU 渲染管线。
*
* 第3个小目标：完善帧循环，把框框边黑
* 
* @author 半人马座beta星
* @date 2026/10/17
*/

#include <iostream>
#include <sstream>

// 完全从主项目复制的代码，包括Matrix4x4的未测试修复。

#include "include/FernPack/basic_funcion/MathUtils.h"
#include "include/FernPack/math/Vector3D.h"
#include "include/FernPack/math/Matrix4x4.h"
#include "include/FernPack/math/Perspective.h"

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
	//上一给提交中把这个语句删掉可以解决程序莫名其妙崩溃问题。原因已经查明。
	//我没有调用一个必须的前置调用。
	std::cout << glGetString(GL_VERSION) << std::endl;


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

		//OpenGL使用双缓冲。这个函数的作用就是交换缓冲区。
		glfwSwapBuffers(window);

		glfwGetTime();
	}
	//不是说好的黑框框吗？怎么是白框框？大晚上编译闪瞎我的眼睛

	//关闭窗口
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
