/* 
 * File:   MixData.cpp
 * Author: ivosh-l
 * 
 * Created on 29. prosinec 2011, 16:28
 */

#include <string.h>
#include <fstream>
#include "MixData.h"
/*
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;
	return split(s, delim, elems);
}
*/
std::vector<std::string> split(const char * data, int size) {
        int i = 0;
	std::vector<std::string> elems;
        const char * dataItem = data;
        while(i < size){
            if(strlen(dataItem)){
                    elems.push_back(dataItem);
            }
            i += (strlen(dataItem)+1);
            dataItem = &data[i];
        }
       
	return elems;
}

using namespace std;

MixData::MixData(ifstream * fh, unsigned int offset, unsigned int size) {
    //bool newrowed = false;
    data = new char[size];
    fh->seekg(offset, ios_base::beg);
    fh->read(data, size);

    filename = split(data, size);
    
    /*for (int i = 0; i < (size); i++) {
        if (data[i] != 0) {
            // is valid?
            // filename.push_back(data[i]);
        }

    }*/
}

MixData::MixData(const MixData& orig) {
}

MixData::~MixData() {
    delete[] data;
}

