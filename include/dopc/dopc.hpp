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
    class String : public std::string 
    {
        public:
            String() { }
            String(const char* x) { this->assign(x); }

            void operator = (String& x) { this->assign(x.c_str()); }  

            void operator = (std::string& x) { this->assign(x.c_str()); }

            void operator = (const char* x) { this->assign(x); }
    };

    inline static bool isElement(std::vector<size_t> vec, size_t elem)
    {
        for(size_t x : vec)
        {
            if(elem == x) return true; 
        }
        return false;
    }

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
            virtual void duplicate(GenericField* destField) { }
            virtual void transcribe(GenericField* destField, size_t destIndex, size_t srcIndex) {};
            virtual size_t getNumElem() { return 0; } 
            virtual void setKey(size_t index, size_t val) { }; 
    };


    typedef bool (* IndexSortFunc)(size_t a, size_t b); 

    class Table
    {
        protected: 
            std::vector<GenericField*> fields = {}; 
            std::vector<size_t> free = {}; 
            std::vector<size_t> keys = {}; 
            std::map<size_t, size_t> keyRows; 
            bool orderLocked = false; 
        public:
            size_t keyToIndex(size_t k) { return keyRows[k]; };
            size_t indexToKey(size_t k) { return keys[k]; };

            Table() {
                keys = {};
            };

            bool isOrderLocked() { return orderLocked; } 
            void orderLock() { orderLocked = true; }
            void orderUnlock() { orderLocked = true; }


            std::vector<GenericField*>& getFields() { return fields; }

            std::vector<size_t>& getKeys()
            {
                return keys;
            }

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

            void reserve(size_t numRows)
            {
                for(int i = 0; i < numRows; i++) insert();
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
                for(GenericField* field : fields) 
                {
                    field->copy(a, b);   
                    field->pop();
                }
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

            template<typename P>
            void indexSort(IndexSortFunc func, size_t tableOffset = 0)
            {
                //first duplicate the table
                P duplicate_master;
                Table* duplicate_table = (Table*)(&duplicate_master + tableOffset);
                std::vector<GenericField*>& fields = this->getFields();
                duplicate_table->reserve(this->keys.size());
                for(int j = 0; j < this->fields.size(); j++)
                {
                    fields[j]->duplicate(duplicate_table->getFields()[j]);                        
                }
                //create an ordered list of indicies
                size_t numRows = this->keys.size();
                size_t indices[numRows]; 
                for(int i = 0; i < numRows; i++) indices[i] = i; 
                std::sort(indices, indices + numRows, func);
                for(int i = 0; i < numRows; i++)
                {
                    for(int j = 0; j < fields.size(); j++)
                    {
                        duplicate_table->getFields()[j]->transcribe(fields[j], i, indices[i]);
                    }
                    this->keys[i] = duplicate_table->getKeys()[i]; 
                }
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
        Field(Table* hostTable = nullptr, FreeFunc freeFunc = dummyFree, size_t capacity = 32)
        {
            this->init(hostTable, freeFunc, capacity); 
        }

        void init(Table* hostTable = nullptr, FreeFunc freeFunc = dummyFree, size_t capacity = 32)
        {
            this->hostTable = hostTable; 
            this->capacity = capacity; 
            if(this->hostTable) this->hostTable->addField(this);
            this->freeFunc = freeFunc;
            this->numElem = 0;
            if(elems) elems = (T*) std::malloc(capacity*sizeof(T)); 
            else elems = (T*) std::realloc(elems, capacity*sizeof(T)); 
        }

        Table* getHostTable() { return  hostTable; }; 

        size_t getNumElem() override { return numElem; }

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

        void duplicate(GenericField* dest0) override
        {
            Field<T>* dest = (Field<T>*) dest0;
            for(int i = 0; i < this->numElem; i++)
            {
                (*dest)[i] = (*this)[i];
            }
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

        void transcribe(GenericField* destField0, size_t destIndex, size_t srcIndex) override
        {
            Field<T>* destField = (Field<T>*) destField0;
            (*destField)[destIndex] = (*this)[srcIndex];
        }
        
        T& operator () (size_t k) { return keyElem(k); }
        T& operator [] (size_t k) { return elem(k); } 

        size_t findFirst(T x, std::vector<size_t> filter = {})
        {
            if(filter.empty())
            {
                for(int index = 0; index < numElem; index++)
                {
                    if(elems[index] == x)
                    {
                        return hostTable->indexToKey(index);
                    }
                } 
            }
            else
            {
                for(size_t key : filter)
                {
                    if(keyElem(key) == x)
                    {
                        return key;
                    }
                }
            }
            return -1;
        }

        size_t findFirst(FindFunc f, std::vector<size_t> filter = {})
        {
            if(filter.empty())
            {
                for(int index = 0; index < numElem; index++)
                {
                    if(f(elems[index])) return hostTable->indexToKey(index);
                }
            }
            else
            {
                for(size_t key : filter)
                {
                    if(f(keyElem(key))) return key;
                }
            }
            return -1;
        }

        
        size_t findFirstIndex(T x, std::vector<size_t> filter = {})
        {
            if(filter.empty())
            {
                for(int index = 0; index < numElem; index++)
                {
                    if(elems[index] == x)
                    {
                        return index;
                    }
                } 
            }
            else
            {
                for(size_t key : filter)
                {
                    if(keyElem(key) == x)
                    {
                        return hostTable->keyToIndex(key);
                    }
                }
            }
            return -1;
        }

        size_t findFirstIndex(FindFunc f, std::vector<size_t> filter = {})
        {
            if(filter.empty())
            {
                for(int index = 0; index < numElem; index++)
                {
                    if(f(elems[index])) return index;
                }
            }
            else
            {
                for(size_t key : filter)
                {
                    if(f(keyElem(key))) return hostTable->keyToIndex(key);
                }
            }
            return -1;
        }
        
        std::vector<size_t> findAll(T x, std::vector<size_t> filter = {})
        {
            std::vector<size_t> hits = {}; 
            if(filter.empty())
            {
                for(int index = 0; index < numElem; index++)
                {
                    if(elems[index] == x)
                    {
                        hits.push_back(hostTable->indexToKey(index));  
                    }
                }
            }
            else
            {
                for(size_t key : filter)
                {
                    if(keyElem(key) == x) 
                    {
                        hits.push_back(key); 
                    }
                }
            }
            return hits; 
        }

        std::vector<size_t> findAll(FindFunc f, std::vector<size_t> filter = {})
        {
            std::vector<size_t> hits = {}; 
            if(filter.empty())
            {
                for(int index = 0; index < numElem; index++)
                {
                    if(f(elems[index]))
                    {
                        hits.push_back(hostTable->indexToKey(index));  
                    }
                }
            }
            else
            {
                for(size_t key : filter)
                {
                    if(f(keyElem(key))) 
                    {
                        hits.push_back(key); 
                    }
                }
            }
            return hits; 
        }

        std::vector<size_t> findAllIndices(T x, std::vector<size_t> filter = {})
        {
            std::vector<size_t> hits = {}; 
            if(filter.empty())
            {
                for(int index = 0; index < numElem; index++)
                {
                    if(elems[index] == x)
                    {
                        hits.push_back(index);  
                    }
                }
            }
            else
            {
                for(size_t key : filter)
                {
                    if(keyElem(key) == x) 
                    {
                        hits.push_back(hostTable->keyToIndex(key)); 
                    }
                }
            }
            return hits; 
        }

        std::vector<size_t> findAllIndices(FindFunc f, std::vector<size_t> filter = {})
        {
            std::vector<size_t> hits = {}; 
            if(filter.empty())
            {
                for(int index = 0; index < numElem; index++)
                {
                    if(f(elems[index]))
                    {
                        hits.push_back(hostTable->indexToKey(index));  
                    }
                }
            }
            else
            {
                for(size_t key : filter)
                {
                    if(f(keyElem(key))) 
                    {
                        hits.push_back(key); 
                    }
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
                if(pairs[i].key != id)
                {
                    hostTable->swap(pairs[i].key, id);
                }
                for(int j = 0; j < numElem; j++)
                {
                    if(pairs[j].key == id) pairs[j].key = pairs[i].key;
                }
            }
        }
    };


    template<typename T>
    class AdditiveMultiverse
    {
        protected:
            Table& original;
            size_t count; 
            std::vector<Table*> copies; 
        public:
            AdditiveMultiverse(Table& original, size_t count, T* structures, size_t offset)
                : original(original), count(count)
            {
                this->copies.reserve(count);
                for(int i = 0; i < count; i++) copies[i] = (Table*)((size_t)structures + i*sizeof(T) + (size_t)offset);
                std::vector<GenericField*>& fields = original.getFields();
                for(int i = 0; i < count; i++)
                {
                    copies[i]->reserve(original.getKeys().size());
                    for(int j = 0; j < original.getFields().size(); j++)
                    {
                        fields[j]->duplicate(copies[i]->getFields()[j]);                        
                    }
                }
            }
            ~AdditiveMultiverse()
            {
            }
            

            void collapse()
            {
                size_t originalNumRows = original.getKeys().size(); //go through each copy and add each of their new rows to the original table
                size_t collapseNumRows = originalNumRows;
                std::vector<GenericField*>& fields = original.getFields();
                for(int i = 0; i < count; i++)
                {
                    size_t copyNumRows = copies[i]->getKeys().size();
                    original.reserve(copyNumRows - originalNumRows); 
                    for(int j = 0; j < copyNumRows; j++)
                    {
                        for(int k = 0; k < original.getFields().size(); k++)
                        {
                            copies[i]->getFields()[k]->transcribe(fields[k], collapseNumRows + j, j); 
                        }
                    }
                    collapseNumRows += originalNumRows;
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

