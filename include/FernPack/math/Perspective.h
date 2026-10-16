/**
* @file FernPack/math/Projection.h
* @brief 投影矩阵生成模块
* @details
* 提供透视投影矩阵的生成函数。
* 相机空间定义：+X 左，+Y 上，+Z 前（右手系）。
* NDC 空间定义：x/y 范围 [-1, 1]，z 范围 [0, 1]。
*
* 文档中有我自己的完全独立的推导过程
* @author 半人马座beta星 
* @date 2026/9/28
*
*/

#pragma once

namespace Fern {
	/**
	* @brief 生成投影矩阵
	* @param int windowWidth 窗口宽度
	* @param int windowHeight 窗口高度
	* @param double fov_radius 弧度制fov
	* @param double near_plane_z 近平面z值
	* @param double far_plane_z 远平面z值
	* @returns 投影矩阵
	*/
	inline Matrix4x4 GeneratePerspectiveMatrix(int windowWidth, int windowHeight, double fov_radius, double near_plane_z, double far_plane_z) {

		if (near_plane_z < 0) return Matrix4x4();
		if (far_plane_z < near_plane_z) return Matrix4x4();

		Matrix4x4 result(MATRIX_INIT_WITH_ALL_0);

		// 1.计算视锥的底面半径
		double radius = Math::Tangent(fov_radius / 2);

		double x_halfrange, y_halfrange;

		// 2.计算包围视锥底面的最小相似矩形
		if (windowWidth == windowHeight) {
			x_halfrange = radius;
			y_halfrange = radius;
		}
		else if (windowWidth > windowHeight) {
			x_halfrange = radius * windowWidth / windowHeight;
			y_halfrange = radius;
		}
		else {
			x_halfrange = radius;
			y_halfrange = radius * windowHeight / windowWidth;
		}

		// 3. 填写矩阵
		result.values[0][0] = 1 / x_halfrange;
		result.values[1][1] = 1 / y_halfrange;
		result.values[2][2] = far_plane_z / (far_plane_z - near_plane_z);
		result.values[2][3] = -1 * near_plane_z * result.values[2][2];
		result.values[3][2] = 1;

		return result;
	}
}