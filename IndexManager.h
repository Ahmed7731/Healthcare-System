#ifndef INDEX_MANAGER_H
#define INDEX_MANAGER_H

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <cstring>
#include "doctor.h" 

using namespace std;


struct PrimaryIndexEntry {
    string key;      
    int offset;   
    
    bool operator<(const PrimaryIndexEntry& other) const {
        return key < other.key;
    }
};


struct SecondaryNode {
    char primaryKey[15]; 
    int next;      
};

struct SecondaryIndexEntry {
    string key;         
    int headPointer;    
    
    bool operator<(const SecondaryIndexEntry& other) const {
        return key < other.key;
    }
};

class IndexManager {
private:
    string secondaryListFile; 

public:

    vector<PrimaryIndexEntry> doctorPrimaryIndex;           
    vector<PrimaryIndexEntry> appointmentPrimaryIndex;     
    vector<SecondaryIndexEntry> appDoctorIdIndex;          
    vector<SecondaryIndexEntry> doctorNameIndex;            
    vector<SecondaryIndexEntry> doctorSpecialtyIndex;       

    IndexManager(string listFileName = "secondary_lists.dat") {
        secondaryListFile = listFileName;
        ofstream check(secondaryListFile, ios::binary | ios::app);
        check.close();
    }

    int binarySearchPrimary(const vector<PrimaryIndexEntry>& index, string searchKey) {
        int left = 0, right = (int)index.size() - 1;
        while (left <= right) {
            int mid = left + (right - left) / 2;
            if (index[mid].key == searchKey) return index[mid].offset;
            if (index[mid].key < searchKey) left = mid + 1;
            else right = mid - 1;
        }
        return -1;
    }

    
    int binarySearchSecondary(const vector<SecondaryIndexEntry>& index, string searchKey) {
        int left = 0, right = (int)index.size() - 1;
        while (left <= right) {
            int mid = left + (right - left) / 2;
            if (index[mid].key == searchKey) return index[mid].headPointer;
            if (index[mid].key < searchKey) left = mid + 1;
            else right = mid - 1;
        }
        return -1;
    }

    void sortAllIndexes() {
        sort(doctorPrimaryIndex.begin(), doctorPrimaryIndex.end());
        sort(appointmentPrimaryIndex.begin(), appointmentPrimaryIndex.end());
        sort(appDoctorIdIndex.begin(), appDoctorIdIndex.end());
        sort(doctorNameIndex.begin(), doctorNameIndex.end());
        sort(doctorSpecialtyIndex.begin(), doctorSpecialtyIndex.end());
    }

    void insertSecondaryIndex(vector<SecondaryIndexEntry>& secIndex, string secondaryKey, string primaryKey) {
        SecondaryNode newNode;
        strncpy(newNode.primaryKey, primaryKey.c_str(), sizeof(newNode.primaryKey));
        newNode.primaryKey[sizeof(newNode.primaryKey) - 1] = '\0';
        newNode.next = -1;

        ifstream readStream(secondaryListFile, ios::binary | ios::ate);
        int newOffset = readStream.tellg();
        readStream.close();
        if (newOffset == -1) newOffset = 0;

        ofstream writeStream(secondaryListFile, ios::binary | ios::app);
        writeStream.write((char*)&newNode, sizeof(SecondaryNode));
        writeStream.close();

        auto it = lower_bound(secIndex.begin(), secIndex.end(), SecondaryIndexEntry{secondaryKey, 0});
        if (it != secIndex.end() && it->key == secondaryKey) {
            

            int oldHead = it->headPointer;
            newNode.next = oldHead;


            fstream updateStream(secondaryListFile, ios::binary | ios::in | ios::out);
            updateStream.seekp(newOffset, ios::beg);
            updateStream.write((char*)&newNode, sizeof(SecondaryNode));
            updateStream.close();

            it->headPointer = newOffset; 
        } else {
            secIndex.push_back({secondaryKey, newOffset});
            sortAllIndexes();
        }
    }

    vector<string> getPrimaryKeysFromSecondary(int headPointer) {
        vector<string> primaryKeys;
        if (headPointer == -1) return primaryKeys;

        ifstream file(secondaryListFile, ios::binary);
        int currentOffset = headPointer;

        while (currentOffset != -1) {
            file.seekg(currentOffset, ios::beg);
            SecondaryNode node;
            file.read((char*)&node, sizeof(SecondaryNode));
            
            primaryKeys.push_back(string(node.primaryKey));

            currentOffset = node.next; 
        }
        file.close();
        return primaryKeys;
    }
};

#endif