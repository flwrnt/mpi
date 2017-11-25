#!/bin/python

from __future__ import print_function
import sys
from random import randint

def generateOne(a, factor = 1, end = False):
    a = randint(factor, 10)
    if not end: 
        print(a, end=' ')
    else:
        print(a)
    return a

def generateSeveral(n):
    a = generateOne(0, 10)
    for i in range(1, n):
        if i == n-1:
            a = generateOne(a, 0, True)
        else:
            a = generateOne(a,0)

n = int(sys.argv[1]);
print(n)
generateSeveral(n*n)
generateSeveral(n*n)
