/**
 * @file Fern3DEngine/FernMath/MathUtils.h
 *
 * @brief 基础数学向量模块
 * @details
 * 在新版本的引擎中被直接复用。
 *
 *
 * @author 半人马座beta星
 * @date 2026/7/22
 */
#pragma once


namespace Fern {
    /**
    * @brief 空间向量
    * @details 
    * 之所以成员会有这种联合形式，是因为使用这种联合形式，可以保证在渲染管线中不出现语法错误的同时追溯一个向量此时应该被变换到哪一步。
    */
	class Vector3D {
    public:
        union {
            struct {
                double x;                   // x坐标
                double y;                   // y坐标
                double z;                   // z坐标
            };
            struct {
                double objectx;
                double objecty;
                double objectz;
            };
            struct {
                double worldx;
                double worldy;
                double worldz;
            };
            struct {
                double screenx;
                double screeny;
                double screendepth;
            };
        };

    public:
        Vector3D() :x(0), y(0), z(0) {}
        Vector3D(double x_input, double y_input, double z_input) :x(x_input), y(y_input), z(z_input) {}
        Vector3D(const Vector3D& P) :x(P.x), y(P.y), z(P.z) {}
        Vector3D(const Vector3D& startAt, const Vector3D endAt) :x(endAt.x - startAt.x), y(endAt.y - startAt.y), z(endAt.z - startAt.z) {}

        Vector3D operator= (const Vector3D& A)  {
            this->x = A.x;
            this->y = A.y;
            this->z = A.z;
            return *this;
        }

        inline bool operator==(const Vector3D& A) const {
            if (!Math::isNearlyEqual(this->x, A.x))return false;
            if (!Math::isNearlyEqual(this->y, A.y))return false;
            if (!Math::isNearlyEqual(this->z, A.z))return false;
            return true;
        }

        Vector3D operator+ (const Vector3D& A) const{
            Vector3D result(*this);
            result.x += A.x;
            result.y += A.y;
            result.z += A.z;
            return result;
        }

        Vector3D& operator+= (const Vector3D& A) {
            this->x += A.x;
            this->y += A.y;
            this->z += A.z;
            return *this;
        }

        Vector3D operator- (const Vector3D& A) const {
            Vector3D result(*this);
            result.x -= A.x;
            result.y -= A.y;
            result.z -= A.z;
            return result;
        }

        Vector3D& operator-= (const Vector3D& A) {
            this->x -= A.x;
            this->y -= A.y;
            this->z -= A.z;
            return *this;
        }

        Vector3D operator * (const double lambda) const {
            Vector3D result(*this);
            result.x *= lambda;
            result.y *= lambda;
            result.z *= lambda;
            return result;
        }

        Vector3D& operator *= (const double lambda) {
            this->x *= lambda;
            this->y *= lambda;
            this->z *= lambda;
            return *this;
        }


        double operator * (const Vector3D& A)const {
            return this->x * A.x + this->y * A.y + this->z * A.z;
        }

        /** 
        * @brief 向量叉乘
        */
        Vector3D operator%(const Vector3D A) const{
            Vector3D result;
            result.x = this->y * A.z - this->z * A.y;
            result.y = this->z * A.x - this->x * A.z;
            result.z = this->x * A.y - this->y * A.x;
            return result;
        }

        Vector3D& operator%=(const Vector3D A) {
            Vector3D result;
            result.x = this->y * A.z - this->z * A.y;
            result.y = this->z * A.x - this->x * A.z;
            result.z = this->x * A.y - this->y * A.x;
            *this = result;
            return *this;
        }

        inline double getLengthSquared() const {
            return (this->x * this->x + this->y * this->y + this->z * this->z);
        }

        inline double getLength() const {
            double i = this->x * this->x + this->y * this->y + this->z * this->z;
            double j = Math::Q_rsqrt_double(i);
            return i * j;
        }

        Vector3D& setPos(double x_input, double y_input, double z_input) {
            this->x = x_input;
            this->y = y_input;
            this->z = z_input;
            return *this;
        }

        Vector3D& setPos(const Vector3D& A) {
            this->x = A.x;
            this->y = A.y;
            this->z = A.z;
            return *this;
        }

        Vector3D& makeLengthUnit() {
            double length = this->getLengthSquared();
            double rsqrtLength = Math::Q_rsqrt_double(length);
            this->x *= rsqrtLength;
            this->y *= rsqrtLength;
            this->z *= rsqrtLength;
            return *this;
        }

        Vector3D makeLengthUnitCopy()const {
            double length = this->getLengthSquared();
            double rsqrtLength = Math::Q_rsqrt_double(length);
            Vector3D A(*this);
            A.x *= rsqrtLength;
            A.y *= rsqrtLength;
            A.z *= rsqrtLength;
            return A;
        }

        //未来考虑增加getter函数

        friend Vector3D operator*(double lambda, const Vector3D& A);
        friend std::ostream& operator<<(std::ostream& os, const Vector3D& v);


        
	};

    

    inline  Vector3D operator* (double lambda, const Vector3D& A) {
        return A * lambda;
    }

    /**
    * @brief 向控制台输出
    * @warning 使用了外部依赖。需要考虑废弃。
    */
    inline std::ostream& operator<<(std::ostream& os, const Vector3D& v) {
        os << "[" << v.x << ", " << v.y << ", " << v.z << "]";
        return os;
    }


    /**
    * @brief 静态工厂方法
    */

    //按轴分

    inline  Vector3D xPositive() { return Vector3D(1, 0, 0); }
    inline  Vector3D yPositive() { return Vector3D(0, 1, 0); }
    inline  Vector3D zPositive() { return Vector3D(0, 0, 1); }
    inline  Vector3D xNegative() { return Vector3D(-1, 0, 0); }
    inline  Vector3D yNegative() { return Vector3D(0, -1, 0); }
    inline  Vector3D zNegative() { return Vector3D(0, 0, -1); }


    //按方向分

    inline  Vector3D East() { return Vector3D(1, 0, 0); }
    inline  Vector3D South() { return Vector3D(0, 0, 1); }
    inline  Vector3D West() { return Vector3D(-1, 0, 0); }
    inline  Vector3D North() { return Vector3D(0, 0, -1); }
    inline  Vector3D Up() { return Vector3D(0, 1, 0); }
    inline  Vector3D Down() { return Vector3D(0, -1, 0); }

}