#!/usr/bin/env bash
##
## Copyright (c) 2012, 2016 The University of Utah
## All rights reserved.
##
## This file is distributed under the University of Illinois Open Source
## License.  See the file COPYING for details.

###############################################################################

rm -f out*.txt

ulimit -t 3
ulimit -v 2000000

if 
  clang -pedantic -Wall -O0 -c small.c  >out.txt 2>&1 &&\
  ! grep 'conversions than data arguments' out.txt &&\
  ! grep 'incompatible redeclaration' out.txt &&\
  ! grep 'ordered comparison between pointer' out.txt &&\
  ! grep 'eliding middle term' out.txt &&\
  ! grep 'end of non-void function' out.txt &&\
  ! grep 'invalid in C99' out.txt &&\
  ! grep 'specifies type' out.txt &&\
  ! grep 'should return a value' out.txt &&\
  ! grep 'uninitialized' out.txt &&\
  ! grep 'incompatible pointer to' out.txt &&\
  ! grep 'incompatible integer to' out.txt &&\
  ! grep 'type specifier missing' out.txt &&\
  gcc -Wall -Wextra -O1 small.c  >outa.txt 2>&1 &&\
  ! grep uninitialized outa.txt &&\
  ! grep 'without a cast' outa.txt &&\
  ! grep 'control reaches end' outa.txt &&\
  ! grep 'return type defaults' outa.txt &&\
  ! grep 'cast from pointer to integer' outa.txt &&\
  ! grep 'useless type name in empty declaration' outa.txt &&\
  ! grep 'no semicolon at end' outa.txt &&\
  ! grep 'type defaults to' outa.txt &&\
  ! grep 'too few arguments for format' outa.txt &&\
  ! grep 'incompatible pointer' outa.txt &&\
  ! grep 'ordered comparison of pointer with integer' outa.txt &&\
  ! grep 'declaration does not declare anything' outa.txt &&\
  ! grep 'expects type' outa.txt &&\
  ! grep 'pointer from integer' outa.txt &&\
  ! grep 'incompatible implicit' outa.txt &&\
  ! grep 'excess elements in struct initializer' outa.txt &&\
  ! grep 'comparison between pointer and integer' outa.txt &&\
  XXOPT1 small.c -o small1 > cc_out1.txt 2>&1 &&\
  RunSafely.sh 3 1 /dev/null out1.txt ./small1 >/dev/null 2>&1 &&\
  XXOPT2 small.c -o small2 > cc_out2.txt 2>&1 &&\
  RunSafely.sh 3 1 /dev/null out2.txt ./small2 >/dev/null 2>&1 &&\
  ! diff out1.txt out2.txt &&\
  kcc -s small.c >/dev/null 2>&1
then
  if 
    RunSafely.sh 125 1 /dev/null out_kcc.txt ./a.out >/dev/null 2>&1
  then
    exit 0
  else
    cat out_kcc.txt
    TMPF="undefined_$$.c"
    echo copying C file to $TMPF
    cp small.c $TMPF
    exit 1
  fi
else
  exit 1
fi
