src=sat.c sat-test.c parse.c component.c canonical.c prime.c basic.c
out=sat
path=-I lib
lib=lib/cache.c lib/linkedlist.c lib/hashtable.c

sat: sat.c sat.h sat-test.c parse.c parse.h component.c canonical.c canonical.h component.h prime.c prime.h basic.c basic.h
	@gcc -std=gnu99 -fbounds-check -fbounds-check -ftree-vectorize -O3 -o ${out} ${src} ${path} ${lib}

