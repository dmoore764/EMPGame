void PolygonAddVertex(collision_polygon *poly, glm::vec2 p)
{
	assert(poly->numVerts + 1 < ArrayCount(poly->vertices));
	poly->vertices[poly->numVerts++] = p;

	vec2 total = {};
	for (int i = 0; i < poly->numVerts; i++)
	{
		total += poly->vertices[i];
	}
	total = (1.0f / poly->numVerts) * total;
	poly->center = total;
}

CollisionObject ColGetTransformedShape(CollisionObject *s, CollisionTransform *tx)
{
	CollisionObject result;
	result.type = s->type;

	switch (s->type)
	{
		case CollisionObject::AABB:
		{
			ERR("Cannot transform a AABB collision rectangle");
			collision_aabb *original = &s->aabb;
			collision_aabb *aabb = &result.aabb;

			aabb->min.x = original->min.x;
			aabb->max.x = original->max.x;
			aabb->min.y = original->min.y;
			aabb->max.y = original->max.y;
		} break;

		case CollisionObject::POINT:
		{
			glm::vec2 *original = &s->point;
			glm::vec2 *p = &result.point;

			glm::mat3 translate = glm::translate(glm::mat3(1.0f), tx->pos);
			glm::mat3 rotate = glm::rotate(translate, tx->rotation * DEGREES_TO_RADIANS);
			glm::mat3 scale = glm::scale(rotate, tx->scale);
			glm::mat3 transform = glm::translate(scale, -(*original));

			glm::vec3 newP = transform * glm::vec3(*original, 1);
			*p = {newP.x, newP.y};
		} break;

		case CollisionObject::CIRCLE:
		{
			collision_circle *original = &s->circle;
			collision_circle *c = &result.circle;

			glm::mat3 translate = glm::translate(glm::mat3(1.0f), tx->pos);
			glm::mat3 rotate = glm::rotate(translate, tx->rotation * DEGREES_TO_RADIANS);
			glm::mat3 transform = glm::scale(rotate, tx->scale);

			glm::vec3 newP = transform * glm::vec3(original->center - tx->offset, 1);
			c->center = {newP.x, newP.y};
			c->radius = original->radius * tx->scale.x;
		} break;

		case CollisionObject::POLYGON:
		{
			collision_polygon *original = &s->poly;
			collision_polygon *p = &result.poly;
			p->center = original->center;
			p->numVerts = 0;

			glm::mat3 translate = glm::translate(glm::mat3(1.0f), tx->pos);
			glm::mat3 rotate = glm::rotate(translate, tx->rotation * DEGREES_TO_RADIANS);
			glm::mat3 scale = glm::scale(rotate, tx->scale);
			glm::mat3 transform = glm::translate(scale, -tx->offset);
			for (int i = 0; i < original->numVerts; i++)
			{
				glm::vec3 newP = transform * glm::vec3(original->vertices[i], 1);
				PolygonAddVertex(p, {newP.x, newP.y});
			}
			glm::vec3 newP = transform * glm::vec3(original->center,1);
			p->center = {newP.x, newP.y};
		} break;

	}
	return result;
}

inline float
Inner(glm::vec2 a, glm::vec2 b) {
	float result = a.x*b.x + a.y*b.y;
	return (result);
}

inline float
LengthSq(glm::vec2 a) {
	float result = Inner(a, a);
	return (result);
}

inline float
Length(glm::vec2 a) {
	float result = sqrt(Inner(a, a));
	return (result);
}

inline glm::vec2
Perp(glm::vec2 a) {
	glm::vec2 result = {-a.y, a.x};
	return (result);
}

//Get a vector that points along the polygon starting at point vertNum
inline glm::vec2
EdgeDirection(int32 vertNum, collision_polygon *p) {
	glm::vec2 v1 = p->vertices[vertNum];
	glm::vec2 v2 = p->vertices[(vertNum+1)%p->numVerts];
	glm::vec2 result = v2 - v1;
	return (result);
}

//Calculate the dot products of the points on the polygon to the axis (to determine if they exist on both sides)
void
CalculateInterval(glm::vec2 axis, collision_polygon *p, float *min, float *max) {
	float dot = Inner(axis, p->vertices[0]);
	*min = *max = dot;
	for (int vertexIndex = 1; vertexIndex < p->numVerts; vertexIndex++) {
		dot = Inner(axis, p->vertices[vertexIndex]);
		if (dot < *min)
			*min = dot;
		else if (dot > *max)
			*max = dot;
	}
}

