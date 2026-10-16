/**
 * @file Fern3DEngine/FernMath/Matrix4x4.h
 *
 * @brief 基础数学齐次矩阵模块
 * @details
 * 该模块利用齐次矩阵执行对向量的变换。
 *
 *
 * @author 半人马座beta星
 * @date 2026/7/22
 */

#pragma once

#define MATRIX_INIT_WITH_UNIT_MATRIX	1
#define MATRIX_INIT_WITH_ALL_0			2
#define MATRIX_INIT_WITH_ALL_1			3
#define MATRIX_INIT_WITH_TEST			4

namespace Fern {
	class Matrix4x4 {
	public:
		double values[4][4];		// values[行号][列标]

	public:
		Matrix4x4(int matrixInitSettings = MATRIX_INIT_WITH_UNIT_MATRIX) {
			switch (matrixInitSettings) {
			default:							
			case MATRIX_INIT_WITH_UNIT_MATRIX:  for (int row = 0;row <= 3;row++) { for (int col = 0;col <= 3;col++) { values[row][col] = (row == col ? 1.0 : 0.0); } }; break;
			case MATRIX_INIT_WITH_ALL_0:		for (int row = 0;row <= 3;row++) { for (int col = 0;col <= 3;col++) { values[row][col] = 0.0; } }; break;
			case MATRIX_INIT_WITH_ALL_1:		for (int row = 0;row <= 3;row++) { for (int col = 0;col <= 3;col++) { values[row][col] = 1.0; } }; break;
			case MATRIX_INIT_WITH_TEST:			for (int row = 0;row <= 3;row++) { for (int col = 0;col <= 3;col++) { values[row][col] = 1.0 * (4 * row + col); } }; break;
			}
		}

		Matrix4x4(const double data[4][4]) {
			for (int i = 0; i < 4; i++)
				for (int j = 0; j < 4; j++)
					values[i][j] = data[i][j];
		}

		Matrix4x4(const Matrix4x4& A) {
			for (int i = 0; i < 4; i++)
				for (int j = 0; j < 4; j++)
					values[i][j] = A.values[i][j];
		}

		Matrix4x4 operator=(const Matrix4x4& A) {
			for (int i = 0; i < 4; i++)
				for (int j = 0; j < 4; j++)
					this->values[i][j] = A.values[i][j];
			return *this;
		}

		Matrix4x4 operator-(const Matrix4x4& A)const {
			Matrix4x4 result;
			for (int row = 0;row <= 3;row++) {
				for (int col = 0;col <= 3;col++) {
					result.values[row][col] = this->values[row][col] - A.values[row][col];
				}
			}
			return result;
		}

		Matrix4x4 operator+(const Matrix4x4& A)const {
			Matrix4x4 result;
			for (int row = 0;row <= 3;row++) {
				for (int col = 0;col <= 3;col++) {
					result.values[row][col] = this->values[row][col] + A.values[row][col];
				}
			}
			return result;
		}

		Matrix4x4 operator+=(const Matrix4x4& A) {
			for (int row = 0;row <= 3;row++) {
				for (int col = 0;col <= 3;col++) {
					this->values[row][col] += A.values[row][col];
				}
			}
			return *this;
		}

		Matrix4x4 operator-=(const Matrix4x4& A) {
			for (int row = 0;row <= 3;row++) {
				for (int col = 0;col <= 3;col++) {
					this->values[row][col] -= A.values[row][col];
				}
			}
			return *this;
		}
		
		/**
		* @brief 左乘
		* @details
		* 重载 Matrix4x4 operator*(const Matrix4x4 A)方法为*this左乘A
		* @param const:Matrix4x4& A 算符左矩阵
		* @return Matrix4x4
		*/
		Matrix4x4 operator*(const Matrix4x4& A)const {
			Matrix4x4 result(MATRIX_INIT_WITH_ALL_0);
			for (int row = 0;row <= 3;row++) {
				for (int col = 0;col <= 3;col++) {
					for (int _ = 0;_ <= 3;_++) {
						//std::ostringstream oss;
						result.values[row][col] += A.values[row][_] * this->values[_][col];
						//oss << "计算过程 （" << row << "," << col << "," << _ << "):\n" << result.toString();
						//LOG_APPEND(oss.str());
					}
				}
			}
			return result;
		}

