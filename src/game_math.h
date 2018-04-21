float Min(float a, float b);
float Max(float a, float b);
float ClampToRange(float min, float val, float max);
void ClampToRange(float min, float *val, float max);
float Abs(float a);
float Sign(float a);

int Min(int a, int b);
int Max(int a, int b);
int ClampToRange(int min, int val, int max);
void ClampToRange(int min, int *val, int max);
int WrapToRange(int min, int val, int max);
int Abs(int a);
int Sign(int a);

float Lerp(float current, float destination, float proportional_change);

uint32_t Min(uint32_t a, uint32_t b);
uint32_t Max(uint32_t a, uint32_t b);
uint32_t ClampToRange(uint32_t min, uint32_t val, uint32_t max);
void ClampToRange(uint32_t min, uint32_t *val, uint32_t max);

inline bool PointInAABB(glm::ivec2 p, glm::ivec2 bl, glm::ivec2 ur);

inline int LengthSq(glm::ivec2 a);

struct v2i
{
	int x,y;
};

struct v2
{
	float x,y;
};

v2 V2(float x, float y)
{
	v2 result;
	result.x = x;
	result.y = y;
	return result;
}

v2i V2I(int x, int y)
{
	v2i result;
	result.x = x;
	result.y = y;
	return result;
}

v2 operator+(const v2 &a, const v2 &b)
{
	v2 result;
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	return result;
}

v2 operator-(const v2 &a, const v2 &b)
{
	v2 result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	return result;
}

v2 operator*(float a, const v2 &b)
{
	v2 result;
	result.x = a * b.x;
	result.y = a * b.y;
	return result;
}

v2 operator*(const v2 &b, float a)
{
	v2 result;
	result.x = a * b.x;
	result.y = a * b.y;
	return result;
}

inline float Inner(const v2 &a, const v2 &b)
{
	float result;
	result = a.x*b.x + a.y*b.y;
	return result;
}

v2 Normalize(const v2 &a)
{
	float factor = 1.0f / sqrtf(a.x*a.x + a.y*a.y);
	v2 result = factor * a;
	return result;
}

float LengthSq(const v2 &a)
{
	float result = Inner(a, a);
	return result;
}

struct v3
{
	float x,y,z;
};

v3 V3(float x, float y, float z)
{
	v3 result;
	result.x = x;
	result.y = y;
	result.z = z;
	return result;
}

v3 operator+(const v3 &a, const v3 &b)
{
	v3 result;
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	result.z = a.z + b.z;
	return result;
}

v3 operator-(const v3 &a, const v3 &b)
{
	v3 result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	result.z = a.z - b.z;
	return result;
}

v3 operator*(float a, const v3 &b)
{
	v3 result;
	result.x = a * b.x;
	result.y = a * b.y;
	result.z = a * b.z;
	return result;
}

v3 operator*(const v3 &b, float a)
{
	v3 result;
	result.x = a * b.x;
	result.y = a * b.y;
	result.z = a * b.z;
	return result;
}

inline float Inner(const v3 &a, const v3 &b)
{
	float result;
	result = a.x*b.x + a.y*b.y + a.z*b.z;
	return result;
}

inline v3 Cross(const v3 &a, const v3 &b)
{
	v3 result;
	result.x = a.y*b.z - a.z*b.y;
	result.y = a.z*b.x - a.x*b.z;
	result.z = a.x*b.y - a.y*b.x;
	return result;
}

v3 Normalize(const v3 &a)
{
	float factor = 1.0f / sqrtf(a.x*a.x + a.y*a.y + a.z*a.z);
	v3 result = factor * a;
	return result;
}

float Length(const v3 &a)
{
	float result;
	result = sqrt(Inner(a, a));
	return result;
}

struct plane
{
	v3 normal;
	float d;
};

struct frust
{
	union
	{
		struct {
			plane N;
			plane L;
			plane R;
			plane T;
			plane B;
			plane F;
		};
		plane NLRTBF[6];
	};
};

plane PlaneFromThreeNonColinearPoints(const v3 &p1, const v3 &p2, const v3 &p3)
{
	v3 p21 = p2 - p1;
	v3 p32 = p3 - p2;
	plane result;
	result.normal = Normalize(Cross(p32, p21));
	result.d = -result.normal.x*p1.x - result.normal.y*p1.y - result.normal.z*p1.z;
	return result;
}

