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

#define DOPC_TABLE(name) inline static dopc::Table;
#define DOPC_FIELD(name, type, table) inline static dopc::Field<type> name = dopc::Field<type>(&table);

namespace dopc
{
    class GenericField;
    template<typename T>
    class Field; 
    class Table; 
    
    class GenericField
    {
        public: 
            virtual void copy(size_t a, size_t b){}; 
            virtual void push(){}; 
            virtual void pop(){}; 
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
        public:

        Field(Table* hostTable, size_t capacity = 32)
        {
            this->hostTable = hostTable;
            this->capacity = capacity; 
            this->hostTable->addField(this); 
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
    };
}


