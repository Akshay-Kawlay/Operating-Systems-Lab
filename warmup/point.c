#include <assert.h>
#include "common.h"
#include "point.h"
#include <math.h>

void
point_translate(struct point *p, double x, double y)
{
	p = point_set(p,point_X(p)+x,point_Y(p)+y);
	
}

double
point_distance(const struct point *p1, const struct point *p2)
{
	double dist;
	dist = sqrt(pow(point_Y(p2)-point_Y(p1),2) + pow(point_X(p2)-point_X(p1),2));
	return dist;
}

int
point_compare(const struct point *p1, const struct point *p2)
{
	double distP1, distP2;
	distP1 = sqrt(pow(point_Y(p1),2) + pow(point_X(p1),2));
	distP2 = sqrt(pow(point_Y(p2),2) + pow(point_X(p2),2));	
	
	if(distP1 > distP2)
	return 1;
	else if (distP1 == distP2)
	return 0;
	else if(distP1 < distP2)
	return -1;
	
	return 2;
}
