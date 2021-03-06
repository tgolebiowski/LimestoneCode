#ifndef MATH3D
#define MATH3D
#include <math.h>

#define PI 3.14159265359
#define TWO_PI ( 2.0f * PI )

static float MAXf(float f1, float f2) { 
	return ( ((float)(f1 >= f2)) * f1 ) + ( ((float)(f1 < f2)) * f2 );
}

static float MINf( float f1, float f2) { 
	return ( ((float)(f1 <= f2)) * f1 ) + ( ((float)(f1 > f2)) * f2 );
}

//Thanks Amazing Thew!
static float ABSf( float f ) {
	return ( MAXf( ( -f ), f ) );
}

//Macro-ness allows this to work on Vec3s and floats alike, templates!
#define LERP( f1, f2, t ) ( ( (1.0f - t) * f1 ) + ( t * f2 ) )

struct Vec2 {
	float x, y;
};

struct Vec3 {
	float x, y, z;
};

struct Vec4 {
	float r, g, b, a;
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
	x = *(float*)&i; // convert new bits into a float
	x = x*(1.5f - xhalf*x*x); // One round of Newton's method
	return x;
}
/*-----------------------------------------------------------------------
	                                Vec2
------------------------------------------------------------------------*/

float Vec2Length( Vec2 v ) {
	return sqrtf( v.x * v.x + v.y * v.y );
}

Vec2 operator - ( Vec2 v1, Vec2 v2 ) {
	return { v1.x - v2.x, v1.y - v2.y };
}

float Dot( Vec2 a, Vec2 b ) {
	return { a.x * b.x + a.y * b.y };
}

void Normalize( Vec2* v ) {
	float l = Vec2Length( *v );
	if( l > 1e-08 ) {
		float invL = 1.0f / l;
		v->x *= invL;
		v->y *= invL;
	}
}

static inline void GetBaryCentricCoords( Vec2* tri, Vec2 p, Vec3* outCoords ) {
	Vec2 p1 = tri[0];
	Vec2 p2 = tri[1];
	Vec2 p3 = tri[2];

	float denominator = ((p2.y - p3.y) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.y - p3.y));

	//a corresponds with edge b/n points 2 & 3
	float a = ((p2.y - p3.y) * (p.x - p3.x) + (p3.x - p2.x) * (p.y - p3.y)) / denominator;
	//b corresponds with edge b/n points 1 & 3
	float b = ((p3.y - p1.y) * (p.x - p3.x) + (p1.x - p3.x) * (p.y - p3.y)) / denominator;
	//c corresponds with edge b/n points 1 & 2
	float c = 1 - a - b;

	outCoords->x = a;
	outCoords->y = b;
	outCoords->z = c;
}


/*-----------------------------------------------------------------------
                                    Vec3
-------------------------------------------------------------------------*/

void Normalize( Vec3* v ) {
	float tL = sqrt( v->x * v->x + v->y * v->y + v->z * v->z );
	if(tL > 1e-08 ) {
		float invTl = 1.0 / tL;
		v->x *= invTl;
		v->y *= invTl;
		v->z *= invTl;
	}
}

float Vec3Length( Vec3 v ) {
	return sqrtf( v.x * v.x + v.y * v.y + v.z * v.z );
}