inline bool PointOnPositivePlaneSide(const plane &p, const v3 &p0)
{
	return (p.normal.x*p0.x + p.normal.y*p0.y + p.normal.z*p0.z + p.d > 0);
}

bool PointInFrust(const frust &f, const v3 &p)
{
	if (!PointOnPositivePlaneSide(f.N, p))
		return false;
	if (!PointOnPositivePlaneSide(f.L, p))
		return false;
	if (!PointOnPositivePlaneSide(f.R, p))
		return false;
	if (!PointOnPositivePlaneSide(f.B, p))
		return false;
	if (!PointOnPositivePlaneSide(f.T, p))
		return false;
	if (!PointOnPositivePlaneSide(f.F, p))
		return false;
	return true;
}

bool OBBInFrust(const frust &f, v3 *p)
{
	for (int i = 0; i < 6; i++)
	{
		const plane *border = &f.NLRTBF[i];
		if (PointOnPositivePlaneSide(*border, p[0]))
			continue;
		if (PointOnPositivePlaneSide(*border, p[1]))
			continue;
		if (PointOnPositivePlaneSide(*border, p[2]))
			continue;
		if (PointOnPositivePlaneSide(*border, p[3]))
			continue;
		if (PointOnPositivePlaneSide(*border, p[4]))
			continue;
		if (PointOnPositivePlaneSide(*border, p[5]))
			continue;
		if (PointOnPositivePlaneSide(*border, p[6]))
			continue;
		if (PointOnPositivePlaneSide(*border, p[7]))
			continue;
		return false;
	}
	return true;
}

struct quaternion
{
	v3 u;
	float w;
};

glm::quat ToGLMQuat(quaternion *q)
{
	glm::quat result;
	result.x = q->u.x;
	result.y = q->u.y;
	result.z = q->u.z;
	result.w = q->w;
	return result;
}

quaternion CreateQuaternion(float angle, v3 vector)
{
	quaternion result;
	float halfTheta = angle * DEGREES_TO_RADIANS * 0.5f;
	float cosHalfTheta = cosf (halfTheta);
	float sinHalfTheta = sinf (halfTheta);

	result.w = cosHalfTheta;
	if (Inner(vector, vector) == 0)
		vector = {1,0,0};
	else
		vector = Normalize(vector);

	vector = sinHalfTheta * vector;

	result.u = vector;
	return (result);
}

quaternion CreateQuaternionRAD(float angle, v3 vector)
{
	quaternion result;
	float halfTheta = angle * 0.5f;
	float cosHalfTheta = cosf (halfTheta);
	float sinHalfTheta = sinf (halfTheta);

	result.w = cosHalfTheta;
	if (Inner(vector, vector) == 0)
		vector = {1,0,0};
	else
		vector = Normalize(vector);

	vector = sinHalfTheta * vector;

	result.u = vector;
	return (result);
}

v3 RotateByQuat(v3 p, quaternion q)
{

	//Test path 2
	v3 t = 2 * Cross(q.u, p);
	v3 result = p + q.w * t + Cross(q.u, t);
	return (result);
}


quaternion RotateQuatByQuat(quaternion pInit, quaternion pRot)
{
	quaternion result;

	result.w = (pRot.w*pInit.w - pRot.u.x*pInit.u.x - pRot.u.y*pInit.u.y - pRot.u.z*pInit.u.z);
	result.u.x = (pRot.w*pInit.u.x + pRot.u.x*pInit.w + pRot.u.y*pInit.u.z - pRot.u.z*pInit.u.y);
	result.u.y = (pRot.w*pInit.u.y - pRot.u.x*pInit.u.z + pRot.u.y*pInit.w + pRot.u.z*pInit.u.x);
	result.u.z = (pRot.w*pInit.u.z + pRot.u.x*pInit.u.y - pRot.u.y*pInit.u.x + pRot.u.z*pInit.w);

	return result;
}

quaternion RotateQuatByRotationVec(quaternion q, v3 rot)
{
	quaternion result;

	float theta = Length(rot); //in radians
	quaternion rotQuat = CreateQuaternionRAD(theta, rot);
	
	result = RotateQuatByQuat(q, rotQuat);

	return result;
}
