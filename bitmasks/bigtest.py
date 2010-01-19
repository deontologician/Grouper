"""This is to do a full benchmark on the packet filter as well as various
functions to help in building different kinds of tests"""

from math import log, ceil
from subprocess import Popen, PIPE, STDOUT, check_call, call
from optparse import OptionParser
from decimal import Decimal, getcontext
import os
import sys
import csv
import time

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
        bdivt, bmodt = divmod(bitlength, t)
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
    filename = '%dbx%drules.pol' % (bits,rules)
    print "** Writing %s ..." % filename
    check_call(['./makerules', str(bits), str(rules), str(filename)])
    print "** Finished writing %s." % filename
    return filename

def build_test_input(inputsize = 12, data_filename="%dMBrandom.bin"):
    """Generates a random data file

    inputsize : size of random input in MB
    data_filename : filename template to use, must include a %%d specifier
    """
    # create a random binary file of inputsize megabytes
    data_filename = data_filename % inputsize
    os.system('head --bytes=%d000000 /dev/urandom > %s' % 
              (inputsize, data_filename))
    return data_filename

def min_bytes(rules, bits):
    "returns the minimum number of bytes needed for given rules and bits"
    return 2*ceil_div(rules,8)*bits

def max_bytes(rules, bits):
    "returns the max number of bytes needed for given rules and bits"
    return ceil_div(int(log(rules,2)),8) * 2**bits

