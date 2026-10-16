/***
* @file /FernPack/basic_funcion/MathUtils.h
* 
* @brief 数学运算基础库
* 
* @author 半人马座beta星 (AIGC:DeepSeek)
* @date 2026-9-18
* 
* @details
* 这个模块里面是各种数学运算的库。
* 我希望自己实现算法，完成数学运算。从这个过程中我希望可以学习一些非常精妙、实用的算法构思与实现技巧
* 
* 今天是”九一八事变“九十五周年。以铜为镜可以正衣冠，以史为镜可以知兴衰。历史中的经验与教训会在未来的岔路口点亮一盏灯。
*/
#pragma once


namespace Fern::Math {


    //常量定义
	const double PIHALF = 1.57079632679489661923;          
	const double PI = 3.14159265358979323846;               
	const double TAU = 6.28318530717958647692;              


	/**
    * @brief 判断两数是否相等
    * @param double A
    * @param double B 待判断数
    * @param double pricision 判断精度
    * @returns 两数相等返回true
    */
	inline bool isNearlyEqual(double A, double B, double pricision = 1e-6) {
        return (A - B < pricision && B - A < pricision);
	}


    /**
    * @brief 快速平方根倒数
	* 灵感来源于偶然刷到的雷神之锤的浮点快速平方根倒数
	* 于是我叫 AI 生成了适配双精度版本的算法
    * @param double number 待处理数
    * @returns 通常是处理结果，但是传负数会出现-1
    */
	inline static double Q_rsqrt_double(double number) {
		if (number < 0) {
			return -1;
		}
		long long i;
		double x2, y;

		x2 = number * 0.5;
		y = number;
		i = *(long long*)&y;                    // 使用64位整数类型
		i = 0x5fe6eb50c7b537a9 - (i >> 1);      // double版的魔法数字
		y = *(double*)&i;                       // 转回双精度浮点数

		// double精度要求高，通常需要2-3次牛顿迭代
		y = y * (1.5 - (x2 * y * y));  // 第一次迭代
		y = y * (1.5 - (x2 * y * y));  // 第二次迭代
		y = y * (1.5 - (x2 * y * y));  // 第三次迭代

		return y;
	}

    /**
    * @brief 阶乘函数
    * @param int x_int 输入你要算谁的阶乘
    * @returns 算好了拿给你
    */
	inline double Factoriaal(int x_int) {
		switch (x_int) {
		default:    return 0;
		case 0:     return 1.0;
		case 1:     return 1.0;
		case 2:     return 2.0;
		case 3:     return 6.0;
		case 4:     return 24.0;
		case 5:     return 120.0;
		case 6:     return 720.0;
		case 7:     return 5040.0;
		case 8:     return 40320.0;
		case 9:     return 362880.0;
		case 10:    return 3628800.0;
		case 11:    return 39916800.0;
		case 12:    return 479001600.0;
		case 13:    return 6227020800.0;
		case 14:    return 87178291200.0;
		case 15:    return 1307674368000.0;
		case 16:    return 20922789888000.0;
		case 17:    return 355687428096000.0;
		case 18:    return 6402373705728000.0;
		case 19:    return 121645100408832000.0;
		case 20:    return 2432902008176640000.0;
		case 21:    return 51090942171709440000.0;
		case 22:    return 1124000727777607680000.0;
		case 23:    return 25852016738884976640000.0;
		case 24:    return 620448401733239439360000.0;
		}
	}

    /**
    * @brief 余弦函数，适配小角度。
    * @details
    * 函数进行余弦值具体计算。适配角度为[0, Fern::Math::PIHALF]
    * @param double x 弧度制角度
    * @param int terms = 6 迭代计算项数，默认6项精度已经足够。
    * @return double 参数x的余弦值
    */
    inline double CosineSmall(double x, int terms = 6) {
        double result = 1.0;  // n=0 项
        double term = 1.0;
        double x2 = x * x;

        for (int n = 1; n <= terms; n++) {
            // 递推：term = x^(2n) / (2n)!
            term *= x2;
            term /= (2 * n - 1) * (2 * n);  // 除以 (2n-1)*(2n)

            // (-1)^n
            if (n % 2 == 1) {
                result -= term;  // n=1,3,5... 负号
            }
            else {
                result += term;  // n=2,4,6... 正号
            }
        }

        return result;
    }

