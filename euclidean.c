#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "llstruct.h"
#include "euclidean.h"

int isInRange(int receiverLocation, int myLocation, int rows, int columns)
{
  int x1, x2, y1, y2;
  int maxLocations = rows * columns;
  int isOutOfGridRange = receiverLocation > maxLocations || receiverLocation < 1;

  if (isOutOfGridRange)
    return 0;

  x1 = getXValue(columns, receiverLocation);
  x2 = getXValue(columns, myLocation);

  y1 = getYValue(x1, columns, receiverLocation);
  y2 = getYValue(x2, columns, myLocation);

  int dist = calculateEuclideanDistance(x1, x2, y1, y2);

  if (dist > 2)
    return 0;

  return 1;
}

int getXValue(int columns, int location)
{
  int x = location % columns;
  if (x == 0)
    x = columns;
  return x;
}

int getYValue(int x, int columns, int location)
{ // given the column/x value, # of columns, and location #
  return (location - x) / columns + 1;
}

int calculateEuclideanDistance(int x1, int x2, int y1, int y2)
{
  double a = (x2 - x1) * (x2 - x1);
  double b = (y2 - y1) * (y2 - y1);
  double dist = sqrt(a + b);
  return (int)trunc(dist);
}