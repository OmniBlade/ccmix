/* 
 * File:   MixData.cpp
 * Author: ivosh-l
 * 
 * Created on 29. prosinec 2011, 16:28
 */

#include <string.h>
#include <fstream>
#include "MixData.h"
#include <iostream>

std::vector<std::string> split(const char * data, int size) {
    int i = 0;
    std::vector<std::string> elems;
    const char * dataItem = data;
    while (i < size) {
        if (strlen(dataItem)) {
            elems.push_back(dataItem);
        }
        i += (strlen(dataItem) + 1);
        dataItem = &data[i];
    }

    return elems;
}

using namespace std;

MixData::MixData(ifstream * fh, uint32_t offset, uint32_t size) {
    data = new char[size];
    dsize = size;
    fh->seekg(offset, ios_base::beg);
    fh->read(data, size);

    filename = split(data, size);
}

MixData::MixData(std::string filePath) {
    ifstream fh;
    uint32_t begin, end, size;
    fh.open(filePath.c_str(), ios::binary);
    if (fh.rdstate() & ifstream::failbit) {
        fh.clear();
        cout << "Unable to load global mix database! (" << filePath << ")" << endl;
    }
    /* get file size */
    begin = fh.tellg();
    fh.seekg(0, ios::end);
    end = fh.tellg();
    size = end - begin;

    data = new char[size];
    dsize = (int) size;

    fh.seekg(0);
    fh.read(data, dsize);

    filename = split(data, dsize);
}

MixData::~MixData() {
    delete[] data;
}

