static int array[4];

int* alloc4(int** p, int a, int b, int c, int d)
{
  array[0] = a;
  array[1] = b;
  array[2] = c;
  array[3] = d;
  *p = array;
  return array;
}