void
CalculateInterval(glm::vec2 axis, collision_circle *c, float *min, float *max) {
	glm::vec2 normalizedAxis = glm::normalize(axis);
	float dot1 = Inner(axis, c->center + c->radius*normalizedAxis);
	float dot2 = Inner(axis, c->center - c->radius*normalizedAxis);
	if (dot1 < dot2)
	{
		*min = dot1;
		*max = dot2;
	} else {
		*min = dot2;
		*max = dot1;
	}
}

//Get the dot products between the tested axis and all the points of the polygons
//and find out if there is no overlap
bool
AxisSeparatesPolygons(glm::vec2 *axis, collision_polygon *p1, collision_polygon *p2) {
	float min1, max1, min2, max2;
	CalculateInterval(*axis, p1, &min1, &max1);
	CalculateInterval(*axis, p2, &min2, &max2);

	if (min1 > max2 || min2 > max1)
		return true;

	float d0 = max1 - min2;
	float d1 = max2 - min1;

	float overlap = (d0 < d1) ? d0 : d1;
	float axisLengthSquared = Inner(*axis, *axis);

	//Get the size of the overlap
	*axis *= overlap / axisLengthSquared;
	return false;
}

bool
AxisSeparatesPolygons(glm::vec2 *axis, collision_circle *c, collision_polygon *p) {
	float min1, max1, min2, max2;
	CalculateInterval(*axis, c, &min1, &max1);
	CalculateInterval(*axis, p, &min2, &max2);

	if (min1 > max2 || min2 > max1)
		return true;

	float d0 = max1 - min2;
	float d1 = max2 - min1;

	float overlap = (d0 < d1) ? d0 : d1;
	float axisLengthSquared = Inner(*axis, *axis);

	*axis *= overlap / axisLengthSquared;
	return false;
}

//find the smallest separation vector
glm::vec2
FindMinimumTranslationDistance(glm::vec2 *axes, int numAxes) {
	glm::vec2 result = axes[0];
    float minD2 = LengthSq(axes[0]);
    for(int index = 1; index < numAxes; index++) 
    { 
        float d2 = LengthSq(axes[index]); 
        if (d2 < minD2) 
        { 
            minD2 = d2; 
			result = axes[index];
        } 
    } 
    return result; 
}


int _MaxPointInDirection(collision_polygon *p1, vec2 dir)
{
	int index = 0;
	float maxD = Inner(p1->vertices[index], dir);

	for (int i = 1; i < p1->numVerts; i++)
	{
		float d = Inner(p1->vertices[i], dir);
		if (d > maxD)
		{
			index = i;
			maxD = d;
		}
	}
	return index;
}

vec2 _GetSupport(collision_polygon *p1, collision_polygon *p2, vec2 dir)
{
	int index1 = _MaxPointInDirection(p1, dir);
	int index2 = _MaxPointInDirection(p2, -dir);
	
	vec2 result = p1->vertices[index1] - p2->vertices[index2];
	return result;
}

vec2 _GetSupport(collision_circle *c, collision_polygon *p, vec2 dir)
{
	//max point in direction for circle is center + normalized(direction) * radius
	vec2 maxDirectionForCircle = c->center + c->radius * glm::normalize(dir);
	int index = _MaxPointInDirection(p, -dir);
	
	vec2 result = maxDirectionForCircle - p->vertices[index];
	return result;
}

vec2 TripleVectorProduct(vec2 a, vec2 b, vec2 c)
{
    vec2 r;
    
    float ac = a.x * c.x + a.y * c.y; // perform a.dot(c)
    float bc = b.x * c.x + b.y * c.y; // perform b.dot(c)
    
    // perform b * a.dot(c) - a * b.dot(c)
    r.x = b.x * ac - a.x * bc;
    r.y = b.y * ac - a.y * bc;
    return r;
}

