#!/bin/sh

swig -o _prelude.c -python -interface _prelude -module _prelude -noproxy ../libprelude.i
