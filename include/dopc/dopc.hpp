/*
 * Copyright 2022 Matthew Peter Smith. 
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 * If a copy of the MPL was not distributed with this file, You can obtain one at 
 * https://mozilla.org/MPL/2.0/.

 */ 

#pragma once

#include<iostream>
#include<vector> 
#include<cstdlib>
#include<cstring>
#include<string>
#include<map>
#include<algorithm>
#include<compare>

#define DOPC_TABLE(name) inline static dopc::Table name;
#define DOPC_FIELD(name, type, table) inline static dopc::Field<type> name = dopc::Field<type>(&table);

#define ORDER_HALT(table) while(table.isOrderLocked()) continue; 

namespace dopc
{

    // very much unoptimised
    inline static std::vector<size_t> intersect(std::vector<size_t> a, std::vector<size_t> b)
    {
        std::vector<size_t> c = {};
        for(size_t x : a)
        {
            for(size_t y : b)
            {
                if(x == y) c.push_back(x); 
            }
        }
        return c;
    }


    inline static std::vector<size_t> unite(std::vector<size_t> a, std::vector<size_t> b)
    {
        std::vector<size_t> c;  
        for(size_t x : a) c.push_back(x); 
        for(size_t y : b)
        {
            bool degen = false; 
            for(size_t x : a)
            {
                if(x == y) degen = true; 
            }
            if(!degen) c.push_back(y);
        }
        return c;
    }

    class GenericField;
    template<typename T>
    class Field; 
    class Table; 
    
    template <typename T>
    class Pair
    {
        public: 
        size_t key; 
        T value; 
    };

    template<typename T> 
    inline static void dummyFree(T& val) {}

    class GenericField
    {
        public: 
            virtual void copy(size_t a, size_t b){}
            virtual void push(){} 
            virtual void pop(){} 
            virtual void free(size_t key) {}
    };


    class Table
    {
        protected: 
            std::vector<GenericField*> fields = {}; 
            std::vector<size_t> free = {}; std::vector<size_t> keys = {}; std::map<size_t, size_t> keyRows; 
            bool orderLocked = false; 
        public:
            size_t keyToIndex(size_t k) { return keyRows[k]; };
            size_t indexToKey(size_t k) { return keys[k]; };

            Table() {};

            bool isOrderLocked() { return orderLocked; } 
            void orderLock() { orderLocked = true; }
            void orderUnlock() { orderLocked = true; }

            size_t insert()
            {
                int id;
                if(free.empty())
                {
                    id = keys.size(); 
                }
                else
                {
                    id = keys[-1];
                    free.pop_back(); 
                }
                for(GenericField* field : fields) field->push();  
                keys.push_back(id); 
                keyRows[id] = keys.size() - 1; 
                return id; 
            }  

            void remove(size_t id)
            {
                if(orderLocked)
                {
                    std::cerr << "Cannot remove from an order-locked table." << std::endl;
                    return; 
                }
                size_t a = keyToIndex(id); 
                size_t b = keys.size() - 1; 
                for(GenericField* field : fields) field->copy(a, b);   
                keys[a] = keys[b];
                keys.pop_back();
            }

            void swap(size_t id1, size_t id2)
            {
                if(orderLocked)
                {
                    std::cerr << "Cannot swap within order-locked table." << std::endl;
                    return; 
                }
                size_t a = keyToIndex(id1);
                size_t b = keyToIndex(id2);
                size_t dummy = this->insert();
                size_t c = keyToIndex(dummy); 
                for(GenericField* field : fields)
                {
                    field->copy(c, a); 
                    field->copy(a, b); 
                    field->copy(b, c); 
                }
                this->remove(dummy);
            }
            
            void addField(GenericField* field)
            {
                fields.push_back(field); 
            }
    };


    template<typename T> 
    class Field : public GenericField
    {
        protected:
        size_t numElem  = 0; 
        size_t capacity = 0;
        T* elems = nullptr;
        Table* hostTable = nullptr; 

        typedef void (* FreeFunc) (T& val); 
        typedef bool (* SortFunc) (Pair<T> a, Pair<T> b); 
        typedef bool (* FindFunc) (T& val); 

        FreeFunc freeFunc = dummyFree<T>; 
        public:
        Field(Table* hostTable, FreeFunc freeFunc = dummyFree, size_t capacity = 32)
        {
            this->hostTable = hostTable;
            this->capacity = capacity; 
            this->hostTable->addField(this); 
            this->freeFunc = freeFunc;
            elems = (T*) std::malloc(capacity*sizeof(T)); 
        }

        Table* getHostTable() { return  hostTable; }; 

        const size_t getNumElem() { return numElem; }


        void push() override
        {
            numElem++; 
            if(numElem == capacity) 
            {
                elems = (T*) std::realloc(elems, 2*numElem*sizeof(T));
                capacity = 2*numElem;
            } 
        }

        void pop() override
        {
            numElem--;
        }

        void copy(size_t a, size_t b) override
        {
            elems[a] = elems[b]; 
        }

        T& keyElem(size_t k)
        {
            return elem(hostTable->keyToIndex(k));
        }

        T& elem(size_t k)
        {
            return ((T*) elems)[k];
        }

        void free(size_t key) override
        {
            (*freeFunc)(keyElem(key));
        }
        
        T& operator () (size_t k) { return keyElem(k); }
        T& operator [] (size_t k) { return elem(k); } 

        size_t findFirst(T x)
        {
            for(int index = 0; index < numElem; index++)
            {
                if(elems[index] == x)
                {
                    return hostTable->indexToKey(index);
                }
            } 
            return -1;
        }

        size_t findFirst(FindFunc f)
        {
            for(int index = 0; index < numElem; index++)
            {
                if(f(elems[index])) return hostTable->indexToKey(index);
            }
            return -1;
        }
        
        std::vector<size_t> findAll(T x)
        {
            std::vector<size_t> hits = {}; 
            for(int index = 0; index < numElem; index++)
            {
                if(elems[index] == x)
                {
                    hits.push_back(hostTable->indexToKey(index));  
                }
            }
            return hits; 
        }

        std::vector<size_t> findAll(FindFunc f)
        {
            std::vector<size_t> hits = {}; 
            for(int index = 0; index < numElem; index++)
            {
                if(f(elems[index]))
                {
                    hits.push_back(hostTable->indexToKey(index));  
                }
            }
            return hits; 
        }

        void sort(SortFunc func)
        {
            //first need to make an array of key-value pairs
            Pair<T> pairs[numElem]; 
            for(int i = 0; i < numElem; i++)
            {
                pairs[i].key = hostTable->indexToKey(i); 
                pairs[i].value = elems[i];
            }
            //then sort this array
            std::sort(pairs, pairs + numElem, func); 
            //then reorder the table - this is O(n^2)
            for(int i = 0; i < numElem; i++)
            {
                size_t id = hostTable->indexToKey(i);
                if(pairs[i].key != id) hostTable->swap(pairs[i].key, id);
                for(int j = 0; j < numElem; j++)
                {
                    if(pairs[j].key == id) pairs[j].key = pairs[i].key;
                }
            }
        }
    };

    template <typename T>
    inline static void SimpleFree(T& val) 
    {
        delete val;
    }
    
    template <typename T>
    inline static void ArrayFree(T& val) 
    {
        delete[] val;
    }

    template <typename T>
    inline static bool SortAscending(Pair<T>  a, Pair<T>  b) 
    {
        if(a.value < b.value) return true; 
        return false;
    }
}