def table_mem(t, n, b):
    N = ceil_div(n,8)
    return (t - (b % t))*(2**(b//t))*N + \
        (b % t)*(2**((b//t) + 1))*N

def min_tables(m, n, b):
    """Returns the minimum tables needed for a policy of n rules and b bits
    given m bytes of memory to work with """
    high = ceil_div(b,2)
    low = 1
    while high - low > 1:
        mid = (high + low) / 2
        memForMid = table_mem(mid,n,b)
        if m < memForMid:
            low = mid
        else:
            high = mid
    
    return high, table_mem(high,n,b)
        
def multi_d_test(mem_steps, rule_steps, bit_steps, 
                 programname = './tblcompile',
                 data_size = 10,
                 test_filename = 'test_file.csv'):
    """Do multi-dimensional test with the given steps"""
    # make a new data file if necessary
    data_filename = build_test_input(data_size) if data_size != 10 else '10MBrandom.bin'
    all_steps = mem_steps is None
    with open(test_filename,'w') as test_file:
        writer = csv.writer(test_file)
        writer.writerow(['memory allowed','number of rules','packet size in bits',
                         'kbps','pps','table build time', 'memory used',
                         'number of tables', 'policy read time', 'total run time',
                         'repeat run', 'command'])
        # dict for memoizing results
        memoizer = {}
        for bits in bit_steps:
            for rules in rule_steps:
                if (bits + 1) * rules > 1000000000: # max is 1GB
                    print "Rule file will be too large to be practical"
                    for mem in mem_steps:
                        writer.writerow([mem,rules,bits,'','','','','','','','',''])
                    continue
                else:
                    rule_filename = make_rule_file(bits, rules)
                print "Working with %s now..." % rule_filename
                print "All-steps =",all_steps
                if all_steps: mem_steps = mem_levels(bits,rules,2000000000)
                for _,mem in mem_steps:
                    print "%d bytes:" % mem
                    if mem < min_bytes(rules, bits):
                        print "\tNot enough memory for rules and bits given"
                        writer.writerow([mem, rules, bits,'','','','','','','',''])
                        continue
                    elif mem >= max_bytes(rules, bits):
                        print "\tNeeds only one table"
                        writer.writerow([mem, rules, bits, 'redo', 'redo',
                                         'redo','redo','redo', 'redo', 'redo', 'redo',
                                         'redo'])
                        continue
                    # find the minimum memory and tables
                    tables, memory_used = min_tables(mem, rules, bits)

                    print "\tShould take %d tables and %d bytes" % \
                        (tables, memory_used)

                    runstring = "%s %d %s %s %s" % (programname, mem, rule_filename,
                                      data_filename, os.devnull)

                    #check if this run has already been done
                    key = (tables, memory_used, bits, rules)
                    if key in memoizer:
                        res = memoizer[key]
                        writer.writerow([mem, rules, bits, res['kbps'],
                                         res['pps'], res['build_time'], 
                                         memory_used, tables, res['pol_read'],
                                         res['total_time'], 'true', runstring])
                        print "\tRun already completed, using cached values"
                        continue

                    
                    #run the benchmark for the current memory size over the input
                    print "\tBenchmarking '", runstring , "' ... "
                    runbench = Popen(runstring , stderr = STDOUT, stdout = PIPE,
                                     shell=True)
                    timings = eval(runbench.communicate()[0]) #expecting a dict
                    print "\tTimings:", timings
                    
                    # calculate final values

                    # data_size is in MB, so we convert it to bits
                    data_bits = Decimal(data_size * 1000000 * 8)

                    # the timing values returned are in microseconds, so we must
                    # divide them by 1,000,000 to get seconds
                    usec2sec = lambda x: Decimal(x) / 1000000
                    read_secs = usec2sec(timings['read'])
                    build_secs = usec2sec(timings['build'])
                    process_secs = usec2sec(timings['process'])
                    total_secs = usec2sec(timings['total'])
                    
                    Kbps = Decimal( data_bits / 1000 ) / process_secs
                    pps  = Decimal( data_bits / bits ) / process_secs
                    
                    # append current run info to the benchmark file
                    writer.writerow([mem, rules, bits, q(Kbps),
                                     q(pps), q2(build_secs),
                                     memory_used, tables, q2(read_secs),
                                     q2(total_secs), 'false', runstring])
                    # add current run info to the memoizer
                    memoizer[key] = dict(kbps = q(Kbps), pps = q(pps), 
                                         build_time = q2(build_secs), 
                                         pol_read = q2(read_secs),
                                         total_time = q2(total_secs))
                #clean up rule file
                os.remove(rule_filename)

def q(v):
    """Convenience function to return Decimal value as a string rounded to the
    nearest 1"""
    return v.quantize(Decimal('1'))

def q2(v):
    """Convenience function to return Decimal value as a string rounded to the
    nearest ten thousandth"""
    return v.quantize(Decimal('0.0001'))

def durationstr(secs):
    secs = int(secs)
    seconds, minutes = divmod(secs, 60)
    minutes, hours = divmod(minutes, 60)
    hours, days = divmod(hours, 24)
    return "%d days %d hours %d minutes %d seconds" % (days,hours,minutes,seconds)

def starttest():
    motd = open("/etc/motd","w")
    Popen(['echo',"Tests running! Don't do anything CPU intensive"], stdout = motd)
    motd.close()

def endtest():
    motd = open("/etc/motd","w")
    Popen(['echo',"No tests currently running."], stdout = motd)
    motd.close()
        

if __name__ == '__main__':

    parser = OptionParser()
    parser.add_option('-o','--output',dest='outfile',
                      help = 'File to write benchmark results to',)
    parser.add_option('-p','--program',dest='programname', 
                      default = './tblcompile',
                      help = 'Name of program to benchmark')
    parser.add_option('-a', '--all-tests', dest='all_tests',
                      action = 'store_true', default = False,
                      help = "Whether to do every table step or not")
    parser.add_option('-r', '--repeat', dest='rounds', type="int",
                      default = 1,
                      help = "Number of times to repeat a test")
    parser.add_option('-b', '--bits', dest = 'bits', type="int",
                      help = "Number of relevent bits to test")
    parser.add_option('-d', '--data-size', dest='data_size', type='int',
                      default = 10,
                      help = "Size in MB of the input file")
    (options, args) = parser.parse_args(sys.argv)

    #set the decimal precision
    getcontext().prec = 32

    #bigtests
    # mem_steps  = 
    # rule_steps = [1, 10, 50,
    #               100, 500, 1000,
    #               5000, 10000, 50000,
    #               100000, 500000, 1000000]
    # bit_steps = 

    # 104 tests
    if options.all_tests:
        bit_steps = [options.bits]
        mem_steps = None
        prefix = "alltest%dbits" % options.bits
    else:
        bit_steps = [1, 10, 40, 
                     70, 100, 200, 
                     500, 1000, 1500,
                     2500, 5000]
        mem_steps = [1,1000,10000,
                     100000,500000,1000000,
                     10000000,100000000,500000000,
                     1000000000, 1500000000, 2000000000]
        prefix = "steptest"

    rule_steps = [10**3, 10**4, 10**5, 10**6]
    
    starttest()
    t_start = time.time()
    for i in xrange(options.rounds):
        if options.outfile is not None and options.rounds != 1:
            filename = "%s_%d.csv" % (prefix, i + 1)
        elif options.rounds > 1:
            filename = "%s_%d.csv" % (options.outfile, i)
        elif options.outfile is None and options.rounds == 1:
            filename = "%s.csv" % options.outfile
        else:
            print "illegal combination of outfile and rounds"
            exit()
        r_start = time.time()
        multi_d_test(mem_steps, rule_steps, bit_steps, 
                     data_size=options.data_size,
                     test_filename = filename,
                     programname = options.programname)
        r_end = time.time()
        print "Round %d took %s" % (i, durationstr(r_end - r_start))
    t_end = time.time()
    print "Total runtime: %s" % durationstr(t_end - t_start)
    endtest()