bool _DoSimplex(vec2 *simplex, int *simplexSize, vec2 *direction)
{
	bool result = false;
	switch (*simplexSize)
	{
		case 2:
		{
			//Find vector pointing toward the origin
			//vector triple product
			vec2 linesegAB = simplex[0] - simplex[1];
			//BA x B0 x BA
			*direction = TripleVectorProduct(linesegAB, -simplex[1], linesegAB);
			
            if (Inner(*direction, *direction) == 0)
                *direction = vec2(linesegAB.y, -linesegAB.x);
		} break;

		case 3:
		{
			vec2 linesegAB = simplex[1] - simplex[2];
			vec2 linesegAC = simplex[0] - simplex[2];

			vec2 acPerp = TripleVectorProduct(linesegAB, linesegAC, linesegAC);
			if (Inner(acPerp, -simplex[2]) >= 0)
			{
				*direction = acPerp;
			}
			else
			{
				vec2 abPerp = TripleVectorProduct(linesegAC, linesegAB, linesegAB);
				if (Inner(abPerp, -simplex[2]) < 0)
				{
					return true;
				}

				simplex[0] = simplex[1];
				*direction = abPerp;
			}
			simplex[1] = simplex[2];
			(*simplexSize)--;
			
		} break;
	}
	return result;
}

bool
_ColShapeIntersectsGJK(collision_polygon *p1, collision_polygon *p2) {
	vec2 dir = p1->vertices[0];
	vec2 support = _GetSupport(p1, p2, dir);
	vec2 simplex[3];
	int simplexSize = 0;
	simplex[simplexSize++] = support;

	vec2 direction = -simplex[0];
	while (true)
	{
		vec2 support = _GetSupport(p1, p2, direction);
		if (Inner(support, direction) <= 0)
			return false;
		simplex[simplexSize++] = support;
		if (_DoSimplex(simplex, &simplexSize, &direction))
			return true;
	}
}


bool
_ColShapeIntersectsGJK(collision_circle *c, collision_polygon *p) {
	vec2 dir = p->vertices[0];
	vec2 support = _GetSupport(c, p, dir);
	vec2 simplex[3];
	int simplexSize = 0;
	simplex[simplexSize++] = support;

	vec2 direction = -simplex[0];
	while (true)
	{
		vec2 support = _GetSupport(c, p, direction);
		if (Inner(support, direction) <= 0)
			return false;
		simplex[simplexSize++] = support;
		if (_DoSimplex(simplex, &simplexSize, &direction))
			return true;
	}
}


//Find out if there is an axis that separates two convex polygons.
//If so, find the minimum vector that will separate the two
bool
_ColShapeIntersects(collision_polygon *p1, collision_polygon *p2, glm::vec2 *separationVector) {
	/*collision_polygon poly1 = p1->GetTransformedPoly();
	collision_polygon poly2 = p2->GetTransformedPoly();*/

	/*
	glm::vec2 separationVectors[32];
	int numVectors = 0;

	for (int vertIndex = 0; vertIndex < p1->numVerts; vertIndex++) {
		separationVectors[numVectors] = Perp(EdgeDirection(vertIndex, p1));
		if (AxisSeparatesPolygons(&separationVectors[numVectors], p1, p2))
			return false;
		numVectors++;
	}
	for (int vertIndex = 0; vertIndex < p2->numVerts; vertIndex++) {
		separationVectors[numVectors] = Perp(EdgeDirection(vertIndex, p2));
		if (AxisSeparatesPolygons(&separationVectors[numVectors], p1, p2))
			return false;
		numVectors++;
	}

	if (separationVector)
	{
		*separationVector = FindMinimumTranslationDistance(separationVectors, numVectors);

		glm::vec2 delta = p1->center - p2->center;
		if (Inner(delta, *separationVector) < 0) {
			*separationVector *= -1;
		}
	}

	return true;
	*/
	return _ColShapeIntersectsGJK(p1, p2);
}

bool
_ColShapeIntersects(collision_circle *c, collision_polygon *p, glm::vec2 *separationVector) {
	/*
	glm::vec2 separationVectors[32];
	int numVectors = 0;

	for (int vertIndex = 0; vertIndex < p->numVerts; vertIndex++) {
		separationVectors[numVectors] = c->center - p->vertices[vertIndex];
		if (AxisSeparatesPolygons(&separationVectors[numVectors], c, p))
			return false;
		numVectors++;
	}
	for (int vertIndex = 0; vertIndex < p->numVerts; vertIndex++) {
		separationVectors[numVectors] = Perp(EdgeDirection(vertIndex, p));
		if (AxisSeparatesPolygons(&separationVectors[numVectors], c, p))
			return false;
		numVectors++;
	}

	if (separationVector)
	{
		*separationVector = FindMinimumTranslationDistance(separationVectors, numVectors);

		glm::vec2 delta = c->center - p->center;
		if (Inner(delta, *separationVector) < 0) {
			*separationVector *= -1;
		}
	}

	return true;*/
	return _ColShapeIntersectsGJK(c, p);
}


