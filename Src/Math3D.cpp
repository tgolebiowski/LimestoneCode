#ifndef MATH3D_CPP
#define MATH3D_CPP
#include <math.h>

struct Vec3 {
	float x, y, z;
};

struct Quaternion {
	float w, x, y, z;
};

struct Matrix4 {
	float m[4][4];
};

/*-----------------------------------------------------------------------------------
                                     Vec3
------------------------------------------------------------------------------------*/

inline Vec3 operator * ( const Vec3 v, const float scalar) {
	return {v.x * scalar, v.y * scalar, v.z * scalar};
}

inline Vec3 operator * ( const float scalar, const Vec3 v) {
	return {v.x * scalar, v.y * scalar, v.z * scalar};
}

inline Vec3 operator + ( const Vec3 v1, const Vec3 v2 ) {
	return {v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
}

Vec3 DiffVec(const Vec3 a, const Vec3 b) {
	Vec3 diff;
	diff.x = b.x - a.x;
	diff.y = b.y - a.y;
	diff.z = b.z - a.z;
	return diff;
}

float VecLength(const Vec3* vec) {
	return sqrtf( vec->x * vec->x + vec->y * vec->y + vec->z * vec->z );
}

float Dot(const Vec3 u, const Vec3 v) {
	return u.x * v.x + u.y * v.y + u.z * v.z;
}

Vec3 CrossProd(const Vec3 u, const Vec3 v) {
	Vec3 crossProdVec;
	crossProdVec.x = u.y * v.z - u.z * v.y;
	crossProdVec.y = u.z * v.x - u.x * v.z;
	crossProdVec.z = u.x * v.y - u.y * v.x;
	return crossProdVec;
}

void Normalize(Vec3* v) {
	float len = sqrtf( v->x * v->x + v->y * v->y + v->z * v->z );
	if(len > 1e-08 ) {
		float invLen = 1.0 / len;
		v->x *= invLen;
		v->y *= invLen;
		v->z *= invLen;
	}
}

float AngleBetween(const Vec3 v1, const Vec3 v2) {
	float lenProduct = VecLength(&v1) * VecLength(&v2);
	if(lenProduct < 1e-6f)
		lenProduct = 1e-6f;

	float f = Dot(v1, v2) / lenProduct;
	if ( f > 1.0f ) f = 1.0f;
	else if ( f < -1.0f ) f = -1.0f;
	return acosf(f);
}

/* ----------------------------------------------------------------------------------
                                  MATRICIES
-----------------------------------------------------------------------------------*/
void Identity(Matrix4* m) {
	memset( &m->m[0][0], 0, 16 * sizeof(float) );
	m->m[0][0] = 1.0f;
	m->m[1][1] = 1.0f;
	m->m[2][2] = 1.0f;
	m->m[3][3] = 1.0f;
}

void SetScale(Matrix4* mat4, const float x, const float y, const float z) {
	memset( &mat4->m, 0, 16 * sizeof(float) );
	mat4->m[0][0] = x;
	mat4->m[1][1] = y;
	mat4->m[2][2] = z;
	mat4->m[3][3] = 1.0f;
}

void SetTranslation(Matrix4* mat4, const float x, const float y, const float z) {
	memset( &mat4->m[0][0], 0, 16 * sizeof(float) );
	mat4->m[0][0] = 1.0f;
	mat4->m[1][1] = 1.0f;
	mat4->m[2][2] = 1.0f;
	mat4->m[3][3] = 1.0f;
	mat4->m[3][0] = x;
	mat4->m[3][1] = y;
	mat4->m[3][2] = z;
}

void SetRotation( Matrix4* mtx, const float x, const float y, const float z, const float angle) {
	memset( &mtx->m[0][0], 0, 16 * sizeof(float) );
	float c = cos( angle );
	float s = sin( angle );
	float t = 1.0f - c;
	mtx->m[0][0] = ( t * x * x ) + c; mtx->m[0][1] = ( x * y * t ) + ( z * s ); mtx->m[0][2] = ( x * z * t ) - ( y * s );
	mtx->m[1][0] = ( x * y * t ) - ( z * s ); mtx->m[1][1] = ( t * y * y ) + c; mtx->m[1][2] = ( t * y * z ) + ( x * s );
	mtx->m[2][0] = ( x * z * t ) + ( y * s ); mtx->m[2][1] = ( t * y * z ) - ( x * s ); mtx->m[2][2] = ( t * z * z ) + c;
	mtx->m[3][3] = 1;
}

void SetOrthoProjection(Matrix4* m, const float l, const float r, const float t, const float b, const float n, const float f) {
	m->m[0][0] = 2.0f / (r - l);
	m->m[0][1] = 0.0f;
	m->m[0][2] = 0.0f;
	m->m[0][3] = 0.0f;

	m->m[1][0] = 0.0f;
	m->m[1][1] = 2.0f / (t - b);
	m->m[1][2] = 0.0f;
	m->m[1][3] = 0.0f;

	m->m[2][0] = 0.0f;
	m->m[2][1] = 0.0f;
	m->m[2][2] = -2.0f / (f - n);
	m->m[2][3] = 0.0f;

	m->m[3][0] = -(r + l) / (r - l);
	m->m[3][1] = -(t + b) / (t - b);
	m->m[3][2] = -(f + n) / (f - n);
	m->m[3][3] = 1.0f;
}

Matrix4 MultMatrix(const Matrix4 m, const Matrix4 m2) {
	Matrix4 r;
	r.m[0][0] = m.m[0][0] * m2.m[0][0] + m.m[0][1] * m2.m[1][0] + m.m[0][2] * m2.m[2][0] + m.m[0][3] * m2.m[3][0];
	r.m[0][1] = m.m[0][0] * m2.m[0][1] + m.m[0][1] * m2.m[1][1] + m.m[0][2] * m2.m[2][1] + m.m[0][3] * m2.m[3][1];
	r.m[0][2] = m.m[0][0] * m2.m[0][2] + m.m[0][1] * m2.m[1][2] + m.m[0][2] * m2.m[2][2] + m.m[0][3] * m2.m[3][2];
	r.m[0][3] = m.m[0][0] * m2.m[0][3] + m.m[0][1] * m2.m[1][3] + m.m[0][2] * m2.m[2][3] + m.m[0][3] * m2.m[3][3];

	r.m[1][0] = m.m[1][0] * m2.m[0][0] + m.m[1][1] * m2.m[1][0] + m.m[1][2] * m2.m[2][0] + m.m[1][3] * m2.m[3][0];
	r.m[1][1] = m.m[1][0] * m2.m[0][1] + m.m[1][1] * m2.m[1][1] + m.m[1][2] * m2.m[2][1] + m.m[1][3] * m2.m[3][1];
	r.m[1][2] = m.m[1][0] * m2.m[0][2] + m.m[1][1] * m2.m[1][2] + m.m[1][2] * m2.m[2][2] + m.m[1][3] * m2.m[3][2];
	r.m[1][3] = m.m[1][0] * m2.m[0][3] + m.m[1][1] * m2.m[1][3] + m.m[1][2] * m2.m[2][3] + m.m[1][3] * m2.m[3][3];

	r.m[2][0] = m.m[2][0] * m2.m[0][0] + m.m[2][1] * m2.m[1][0] + m.m[2][2] * m2.m[2][0] + m.m[2][3] * m2.m[3][0];
	r.m[2][1] = m.m[2][0] * m2.m[0][1] + m.m[2][1] * m2.m[1][1] + m.m[2][2] * m2.m[2][1] + m.m[2][3] * m2.m[3][1];
	r.m[2][2] = m.m[2][0] * m2.m[0][2] + m.m[2][1] * m2.m[1][2] + m.m[2][2] * m2.m[2][2] + m.m[2][3] * m2.m[3][2];
	r.m[2][3] = m.m[2][0] * m2.m[0][3] + m.m[2][1] * m2.m[1][3] + m.m[2][2] * m2.m[2][3] + m.m[2][3] * m2.m[3][3];

	r.m[3][0] = m.m[3][0] * m2.m[0][0] + m.m[3][1] * m2.m[1][0] + m.m[3][2] * m2.m[2][0] + m.m[3][3] * m2.m[3][0];
	r.m[3][1] = m.m[3][0] * m2.m[0][1] + m.m[3][1] * m2.m[1][1] + m.m[3][2] * m2.m[2][1] + m.m[3][3] * m2.m[3][1];
	r.m[3][2] = m.m[3][0] * m2.m[0][2] + m.m[3][1] * m2.m[1][2] + m.m[3][2] * m2.m[2][2] + m.m[3][3] * m2.m[3][2];
	r.m[3][3] = m.m[3][0] * m2.m[0][3] + m.m[3][1] * m2.m[1][3] + m.m[3][2] * m2.m[2][3] + m.m[3][3] * m2.m[3][3];

	return r;
}

inline Matrix4 operator * ( const Matrix4 m1, const Matrix4 m2 ) {
	return MultMatrix(m1, m2);
}

/*------------------------------------------------------------------------------------
	                                QUATERNIONS
-------------------------------------------------------------------------------------*/
Quaternion MultQuats (const Quaternion lQ, const Quaternion rQ) {
	Quaternion quat;
	quat.w = lQ.w * rQ.w - lQ.x * rQ.x - lQ.y * rQ.y - lQ.z * rQ.z;
	quat.x = lQ.w * rQ.x + lQ.x * rQ.w + lQ.y * rQ.z - lQ.z * rQ.y;
	quat.y = lQ.w * rQ.y + lQ.y * rQ.w + lQ.z * rQ.x - lQ.x * rQ.z;
	quat.z = lQ.w * rQ.z + lQ.z * rQ.w + lQ.x * rQ.y - lQ.y * rQ.x;
	return quat;
}

Quaternion FromAngleAxis(const float axisX, const float axisY, const float axisZ, const float angle) {
	Quaternion quat;
	float halfAngle = ( 0.5 * angle );
	float fSin = sin(halfAngle);
	quat.w = cos(halfAngle);
	quat.x = fSin * axisX;
	quat.y = fSin * axisY;
	quat.z = fSin * axisZ;
	return quat;
}

void Inverse(Quaternion* quat) {
	float fNorm = quat->w * quat->w + quat->x * quat->x + quat->y * quat->y + quat->z * quat->z;
	float fInvNorm = 1.0f/fNorm;
	quat->w *= fInvNorm;
	quat->x *= ( -1.0f * fInvNorm );
	quat->y *= ( -1.0f * fInvNorm );
	quat->z *= ( -1.0f * fInvNorm );
}

Vec3 ApplyTo( Quaternion q, Vec3 v ) {
	Vec3 t = CrossProd( {q.x, q.y, q.z }, v ) * 2.0f;
	return v + t * q.w + CrossProd( {q.x, q.y, q.z}, t );
}

Matrix4 MatrixFromQuat(const Quaternion quat) {
	Matrix4 matx;

	// 	float qw = quat.w;
	// float qx = quat.x;
	// float qy = quat.y;
	// float qz = quat.z;
	// float n = 1.0f / sqrt( qw * qw + qx * qx + qy * qy + qz * qz );

	// matx.m[0][0] = 1.0f - 2.0f*qy*qy - 2.0f*qz*qz;

	float fTx  = 2.0*quat.x;
	float fTy  = 2.0*quat.y;
	float fTz  = 2.0*quat.z;
	float fTwx = fTx*quat.w;
	float fTwy = fTy*quat.w;
	float fTwz = fTz*quat.w;
	float fTxx = fTx*quat.x;
	float fTxy = fTy*quat.x;
	float fTxz = fTz*quat.x;
	float fTyy = fTy*quat.y;
	float fTyz = fTz*quat.y;
	float fTzz = fTz*quat.z;

	matx.m[0][0] = 1.0-(fTyy+fTzz);
	matx.m[1][0] = fTxy-fTwz;
	matx.m[2][0] = fTxz+fTwy;
	matx.m[3][0] = 0.0f;
	matx.m[0][1] = fTxy+fTwz;
	matx.m[1][1] = 1.0-(fTxx+fTzz);
	matx.m[2][1] = fTyz-fTwx;
	matx.m[3][1] = 0.0f;
	matx.m[0][2] = fTxz-fTwy;
	matx.m[1][2] = fTyz+fTwx;
	matx.m[2][2] = 1.0-(fTxx+fTyy);
	matx.m[3][2] = 0.0f;
	matx.m[0][3] = 0.0f;
	matx.m[1][3] = 0.0f;
	matx.m[2][3] = 0.0f;
	matx.m[3][3] = 1.0f;

	return matx;
}
#endif