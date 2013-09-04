#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/* Input:


   double *time:  array of measurement results
   int n:         number of array elements in time
   double alpha:  number in [0, 0.5) that specifies how many array elements
                  should NOT be considered - the smallest and greatest 
                     floor(alpha * n)
                  elements are dropped.

   Result:

                  Average of the values in the array without the
                     floor(alpha * n)
                  greatest and floor(alpha * n) smallest elements.

*/


static int compr(const void *ii, const void *jj)
{
  double i =  *(double*)ii;
  double j = *(double*)jj;

  return((i>j?1:(i==j?0:-1)));
}


double pruned_average(double *time, int n, double alpha)
{
  int drop, i;
  double sum, *tmp;

  tmp = (double *) malloc(sizeof(double) * n); 
  drop = (int)(alpha * n);
  if (tmp == 0) {
    puts("pruned_average: malloc failed");
    return 0;
  }
  if (alpha<0 || alpha>=0.5)
  { 
    puts("pruned_average: alpha not in [0, 0.5)");
    free(tmp);
    return 0;
  }

  for (i=0; i<n; i++)
  {
    tmp[i] = time[i];
  }
  qsort(tmp, n, sizeof(double), compr);

  sum = 0;
  for (i=drop; i<n-drop; i++)
    sum += tmp[i];

  if (2*drop>=n)
  {
    puts("pruned_average: alpha too big for n");
    free(tmp);
    return 0;
  }

  free(tmp);
  return sum/(n-2*drop);
}
