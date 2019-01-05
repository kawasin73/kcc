# kcc

[![Build Status](https://travis-ci.com/kawasin73/kcc.svg?branch=master)](https://travis-ci.com/kawasin73/kcc)

C compiler written by C.

## spec

it supports intel x86_64 on **macOS**. not Linux.
it requires `make` and `gcc` (which is installed by default on macOS).

it is tested on Macbook Pro (13-inch, 2018), macOS: `10.14.2`, Processor Name: `Intel Core i7`.

## how to run

```bash
$ git clone https://github.com/kawasin73/kcc.git
$ cd kcc
$ make test # run test
$ make
$ ./kcc "int printf();int main(){printf(\"hello world\n\");return 0;}" > tmp.s && gcc -o tmp tmp.s test/head.o && ./tmp # hello world
```

## inspired by

- https://www.sigbus.info/compilerbook/
- https://github.com/rui314/9cc
- https://github.com/ushitora-anqou/aqcc

## LICENSE

MIT
