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

float InvSqrt(float x) {
	float xhalf = 0.5f * x;
	int i = *(int*)&x; // store Numbering-point bits in integer
	i = 0x5f3759d5 - (i >> 1); // initial guess for Newton's method
	x = *(float*)&i; // convert new bits into Number
	x = x*(1.5f - xhalf*x*x); // One round of Newton's method
	return x;
}

/*-----------------------------------------------------------------------------------
                                     
------------------------------------------------------------------------------------*/

Vec3 operator * ( const Vec3 v, const float scalar) {
	return {v.x * scalar, v.y * scalar, v.z * scalar};
}

Vec3 operator * ( const float scalar, const Vec3 v) {
	return {v.x * scalar, v.y * scalar, v.z * scalar};
}

Vec3 operator + ( const Vec3 v1, const Vec3 v2 ) {
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

float AngleBetween( Vec3 v1, Vec3 v2) {
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

Matrix4 InverseMatrix( Matrix4 m ) {
	float m00 = m.m[0][0], m01 = m.m[0][1], m02 = m.m[0][2], m03 = m.m[0][3];
	float m10 = m.m[1][0], m11 = m.m[1][1], m12 = m.m[1][2], m13 = m.m[1][3];
	float m20 = m.m[2][0], m21 = m.m[2][1], m22 = m.m[2][2], m23 = m.m[2][3];
	float m30 = m.m[3][0], m31 = m.m[3][1], m32 = m.m[3][2], m33 = m.m[3][3];

	float v0 = m20 * m31 - m21 * m30;
	float v1 = m20 * m32 - m22 * m30;
	float v2 = m20 * m33 - m23 * m30;
	float v3 = m21 * m32 - m22 * m31;
	float v4 = m21 * m33 - m23 * m31;
	float v5 = m22 * m33 - m23 * m32;

	float t00 = + (v5 * m11 - v4 * m12 + v3 * m13);
	float t10 = - (v5 * m10 - v2 * m12 + v1 * m13);
	float t20 = + (v4 * m10 - v2 * m11 + v0 * m13);
	float t30 = - (v3 * m10 - v1 * m11 + v0 * m12);

	float invDet = 1 / (t00 * m00 + t10 * m01 + t20 * m02 + t30 * m03);

	float d00 = t00 * invDet;
	float d10 = t10 * invDet;
	float d20 = t20 * invDet;
	float d30 = t30 * invDet;

	float d01 = - (v5 * m01 - v4 * m02 + v3 * m03) * invDet;
	float d11 = + (v5 * m00 - v2 * m02 + v1 * m03) * invDet;
	float d21 = - (v4 * m00 - v2 * m01 + v0 * m03) * invDet;
	float d31 = + (v3 * m00 - v1 * m01 + v0 * m02) * invDet;

	v0 = m10 * m31 - m11 * m30;
	v1 = m10 * m32 - m12 * m30;
	v2 = m10 * m33 - m13 * m30;
	v3 = m11 * m32 - m12 * m31;
	v4 = m11 * m33 - m13 * m31;
	v5 = m12 * m33 - m13 * m32;

	float d02 = + (v5 * m01 - v4 * m02 + v3 * m03) * invDet;
	float d12 = - (v5 * m00 - v2 * m02 + v1 * m03) * invDet;
	float d22 = + (v4 * m00 - v2 * m01 + v0 * m03) * invDet;
	float d32 = - (v3 * m00 - v1 * m01 + v0 * m02) * invDet;

	v0 = m21 * m10 - m20 * m11;
	v1 = m22 * m10 - m20 * m12;
	v2 = m23 * m10 - m20 * m13;
	v3 = m22 * m11 - m21 * m12;
	v4 = m23 * m11 - m21 * m13;
	v5 = m23 * m12 - m22 * m13;

	float d03 = - (v5 * m01 - v4 * m02 + v3 * m03) * invDet;
	float d13 = + (v5 * m00 - v2 * m02 + v1 * m03) * invDet;
	float d23 = - (v4 * m00 - v2 * m01 + v0 * m03) * invDet;
	float d33 = + (v3 * m00 - v1 * m01 + v0 * m02) * invDet;

	Matrix4 retVal;
	float data[16] = { d00, d01, d02, d03,
		               d10, d11, d12, d13,
		               d20, d21, d22, d23,
		               d30, d31, d32, d33 };
    memcpy( &retVal.m[0], &data[0], sizeof(float) * 16 );
    return retVal;
}

Matrix4 Transpose( Matrix4 m ) {
	Matrix4 t;

	for( int i = 0; i < 4; i++ ) {
		for( int j = 0; j < 4; j++ ) {
			t.m[i][j] = m.m[j][i];
		}
	}

	return t;
}

// void SetOrthoProjection(Matrix4* m, const float l, const float r, const float t, const float b, const float n, const float f) {
// 	m->m[0][0] = 2.0f / (r - l);
// 	m->m[0][1] = 0.0f;
// 	m->m[0][2] = 0.0f;
// 	m->m[0][3] = 0.0f;

// 	m->m[1][0] = 0.0f;
// 	m->m[1][1] = 2.0f / (t - b);
// 	m->m[1][2] = 0.0f;
// 	m->m[1][3] = 0.0f;

// 	m->m[2][0] = 0.0f;
// 	m->m[2][1] = 0.0f;
// 	m->m[2][2] = -2.0f / (f - n);
// 	m->m[2][3] = 0.0f;

// 	m->m[3][0] = -(r + l) / (r - l);
// 	m->m[3][1] = -(t + b) / (t - b);
// 	m->m[3][2] = -(f + n) / (f - n);
// 	m->m[3][3] = 1.0f;
// }

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

Matrix4 operator * ( const Matrix4 m1, const Matrix4 m2 ) {
	return MultMatrix(m1, m2);
}

Vec3 MultVector( Matrix4 m, Vec3 v ) {
	return { v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0],
		     v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1],
		     v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2] };
}

