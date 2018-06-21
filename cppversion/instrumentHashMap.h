//https://www.sanfoundry.com/cpp-program-implement-hash-tables/

/*
 *C++ Program to Implement Hash Tables
 */
#pragma once
#include<iostream>
#include<cstdlib>
#include<string>
#include<cstdio>

#include "instrument.h"
using namespace std;
const int TABLE_SIZE = 128;         //TODO: change to need?
 
/*
 * HashEntry Class Declaration
 */
class HashEntry
{
    public:
        int key;
        Instrument* instrument;
        HashEntry(int key, Instrument* instrument)
        {
            this->key = key;
            this->instrument = instrument;
        }
};
 
/*
 * HashMap Class Declaration
 */
class HashMap
{
    private:
        HashEntry **table;
    public:   
        HashMap(){
            table = new HashEntry * [TABLE_SIZE];
            for (int i = 0; i< TABLE_SIZE; i++)
            {
                table[i] = NULL;
            }
        }
        /*
         * Hash Function
         */
        int HashFunc(int key){
            return key % TABLE_SIZE;
        }
        /*
         * Insert Element at a key
         */
        void Insert(int key, Instrument* instrument){
                int hash = HashFunc(key);
                while (table[hash] != NULL && table[hash]->key != key)
                {
                    hash = HashFunc(hash + 1);
                }
                if (table[hash] != NULL)
                    delete table[hash];
                table[hash] = new HashEntry(key, instrument);
        }
        /*
         * Search Element at a key
         */
        Instrument* Search(int key){
            int  hash = HashFunc(key);
            while (table[hash] != NULL && table[hash]->key != key)
            {
                hash = HashFunc(hash + 1);
            }
            if (table[hash] == NULL)
                return NULL;
            else
                return table[hash]->instrument;
        }

        /*
         * Remove Element at a key
         */
        void Remove(int key){
            int hash = HashFunc(key);
            while (table[hash] != NULL){
                if (table[hash]->key == key)
                    break;
                hash = HashFunc(hash + 1);
            }
                if (table[hash] == NULL){
                    cout<<"No Element found at key "<<key<<endl;
                    return;
                }
                else
                {
                    delete table[hash];
                }
                cout<<"Element Deleted"<<endl;
        }

        ~HashMap(){
            for (int i = 0; i < TABLE_SIZE; i++)
            {
                if (table[i] != NULL)
                    delete table[i];
                delete[] table;
            }
        }
};