bool
_ColShapeIntersects(collision_circle *c1, collision_circle *c2, glm::vec2 *separationVector)
{
	float distanceFromCenterToCenter = glm::length(c2->center - c1->center);
	if (distanceFromCenterToCenter <= c1->radius + c2->radius)
	{
		if (separationVector)
			*separationVector = (1.0f / distanceFromCenterToCenter) * (c1->center - c2->center) * (c1->radius + c2->radius - distanceFromCenterToCenter);
		return true;
	}
	return false;
}

bool
_ColPointInside(CollisionObject *s, glm::vec2 point)
{
	bool result = true;
	switch (s->type)
	{
		case CollisionObject::AABB:
		{
			result = (point.x >= s->aabb.min.x &&
					  point.x <= s->aabb.max.x &&
					  point.y >= s->aabb.min.y &&
					  point.y <= s->aabb.max.y);
		} break;

		case CollisionObject::POINT:
		{
			result = (s->point == point);
		} break;

		case CollisionObject::CIRCLE:
		{
			collision_circle *c = &s->circle;
			result = (LengthSq(point - c->center) < (c->radius*c->radius));
		} break;

		case CollisionObject::POLYGON:
		{
			collision_polygon *p = &s->poly;
			for (int i = 0; i < p->numVerts; i++)
			{
				vec2 segPerp = Perp(p->vertices[(i+1)%p->numVerts] - p->vertices[i]);
				vec2 pointVec = point - p->vertices[i];
				if (Inner(segPerp, pointVec) > 0)
				{
					result = false;
					break;
				}
			}
		} break;
	}
	return result;
}

bool
ColShapesIntersect(CollisionObject *s1, CollisionObject *s2, glm::vec2 *separationVector) 
{
	//Convert AABB type to polygon if we're testing against any other type
	CollisionObject transformedS1;
	CollisionObject transformedS2;
	if (s1->type == CollisionObject::AABB && (s2->type == CollisionObject::CIRCLE || s2->type == CollisionObject::POLYGON))
	{
		transformedS1.type = CollisionObject::POLYGON;
		transformedS1.poly.numVerts = 0;
		PolygonAddVertex(&transformedS1.poly, {s1->aabb.min.x, s1->aabb.min.y});
		PolygonAddVertex(&transformedS1.poly, {s1->aabb.max.x, s1->aabb.min.y});
		PolygonAddVertex(&transformedS1.poly, {s1->aabb.max.x, s1->aabb.max.y});
		PolygonAddVertex(&transformedS1.poly, {s1->aabb.min.x, s1->aabb.max.y});
		transformedS1.poly.center = {s1->aabb.min.x + (s1->aabb.max.x - s1->aabb.min.x)*0.5f, s1->aabb.min.y + (s1->aabb.max.y - s1->aabb.min.y)*0.5f};
		s1 = &transformedS1;
	}	
	
	if (s2->type == CollisionObject::AABB && (s1->type == CollisionObject::CIRCLE || s1->type == CollisionObject::POLYGON))
	{
		transformedS2.type = CollisionObject::POLYGON;
		transformedS2.poly.numVerts = 0;
		PolygonAddVertex(&transformedS2.poly, {s2->aabb.min.x, s2->aabb.min.y});
		PolygonAddVertex(&transformedS2.poly, {s2->aabb.max.x, s2->aabb.min.y});
		PolygonAddVertex(&transformedS2.poly, {s2->aabb.max.x, s2->aabb.max.y});
		PolygonAddVertex(&transformedS2.poly, {s2->aabb.min.x, s2->aabb.max.y});
		transformedS2.poly.center = {s2->aabb.min.x + (s2->aabb.max.x - s2->aabb.min.x)*0.5f, s2->aabb.min.y + (s2->aabb.max.y - s2->aabb.min.y)*0.5f};
		s2 = &transformedS2;
	}

	switch (s1->type)
	{
		case CollisionObject::AABB:
		{
			switch (s2->type)
			{
				case CollisionObject::AABB:
				{
					return (s1->aabb.min.x < s2->aabb.max.x &&
							s1->aabb.max.x > s2->aabb.min.x &&
							s1->aabb.min.y < s2->aabb.max.y &&
							s1->aabb.max.y > s2->aabb.min.y);
				} break;

				case CollisionObject::POINT:
				{
                    glm::vec2 point = s2->point;
					return _ColPointInside(s1, point);
				} break;

				default:
				{
					assert(false);
				} break;
			}

		} break;

		case CollisionObject::POINT:
		{
			switch (s2->type)
			{
				case CollisionObject::AABB: 
				{
					return _ColPointInside(s2, s1->point);
				} break;

				case CollisionObject::POINT:
				{
					return (s1->point == s2->point);
				} break;

				case CollisionObject::CIRCLE:
				{
					return _ColPointInside(s2, s1->point);
				} break;

				case CollisionObject::POLYGON:
				{
					return _ColPointInside(s2, s1->point);
				} break;
			}
		} break;

		case CollisionObject::CIRCLE:
		{
			switch (s2->type)
			{
				case CollisionObject::AABB: { assert(false); } break;

				case CollisionObject::POINT:
				{
					return _ColPointInside(s1, s2->point);
				} break;

				case CollisionObject::CIRCLE:
				{
					return _ColShapeIntersects(&s1->circle, &s2->circle, separationVector);
				} break;

				case CollisionObject::POLYGON:
				{
					return _ColShapeIntersects(&s1->circle, &s2->poly, separationVector);
				} break;
			}

		} break;

		case CollisionObject::POLYGON:
		{
			switch (s2->type)
			{
				case CollisionObject::AABB: { assert(false); } break;

				case CollisionObject::POINT:
				{
					return _ColPointInside(s1, s2->point);
				} break;

				case CollisionObject::CIRCLE:
				{
					return _ColShapeIntersects(&s2->circle, &s1->poly, separationVector);
				} break;

				case CollisionObject::POLYGON:
				{
					return _ColShapeIntersects(&s1->poly, &s2->poly, separationVector);
				} break;
			}
		} break;
	}

	return false;
}

