#! /bin/bash

assert() {
	expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s
  gcc -o tmp tmp.s
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
}

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

echo OK
