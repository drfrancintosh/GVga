#!/bin/bash

for i in *.bin; do
	node font2h.js "$i" $(basename $i .bin)_pico.h
done