Vec3 operator + ( Vec3 v1, Vec3 v2 ) {
	return { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
}

Vec3 operator * (Vec3 v, float s ) {
	return { v.x * s, v.y * s, v.z * s };
}

Vec3 operator - ( Vec3 v1, Vec3 v2 ) {
	return { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
}

Vec3 operator * (float s, Vec3 v ) {
	return { v.x * s, v.y * s, v.z * s };
}

Vec3 DiffVec( Vec3 a, Vec3 b ) {
	return { b.x - a.x, b.y - a.y, b.z - a.z };
}

float Dot( Vec3 v1, Vec3 v2 ) {
	return 
	    ( v1.x * v2.x ) + 
	    ( v1.y * v2.y ) + 
	    ( v1.z * v2.z );
}

Vec3 Cross( Vec3 v1, Vec3 v2 ) {
	Vec3 crossProdVec;
	crossProdVec.x = v1.y * v2.z - v1.z * v2.y;
	crossProdVec.y = v1.z * v2.x - v1.x * v2.z;
	crossProdVec.z = v1.x * v2.y - v1.y * v2.x;
	return crossProdVec;
}

float AngleBetween( Vec3 v1, Vec3 v2) {
	float lenProduct = Vec3Length(v1) * Vec3Length(v2);
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

static Mat4 IdentityMatrix = {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
};

static void SetToIdentity( Mat4* m ) {
	*m = IdentityMatrix;
}

void SetScale( Mat4* m , float x, float y, float z) {
	m->m[0][0] = x;
	m->m[1][1] = y;
	m->m[2][2] = z;
}

void SetScale( Mat4* m, Vec3 v ) {
	SetScale( m, v.x, v.y, v.z );
}

void SetScale( Mat4* m, float s ) {
	SetScale( m, s, s, s );
}

//TODO: spring lerp macro or function
Mat4 CreateScaleMatByAxis( Vec3 n, float scaling ) {
	Normalize( &n );

	float fI = scaling - 1.0f;

	Mat4 scaleMat = 
	{
		1.0f + fI * n.x * n.x, fI * n.x * n.y, fI * n.x * n.z, 0.0f,
		fI * n.y * n.x, 1.0f + fI * n.y * n.y, fI * n.y * n.z, 0.0f,
		fI * n.z * n.x, fI * n.z * n.y, 1.0f + fI * n.z * n.z, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	return scaleMat;
}

void SetTranslation( Mat4* m, float x, float y, float z ) {
	m->m[3][0] = x;
	m->m[3][1] = y;
	m->m[3][2] = z;
}

void SetTranslation( Mat4* m, Vec3 p ) {
	m->m[3][0] = p.x;
	m->m[3][1] = p.y;
	m->m[3][2] = p.z;
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

Mat4 TransposeMatrix( Mat4 m ) {
	Mat4 r;
	for( uint8 y = 0; y < 4; y++ ) {
		for( uint8 x = 0; x < 4; x++ ) {
			r[y][x] = m[x][y];
		}
	}

	return r;
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

Vec3 MultVec( Mat4 m, Vec3 v, float w = 1.0f ) {
	return { v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0] * w,
		     v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1] * w,
		     v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2] * w};
}

/*----------------------------------------------------------------------------
                                    Quat
------------------------------------------------------------------------------*/

Quat MultQuats ( const Quat a, const Quat b ) {
	Quat quat;
	quat.w = -a.x * b.x - a.y * b.y - a.z * b.z + a.w * b.w;
	quat.x = a.x * b.w + a.y * b.z - a.z * b.y + a.w * b.x;
	quat.y = -a.x * b.z + a.y * b.w + a.z * b.x + a.w * b.y;
	quat.z = a.x * b.y - a.y * b.x + a.z * b.w + a.w * b.z;
	return quat;
}

Quat RotationBtwnVec3( Vec3 a, Vec3 b ) {
	Normalize( &a );
	Normalize( &b );
	float cosTheta = Dot( a, b );

	Vec3 rotationAxis;
	if( cosTheta < -0.999f ) {
		rotationAxis = Cross( { 0.0f, 0.0f, 1.0f }, a );
	}

	rotationAxis = Cross( a, b );
	float s = sqrtf( ( 1.0f + cosTheta ) * 2.0f );
	float invs = 1.0f / s;

	return {
		s * 0.5f,
		rotationAxis.x * invs,
		rotationAxis.y * invs,
		rotationAxis.z * invs
	};
}

static inline Quat FromAngleAxis(
	const float axisX,
	const float axisY, 
	const float axisZ, 
	const float angle
) {
	Quat quat;
	float halfAngle = ( 0.5 * angle );
	float fSin = sin(halfAngle);
	quat.w = cos(halfAngle);
	quat.x = fSin * axisX;
	quat.y = fSin * axisY;
	quat.z = fSin * axisZ;
	return quat;
}

static inline Quat FromAngleAxis( const Vec3 v, float angle ) {
	return FromAngleAxis( v.x, v.y, v.z, angle );
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
	Quat rq = { 1.0f, 0.0f, 0.0f, 0.0f };
	rq.w *= fInvNorm;
	rq.x *= ( -1.0f * fInvNorm );
	rq.y *= ( -1.0f * fInvNorm );
	rq.z *= ( -1.0f * fInvNorm );

	return rq;
}

static inline Quat Lerp( Quat q1, Quat q2, float weight ) {
	float oneMinusWeight = 1.0f - weight;
	return {
		q1.w * weight + q2.w * oneMinusWeight,
		q1.x * weight + q2.x * oneMinusWeight,
		q1.y * weight + q2.y * oneMinusWeight,
		q1.z * weight + q2.z * oneMinusWeight
	};
}

Quat Slerp( const Quat q1, const Quat q2, float weight ) {
	double dotproduct = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;

	if( abs( dotproduct ) >= 1.0f ) return q1;

	double halfTheta = acos( dotproduct );
	double sinHalfTheta = sqrt( 1.0 - ( dotproduct * dotproduct ) );

	if( fabs( sinHalfTheta ) < 0.0001 ) {
		Quat qm;
		qm.w = (q1.w * 0.5 + q2.w * 0.5);
		qm.x = (q1.x * 0.5 + q2.x * 0.5);
		qm.y = (q1.y * 0.5 + q2.y * 0.5);
		qm.z = (q1.z * 0.5 + q2.z * 0.5);
		return qm;	
	}
	double ratioA = sin( ( 1 - weight ) * halfTheta ) / sinHalfTheta;
	double ratioB = sin( weight * halfTheta ) / sinHalfTheta;

	Quat qm;
	qm.w = ( q1.w * ratioA + q2.w * ratioB );
	qm.x = ( q1.x * ratioA + q2.x * ratioB );
	qm.y = ( q1.y * ratioA + q2.y * ratioB );
	qm.z = ( q1.z * ratioA + q2.z * ratioB );
	return qm;
}

Vec3 ApplyQuatToVec( Quat q, Vec3 v ) {
	//Credit to Fabien Giesen?? 
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
	matx.m[0][1] = fTxy-fTwz;
	matx.m[0][2] = fTxz+fTwy;
	matx.m[0][3] = 0.0f;

	matx.m[1][0] = fTxy+fTwz;
	matx.m[1][1] = 1.0-(fTxx+fTzz);
	matx.m[1][2] = fTyz-fTwx;
	matx.m[1][3] = 0.0f;

	matx.m[2][0] = fTxz-fTwy;
	matx.m[2][1] = fTyz+fTwx;
	matx.m[2][2] = 1.0-(fTxx+fTyy);
	matx.m[2][3] = 0.0f;

	matx.m[3][0] = 0.0f;
	matx.m[3][1] = 0.0f;
	matx.m[3][2] = 0.0f;
	matx.m[3][3] = 1.0f;

	return matx;
}

Quat QuatFromMatrix( const Mat4 matrix ) {
	Quat quat;
	float tr, s, q[4];
	int i, j, k;

	const int nxt[3] = {1, 2, 0};			

	tr = matrix.m[0][0] + matrix.m[1][1] + matrix.m[2][2];

	// check the diagonal
	if (tr > 0.0f)
	{
		s = sqrtf(tr + 1.0f);
		quat.w = s / 2.0f;
		s = 0.5f / s;
		quat.x = ( matrix.m[2][1] - matrix.m[1][2] ) * s;
		quat.y = ( matrix.m[0][2] - matrix.m[2][0] ) * s;
		quat.z = ( matrix.m[1][0] - matrix.m[0][1] ) * s;
	}
	else
	{
		// diagonal is negative
		i = 0;
		if (matrix.m[1][1] > matrix.m[0][0]) i = 1;
		if (matrix.m[2][2] > matrix.m[i][i]) i = 2;
		j = nxt[i];
		k = nxt[j];

		s = sqrtf((matrix.m[i][i] - (matrix.m[j][j] + matrix.m[k][k])) + 1.0f);

		q[i] = s * 0.5f;

		if (s != 0.0f) s = 0.5f / s;

		q[3] = (matrix.m[j][k] - matrix.m[k][j]) * s;
		q[j] = (matrix.m[i][j] + matrix.m[j][i]) * s;
		q[k] = (matrix.m[i][k] + matrix.m[k][i]) * s;

		quat.w = q[0];
		quat.x = q[1];
		quat.y = q[2];
		quat.z = q[3];
	}

	return quat;
}

Vec3 QuatToEuler( const Quat q ) {
	return {
		atan2( 2.0f * ( q.w * q.x + q.y * q.z ), 1.0f - 2.0f * ( q.x * q.x + q.y * q.y ) ),
		asinf( 2.0f * ( q.w * q.y - q.z * q.x ) ),
		atan2( 2.0f * ( q.w * q.z + q.x * q.y ), 1.0f - 2.0f * ( q.y * q.y + q.z * q.z ) )
	};
}

/*---------------------------------------------------------------------------------------------------
                                           Extra Utility Functions
-----------------------------------------------------------------------------------------------------*/

Mat4 Mat4FromComponents( Vec3 scale, Quat rotation, Vec3 translation ) {
	Vec3 x = ApplyQuatToVec( rotation, { 1.0f * scale.x, 0.0f, 0.0f } );
	Vec3 y = ApplyQuatToVec( rotation, { 0.0f, 1.0f * scale.y, 0.0f } );
	Vec3 z = ApplyQuatToVec( rotation, { 0.0f, 0.0f, 1.0f * scale.z } );

	Mat4 m = {
		x.x, x.y, x.z, 0.0f,
		y.x, y.y, y.z, 0.0f,
		z.x, z.y, z.z, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	//TODO: Make this work! Stop doing things the dumb way
	//Mat4 m = MatrixFromQuat( rotation );

	m[3][0] = translation.x;
	m[3][1] = translation.y;
	m[3][2] = translation.z;

	return m;
}

//Sets scale scale and translation to 0 and rotation to { 1,0,0,0 } if it cannot be decomposed
void DecomposeMat4( Mat4 m, Vec3* scale, Quat* rotation, Vec3* translation ) {
	//TODO: simpler decomposition code
	Vec3 p = { 0.0f, 0.0f, 0.0f };
	Vec3 xp = { 1.0f, 0.0f, 0.0f };
	Vec3 yp = { 0.0f, 1.0f, 0.0f };
	Vec3 zp = { 0.0f, 0.0f, 1.0f };
	p = MultVec( m, p );
	xp = MultVec( m, xp );
	yp = MultVec( m, yp );
	zp = MultVec( m, zp );
	*translation = p;
	Vec3 x = DiffVec( p, xp );
	Vec3 y = DiffVec( p, yp );
	Vec3 z = DiffVec( p, zp );
	scale->x = Vec3Length( x );
	scale->y = Vec3Length( y );
	scale->z = Vec3Length( z );
	Normalize( &x );
	Normalize( &y );
	Normalize( &z );

	Quat rot1 = RotationBtwnVec3( { 0.0f, 0.0f, 1.0f }, z );
	Vec3 halfUp = ApplyQuatToVec( rot1, { 0.0f, 1.0f, 0.0f } );
	Quat rot2 = RotationBtwnVec3( halfUp, y );
	*rotation = MultQuats( rot2, rot1 );
}

struct Srt {
	bool isNonAxisAlignedScaled;

	union {
		Vec3 vec;
		Mat4 mat;
	} scale;

	Quat rotation;
	Vec3 translation;
};

//----------------------------------------------------
//Srt1 is the "base" or "anchor" transform, so the result transform is
//srt1 and then srt2
//Scale->rotation->translation
//----------------------------------------------------
static Srt operator + ( Srt srt1, Srt srt2 ) {
	Srt result = { };
	if( !srt1.isNonAxisAlignedScaled && !srt2.isNonAxisAlignedScaled ) {
		Vec3 scale = {
			srt1.scale.vec.x * srt2.scale.vec.x,
			srt1.scale.vec.y * srt2.scale.vec.y,
			srt1.scale.vec.z * srt2.scale.vec.z
		};

		result.isNonAxisAlignedScaled = false;
		result.scale.vec = scale;
	} else {
		Mat4 s1, s2;
		if( srt1.isNonAxisAlignedScaled ) {
			s1 = srt1.scale.mat;
		} else {
			s1 = IdentityMatrix;
			SetScale( &s1, srt1.scale.vec );
		}

		if( srt2.isNonAxisAlignedScaled ) {
			s2 = srt2.scale.mat;
		} else {
			s2 = IdentityMatrix;
			SetScale( &s2, srt2.scale.vec );
		}

		result.isNonAxisAlignedScaled = true;
		result.scale.mat = MultMatrix( s2, s1 );
	}

	Quat rotation = MultQuats( srt1.rotation, srt2.rotation );

	Vec3 translation = ApplyQuatToVec( srt1.rotation, srt2.translation );
	translation.x *= srt1.scale.vec.x;
	translation.y *= srt1.scale.vec.y;
	translation.z *= srt1.scale.vec.z;
	translation = translation + srt1.translation;

	result.rotation = rotation;
	result.translation = translation;

	return result;
}

static Mat4 SrtAsMat4( Srt srt ) {
	if( srt.isNonAxisAlignedScaled ) {
		const Vec3 noScale = { 1.0f, 1.0f, 1.0f };
		return 
		MultMatrix( 
			srt.scale.mat,
			Mat4FromComponents( noScale, srt.rotation, srt.translation )
		);
	} else {
		return Mat4FromComponents( srt.scale.vec, srt.rotation, srt.translation );
	}
}

static Vec3 TransformVec( Srt transform, Vec3 startVec ) {
	startVec.x *= transform.scale.vec.x;
	startVec.y *= transform.scale.vec.y;
	startVec.z *= transform.scale.vec.z;

	startVec = ApplyQuatToVec( transform.rotation, startVec );

	startVec = startVec + transform.translation;

	return startVec;
}

#endif