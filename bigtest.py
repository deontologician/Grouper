"""This is to do a full benchmark on the packet filter as well as various
functions to help in building different kinds of tests"""

from math import log, ceil
from subprocess import Popen, PIPE, STDOUT
from optparse import OptionParser
from decimal import Decimal
import os
import sys
import random
import csv

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

def min_bytes(rules, bits):
    "returns the minimum number of bytes needed for given rules and bits"
    return 2*ceil_div(rules,8)*bits

def max_bytes(rules, bits):
    "returns the max number of bytes needed for given rules and bits"
    return ceil_div(int(log(rules,2)),8) * 2**bits

def min_tables(m, n, b):
    """Returns the minimum tables needed for a policy of n rules and b bits
    given m bytes of memory to work with """
    N = ceil_div(n,8) # number of bytes needed to contain n rules
    high = ceil_div(b,2)
    low = 1
    while high - low > 1:
        mid = (high + low) / 2
        memForMid = (mid - (b % mid))*2**(b/mid)*N + \
            (b %mid)*2**((b/mid) + 1)*N
        if m < memForMid:
            low = mid
        else:
            high = mid
    return high, memForMid
        
def multi_d_test(mem_steps, rule_steps, bit_steps, programname = './tblcompile',
                 data_filename = '10MBrandom.bin',
                 test_filename = 'test_file.csv'):
    """Do multi-dimensional test with the given steps"""
    quant = Decimal('0.1') #number to round to when printing Decimal results
    with open(test_filename,'w') as test_file:
        writer = csv.writer(test_file)
        writer.writerow(['memory','rules','bits','kbps','pps','"table build time"',
                         '"table size"','"number of tables"'])
        for bits in bit_steps:
            for rules in rule_steps:
                rule_filename = make_rule_file(bits, rules)
                print "Working with %s now..." % rule_filename
                for mem in mem_steps:
                    print "%d bytes:" % mem
                    if mem < min_bytes(rules, bits):
                        print "Not enough memory for rules and bits given"
                        writer.writerow([mem, rules, bits, '', '', '', '', ''])
                        continue
                    elif mem >= max_bytes(rules, bits):
                        print "Needs only one table"
                        writer.writerow([mem, rules, bits, 'redo','redo',
                                         'redo','redo','redo'])
                        continue
                    print "Should take %d tables and %d bytes" % \
                        min_tables(mem,rules,bits)
                    print "Benching table build time..."
                    runstring = "/usr/bin/time -f '%%U' %s %d %s %s %s"
                    print runstring % (programname, mem, rule_filename, os.devnull,
                                       os.devnull)
                    buildbench = Popen( runstring %
                                       (programname, mem, rule_filename,
                                        os.devnull, os.devnull),
                                       stderr=STDOUT, stdout=PIPE, shell=True)
                    build_time = Decimal(buildbench.communicate()[0])
                    
                    #run the benchmark for the current memory size over the input
                    print "Benching processing time..."
                    print runstring % (programname, mem, rule_filename, data_filename,
                                       os.devnull)
                    runbench = Popen(runstring %
                                     (programname, mem, rule_filename,
                                      data_filename,os.devnull),
                                     stderr = STDOUT, stdout = PIPE, shell=True)
                    run_time = Decimal(runbench.communicate()[0])
                    
                    #calculate final values
                    process_time = run_time - build_time
                    Kbps = Decimal('9600000') / process_time
                    pps = (Decimal('12000000') / Decimal(ceil_div(bits,8)))\
                        / process_time
  
                    # append current run info to the benchmark file
                    tables, memory_used = min_tables(mem, rules, bits)
                    writer.writerow([mem, rules, bits, Kbps.quantize(quant),
                                     pps.quantize(quant), build_time,
                                     memory_used, tables])
                #clean up rule file
                os.remove(os.getcwd() + '/' + rule_filename)

def build_test_input(inputsize = 12, data_filename="%dMBrandom.bin"):
    """Generates a random data file

    inputsize : size of random input in MB
    data_filename : filename template to use, must include a %%d specifier
    """
    # create a random binary file of inputsize megabytes
    data_filename = data_filename % inputsize
    os.system('head --bytes=%d000000 /dev/urandom > %s' % 
              (inputsize, data_filename))
    print "Built random data file of size %dMB" % inputsize

        

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

    # mem_steps  = [1,1000,10000,
    #               100000,500000,1000000,
    #               10000000,100000000,500000000,
    #               1000000000,1500000000,2000000000]
    # rule_steps = [1, 10, 50,
    #               100, 500, 1000,
    #               5000, 10000, 50000,
    #               100000, 500000, 1000000]
    # bit_steps = [1, 10, 40, 
    #              70, 100, 200, 
    #              500, 1000, 1500,
    #              2500, 5000]
    mem_steps = [10,100,1000,10000,100000]
    rule_steps = [10,100, 1000]
    bit_steps = [10,100, 200]
    multi_d_test(mem_steps, rule_steps, bit_steps)
