/**
* @file Stark.cpp
* @brief 从零开始学习OpenGL API，目标是还原我的里程碑成果
* @details
* 目标：验证 GLFW + GLAD 工具链打通，并跑通一条 GPU 渲染管线。
*
* 目标4：实现简单场景渲染
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

// 点着色器源码。
const GLchar* vertice_src = "#version 460 core\n\nlayout(location=0) in vec3 aPos;\nlayout(location=1) in vec3 aColor;\nuniform mat4 uMVP;\nout vec3 vColor;\nvoid main(){\n\tgl_Position = uMVP * vec4(aPos,1.0);\nvColor = aColor;\n}";

// 片元着色器源码。
const GLchar* plate_src = "#version 460 core\n\nin vec3 vColor;\nout vec4 FragColor;\n\nvoid main() {\n\tFragColor = vec4(vColor, 1.0);\n}";



int main() {

	//初始化
	if (!glfwInit()) { return 1; }



	//指定上下文版本
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	

	//创建窗口
	GLFWwindow* window = glfwCreateWindow(1280, 720, "4.Render A Simple Scene", nullptr, nullptr);
	//返回值如果非空，就是窗口句柄指针，如果是空是报错。

	//设置上下文为当前
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);     // 1 = 开启垂直同步

	//OpenGL的函数是在运行时从显卡里动态取的。
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { return 2; }
	std::cout << glGetString(GL_VERSION) << std::endl;

	glEnable(GL_DEPTH_TEST);

	int fbw = 0, fbh = 0;
	double currentSecond = 0;
	float glMat[16];

	/*
	// 三角形顶点数据。
	// 这里填写的已经是 NDC 空间的数据。
	// OpenGL 的 NDC 空间定义，xy两个轴都应该是[-1,1]。 
	const float NDC_vertices[] = {
		0.0f,  0.5f, 0.0f,
		-0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
	};
	std::cout << "NDC_vertices 数组大小：" << sizeof(NDC_vertices) << std::endl;
	*/

	// 为了三角形的清楚显示，我把背景颜色换一下，并且这一行代码放到循环外，以节省每循环一次的状态设置函数调用（而且循环内实际调用了也是啥也没改）
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// 接下来的代码片段是 AI 生成的测试数据片段。
	// 由于我没有预先计算一组可以复用的数据，所以我需要使用 AI 生成一份。
	// 立方体：36 个顶点（12 三角形），每顶点 = 位置(3) + 颜色(3)
	// 六个面六种颜色，便于肉眼验证深度测试
	const float cube_vertices[] = {
		// 顶点       颜色   法线    
		// +X 面 红
		 1,-1,-1,  1,0,0,   1,0,0,
		 1, 1,-1,  1,0,0,   1,0,0,
		 1, 1, 1,  1,0,0,	1,0,0,

		 1,-1,-1,  1,0,0,   1,0,0,
		 1, 1, 1,  1,0,0,   1,0,0,
		 1,-1, 1,  1,0,0,   1,0,0,
		 // -X 面 青
		 -1,-1,-1,  0,1,1,  -1,0,0, 
		 -1,-1, 1,  0,1,1,  -1,0,0,
		 -1, 1, 1,  0,1,1,	-1,0,0,

		 -1,-1,-1,  0,1,1,  -1,0,0,
		 -1, 1, 1,  0,1,1,  -1,0,0,
		 -1, 1,-1,  0,1,1,  -1,0,0,
		 // +Y 面 绿
		 -1, 1,-1,  0,1,0,  0,1,0,
		 -1, 1, 1,  0,1,0,  0,1,0,
		 1, 1, 1,  0,1,0,   0,1,0,

		 -1, 1,-1,  0,1,0,  0,1,0,
		 1, 1, 1,  0,1,0,   0,1,0,
		 1, 1,-1,  0,1,0,   0,1,0,
		 // -Y 面 品红
		 -1,-1,-1,  1,0,1,   0,-1,0,
		 1,-1,-1,  1,0,1,    0,-1,0,
		 1,-1, 1,  1,0,1,    0,-1,0,

		 -1,-1,-1,  1,0,1,   0,-1,0,
		 1,-1, 1,  1,0,1,    0,-1,0,
		 -1,-1, 1,  1,0,1,   0,-1,0,
		 // +Z 面 蓝
		 -1,-1, 1,  0,0,1,   0,0,1,
		 1,-1, 1,  0,0,1,    0,0,1,
		 1, 1, 1,  0,0,1,    0,0,1,

		 -1,-1, 1,  0,0,1,   0,0,1,
		 1, 1, 1,   0,0,1,   0,0,1,
		 -1, 1, 1,  0,0,1,   0,0,1,
		 // -Z 面 黄
		 -1,-1,-1,  1,1,0,   0,0,-1,
		 -1, 1,-1,  1,1,0,   0,0,-1,
		 1, 1,-1,  1,1,0,    0,0,-1,

		 -1,-1,-1,  1,1,0,   0,0,-1,
		 1, 1,-1,  1,1,0,    0,0,-1,
		 1,-1,-1,  1,1,0,    0,0,-1,
	};

	std::cout << "正方体顶点数组大小：（预期：1296）\t" << sizeof(cube_vertices) << std::endl;

	// 地面顶点数据
	const float ground_vertices[] = {
		//顶点		  颜色					法向量
		-10, 0, -10,  0.35f,0.35f,0.35f,	0,1,0,
		10, 0,  10,   0.35f,0.35f,0.35f,	0,1,0,
		10, 0, -10,   0.35f,0.35f,0.35f,	0,1,0,

		-10, 0, -10,  0.35f,0.35f,0.35f,	0,1,0,
		-10, 0,  10,  0.35f,0.35f,0.35f,	0,1,0,
		10, 0,  10,   0.35f,0.35f,0.35f,	0,1,0,
	};

	std::cout << "地板顶点数组大小：（预期：216）\t" << sizeof(ground_vertices) << std::endl;

	// VAO 和 VBO 
	GLuint vao = 0, vbo = 0;
	glGenVertexArrays(1, &vao);     
	glGenBuffers(1, &vbo);			

	glBindVertexArray(vao);                        
	glBindBuffer(GL_ARRAY_BUFFER, vbo);            

	// 上传顶点数据
	glBufferData(GL_ARRAY_BUFFER,               
		sizeof(cube_vertices),
		cube_vertices,
		GL_STATIC_DRAW);	// 静态数据用static而不是静态绘制用static					
	


	// 配置正方体顶点数据解密卡
	GLsizei stride = 9 * sizeof(float);
	//					插槽	大小	类型		归一化		间隔		偏移
	glVertexAttribPointer(0,	3,		GL_FLOAT,	GL_FALSE,	stride,		(void*)0);
	glEnableVertexAttribArray(0);     

	// 配置颜色数据解密卡
	//					插槽	大小	类型		归一化		间隔		偏移
	glVertexAttribPointer(1,	3,		GL_FLOAT,	GL_FALSE,	stride,		(void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// 配置法向量数据解密卡
	//					插槽	大小	类型		归一化		间隔		偏移
	glVertexAttribPointer(2,	3,		GL_FLOAT,	GL_FALSE,	stride,		(void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);
	

	// 这一步在 API 文档里面的编译着色器的步骤。
	GLuint vertice_vs = glCreateShader(GL_VERTEX_SHADER);   
	// 在这里好像就应该预定义一个src数组存储源码了。
	glShaderSource(vertice_vs, 1, &vertice_src, nullptr);           
	glCompileShader(vertice_vs);	

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

	std::cout << "链接程序完成" << std::endl;

	// 链接完了就可以删程序
	glDeleteShader(vertice_vs);
	glDeleteShader(plate_fs);

	GLint uMVP = glGetUniformLocation(prog, "uMVP");
	if (uMVP < 0) {
		std::cerr << "[warn] uMVP 未找到" << std::endl;
	}

	std::cout << "删除着色器源码完成" << std::endl;

	

	// std::cout << "上传矩阵完成" << std::endl;
	std::cout << "准备进入循环" << std::endl;
	std::cout << "near 随时间递增，约 12 秒后画面全黑属预期；按 R 重设" << std::endl;

	//循环，和我的引擎有相似之处
	while(!glfwWindowShouldClose(window)){
		
		//处理键鼠事件
		glfwPollEvents();

		// 处理键盘事件
		// 关闭
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}
		//暂停
		else if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
			std::cout << "时间已暂停，按 E 继续" << std::endl;
			while (!glfwWindowShouldClose(window)) {
				glfwWaitEvents();
				if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)break;
				if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
					std::cout << "时间已暂停，若欲退出请先按 E 继续" << std::endl;
				}
			}
			glfwSetTime(currentSecond);
		}
		// 重设动画
		else if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
			glfwSetTime(0);
		}
		// 关键时间
		else if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
			std::cout << "时间已暂停，按 E 继续" << std::endl;
			std::cout << "关键时间点：\n\t按 A 渲染第 13.25 秒\n\t按 S 渲染第 20 秒\n\t按 D 渲染第 25 秒\n\t按 F 渲染第 0 秒" << std::endl;
			double tempSecond = 0;
			while (!glfwWindowShouldClose(window)) {
				glfwPollEvents();
				if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)break;
				else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)tempSecond = 13.25;
				else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)tempSecond = 20;
				else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)tempSecond = 25;
				else if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)tempSecond = 0;

				glfwGetFramebufferSize(window, &fbw, &fbh);
				glViewport(0, 0, fbw, fbh);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				Fern::Matrix4x4 model = Fern::Matrix4x4::RotationX(tempSecond / 10) * Fern::Matrix4x4::RotationY(tempSecond / 20);
				Fern::Matrix4x4 view = Fern::Matrix4x4::Translation(0, 0, 4 - tempSecond / 5);
				Fern::Matrix4x4 proj = Fern::GeneratePerspectiveMatrix(
					fbw, fbh,
					Fern::Math::PI / 3,
					0.1, 100.0);
				Fern::Matrix4x4 MVP = model * view * proj;
				for (int row = 0; row < 4; ++row)
					for (int col = 0; col < 4; ++col)
						glMat[col * 4 + row] = (float)MVP.values[row][col];
				glUseProgram(prog);                             
				glUniformMatrix4fv(uMVP, 1, GL_FALSE, glMat);

				glBindVertexArray(vao);
				glDrawArrays(GL_TRIANGLES, 0, 36);

				glfwSwapBuffers(window);
			}
			glfwSetTime(currentSecond);
		}
		

		// 每一帧都要重设深度缓冲。
		// 大小需要每帧核验。
		// 首先获取一下屏幕大小。看英语提示，两个引用参数应该是用来存储函数本来应该返回的值的。
		// 所以循环体外部会有 fbw 和 fbh 的定义。
		// 窗口尺寸变化，深度缓冲的尺寸应当跟随变化。所以需要首先核验窗口大小。
		glfwGetFramebufferSize(window, &fbw, &fbh);		

		// 对最小化尺寸做一些防护
		if (fbw == 0 || fbh == 0) { continue; }

		// 视口变换矩阵重设
		glViewport(0, 0, fbw, fbh);                             

		// 清缓冲
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);      
		

		// 根据已有开发经验，绘制代码集中在清屏与交换缓冲区之间。

		// 动态旋转需要每帧更新矩阵并上传
		//Fern::Matrix4x4 model = Fern::Matrix4x4::Identity();
		Fern::Matrix4x4 model = Fern::Matrix4x4::RotationX(Fern::Math::PI / 3) * Fern::Matrix4x4::RotationY(Fern::Math::PI / 4);
		//Fern::Matrix4x4 model = Fern::Matrix4x4::RotationX(currentSecond / 10) * Fern::Matrix4x4::RotationY(currentSecond / 20);
		Fern::Matrix4x4 view = Fern::Matrix4x4::Translation(0, 0, 4);
		//Fern::Matrix4x4 view = Fern::Matrix4x4::Translation(0, 0, 4-currentSecond/5);
		//Fern::Matrix4x4 proj = Fern::GeneratePerspectiveMatrix(fbw, fbh,Fern::Math::PI /3, 0.1, 100.0);
		Fern::Matrix4x4 proj = Fern::GeneratePerspectiveMatrix(fbw, fbh,Fern::Math::PI / 3, 0.1+currentSecond, 100.0);
		Fern::Matrix4x4 MVP = model * view * proj;
		// 转置：行优先存储 + 列向量作用  →  GLSL 要列主序
		
		for (int row = 0; row < 4; ++row)
			for (int col = 0; col < 4; ++col)
				glMat[col * 4 + row] = (float)MVP.values[row][col];

		glUseProgram(prog);                              // 必须在 glUniform 之前
		glUniformMatrix4fv(uMVP, 1, GL_FALSE, glMat);

		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, 36);





		//OpenGL使用双缓冲。这个函数的作用就是交换缓冲区。
		glfwSwapBuffers(window);

		// 获取 GLFW 时间
		currentSecond = glfwGetTime();
	}


	//关闭窗口
	glfwDestroyWindow(window);

	// 这个函数是不是还要掌管资源的释放？
	glfwTerminate();

	return 0;
}
