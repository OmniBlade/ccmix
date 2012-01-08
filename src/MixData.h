/* 
 * File:   MixData.h
 * Author: ivosh-l
 *
 * Created on 29. prosinec 2011, 16:28
 */

#ifndef MIXDATA_H
#define	MIXDATA_H
#include <fstream>
#include <vector>
#include <string>
#include <sstream>


class MixData {
public:
    MixData(std::ifstream * fh, unsigned int offset, unsigned int size);
    MixData(std::string filePath);
    //MixData(const MixData& orig);
    virtual ~MixData();
    
    std::vector<std::string> getFileNames(){ return filename; }
private:
    char * data;
    int dsize;
    std::vector<std::string> filename;
};

#endif	/* MIXDATA_H */

