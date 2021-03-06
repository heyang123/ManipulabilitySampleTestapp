#include "MatrixDefs.h"
#include "math.h"

using namespace matrices;

void matrices::Matrix3ToMatrix4(const Matrix3& from, Matrix4& to)
{
	to.block(0,0,3,3) = from;
	to(3,3) = 1;
}

void matrices::Matrix4ToMatrix3(const Matrix4& from, Matrix3& to)
{
	to = from.block(0,0,3,3);
}

/* Rotates rotated around axis by angle theta, and returns it */
Matrix4 matrices::Translate(NUMBER x, NUMBER y, NUMBER z)
{								
	Matrix4 mat = Matrix4::Zero();				
	mat(3,0) = x;
	mat(3,1) = y;
	mat(3,2) = z;
	mat(3,3) = 1;
	return mat;
}
Matrix4 matrices::Translate(Vector3 vector)
{								
	Matrix4 mat = Matrix4::Zero();				
	mat(0,3) = vector(0);
	mat(1,3) = vector(1);
	mat(2,3) = vector(2);
	mat(3,3) = 1;
	return mat;
}
Matrix4 matrices::Rotx4(NUMBER theta)
{
	NUMBER c = cos(theta);								
	NUMBER s = sin(theta);								
	Matrix4 mat = Matrix4::Zero();				
	mat(0,0) = 1;					//	1 	0	0
	mat(1, 1) = c; mat(1, 2) = -s;	//	0	cos -sin
	mat(2, 1) = s; mat(2, 2) = c;	//	0	sin	cos	
	return mat;
}
Matrix4 matrices::Roty4(NUMBER theta)
{
	NUMBER c = cos(theta);										
	NUMBER s = sin(theta);										
	Matrix4 mat = Matrix4::Zero();		
	mat(0,0) = c; mat(0,2) = s;		//	cos	 0	sin
	mat(1, 1) = 1;					//	0	 1	0
	mat(2,0) = -s; mat(2, 2) = c;	//	-sin 0	cos
	return mat;
}
Matrix4 matrices::Rotz4(NUMBER theta)
{
	NUMBER c = cos(theta);										
	NUMBER s = sin(theta);										
	Matrix4 mat = Matrix4::Zero();		
	mat(0,0) = c; mat(0,1) = -s ;	//	cos	-sin 0
	mat(1, 0) = s; mat(1, 1) = c;	//	sin	cos	 0
	mat(2, 2) = 1;					//	0	0	 1
	return mat;
}

Matrix3 matrices::Rotx3(NUMBER theta)
{
	NUMBER c = cos(theta);								
	NUMBER s = sin(theta);								
	Matrix3 mat = Matrix3::Zero();				
	mat(0,0) = 1;					//	1 	0	0
	mat(1,1) = c; mat(1, 2) = -s;	//	0	cos -sin
	mat(2, 1) = s; mat(2, 2) = c;	//	0	sin	cos	
	return mat;
}
Matrix3 matrices::Roty3(NUMBER theta)
{
	NUMBER c = cos(theta);										
	NUMBER s = sin(theta);										
	Matrix3 mat = Matrix3::Zero();		
	mat(0,0) = c; mat(0,2) = s;		//	cos	 0	sin
	mat(1, 1) = 1;					//	0	 1	0
	mat(2,0) = -s; mat(2, 2) = c;	//	-sin 0	cos
	return mat;
}
Matrix3 matrices::Rotz3(NUMBER theta)
{
	NUMBER c = cos(theta);										
	NUMBER s = sin(theta);										
	Matrix3 mat = Matrix3::Zero();		
	mat(0,0) = c; mat(0,1) = -s ;	//	cos	-sin 0
	mat(1, 0) = s; mat(1, 1) = c;	//	sin	cos	 0
	mat(2, 2) = 1;					//	0	0	 1
	return mat;
}

// project vector onto another and returns cosinnus angle
double matrices::Project(const Vector3& from, const Vector3& to)
{
	double fromNorm = from.norm();
	if(fromNorm == 0) return 0.;
	Vector3 toNormalized = to; toNormalized.normalize();
	double cosA = (from(0) * toNormalized(0) + from(1) * toNormalized(1) + from(2) * toNormalized(2) ) / fromNorm;
	return cosA;
}

Vector3 matrices::ProjectOnPlan(const Vector3& normalAxis, const Vector3& vector)
{
	return normalAxis.cross(vector.cross(normalAxis));
}

bool matrices::NotZero(const Vector3& vect)
{
	return (!(vect(0) == 0 && vect(1) == 0 && vect(2) == 0));
}

/*Tu consid�res u,v deux vecteurs de norme L2 = 1 dans R^3
Tu cherches la rotation R, telle que Ru=v.
R = cos theta * I + (I x [a,a,a])^T * sin theta + (1 - cos theta) * a*a^T
avec :
cos theta = u . v
sin theta = ||u x v||
a=u x v / sin theta
I �tant l'identit�, * le produit matriciel, x le cross product et ^T la transpos�e.
http://fr.wikipedia.org/wiki/Rotation_vectorielle
D�riv�e de la formule de Rodriguez*/
void matrices::GetRotationMatrix(const Vector3& from, const Vector3& to, Matrix3& result)
{
	Vector3 u, v, uXv, a;
	u = from; u.normalize();
	v = to  ; v.normalize();
	uXv = u.cross(v);
	NUMBER sinTheta = uXv.norm();
	if (sinTheta == 0) // angle is 0
	{
		result = Matrix3::Identity();
	}
	else
	{
		NUMBER cosTheta = u.dot(v);
		a = uXv / sinTheta;

		Matrix3 I = Matrix3::Identity();

		Matrix3 Iaaa = Matrix3::Zero();

		Iaaa(0,1) = - a(2); Iaaa(0,2) =  a(1); //  0  -z   y
		Iaaa(1,0) =   a(2); Iaaa(1,2) = -a(0); //  z   0  -x
		Iaaa(2,0) = - a(1); Iaaa(2,1) =  a(0); // -y   x   0

		result = I * cosTheta + sinTheta * Iaaa + (1 - cosTheta) * (a*a.transpose());
	}
}