		/**
		* @brief 左乘并赋值
		* @details
		* 重载 Matrix4x4 operator*=(const Matrix4x4 A)方法为*this左乘A并且存储在*this中
		* @param const:Matrix4x4& A 算符左矩阵
		* @return Matrix4x4 *this
		*/
		Matrix4x4 operator*=(const Matrix4x4& A) {
			Matrix4x4 result(MATRIX_INIT_WITH_ALL_0);
			for (int row = 0;row <= 3;row++) {
				for (int col = 0;col <= 3;col++) {
					for (int _ = 0;_ <= 3;_++) {
						//std::ostringstream oss;
						result.values[row][col] += A.values[row][_] * this->values[_][col];
						//oss << "计算过程 （" << row << "," << col << "," << _ << "):\n" << result.toString();
						//LOG_APPEND(oss.str());
					}
				}
			}
			for (int i = 0; i < 4; i++)
				for (int j = 0; j < 4; j++)
					this->values[i][j] = result.values[i][j];
			return *this;
		}

		/**
		* @brief 右乘
		* @details
		* 重载 Matrix4x4 operator*(const Matrix4x4 A)方法为*this右乘A。
		* 数学运算已经被核验正确
		* @param const:Matrix4x4& A 算符右矩阵
		* @return Matrix4x4
		*/
		Matrix4x4 operator^(const Matrix4x4& A)const {//右乘
			Matrix4x4 result(MATRIX_INIT_WITH_ALL_0);
			for (int row = 0;row <= 3;row++) {
				for (int col = 0;col <= 3;col++) {
					for (int _ = 0;_ <= 3;_++) {
						//std::ostringstream oss;
						result.values[row][col] += this->values[row][_] * A.values[_][col];
						//oss << "计算过程 （" << row << "," << col << "," << _ << "):\n" << result.toString();
						//LOG_APPEND(oss.str());
					}
				}
			}
			return result;
		}


		/**
		* @brief 将向量应用矩阵变换到新的向量，变换空间点
		* @param const:Vector3D& v 待变换空间点
		* @return Vector3D 变换结果
		*/
		Vector3D operator*(const Vector3D& v) const {
			double w = values[3][0] * v.x + values[3][1] * v.y + values[3][2] * v.z + values[3][3];
			
			if (Math::isNearlyEqual(w, 0.0)) {
				return Vector3D(
					114,
					514,
					1919
				);
			}
			
			return Vector3D(
				(values[0][0] * v.x + values[0][1] * v.y + values[0][2] * v.z + values[0][3]) / w,
				(values[1][0] * v.x + values[1][1] * v.y + values[1][2] * v.z + values[1][3]) / w,
				(values[2][0] * v.x + values[2][1] * v.y + values[2][2] * v.z + values[2][3]) / w
			);
		}

		/**
		* @brief  将矩阵变换应用于向量得到新的向量，变换空间方向。没有内置矩阵求逆和转置便于批量处理法向量。
		* @param const:Vector3D& v 待变换法向量
		* @return Vector3D 变换结果
		*/
		Vector3D operator^ (const Vector3D& v) const {
			return Vector3D(
				(values[0][0] * v.x + values[0][1] * v.y + values[0][2] * v.z ) ,
				(values[1][0] * v.x + values[1][1] * v.y + values[1][2] * v.z ) ,
				(values[2][0] * v.x + values[2][1] * v.y + values[2][2] * v.z ) 
			);
		}