    /**
        * @brief 余弦函数，适配全实数。
        * @details
        * 函数只进行角度的规约。具体计算依然需要double CosineSmall(double, int)
        * @param double x
        * @param int terms = 6 迭代计算项数，默认6项精度已经足够。
        * @return double 参数x的余弦值
        */
    inline double Cosine(double x, int terms = 6) {
        // 1. 角度归约
        if (x < 0) x = -x;              // cos(-x) = cos(x)
        while (x > TAU) { x -= TAU; }   // cos(x + 2π) = cos(x)
        if (x > PI) { x = TAU - x; }   // cos(2π - x) = cos(x)
        if (x > PIHALF) { return -CosineSmall(PI - x, terms); }// cos(π - x) = -cos(x)
        return CosineSmall(x, terms);
    }

    

    /**
    * @brief 正弦函数，适配全实数。
    * @details
    * 与余弦函数不同，正弦函数的实现被浓缩在一个函数之中
    * @param double x 弧度制角度
    * @param int terms = 6 迭代计算项数，默认6项精度已经足够。
    * @return double 参数x的正弦值
    */
    inline double Sine(double x, int terms = 6) {
        // ========== 第一步：角度归约 ==========

        // 1. sin(-x) = -sin(x)
        bool negateResult = false;
        if (x < 0) {
            x = -x;
            negateResult = true;
        }

        // 2. 周期性：sin(x + 2π) = sin(x)
        while (x >= TAU) {
            x -= TAU;
        }

        // 3. 归约到 [0, π]
        // sin(π + x) = -sin(x)
        // sin(2π - x) = -sin(x)
        if (x > PI) {
            x = TAU - x;      // sin(2π - x) = -sin(x)
            negateResult = !negateResult;
        }

        // 4. 归约到 [0, π/2]
        // sin(π - x) = sin(x)
        if (x > PIHALF) {
            x = PI - x;
            // 符号不变（sin(π - x) = sin(x)）
        }

        // ========== 第二步：泰勒级数计算 ==========
        // sin(x) = x - x?/3! + x?/5! - x?/7! + ...

        double result = 0.0;
        double term = x;      // n=0 项
        double x2 = x * x;
        

        for (int n = 0; n < terms; n++) {
            if (n == 0) {
                result = x;   // 第一项
            }
            else {
                // 递推：term_n = term_{n-1} * (-x?) / ((2n) * (2n+1))
                term *= -x2;
                term /= (2 * n) * (2 * n + 1);
                result += term;
            }
        }

        // ========== 第三步：应用符号 ==========
        return negateResult ? -result : result;
    }

    /**
    * @brief 正切函数
    * @details
    * 实现依靠余弦和正弦函数。但是到2026/7/22系统性整理API为止，只有利用fov计算投影矩阵的时候会用到这个函数。
    * @note
    * 需要快速正切的时候，再实现快速正切。
    * @param double x 弧度制角度
    * @param int terms = 6 迭代项数，默认6项精度已经足够。
    * @return double 参数x的正切值
    */
    inline double Tangent(double x, int terms = 6) {
        return Sine(x, terms) / Cosine(x, terms);
    }

    /**
    * @brief 角度制角度转换成弧度
    * @param double deg 角度制下角度
    * @return double 弧度制下角度
    */
    inline double DegToRad(double deg) { return deg * PI / 180.0; }

    /**
    * @brief 弧度制角度转换成角度
    * @param double rad 弧度制下角度
    * @return double 角度制下角度
    */
    inline double RadToDeg(double rad) { return rad * 180.0 / PI; }

    /**
    * @brief 线性插值
    * @param double start 起始值
    * @param double end 结束值
    * @param double t 插值参数，通常在 [0, 1] 范围内
    * @return double 插值结果
    */
	inline double LiniarInterpolation(double start, double end, double t) {
		return start + (end - start) * t;
	}

    /**
    * @brief 解线性插值
    * @param double start 起始值
    * @param double end 结束值
    * @param double value 目标值
    * @return double 插值参数
    */
	inline double SolveLiniarInterpolation(double start, double end, double value) {
		if (end - start == 0) return start; // 避免除以零
		return (value - start) / (end - start);
	}
}