// void MultiplyComponents( Matrix4* m, float scalar ) {
// 	for( int i = 0; i < 4; i++ ) {
// 		for( int j = 0; j < 4; j++ ) {
// 			m.m[i][j] *= scalar;
// 		}
// 	}
// }

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

//Code credit to Polycode
void ToAngleAxis( const Quaternion q, float* angle, Vec3* axis) {
	float fSqrLength = q.x * q.x + q.y * q.y + q.z * q.z;
	if ( fSqrLength > 0.0f )
	{
		*angle = 2.0f * acos(q.w);
		float fInvLength = InvSqrt(fSqrLength);
		axis->x = q.x * fInvLength;
		axis->y = q.y * fInvLength;
		axis->z = q.z * fInvLength;
	}
	else
	{
		// angle is 0 (mod 2*pi), so any axis will do
		*angle = 0.0f;
		axis->x = 1.0;
		axis->y = 0.0;
		axis->z = 0.0;
	}
}

Quaternion InverseQuat( Quaternion quat ) {
	float fNorm = quat.w * quat.w + quat.x * quat.x + quat.y * quat.y + quat.z * quat.z;
	float fInvNorm = 1.0f/fNorm;
	Quaternion rq;
	rq.w *= fInvNorm;
	rq.x *= ( -1.0f * fInvNorm );
	rq.y *= ( -1.0f * fInvNorm );
	rq.z *= ( -1.0f * fInvNorm );

	return rq;
}

Vec3 ApplyTo( Quaternion q, Vec3 v ) {
	//Credit to Casey Muratori
	Vec3 t = CrossProd( {q.x, q.y, q.z }, v ) * 2.0f;
	return v + t * q.w + CrossProd( {q.x, q.y, q.z}, t );
}

Matrix4 MatrixFromQuat(const Quaternion quat) {
	//Credit to Ivan Safrin, code lifted from Polycode
	Matrix4 matx;

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