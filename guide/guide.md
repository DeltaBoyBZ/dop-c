# DOP-C : A Guide
### Matthew Smith 

## Introduction
DOP-C is a header-only C++ library which makes it easy for developers to create data tables in C++. 
The idea is in the same vein as [Tabular-C++](https://deltaboybz.github.io/tabular-cpp), but learns from the shortcomings of that project. 
While Tabular-C++ has an awkward workflow relying on external code generation (via Python), 
DOP-C simply works if you include it in your source files, and all tables are constructed with C++ code. 

DOP-C is being developed in-tandem with a new version of the [Tabitha SDK](https://github.com/DeltaBoyBZ/tabitha-sdk). 
As of writing, Tabitha's new compiler is very early in development and not due for release anytime soon. 

## Getting DOP-C

All you need to start using DOP-C, is the header file `dopc.hpp`. 
The easiest way to get this is to clone the repository from GitHub:

    git clone https://github.com/DeltaBoyBZ/dop-c

Make sure the directory `dop-c/include` is in your build system's include path. 

## Creating a Table
We introduce the basic usage of DOP-C by example.
Create a new file `foo.cpp`: 

    #include"dopc/dopc.hpp"
    
    #include<cstdlib> 
    #include<iostream>
    
    class Data
    {
        public:
            static dopc::Table table; 
            static dopc::Field<int> a; 
            static dopc::Field<float> b; 
    };
    
    dopc::Table Data::table; 
    dopc::Field<int> Data::a(&Data::table); 
    dopc::Field<float> Data::b(&Data::table); 
    
    /*
    rest of the file
    */
    
The effect of this code, is the declaraion of a table with two fields (columns): an integer column `a`; and a float column `b`.  
By default, the table will have a `32` row capacity, but this will dynamically grow when needed. 
The capacity is not the same as the number of used rows, since of course this starts as `0`. 
The number of used rows is tracked by the `dopc::Table` class. 

## Adding Rows
We continue with our example in `foo.cpp`, and shall do so for the rest of this page.
We manipulate the table inside the main method:

    /*
    table creation code
    */
    
    int main()
    {
        //inserting a row and getting the row ID
        size_t x = Data::table.insert();   
        //setting table elements
        Data::a(x) = 69;
        Data::b(x) = 3.14f;
        //check elements have stored correctly
        std::cout << Data::a(x) << ", " << Data::b(x) << std::endl;
        return 0;
    }
    
We can add as many rows as we like. 
If you were to check the value of `x`, you should find that it is `0`. 
The row corresponding to ID `x` will also happen to be `0`, 
however in general this shall not be the case;
there is a distinction between row indices and row IDs (or *keys*). 

## Deleting Rows
To remove a row with key `x` from a table, is to do the following: 

*Suppose the table has* `n` *used rows. Then copy the row indexed* `(n-1)` *into the row with key* `x`.* 
*Then decrement the number of used rows by 1*. 

Do delete a row with key `x`, simply use:

    Data:::table.remove(x);
    
## Referencing Elements
We have already seen how to reference table elements according to row keys. 
We use round parentheses like:

    Data::a(x) = 27; //sets the element in field `a` and row with key `x` to value `27` 
    
We may sometimes choose to reference elements by their actual position in the table. 
For this, we index with square brackets `[]`.

    Data::b[k] = 9.81; // sets the element in field `b` in the `k`th row to value `9.81`.  


