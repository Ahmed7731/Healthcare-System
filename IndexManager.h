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
    char primaryKey[16];
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

    int getListFileSize() {
        ifstream file(secondaryListFile, ios::binary | ios::ate);
        int size = (int)file.tellg();
        file.close();
        if (size < 0) size = 0;
        return size;
    }

public:
    vector<PrimaryIndexEntry> doctorPrimaryIndex;
    vector<PrimaryIndexEntry> appointmentPrimaryIndex;
    vector<SecondaryIndexEntry> appDoctorIdIndex;
    vector<SecondaryIndexEntry> doctorNameIndex;
    vector<SecondaryIndexEntry> doctorSpecialtyIndex;

    // The indexes live in memory and are rebuilt at every start,
    // so the linked list file always starts empty too.
    IndexManager(string listFileName = "secondary_lists.dat") {
        secondaryListFile = listFileName;
        ofstream clear(secondaryListFile, ios::binary | ios::trunc);
        clear.close();
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

    // ---------- primary index ----------

    // Inserts in the right place, so the index stays sorted.
    void insertPrimary(vector<PrimaryIndexEntry>& index, string key, int offset) {
        auto it = lower_bound(index.begin(), index.end(), PrimaryIndexEntry{key, 0});
        index.insert(it, PrimaryIndexEntry{key, offset});
    }

    void removePrimary(vector<PrimaryIndexEntry>& index, string key) {
        auto it = lower_bound(index.begin(), index.end(), PrimaryIndexEntry{key, 0});
        if (it != index.end() && it->key == key)
            index.erase(it);
    }

    // Key update: change the key, then shift the entry left or right
    // until the array is sorted again.
    bool changePrimaryKey(vector<PrimaryIndexEntry>& index, string oldKey, string newKey) {
        int pos = -1;
        for (int i = 0; i < (int)index.size(); i++) {
            if (index[i].key == oldKey) {
                pos = i;
                break;
            }
        }
        if (pos == -1) return false;

        index[pos].key = newKey;

        while (pos > 0 && index[pos].key < index[pos - 1].key) {
            swap(index[pos], index[pos - 1]);
            pos--;
        }
        while (pos < (int)index.size() - 1 && index[pos + 1].key < index[pos].key) {
            swap(index[pos], index[pos + 1]);
            pos++;
        }
        return true;
    }

    // ---------- secondary index (linked lists in another file) ----------

    void insertSecondaryIndex(vector<SecondaryIndexEntry>& secIndex, string secondaryKey, string primaryKey) {
        auto it = lower_bound(secIndex.begin(), secIndex.end(), SecondaryIndexEntry{secondaryKey, 0});
        bool found = (it != secIndex.end() && it->key == secondaryKey);

        SecondaryNode newNode;
        memset(&newNode, 0, sizeof(SecondaryNode));
        strncpy_s(newNode.primaryKey, primaryKey.c_str(), sizeof(newNode.primaryKey) - 1);
        if (found) newNode.next = it->headPointer;
        else newNode.next = -1;

        int newOffset = getListFileSize();
        ofstream writeStream(secondaryListFile, ios::binary | ios::app);
        writeStream.write((char*)&newNode, sizeof(SecondaryNode));
        writeStream.close();

        if (found) secIndex[it - secIndex.begin()].headPointer = newOffset;
        else secIndex.insert(it, SecondaryIndexEntry{secondaryKey, newOffset});
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
            if (!file) break;

            primaryKeys.push_back(string(node.primaryKey));
            currentOffset = node.next;
        }
        file.close();
        return primaryKeys;
    }

    // Removes one primary key from the list of a secondary key.
    // If the list becomes empty, the secondary key is removed too.
    void removeFromSecondary(vector<SecondaryIndexEntry>& secIndex, string secondaryKey, string primaryKey) {
        auto it = lower_bound(secIndex.begin(), secIndex.end(), SecondaryIndexEntry{secondaryKey, 0});
        if (it == secIndex.end() || it->key != secondaryKey) return;

        fstream file(secondaryListFile, ios::binary | ios::in | ios::out);
        int previousOffset = -1;
        int currentOffset = it->headPointer;

        while (currentOffset != -1) {
            SecondaryNode node;
            file.seekg(currentOffset, ios::beg);
            file.read((char*)&node, sizeof(SecondaryNode));
            if (!file) break;

            if (string(node.primaryKey) == primaryKey) {
                if (previousOffset == -1) {
                    if (node.next == -1) secIndex.erase(it);
                    else it->headPointer = node.next;
                } else {
                    SecondaryNode previous;
                    file.seekg(previousOffset, ios::beg);
                    file.read((char*)&previous, sizeof(SecondaryNode));
                    previous.next = node.next;
                    file.seekp(previousOffset, ios::beg);
                    file.write((char*)&previous, sizeof(SecondaryNode));
                }
                break;
            }
            previousOffset = currentOffset;
            currentOffset = node.next;
        }
        file.close();
    }

    // Used when a primary key changes: the list node keeps the secondary key
    // but now holds the new primary key.
    void changePrimaryKeyInSecondary(vector<SecondaryIndexEntry>& secIndex, string secondaryKey, string oldPrimaryKey, string newPrimaryKey) {
        int head = binarySearchSecondary(secIndex, secondaryKey);
        if (head == -1) return;

        fstream file(secondaryListFile, ios::binary | ios::in | ios::out);
        int currentOffset = head;

        while (currentOffset != -1) {
            SecondaryNode node;
            file.seekg(currentOffset, ios::beg);
            file.read((char*)&node, sizeof(SecondaryNode));
            if (!file) break;

            if (string(node.primaryKey) == oldPrimaryKey) {
                memset(node.primaryKey, 0, sizeof(node.primaryKey));
                strncpy_s(node.primaryKey, newPrimaryKey.c_str(), sizeof(node.primaryKey) - 1);
                file.seekp(currentOffset, ios::beg);
                file.write((char*)&node, sizeof(SecondaryNode));
                break;
            }
            currentOffset = node.next;
        }
        file.close();
    }

    // Used when a doctor ID changes: the appointments index key changes
    // and the entry is shifted to its new sorted place.
    bool renameSecondaryKey(vector<SecondaryIndexEntry>& secIndex, string oldKey, string newKey) {
        int pos = -1;
        for (int i = 0; i < (int)secIndex.size(); i++) {
            if (secIndex[i].key == oldKey) {
                pos = i;
                break;
            }
        }
        if (pos == -1) return false;

        secIndex[pos].key = newKey;

        while (pos > 0 && secIndex[pos].key < secIndex[pos - 1].key) {
            swap(secIndex[pos], secIndex[pos - 1]);
            pos--;
        }
        while (pos < (int)secIndex.size() - 1 && secIndex[pos + 1].key < secIndex[pos].key) {
            swap(secIndex[pos], secIndex[pos + 1]);
            pos++;
        }
        return true;
    }
};

#endif
