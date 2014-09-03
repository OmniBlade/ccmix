/* 
 * File:   gmdedit.cpp
 * Author: aidan
 *
 * Created on 03 September 2014, 19:33
 */

#include "../mix_db_gmd.h"
#include <stdio.h>

typedef std::pair<std::string, std::string> t_namepair;

/*
 * 
 */
int main(int argc, char** argv) {
    
    MixGMD gmd;
    std::fstream ifh;
    std::fstream ofh;
    std::vector<t_namepair> names; 
    
    ifh.open(argv[1], std::ios_base::in|std::ios_base::binary);
    gmd.readDB(ifh);
    ifh.close();
    
    ifh.open(argv[2], std::ios_base::in);
    
    while(!ifh.eof()){
        t_namepair entry;
        std::getline(ifh, entry.first, ',');
        std::getline(ifh, entry.second, ',');
        names.push_back(entry);
    }
    
    for(int i = 0; i < names.size(); i++)
    
    return 0;
}