		/**
		* @brief 仿射矩阵求逆
		* @return Vector3D 变换结果
		*/
		inline Matrix4x4 operator~()const {
			// 0. 先判定最后一行是不是[0, 0, 0, 1]。如果不是，需要接入后续的完整求逆逻辑。此处着重仿射矩阵求逆。
			if (!Math::isNearlyEqual(this->values[3][0], 0.0))return Matrix4x4();
			if (!Math::isNearlyEqual(this->values[3][1], 0.0))return Matrix4x4();
			if (!Math::isNearlyEqual(this->values[3][2], 0.0))return Matrix4x4();
			if (!Math::isNearlyEqual(this->values[3][3], 1.0))return Matrix4x4();

			// 1. 提取 3×3 线性部分
			double a00 = values[0][0], a01 = values[0][1], a02 = values[0][2];
			double a10 = values[1][0], a11 = values[1][1], a12 = values[1][2];
			double a20 = values[2][0], a21 = values[2][1], a22 = values[2][2];

			// 2. 计算 3×3 行列式
			double det = a00 * (a11 * a22 - a12 * a21)
				- a01 * (a10 * a22 - a12 * a20)
				+ a02 * (a10 * a21 - a11 * a20);

			if (Math::isNearlyEqual(det, 0.0, 1e-10)) {
				// 矩阵不可逆，返回单位矩阵或抛出异常
				return Matrix4x4();  // 单位矩阵
			}

			double invDet = 1.0 / det;

			// 3. 计算 3×3 部分的逆（伴随矩阵 / det）
			Matrix4x4 result(MATRIX_INIT_WITH_ALL_0);

			result.values[0][0] = (a11 * a22 - a12 * a21) * invDet;
			result.values[0][1] = (a02 * a21 - a01 * a22) * invDet;
			result.values[0][2] = (a01 * a12 - a02 * a11) * invDet;

			result.values[1][0] = (a12 * a20 - a10 * a22) * invDet;
			result.values[1][1] = (a00 * a22 - a02 * a20) * invDet;
			result.values[1][2] = (a02 * a10 - a00 * a12) * invDet;

			result.values[2][0] = (a10 * a21 - a11 * a20) * invDet;
			result.values[2][1] = (a01 * a20 - a00 * a21) * invDet;
			result.values[2][2] = (a00 * a11 - a01 * a10) * invDet;

			// 4. 计算平移部分的逆：-R?? · t
			double tx = values[0][3];
			double ty = values[1][3];
			double tz = values[2][3];

			result.values[0][3] = -(result.values[0][0] * tx + result.values[0][1] * ty + result.values[0][2] * tz);
			result.values[1][3] = -(result.values[1][0] * tx + result.values[1][1] * ty + result.values[1][2] * tz);
			result.values[2][3] = -(result.values[2][0] * tx + result.values[2][1] * ty + result.values[2][2] * tz);

			// 5. 最后一行保持 (0, 0, 0, 1)
			result.values[3][3] = 1.0;

			return result;
		}

		/**
		* @brief 矩阵转置
		* @return static Matrix4x4
		*/
		inline  Matrix4x4 operator!() {
			Matrix4x4 result;
			for (int i = 0;i <= 3;i++) {
				for (int j = 0;j <= 3;j++) {
					result.values[i][j] = this->values[j][i];
				}
			}
			return result;
		}

		// 静态工厂方法
		/**
		* @brief 单位矩阵
		* @return static Matrix4x4
		*/
		inline static Matrix4x4 Identity() {
			return Matrix4x4();
		}

		/**
		* @brief 平移变换矩阵
		* @param const:double& tx 平移x分量
		* @param const:double& ty 平移y分量
		* @param const:double& tz 平移z分量
		* @return static Matrix4x4 
		*/
		inline static Matrix4x4 Translation(const double& tx, const double& ty, const double& tz) {
			Matrix4x4 result;
			result.values[0][3] = tx;
			result.values[1][3] = ty;
			result.values[2][3] = tz;
			return result;
		}

		/**
		* @brief 按向量平移变换矩阵
		* @param const:Vector3D& v 平移向量
		* @return static Matrix4x4
		*/
		inline static Matrix4x4 Translation(const Vector3D& v) {
			Matrix4x4 result;
			result.values[0][3] = v.x;
			result.values[1][3] = v.y;
			result.values[2][3] = v.z;
			return result;
		}

		/**
		* @brief 缩放变换矩阵
		* @param const:double& sx 缩放x轴比例
		* @param const:double& sy 缩放y轴比例
		* @param const:double& sz 缩放z轴比例
		* @return static Matrix4x4
		*/
		inline static Matrix4x4 Scale(const double& sx, const double& sy, const double& sz) {
			Matrix4x4 result;
			result.values[0][0] = sx;
			result.values[1][1] = sy;
			result.values[2][2] = sz;
			return result;
		}

		/**
		* @brief 缩放变换矩阵
		* @param const:Vector3D& v 缩放三轴比例向量
		* @return static Matrix4x4
		*/
		inline static Matrix4x4 Scale(const Vector3D& v) {
			Matrix4x4 result;
			result.values[0][0] = v.x;
			result.values[1][1] = v.y;
			result.values[2][2] = v.z;
			return result;
		}

		/**
		* @brief 绕x轴正方向逆时针旋转变换矩阵
		* @param const:double& angle 旋转角度
		* @return static Matrix4x4
		*/
		inline static Matrix4x4 RotationX(const double& angle) {
			double c = Fern::Math::Cosine(angle,8), s = Fern::Math::Sine(angle,8);
			Matrix4x4 result;
			result.values[1][1] = c; result.values[1][2] = -s;
			result.values[2][1] = s; result.values[2][2] = c;
			return result;
		}

		/**
		* @brief 绕y轴正方向逆时针旋转变换矩阵
		* @param const:double& angle 旋转角度
		* @return static Matrix4x4
		*/
		inline static Matrix4x4 RotationY(const double& angle) {
			double c = Fern::Math::Cosine(angle, 8), s = Fern::Math::Sine(angle, 8);
			Matrix4x4 result;
			result.values[0][0] = c; result.values[0][2] = s;
			result.values[2][0] = -s; result.values[2][2] = c;
			return result;
		}

