/*
 * Copyright 2022 Matthew Peter Smith. 
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 * If a copy of the MPL was not distributed with this file, You can obtain one at 
 * https://mozilla.org/MPL/2.0/.

 */ #pragma once

#include<iostream>
#include<vector> 
#include<cstdlib>
#include<cstring>
#include<string>
#include<map>
#include<algorithm>

#define DOPC_TABLE(name) inline static dopc::Table name;
#define DOPC_FIELD(name, type, table) inline static dopc::Field<type> name = dopc::Field<type>(&table);

namespace dopc
{
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
    
    template <typename T>
    class FreeFunc
    {
        public:
        virtual void f(T& val) {} 
    };

    template<typename T>
    class SortFunc {    
        public:
            virtual bool f(Pair<T> a, Pair<T> b) { return 0; }
    };

    inline void dummyFree(size_t key) {}

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
            std::vector<size_t> free = {}; 
            std::vector<size_t> keys = {}; 
            std::map<size_t, size_t> keyRows; 
        public:
            size_t keyToIndex(size_t k) { return keyRows[k]; };
            size_t indexToKey(size_t k) { return keys[k]; };

            Table() {};

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
                size_t a = keyToIndex(id); 
                size_t b = keys.size() - 1; 
                for(GenericField* field : fields) field->copy(a, b);   
                keys[a] = keys[b];
                keys.pop_back();
            }

            void swap(size_t id1, size_t id2)
            {
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
        FreeFunc<T>* freeFunc;
        inline static FreeFunc<T> dummyFree = FreeFunc<T>();
        public:
        Field(Table* hostTable, FreeFunc<T>* freeFunc = &dummyFree, size_t capacity = 32)
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
            if(freeFunc != &dummyFree) freeFunc->f(keyElem(key));
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

        void sort(SortFunc<T>* func)
        {
            //first need to make an array of key-value pairs
            Pair<T> pairs[numElem]; 
            for(int i = 0; i < numElem; i++)
            {
                pairs[i].key = hostTable->indexToKey(i); 
                pairs[i].value = elems[i];
            }
            //then sort this array
            std::sort(pairs, pairs + numElem, func->f); 
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
    class SimpleFree : public FreeFunc<T>
    {
        public:
            void f(T& val) override
            {
                delete val;
            }
    };
    
    template <typename T>
    class ArrayFree : public FreeFunc<T>
    {
        public:
            void f(T& val) override
            {
                delete[] val;
            }
    };

    template <typename T>
    class SortAscending : public SortFunc<T>
    {
        public:
            bool f(Pair<T>  a, Pair<T>  b) override
            {
                if(a.value < b.value) return true; 
                return false;
            }
    };
}