collision_aabb GetAabbForObject(CollisionObject *shape)
{
	collision_aabb result;
	result.initialized = false;

	switch (shape->type)
	{
		case CollisionObject::POINT:
		{
			AabbAddPoint(&result, shape->point);
		} break;

		case CollisionObject::CIRCLE:
		{
			AabbAddPoint(&result, shape->circle.center - vec2(shape->circle.radius, shape->circle.radius));
			AabbAddPoint(&result, shape->circle.center + vec2(shape->circle.radius, shape->circle.radius));
		} break;

		case CollisionObject::POLYGON:
		{
			for (int i = 0; i < shape->poly.numVerts; i++)
			{
				AabbAddPoint(&result, shape->poly.vertices[i]);
			}
		} break;

		case CollisionObject::AABB:
		{
			AabbGrowToContain(&result, &shape->aabb);
		}
	}

	return result;
}


void AabbAddPoint(collision_aabb* aabb, const vec2& point)  {
	if (!aabb->initialized)
	{
		aabb->initialized = true;
		aabb->min.x = point.x;
		aabb->max.x = point.x;
		aabb->min.y = point.y;
		aabb->max.y = point.y;
	}
	else
	{
		aabb->max.x = glm::max(aabb->max.x, point.x);
		aabb->max.y = glm::max(aabb->max.y, point.y);
		aabb->min.x = glm::min(aabb->min.x, point.x);
		aabb->min.y = glm::min(aabb->min.y, point.y);
	}
}

void AabbGrowToContain(collision_aabb* aabb, collision_aabb* other) {
	if (!aabb->initialized)
	{
		aabb->max.x = other->max.x;
		aabb->max.y = other->max.y;
		aabb->min.x = other->min.x;
		aabb->min.y = other->min.y;
	}
	else
	{
		aabb->max.x = glm::max(aabb->max.x, other->max.x);
		aabb->max.y = glm::max(aabb->max.y, other->max.y);
		aabb->min.x = glm::min(aabb->min.x, other->min.x);
		aabb->min.y = glm::min(aabb->min.y, other->min.y);
	}
}
