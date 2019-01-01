#!/bin/bash

try() {
    expected="$1"
    input="$2"
    ./kcc "$input" > tmp.s
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

try 0 "0;"
try 42 "42;"
try 21 "5+20-4;"
try 41 " 12 + 34 - 5 ; "
try 47 "5+6*7;"
try 15 "5*(9-6);"
try 4 "(3+5)/2;"
try 2 "a=2; a;"
try 10 "a=2; b=3+2; a*b;"
try 25 "a=b=3*(3+1);a+b+1;"
try 0 "10==5;"
try 1 "10==10;"
try 1 "10!=5;"
try 1 "a=10;a==10;"
try 69 "abc=3;_23=23;abc*_23;"
try 5 "a=10;b=0;if(a==10)b=5;b;"
try 5 "a=0;if(a=10)a=5;a;"
try 0 "a=0;if(2*(1-1))a=5;a;"
try 1 "if0=1;if0;"

echo OK
