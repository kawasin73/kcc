int printf();
int exit(int i);

#define EXPECT(expected, expr)                                      \
    do {                                                            \
        int e1 = (expected);                                        \
        int e2 = (expr);                                            \
        if (e1 == e2) {                                             \
            printf("%s => %d\n", #expr, e2);                        \
        } else {                                                    \
            printf("%d: %s: %d expected, but got %d\n",             \
                __LINE__, #expr, e1, e2);                           \
            exit(1);                                                \
        }                                                           \
    } while (0)

extern int global_arr[1];
int x=1;
int _x=2;
int *y;
int z[1024];
int z2[1024][2048];
char *abc = "abc";

int incr() {x=x+1;return x;}
int plus(int a, int b);

int fake() {}
int sum(int a, int b) {
    return a + b;
}
int mul6(int a, int b, int c, int d, int e, int f) {
    return a * b * c * d * e * f;
}
int div(int a, int b);
// TODO: change to maximum
int equal(int a, int b) {
    if (a == b)
        return a;
    else
        return b;
}
int *bigarray() {
    for (int i = 0; i != 1024; i = i + 1) {
        z[i] = i + 1;
    }
    return z+1;
}
int setarray(int *ary, int idx, int val) {
    return ary[idx] = val;
}


int main() {
    // calculation
    EXPECT(0, 0);
    EXPECT(2, 2);
    EXPECT(18, 5+20-4-3);
    EXPECT(47, 5+6*7);
    EXPECT(15, 5*(9-6));
    EXPECT(2, (3+5)/2/2);
    EXPECT(0, 10==5);
    EXPECT(1, 10==10);
    EXPECT(1, 10!=5);
    EXPECT(1, 2==2==1);
    EXPECT(1, 2==2!=0);

    // statement expression
    EXPECT(1, {1;});
    EXPECT(2, {return 2;});

    // assignment
    EXPECT(3, {int a=3;});
    EXPECT(3, {int a=3;return a;});
    EXPECT(3, {int _10=3;return _10;});
    EXPECT(3, {int inti=3;return inti;});
    EXPECT(10, {int a=2;int b=3+2;return a*b;});
    EXPECT(5, {int a;int b=a=2;return a+b+1;});
    EXPECT(25, {int a;int b;a=b=3*(3+1);return a+b+1;});

    // function call
    EXPECT(0, fake());
    EXPECT(3, sum(1, 2));
    EXPECT(720, mul6(1, 2, 3, 4, 5, 6));
    EXPECT(3, div(6, 2));
    EXPECT(38, sum(sum(1,2), sum(3, 4)*5));
    EXPECT(1, equal(1,1));
    EXPECT(3, equal(2,3));

    // global variable
    EXPECT(1, {return x;});
    EXPECT(2, {return _x;});
    EXPECT(2, {x=2;return x;});
    EXPECT(2, {x=2;return x;});
    EXPECT(4, {x=3;incr();return x;});

    // variable scope
    EXPECT(2,{int a=1;{a=2;}return a;});
    EXPECT(1,{int a=1;{int a=2;}return a;});
    EXPECT(1,{x=1;{int x=2;}return x;});
    EXPECT(2,{int x=4;return incr();});
    EXPECT(6,{int x=4;return x+_x;});
    EXPECT(1,{int a=1;if(a){int a=2;}return a;});
    EXPECT(1,{int a=1;do{int a=2;}while(0);return a;});

    // external
    EXPECT(3, plus(1, 2));
    EXPECT(5, global_arr[0]);

    // pointer
    EXPECT(2, {int a=2;int *b=&a;return *b;});
    EXPECT(2, {int a=2;int *b;b=&a;return *b;});
    EXPECT(3, {int a=2;int *b=&a;*b=3;return a;});
    EXPECT(2, {int a=2;int *b=&a;int **c=&b;return **c;});
    EXPECT(3, {int a=2;int *b=&a;int **c=&b;**c=3;return a;});
    EXPECT(1, {y=&x;*y=1;return x;});
    EXPECT(4, {int a=2;int *b=&a;return *b**b;});
    EXPECT(3, {int a=2;int*b=&a;b[0]=3;return b[0];});
    EXPECT(3, {int a=2;int*b=&a;b[0]=3;return a;});

    // array
    EXPECT(5, {int a[1024];a[0]=2;a[1023]=a[0]+1;return a[0]+a[1023];});
    EXPECT(5, {z[0]=2;z[1023]=z[0]+1;return z[0]+z[1023];});
    EXPECT(3, {int a[2];int*b=a;b[1]=3;return a[1];});
    EXPECT(5, {int a[2][3];a[0][0]=2;a[1][2]=a[0][0]+1;return a[0][0]+a[1][2];});
    EXPECT(7, {z2[0][0]=2;z2[1023][2047]=z2[0][0]+3;return z2[0][0]+z2[1023][2047];});
    EXPECT(5,{int a[2];*a=2;*(a+1)=*a+1;return *(a)+*(a+1);});
    EXPECT(3, {int a[2][3];a[1][2]=3;return *(*a+5);});
    EXPECT(3, {int a[2][3];a[1][2]=3;return *(a[1]+2);});
    EXPECT(2, bigarray()[0]);
    EXPECT(1024, bigarray()[1022]);
    EXPECT(3, {int ary[1024];setarray(ary, 1023, 3);return ary[1023];});

    // logand logor
    EXPECT(1, 10&&2);
    EXPECT(0, 0&&10);
    EXPECT(0, 10&&0);
    EXPECT(0, 0||0);
    EXPECT(1, 2||0);
    EXPECT(1, 0||2);
    EXPECT(2, {x=1;2&&incr();return x;});
    EXPECT(1, {x=1;0&&incr();return x;});
    EXPECT(1, {x=1;2||incr();return x;});
    EXPECT(2, {x=1;0||incr();return x;});

    // control syntax
    // if
    EXPECT(4,{int a=1;int b=2;if(a==1)b=4;return b;});
    EXPECT(2,{int a=1;int b=2;if(a==3)b=4;return b;});
    EXPECT(4,{int a=1;int b=2;if(a=3)b=4;return b;});
    EXPECT(3,{int a=1;int b=2;if(a=3)b=4;return a;});
    EXPECT(4,{int a=1;int b=2;if(a==1)b=4;else b=5;return b;});
    EXPECT(5,{int a=1;int b=2;if(a==3)b=4;else b=5;return b;});
    EXPECT(3,{if(0)return 2;else return 3;return 4;});
    EXPECT(2,{if(1)return 2;else return 3;return 4;});
    EXPECT(2,{if (1) {return 2;}else{return 3;}return 4;});
    EXPECT(5,{if (1) {int a=2;int b=3;return a+b;}return 4;});
    // for
    EXPECT(5, {int a=0;for(int i=0;i!=5;i=i+1)a=a+1;return a;});
    EXPECT(5, {int a=0;int i;for(i=0;i!=5;i=i+1)a=a+1;return a;});
    EXPECT(5, {int a=0;for(int i=0;i!=5;i=i+1){a=a+1;}return a;});
    EXPECT(5, {int a=0;for(int i=0;1;i=i+1){if(a==5)return a;a=a+1;}return a;});
    // do ... while
    EXPECT(5, {int a=0;do{a=a+1;}while(a!=5);return a;});
    EXPECT(10, {int a=0;do{a=10;}while(0);return a;});

    // sizeof
    EXPECT(8, sizeof(1));
    EXPECT(8, sizeof 1);
    EXPECT(8, {int a;return sizeof(a);});
    EXPECT(1, {char a;return sizeof(a);});
    EXPECT(8, {int *a;return sizeof(a);});
    EXPECT(40, {int a[5];return sizeof(a);});

    // char
    EXPECT(2, {char c=2;return c;});
    EXPECT(6, {char c=2;int a=3;c=a*c;return c;});
    EXPECT(9, {char c[3];c[0]=1;c[1]=3;c[2]=5;return c[0]+c[1]+c[2];});
    EXPECT(9, {char c[3];char *a=c;*a=1;*(a+1)=3;*(a+2)=5;return c[0]+c[1]+c[2];});

    // string literal
    EXPECT(97, {char *s="abc";return s[0];});
    EXPECT(98, {char *s="abc";return s[1];});
    EXPECT(99, {char *s="abc";return s[2];});
    EXPECT(0, {char *s="abc";return s[3];});
    EXPECT(97, {return abc[0];});
    EXPECT(98, {return abc[1];});
    EXPECT(99, {return abc[2];});
    EXPECT(0, {return abc[3];});
    EXPECT(7, {char *s="\a";return s[0];});
    EXPECT(8, {char *s="\b";return s[0];});
    EXPECT(12, {char *s="\f";return s[0];});
    EXPECT(10, {char *s="\n";return s[0];});
    EXPECT(13, {char *s="\r";return s[0];});
    EXPECT(9, {char *s="\t";return s[0];});
    EXPECT(11, {char *s="\v";return s[0];});
    EXPECT(34, {char *s="\"";return s[0];});
    EXPECT(39, {char *s="\'";return s[0];});
    EXPECT(92, {char *s="\\";return s[0];});
    
    // EXPECT(,);
    // EXPECT(,);
    // EXPECT(,);
    // EXPECT(,);
    // EXPECT(,);
    // EXPECT(,);
    // EXPECT(,);


    printf("OK\n");
}

int div(int a, int b) {
    return a / b;
}
