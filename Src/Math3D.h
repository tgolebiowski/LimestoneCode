#include <math.h>

#define PI 3.14159265359

struct Vec3 {
	float x, y, z;
};

struct Quat {
	float w, x, y, z;
};

struct Mat4 {
	float m[4][4];

	float* operator [] ( int i ) {
		return m[i];
	}
};

float InvSqrt(float x) {
	float xhalf = 0.5f * x;
	int i = *(int*)&x; // store Numbering-point bits in integer
	i = 0x5f3759d5 - (i >> 1); // initial guess for Newton's method
	x = *(float*)&i; // convert new bits into Number
	x = x*(1.5f - xhalf*x*x); // One round of Newton's method
	return x;
}

/*-----------------------------------------------------------------------
                                    Vec3
-------------------------------------------------------------------------*/

void PrintVec( Vec3 v ) {
	#include <stdio.h>
	printf("x:%.3f, y:%.3f, z:%.3f", v.x, v.y, v.z );
}

void Normalize( Vec3* v ) {
	float tL = sqrt( v->x * v->x + v->y * v->y + v->z * v->z );
	if(tL > 1e-08 ) {
		float invTl = 1.0 / tL;
		v->x *= invTl;
		v->y *= invTl;
		v->z *= invTl;
	}
}

float VecLength( Vec3 v ) {
	return sqrtf( v.x * v.x + v.y * v.y + v.z * v.z );
}

Vec3 operator + ( Vec3 v1, Vec3 v2 ) {
	return { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
}

Vec3 operator * (Vec3 v, float s ) {
	return { v.x * s, v.y * s, v.z * s };
}

Vec3 operator * (float s, Vec3 v ) {
	return { v.x * s, v.y * s, v.z * s };
}

Vec3 DiffVec( Vec3 a, Vec3 b ) {
	return { b.x - a.x, b.y - a.y, b.z - a.z };
}

float Dot( Vec3 v1, Vec3 v2 ) {
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

Vec3 Cross( Vec3 v1, Vec3 v2 ) {
	Vec3 crossProdVec;
	crossProdVec.x = v1.y * v2.z - v1.z * v2.y;
	crossProdVec.y = v1.z * v2.x - v1.x * v2.z;
	crossProdVec.z = v1.x * v2.y - v1.y * v2.x;
	return crossProdVec;
}

float AngleBetween( Vec3 v1, Vec3 v2) {
	float lenProduct = VecLength(v1) * VecLength(v2);
	if(lenProduct < 1e-6f)
		lenProduct = 1e-6f;

	float f = Dot(v1, v2) / lenProduct;
	if ( f > 1.0f ) f = 1.0f;
	else if ( f < -1.0f ) f = -1.0f;
	return acosf(f);
}

/*-------------------------------------------------------------------
                                 Mat4
---------------------------------------------------------------------*/

void PrintMat4( Mat4 m ) {
	printf("Row 1: %.2f, %.2f, %.2f, %.2f\n", m[0][0], m[0][1], m[0][2], m[0][3] );
	printf("Row 2: %.2f, %.2f, %.2f, %.2f\n", m[1][0], m[1][1], m[1][2], m[1][3] );
	printf("Row 3: %.2f, %.2f, %.2f, %.2f\n", m[2][0], m[2][1], m[2][2], m[2][3] );
	printf("Row 4: %.2f, %.2f, %.2f, %.2f\n", m[3][0], m[3][1], m[3][2], m[3][3] );
}

void SetToIdentity(Mat4* m) {
	m->m[0][0] = 1.0f; m->m[0][1] = 0.0f; m->m[0][2] = 0.0f; m->m[0][3] = 0.0f;
	m->m[1][0] = 0.0f; m->m[1][1] = 1.0f; m->m[1][2] = 0.0f; m->m[1][3] = 0.0f;
	m->m[2][0] = 0.0f; m->m[2][1] = 0.0f; m->m[2][2] = 1.0f; m->m[2][3] = 0.0f;
	m->m[3][0] = 0.0f; m->m[3][1] = 0.0f; m->m[3][2] = 0.0f; m->m[3][3] = 1.0f;
}

void SetScale( Mat4* m , float x, float y, float z) {
	m->m[0][0] = x;
	m->m[1][1] = y;
	m->m[2][2] = z;
}

void SetTranslation( Mat4* m, float x, float y, float z ) {
	m->m[3][0] = x;
	m->m[3][1] = y;
	m->m[3][2] = z;
}

void SetRotation( Mat4* mtx, float x, float y, float z, float angle) {
	float c = cos( angle );
	float s = sin( angle );
	float t = 1.0f - c;
	mtx->m[0][0] = ( t * x * x ) + c; mtx->m[0][1] = ( x * y * t ) + ( z * s ); mtx->m[0][2] = ( x * z * t ) - ( y * s ); mtx->m[0][3];
	mtx->m[1][0] = ( x * y * t ) - ( z * s ); mtx->m[1][1] = ( t * y * y ) + c; mtx->m[1][2] = ( t * y * z ) + ( x * s ); mtx->m[1][3];
	mtx->m[2][0] = ( x * z * t ) + ( y * s ); mtx->m[2][1] = ( t * y * z ) - ( x * s ); mtx->m[2][2] = ( t * z * z ) + c; mtx->m[2][3];
	mtx->m[3][0] = 0.0f; mtx->m[3][1] = 0.0f; mtx->m[3][1] = 0.0f;	mtx->m[3][3] = 1.0f;
}

Mat4 MultMatrix(const Mat4 m, const Mat4 m2) {
	Mat4 r;
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

Mat4 InverseMatrix( Mat4 m ) {
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

	Mat4 retVal;
    retVal.m[0][0] = d00; retVal.m[0][1] = d01; retVal.m[0][2] = d02; retVal.m[0][3] = d03;
    retVal.m[1][0] = d10; retVal.m[1][1] = d11; retVal.m[1][2] = d12; retVal.m[1][3] = d13;
    retVal.m[2][0] = d20; retVal.m[2][1] = d21; retVal.m[2][2] = d22; retVal.m[2][3] = d23;
    retVal.m[3][0] = d30; retVal.m[3][1] = d31; retVal.m[3][2] = d32; retVal.m[3][3] = d33;
    return retVal;
}

Mat4 operator * ( const Mat4 m1, const Mat4 m2 ) {
	return MultMatrix(m1, m2);
}

Vec3 MultVec( Mat4 m, Vec3 v ) {
	return { v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0],
		     v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1],
		     v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2] };
}

