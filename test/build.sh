#!/bin/sh

PPM_FILES="$(find . -regex '.*\.ppm')"
! [ -z "$PPM_FILES" ] && rm *.ppm

gcc -I../include test.c -o run_test -ggdb -limc