		/**
		* @brief 绕z轴正方向逆时针旋转变换矩阵
		* @param const:double& angle 旋转角度
		* @return static Matrix4x4
		*/
		inline static Matrix4x4 RotationZ(const double& angle) {
			double c = Fern::Math::Cosine(angle, 8), s = Fern::Math::Sine(angle, 8);
			Matrix4x4 result;
			result.values[0][0] = c; result.values[0][1] = -s;
			result.values[1][0] = s; result.values[1][1] = c;
			return result;
		}

		/**
		* @brief 指定旋转基点绕X旋转
		* @param const:double& angle 旋转角度
		* @param const:Fern::Vector3D& origin 旋转基点
		* @return static Matrix4x4
		*/
		inline static Matrix4x4 RotationAroundOriginX(const double& angle,const Fern::Vector3D& origin) {
			double c = Fern::Math::Cosine(angle, 8), s = Fern::Math::Sine(angle, 8);
			Matrix4x4 result;
			result.values[0][0] = c; result.values[0][1] = -s;
			result.values[1][0] = s; result.values[1][1] = c;

			result = result ^ Translation(-1 * origin);
			result = result * Translation(origin);

			return result;
		}

		/**
		* @brief 指定旋转基点绕Y旋转
		* @param const:double& angle 旋转角度
		* @param const:Fern::Vector3D& origin 旋转基点
		* @return static Matrix4x4
		*/
		inline static Matrix4x4 RotationAroundOriginY(const double& angle, const Fern::Vector3D& origin) {
			double c = Fern::Math::Cosine(angle, 8), s = Fern::Math::Sine(angle, 8);
			Matrix4x4 result;
			result.values[0][0] = c; result.values[0][2] = s;
			result.values[2][0] = -s; result.values[2][2] = c;

			result = result ^ Translation(-1 * origin);
			result = result * Translation(origin);

			return result;
		}

		/**
		* @brief 指定旋转基点绕Z旋转
		* @param const:double& angle 旋转角度
		* @param const:Fern::Vector3D& origin 旋转基点
		* @return static Matrix4x4
		*/
		inline static Matrix4x4 RotationAroundOriginZ(const double& angle, const Fern::Vector3D& origin) {
			double c = Fern::Math::Cosine(angle, 8), s = Fern::Math::Sine(angle, 8);
			Matrix4x4 result;
			result.values[0][0] = c; result.values[0][1] = -s;
			result.values[1][0] = s; result.values[1][1] = c;

			result = result ^ Translation(-1 * origin);
			result = result * Translation(origin);

			return result;
		}

		/**
		* @brief 旋转矩阵的生成。欧拉角顺序为ZYX。
		* @details 此函数由AI生成，为测试旋转时所用函数。实际上只是将旋转过程做了内联。我认为这个函数是不必要的。
		* @param const Vector3D& eulerAngles 弧度制角度
		* @return 合成好的旋转矩阵
		*/
		static Matrix4x4 Rotation(const Vector3D& eulerAngles) {
			// 按照 Z → Y → X 的顺序旋转（可根据需要调整）
			return RotationZ(eulerAngles.z)
				* RotationY(eulerAngles.y)
				* RotationX(eulerAngles.x);
		}

		/**
		* @brief 流式输出
		* @warning 没有 #include <iostream> 时可能会导致编译不通过。如果需要彻底取消流式输出请注意。
		* @param std::ostream& os 输出流，可以是控制台。
		* @param const:Fern::Matrix4x4& mat 待输出矩阵
		* @return std::ostream std::ostream os 以便支持链式调用。
		*/
		friend std::ostream& operator<<(std::ostream& os, const Matrix4x4& mat) {
			for (int i = 0; i < 4; i++) {
				for (int j = 0; j < 4; j++) {
					os << mat.values[i][j] << " ";
				}
				os << "\n";
			}
			return os;
		}

		/**
		* @brief 向字符串转换
		* @warning 没有 #include <ostringstream> 时可能会导致编译不通过。如果需要彻底取消流式输出请注意。
		* @return std::string 输出转换结果
		*/
		std::string toString() const {
			std::ostringstream oss;
			for (int i = 0; i < 4; ++i) {
				oss << '|';
				for (int j = 0; j < 4; ++j) {
					oss << this->values[i][j] ;
					if (j != 4)oss << "\t";
				}
				oss << "|\n";
			}
			oss << std::flush;
			return oss.str();
		}
		

	};

	
}