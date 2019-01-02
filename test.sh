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

try 0 "int main(){0;}"
try 42 "int main(){42;}"
try 18 "int main(){5+20-4-3;}"
try 41 " int main(){ 12 + 34 - 5 ; } "
try 41 " int main(){ int a1 = 12 ; int a2 = 34 - 5 ; return a1 + a2; } "
try 47 "int main(){5+6*7;}"
try 15 "int main(){5*(9-6);}"
try 2 "int main(){(3+5)/2/2;}"
try 2 "int main(){int a=2; a;}"
try 10 "int main(){int a=2;int b=3+2;a*b;}"
try 25 "int main(){int a; int b;a=b=3*(3+1);a+b+1;}"
try 1 "int main(){int a=2;int b=3; a=b=1;return a;}"
try 1 "int main(){int a=2;int b=3; a=b=1;return b;}"
try 0 "int main(){10==5;}"
try 1 "int main(){10==10;}"
try 1 "int main(){10!=5;}"
try 1 "int main(){return 2==2==1;}";
try 1 "int main(){return 2==2!=0;}"
try 1 "int main(){int a=10;a==10;}"
try 69 "int main(){int abc=3;int _23=23;abc*_23;}"
try 5 "int main(){int a=10;int b=0;if(a==10)b=5;b;}"
try 5 "int main(){int a=0;if(a=10)a=5;a;}"
try 0 "int main(){int a=0;if(2*(1-1))a=5;a;}"
try 1 "int main(){int if0=1;if0;}"
try 2 "int main(){int a=0;if(0)a=1;else a=2;a;}"
try 5 "int main(){int a=1; if(a){int b=2;a=3;return a+b;}else{int c=3; return a+c;}}"
try 5 "int main(){int a=1; if(a){int b=2;a=3;return a+b;}else{int b=3; return a+b;}}"
try 1 "int main(){ret1();}"
try 3 "int main(){ret1()+ret2();}"
try 3 "int main(){plus(1,2);}"
try 12 "int main(){plus(plus(1,2),3*(1+2));}"
try 1 "int main(){tmp(); 1;}int tmp(){}"
try 10 "int main(){int a=10;tmp(); tmp(); a;}int tmp(){int a=1;}"
try 11 "int main(){int a=10;int b=a+tmp();return b;}int tmp(){return 1;}"
try 1 "int main(){return tmp();}int tmp(){if(1)return 1;else return 2;}"
try 1 "int main(){return sum(1);}int sum(int a){return a;}"
try 9 "int main(){return sum(2,3,4);}int sum(int a,int b,int c){return a+b+c;}"
try 14 "int main(){return sum(2,3,4);}int sum(int a,int b,int c){int d=5;return a+b+c+d;}"
try 1 "int main(){int a;{a=1;}return a;}"
try 0 "int main(){int a = 0;{int a=1;}return a;}"
try 3 "int x;int y;int main(){x=1;y=2;return x+y;}"
try 2 "int x;int y;int main(){int y=2;return x+y;}"
try 3 "int x;int y=2;int main(){int x=1;return x+y;}"

echo OK
