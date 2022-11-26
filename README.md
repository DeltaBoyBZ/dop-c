# DOP-C
## Introduction
DOP-C (pronounced "dope-see") is a header-only library which provides C++ programmers 
with a straight-forward way of implementing data tables. 
This was inspired by an earlier project [Tabular-C++](https://deltaboybz.github.io/tabular-cpp),
which has had considerably more work.  
This project has taken on some lessons learned during that project, 
and it is expected that this project will soon dominate thanks to is far more streamlined workflow.  

## Getting Started
To start using DOP-C, simply clone the repository,

    git clone https://github.com/DeltaBoyBZ/dop-c
    
Then just make sure the directory `dop-c/include` is in your include path. 

To learn about actually using the library, check out the [DOP-C Guide](guide/introduction.html).

## Licensing
See [NOTICE.txt](NOTICE.txt)

DOP-C is Copyright 2022 Matthew Peter Smith 

The library DOP-C, is header only, and consists enetirely of a single header file `include/dopc/dopc.hpp`. 
This particular file is provided under the [Mozilla Public License - Version 2.0 (MPLv2)](MOZILLA_PUBLIC_LICENSE_V2.txt). 

The main points to note are:

1. You may freely use and distribute the header file, modified or unmodified. 

2. Any restribution of DOP-C must include at least the statement of copyright in `NOTICE.txt`, 
   as well as a copy of the [MPLv2](MOZILLA_PUBLIC_LICENSE_V2.txt). 

3. Licensing conditions of the MPLv2 are given on a file-by-file basis. 
   Hence in particular, a binary compiled from source code including `dopc.hpp` is freely licensable. 
