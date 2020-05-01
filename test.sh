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

assert 0 0
assert 42 42
assert 21 '5+20-4'
assert 41 ' 12 + 34 -5 '
assert 14 '10/2+3*3'
assert 6 '10/(2+3)*3'
assert 24 '((1+2)+3)*4'
assert 85 '(43-8)*3-20'
assert 16 '((-1+2)+3)*4'
assert 1 '-1--2'
assert 15 '-3*+5/-1'
assert 6 '(3==1+2) + (3!=1+3) + (3>2) + (3>=1) + (3<4) + (3<=5)'
assert 0 '(3!=1+2) + (3==1+3) + (3<=2) + (3<1) + (3>=4) + (3>5)'

echo OK
