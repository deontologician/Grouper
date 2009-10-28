"""This is to do a full benchmark on the packet filter as well as various
functions to help in building different kinds of tests"""

from math import log
from subprocess import Popen, PIPE, STDOUT
from optparse import OptionParser
from decimal import Decimal
import os
import sys
import random

def ceil_div(num, denom):
    """Returns the result of division by integers rounded up"""
    return (num + denom - 1) / denom

def mem_levels(bitlength = 104, rules = 10000, max_space = 3500000000):
    """This is a generator for the number of tables needed for a given packet
    length and number of rules. It generates from minimum space needed up to
    maximum_space as well as the number of tables for each memory size
    
    Args:
    rules : number of rules in the policy file. defaults to 10,000
    bitlength: number of relevant bits in each rule. defaults to 104
    max_space: maximum space to work in. defaults to 4GB
    
    Yields:
    (number of tables, amount of space for it)"""

    if bitlength <= 0 or rules <= 0 or max_space <= 0 : return
    
    max_tables = ceil_div(bitlength,2)
    
    for t in xrange(max_tables,2,-1):
        bmodt = bitlength % t
        bdivt = bitlength / t
        total_bits = rules * ((t-bmodt)*(2**bdivt) + bmodt*(2**(bdivt+1)))
        total_bytes = ceil_div(total_bits, 8)
        if total_bytes <= max_space:            
            yield (t, total_bytes)
        else:
            return
    # handle the 1 table case if we haven't returned yet
    table_bytes = ceil_div((2**bitlength)*int(log(rules,2)), 8)
    if table_bytes <= max_space:
        yield (1, table_bytes)
    return 

def make_rule_file(bits, rules):
    """Create a random rule file with the number of bits and rules specified"""
    with open('%dbx%drules.pol' % (bits,rules),'w') as rulefile:
        rulefile.write('%d\n' % ceil_div(bits, 8))
        for _ in xrange(rules):
            for _ in xrange(bits):
                # implement weighting for symbols later
                rulefile.write(random.choice('10?'))
            rulefile.write('\n')
        return rulefile.name

executor = ""

def test_table_amounts(bits = 104, rules = 10000, inputsize = 12,
                       max_size = 3500000000,
                       benchfile = 'benchmarks.txt',
                       programname = './tblcompile.profile'):
    """Run the tests for all relevant sizes of memory for the given
    parameters"""

    # create the random rule file to the specifications
    rule_filename = make_rule_file(bits, rules)
    print "Built %dx%d random rulefile" % (bits, rules)

    # create a random binary file of inputsize megabytes
    data_filename = '%dMBrandom.bin' % inputsize
    os.system('head --bytes=%d000000 /dev/urandom > %s' % 
              (inputsize, data_filename))
    print "Built random data file of size %dMB" % inputsize

    with open(benchfile, 'w') as resultfile:
        for _, mem in mem_levels(bits, rules, max_size):
            # run the benchmark to determine what the table build time is for
            # the current memory size
            print "Benching table build time..."
            buildbench = Popen('/usr/bin/time -f %%U %s %d %s %s %s' %
                (programname, mem, rule_filename, os.devnull, os.devnull), 
                               stderr=STDOUT, stdout=PIPE, shell=True)
            build_time = Decimal(buildbench.communicate()[0])

            #run the benchmark for the current memory size over the input
            print "Benching processing time..."
            runbench = Popen('/usr/bin/time  -f %%U %s %d %s %s %s' % 
                              (programname, mem, rule_filename, data_filename,
                               os.devnull), 
                             stderr = STDOUT, stdout = PIPE, shell=True)
            run_time = Decimal(runbench.communicate()[0])
            
            #calculate final values
            process_time = run_time - build_time
            KBps = Decimal('%d000' % inputsize) / process_time
            pps = (Decimal('%d000000' % inputsize) / Decimal(ceil_div(bits,8)))\
                / process_time
  
            # append current run info to the benchmark file
            result1 = '%d bits, %d rules, %d Bytes for tables\n' % (bits,rules,mem)
            result2 = '\t%s - %s = %s = %s KBps and %s pps\n\n' %\
                (str(run_time), str(build_time), str(process_time), 
                 str(KBps.quantize(Decimal('.01'))),# need these to show
                 str(pps.quantize(Decimal('.01')))) # less precision
            result = "".join([result1,result2])
            print result
            resultfile.write(result)
        

if __name__ == '__main__':

    parser = OptionParser()
    parser.add_option('-o','--output',dest='outfile',default = 'benchmark.txt',
                      help = 'File to write benchmark results to',)
    parser.add_option('-b','--bits', type='int', dest='bits', default = 104,
                      help = 'Number of relevant bits in a pattern')
    parser.add_option('-r','--rules',type='int',dest='rules', default = 10000,
                      help = 'Number of rules to generate')
    parser.add_option('-p','--program',dest='programname', 
                      default = './tblcompile.profile',
                      help = 'Name of program to benchmark')
    parser.add_option('-l','--mem-limit', dest='memlimit', type = 'int',
                      default = 3500000000,
                      help = 'Maximum memory allowed for tables')
    (options, args) = parser.parse_args(sys.argv)

    test_table_amounts(bits = options.bits, rules = options.rules, 
                       max_size = options.memlimit, benchfile = options.outfile,
                       programname = options.programname)
