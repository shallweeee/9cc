#! /bin/bash

HELPER=
if [ -f helper.c ]; then
  make helper.o
  HELPER=helper.o
fi

assert() {
	expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s
  gcc -o tmp tmp.s $HELPER
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

test_old() {
assert 0 '0;'
assert 42 '42;'
assert 21 '5+20-4;'
assert 41 ' 12 + 34 -5 ;'
assert 14 '10/2+3*3;'
assert 6 '10/(2+3)*3;'
assert 24 '((1+2)+3)*4;'
assert 85 '(43-8)*3-20;'
assert 16 '((-1+2)+3)*4;'
assert 1 '-1--2;'
assert 15 '-3*+5/-1;'
assert 6 '(3==1+2) + (3!=1+3) + (3>2) + (3>=1) + (3<4) + (3<=5);'
assert 0 '(3!=1+2) + (3==1+3) + (3<=2) + (3<1) + (3>=4) + (3>5);'
assert 12 'a = 1; b = 2; z = a + b; z * 4;'
assert 12 'foo = 1; bar = 2; foobar = foo + bar; foobar * 4;'
assert 12 'foo = 1; bar = 2; foobar = foo + bar; return foobar * 4;'
assert 2 'foo = 1; if (foo == 1) foo = 2; else foo = 3;'
assert 3 'foo = 2; if (foo == 1) foo = 2; else foo = 3;'
assert 10 'foo = 0; while (foo < 10) foo = foo + 1; return foo;'
assert 55 'foo = 0; for (count = 1; count <= 10; count = count + 1) foo = foo + count; return foo;'
assert 55 'count = 1; foo = 0; while (count <= 10) { foo = foo + count; count = count + 1; } foo;'

assert 4 'main() { return 4; }'
assert 5 'test() { return 5; } main() { return test(); }'
assert 10 'test(a,b,c,d) { return a+b+c+d; } main() { return test(1,2,3,4); }'
assert 21 'test(a,b,c,d,e,f) { return a+b+c+d+e+f; } main() { return test(1,2,3,4,5,6); }'
assert 9  'test(a,b,c,d,e,f) { return a+b+c+d+e-f; } main() { return test(1,2,3,4,5,6); }'
assert 24 'fact(n) { val = 1; for (i = n; i > 1; i = i - 1) val = val * i; return val; } main() { return fact(4); }'
assert 12 'a(n){return n + 1;} b(n){return n*2;} main(){c=a(3)+b(4); return c;}'
assert 55 'r(n){if(n==1)return n; else return n + r(n-1);} main(){return r(10);}'
assert 5 'fibo(n) { if (n < 2) return n; else fibo(n - 1) + fibo(n - 2); } main() { return fibo(5); }'
assert 13 'fibo(n) { if (n < 2) return n; else fibo(n - 1) + fibo(n - 2); } main() { return fibo(7); }'
assert 233 'fibo(n) { if (n < 2) return n; else fibo(n - 1) + fibo(n - 2); } main() { return fibo(13); }'
assert 3 'main() {x = 3; y = &x; return *y; }'
assert 3 'main() {x = 3; y = 5; z = &y + 4; return *z; }'
}

assert 12 'int add(int a, int b, int c) { return a+b+c; } int main() { int x; int y; x=3; y=4; return add (x, y, 5); }'
assert 233 'int fibo(int n) { if (n<2) return n; else fibo(n-1) + fibo(n-2); } int main() { return fibo(13); }'
assert 3 'int main() { return 3; }'
assert 3 'int main() { int x; int* y; y = &x; *y = 3; return x; }'
assert 8 'int main() { int *p; alloc4(&p,1,2,4,8); int *q; q=p+2; *q; q=p+3; return *q; }'
assert 4 'int main() { int *p; alloc4(&p,1,2,4,8); int *q; q=p+3; q = q-1; return *q; }'
assert 4 'int main() { int x; return sizeof x; }'
assert 4 'int main() { int x; return sizeof(x + 3); }'
assert 4 'int main() { int* x; return sizeof(x); }'
assert 4 'int main() { return sizeof(sizeof(1)); }'
assert 40 'int main() { int ar[10]; return sizeof(ar); }'
assert 1 'int main() {int a2; int a1; int* q; q = &a1; int* p; p=q; *p=1; return *p;}'
assert 3 'int main() {int a2; int a1; a1=1; a2=2; int *p; p=&a1; return *(p+0) + *(p+1);}'
assert 3 'int main() {int a[2]; *(a+0)=1; *(a+1)=2; int *p; p=a; return *(p+0) + *(p+1);}'
assert 3 'int main() {int a[2]; *a=1; *(a+1)=2; int *p; p=a; return *p + *(p+1);}'
assert 3 'int main() {int a[2]; a[0]=1; a[1]=2; return a[0] + 1[a];}'
assert 1 'int g[2]; int main() {g[0]=1; g[1]=2; int* p; p=g; return *(p+1) - *p;}'
assert 3 'int main() {char x[3]; x[0]=-1; x[1]=2; int y; y=4; return x[0]+y;}'

echo OK
