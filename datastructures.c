/* 

Data structures for building Lookup tables for Expressive Packet
Classification
   
Data structures algorithm design by Jay Ligatti
Implementation written by Joshua Kuhn

The data structures in this file are used by a dimension independent
table-based algorithm for classifying packets. We use the following
variables to describe different aspects of rules and packet data.

 - n is the number of rules in a classification policy
 - b is the number of packet bits relevant to classification policy
 - d is the number of dimensions relevant to classification policy
 - t is the number of lookup tables

The dimension-independent algorithm uses t lookup tables, each of
which maps approximately b/t packet bits to n-sized bit arrays.

*/