/*----------------------------------------------------------------------------
                                    Quat
------------------------------------------------------------------------------*/

Quat MultQuats (const Quat lQ, const Quat rQ) {
	Quat quat;
	quat.w = lQ.w * rQ.w - lQ.x * rQ.x - lQ.y * rQ.y - lQ.z * rQ.z;
	quat.x = lQ.w * rQ.x + lQ.x * rQ.w + lQ.y * rQ.z - lQ.z * rQ.y;
	quat.y = lQ.w * rQ.y + lQ.y * rQ.w + lQ.z * rQ.x - lQ.x * rQ.z;
	quat.z = lQ.w * rQ.z + lQ.z * rQ.w + lQ.x * rQ.y - lQ.y * rQ.x;
	return quat;
}

Quat FromAngleAxis(const float axisX, const float axisY, const float axisZ, const float angle) {
	Quat quat;
	float halfAngle = ( 0.5 * angle );
	float fSin = sin(halfAngle);
	quat.w = cos(halfAngle);
	quat.x = fSin * axisX;
	quat.y = fSin * axisY;
	quat.z = fSin * axisZ;
	return quat;
}

void ToAngleAxis( const Quat q, float* angle, Vec3* axis) {
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

Quat InverseQuat( Quat quat ) {
	float fNorm = quat.w * quat.w + quat.x * quat.x + quat.y * quat.y + quat.z * quat.z;
	float fInvNorm = 1.0f/fNorm;
	Quat rq;
	rq.w *= fInvNorm;
	rq.x *= ( -1.0f * fInvNorm );
	rq.y *= ( -1.0f * fInvNorm );
	rq.z *= ( -1.0f * fInvNorm );

	return rq;
}

Vec3 ApplyTo( Quat q, Vec3 v ) {
	//Credit to Casey Muratori
	Vec3 t = Cross( {q.x, q.y, q.z }, v ) * 2.0f;
	return v + t * q.w + Cross( {q.x, q.y, q.z}, t );
}

Mat4 MatrixFromQuat(const Quat quat) {
	Mat4 matx;

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