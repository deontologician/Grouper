#!/usr/bin/env python

# This creates a random rules file of arbitrary size

import random as r
import os 
import sys
import getopt

def main(argv):
    bytenum = int(argv[0])
    rulefile = open(argv[1],"w")
    numrules = int(argv[2])
    rulefile.write("%d\n" % bytenum)
    for i in xrange(numrules):
        for j in xrange(bytenum * 8):
            rulefile.write(r.choice('10?'))
        rulefile.write('\n')
    rulefile.close()
    
if __name__ == "__main__":
    main(sys.argv[1:])
