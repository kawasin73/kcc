#!/bin/bash

try() {
    expected="$1"
    input="$2"
    ./kcc "$input" > tmp.s
    gcc -o tmp tmp.s tmp-ret1.o tmp-ret2.o tmp-plus.o
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

echo 'int ret1() { return 1; }' | gcc -xc -c -o tmp-ret1.o -
echo 'int ret2() { return 2; }' | gcc -xc -c -o tmp-ret2.o -
echo 'int plus(int x, int y) { return x + y; }' | gcc -xc -c -o tmp-plus.o -

try 0 "main(){0;}"
try 42 "main(){42;}"
try 21 "main(){5+20-4;}"
try 41 " main(){ 12 + 34 - 5 ; } "
try 47 "main(){5+6*7;}"
try 15 "main(){5*(9-6);}"
try 4 "main(){(3+5)/2;}"
try 2 "main(){a=2; a;}"
try 10 "main(){a=2; b=3+2; a*b;}"
try 25 "main(){a=b=3*(3+1);a+b+1;}"
try 0 "main(){10==5;}"
try 1 "main(){10==10;}"
try 1 "main(){10!=5;}"
try 1 "main(){a=10;a==10;}"
try 69 "main(){abc=3;_23=23;abc*_23;}"
try 5 "main(){a=10;b=0;if(a==10)b=5;b;}"
try 5 "main(){a=0;if(a=10)a=5;a;}"
try 0 "main(){a=0;if(2*(1-1))a=5;a;}"
try 1 "main(){if0=1;if0;}"
try 2 "main(){a=0;if(0)a=1;else a=2;a;}"
try 1 "main(){ret1();}"
try 3 "main(){ret1()+ret2();}"
try 3 "main(){plus(1,2);}"
try 12 "main(){plus(plus(1,2),3*(1+2));}"
try 1 "main(){tmp(); 1;}tmp(){}"

echo OK
