float Min(float a, float b)
{
	float result = a;
	if (b < a)
		result = b;
	return result;
}

float Max(float a, float b)
{
	float result = a;
	if (b > a)
		result = b;
	return result;
}

float ClampToRange(float min, float val, float max)
{
	float result = Max(Min(val, max), min);
	return result;
}

void ClampToRange(float min, float *val, float max)
{
	*val = ClampToRange(min, *val, max);
}

float Abs(float a)
{
	float result = a;
	if (a < 0)
		result *= -1;
	return result;
}

float Sign(float a)
{
	float result = 0;
	if (a < 0)
		result = -1;
	if (a > 0)
		result = 1;
	return result;
}

int Min(int a, int b)
{
	int result = a;
	if (b < a)
		result = b;
	return result;
}

int Max(int a, int b)
{
	int result = a;
	if (b > a)
		result = b;
	return result;
}

int ClampToRange(int min, int val, int max)
{
	int result = Max(Min(val, max), min);
	return result;
}

void ClampToRange(int min, int *val, int max)
{
	*val = ClampToRange(min, *val, max);
}

int WrapToRange(int min, int val, int max)
{
	int result = val;
	while (result < min)
	{
		result += (max - min);
	}
	while (result >= max)
	{
		result -= (max - min);
	}
	return result;
}

float Lerp(float current, float destination, float proportional_change)
{
	float result = current + (destination - current)*proportional_change;
	return result;
}

int Abs(int a)
{
	int result = a;
	if (a < 0)
		result *= -1;
	return result;
}

int Sign(int a)
{
	int result = 0;
	if (a < 0)
		result = -1;
	if (a > 0)
		result = 1;
	return result;
}


uint32_t Min(uint32_t a, uint32_t b)
{
	uint32_t result = a;
	if (b < a)
		result = b;
	return result;
}

uint32_t Max(uint32_t a, uint32_t b)
{
	uint32_t result = a;
	if (b > a)
		result = b;
	return result;
}

uint32_t ClampToRange(uint32_t min, uint32_t val, uint32_t max)
{
	uint32_t result = Max(Min(val, max), min);
	return result;
}

void ClampToRange(uint32_t min, uint32_t *val, uint32_t max)
{
	*val = ClampToRange(min, *val, max);
}

inline bool PointInAABB(ivec2 p, ivec2 bl, ivec2 ur)
{
	return (p.x >= bl.x && p.x <= ur.x && p.y >= bl.y && p.y <= ur.y);
}



inline int LengthSq(ivec2 a)
{
	int result = (a.x*a.x) + (a.y*a.y);
	return (result);
}