// TODO forward dec
Vector3& matrices::Rotate(const Vector3& axis, Vector3& rotated, NUMBER theta)
{
	NUMBER x  = rotated.x(); NUMBER y  = rotated.y(); NUMBER z = rotated.z();
	NUMBER x1 = axis.x(); NUMBER y1 = axis.y(); NUMBER z1 = axis.z();

	NUMBER c = cos(theta);
	NUMBER s = sin(theta);
	NUMBER dotw = (x*x1 + y*y1 + z*z1);
	NUMBER v0x = dotw*x1;
	NUMBER v0y = dotw*y1;		// v0 = provjection onto axis
	NUMBER v0z = dotw*z1;
	NUMBER v1x = x-v0x;
	NUMBER v1y = y-v0y;			// v1 = projection onto plane normal to axis
	NUMBER v1z = z-v0z;
	NUMBER v2x = y1*v1z - z1*v1y;
	NUMBER v2y = z1*v1x - x1*v1z;	// v2 = axis * v1 (cross product)
	NUMBER v2z = x1*v1y - y1*v1x;
		
	rotated(0) = v0x + c*v1x + s*v2x;
	rotated(1) = v0y + c*v1y + s*v2y;
	rotated(2) = v0z	+ c*v1z + s*v2z;
	return rotated;
}


void matrices::vect4ToVect3(const VectorX& from, Vector3& to)
{
	to = from.block(0,0,3,1);
}

void matrices::vect3ToVect4(const Vector3& from, VectorX& to)
{
	to.block(0,0,3,1) = from;
	to(3) = 1;
}

Vector3 matrices::matrix4TimesVect3(const Matrix4& mat4, const Vector3& vect3)
{
	VectorX vect4(4);
	Vector3 res;
	vect3ToVect4(vect3, vect4);
	vect4 = mat4 * vect4;
	vect4ToVect3(vect4, res);
	return res;
}

Vector3 matrices::matrix4TimesVect4(const Matrix4& mat4, const VectorX& vect4)
{
	Vector3 vect3; VectorX res(4);
	res = mat4 * vect4;
	vect4ToVect3(res, vect3);
	return vect3;
}

void matrices::matrixToArray(float * tab, const Matrix4& mat4)
{
	for(int i =0; i< 3; ++i)
	{
		for(int j =0; j< 4; ++j)
		{
			tab[ 4*i + j ] = (float)mat4(i,j);
		}
	}
}

void matrices::matrix3ToArray(float * tab, const Matrix3& mat3)
{
	for(int i =0; i< 3; ++i)
	{
		for(int j =0; j< 3; ++j)
		{
			tab[ 4*i + j ] = (float)mat3(i,j);
		}
	}
}

void matrices::matrix3ToArrayD(double * tab, const Matrix3& mat3)
{
	for(int i =0; i< 3; ++i)
	{
		for(int j =0; j< 3; ++j)
		{
			tab[ 4*i + j ] = (double)mat3(i,j);
		}
	}
}

void matrices::matrixID(float * tab)
{
	for(int i =0; i< 3; ++i)
	{
		for(int j =0; j< 4; ++j)
		{
			tab[ 4*i + j ] = i == j ? 1.f : 0.f;
		}
	}
}

void matrices::matrixTo16Array(float * tab, const Matrix4& mat4)
{
	for(int i =0; i< 4; ++i)
	{
		for(int j =0; j< 4; ++j)
		{
			tab[ 4*i + j ] = (float)mat4(i,j);
		}
	}
}

void matrices::matrixTo16Array(double * tab, const Matrix4& mat4)
{
	for(int i =0; i< 4; ++i)
	{
		for(int j =0; j< 4; ++j)
		{
			tab[ 4*i + j ] = mat4(i,j);
		}
	}
}

void matrices::array16ToMatrix4(const float  * tab, Matrix4& mat4)
{
	for(int i =0; i< 4; ++i)
	{
		for(int j =0; j< 4; ++j)
		{
			mat4(i,j) = tab[ 4*i + j ];
		}
	}
}

void matrices::array16ToMatrix4(const double * tab, Matrix4& mat4)
{
	for(int i =0; i< 4; ++i)
	{
		for(int j =0; j< 4; ++j)
		{
			mat4(i,j) = (NUMBER)(tab[ 4*i + j ]);
		}
	}
}

void matrices::vect4ToArray(float * tab, const VectorX& vect)
{
	for(int i =0; i< 3; ++i)
	{
		tab[i] = (float)vect(i);
	}
}

void matrices::vect4ToArray(double * tab, const VectorX& vect)
{
	for(int i =0; i< 3; ++i)
	{
		tab[i] = vect(i);
	}
}

void matrices::vect3ToArray(float * tab, const Vector3& vect)
{
	for(int i =0; i< 3; ++i)
	{
		tab[i] = (float)vect(i);
	}
}

void matrices::vect3ToArray(double * tab, const Vector3& vect)
{
	for(int i =0; i< 3; ++i)
	{
		tab[i] = vect(i);
	}
}

void matrices::arrayToVect3(const float * tab, Vector3& vect)
{
	for(int i =0; i< 3; ++i)
	{
		vect(i) = tab[i];
	}
}

void matrices::arrayToVect3(const double * tab, Vector3& vect)
{
	for(int i =0; i< 3; ++i)
	{
		vect(i) = (NUMBER)(tab[i]);
	}
}

