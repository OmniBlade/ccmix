/* 
 * File:   main.cpp
 * Author: ivosh-l
 *
 * Created on 29. prosinec 2011, 11:30
 */

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
//#include <boost/system/system_error.hpp>
#include <cstdlib>
#include "MixEditor.h"
#include <iostream>
#include <string.h>

using namespace std;
namespace po = boost::program_options;
//using namespace boost::filesystem;

#ifdef WINDOWS
#define DIR_SEPARATOR '\\'
#else
#define DIR_SEPARATOR '/'
#endif

#define TSMIX_VERSION "0.1"

static const char*
getCurrentDir(const char* const program_location) {
    register int i;
    const char* const loc = program_location;

    size_t size = strlen(loc);

    for (i = (size - 1); (i >= 0) && (loc[i] != DIR_SEPARATOR); --i)
        ;

    if (loc[i] == DIR_SEPARATOR) {
        char* curdir = (char*) malloc((i + 1) * sizeof (char));
        if (curdir != NULL) {
            strncpy(curdir, loc, (size_t) i);
            curdir[i] = '\0';
            return curdir;
        }
    }

    const char* curdir = ".";
    return curdir;
}


int main(int argc, char** argv) {
    vector<string> infiles;
    vector<string> outfiles;
    po::options_description desc("Allowed options");
    po::positional_options_description pd;
    t_game openGame;
    desc.add_options()
            ("help,h", "produce help message")
            ("version,v", "show tsunmix version")
            ("list,l", "show mix file content")
            ("decrypt,d", po::value<string > (), "decrypt archive")
            //("encrypt,e", po::value<string > (), "encrypt archive")
            ("add-file,a", "add file to archive")
            ("remove-file,r", "remove file from archive")
            ("extract,x", "extract file from archive")
            ("extract-id,n", "extract file from archive by it's numeric ID\n      use 0x prefix for hex-formatted number")
            ("extract-all,A", "extract file from archive")
            ("open-mix,m", po::value<string > (), "open mix file")
            ("output-file,o", po::value< vector<string> >(), "specify output file")
            ("input-file,I", po::value< vector<string> >(), "input file")
            ("game,g", po::value<string > (), "set game\n"
            "  * game_ts: Tiberian Sun (default)\n"
            "  * game_ra: Red Aletr\n"
            "  * game_td: Tiberian Dawn\n"
            )
            ;
    pd.add("open-mix", 1).add("input-file", -1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).positional(pd).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << "\n";
        return 0;
    }

    if (vm.count("version")) {
        cout << TSMIX_VERSION << "\n";
        return 0;
    }

    MixEditor mix(getCurrentDir(argv[0]));

    if(vm.count("game")){
        string gameStr = vm["open-mix"].as<string > ();
        if(gameStr == "game_ts")
            openGame = game_ts;
        else if(gameStr == "game_ra")
            openGame = game_ra;
        else if(gameStr == "game_td")
            openGame = game_td;
    } else{
        openGame = game_ts;
    }
    
    if (vm.count("open-mix")) {
        if (!mix.open(vm["open-mix"].as<string > (),openGame)) {
            cout << "File " << vm["open-mix"].as<string > () << " not found." << endl;
            return 1;
        }
    } else {
        cout << "No mix file specified. Use -h for help." << endl;
        return 1;
    }

    if (vm.count("list")) {
        cout << mix.printFileList(1);
        return 0;
    }



    if (vm.count("input-file")) {
        infiles = vm["input-file"].as< vector<string> >();
    }
    if (vm.count("output-file")) {
        outfiles = vm["output-file"].as< vector<string> >();
    }

    if (vm.count("extract")) {
        if (!infiles.size()) {
            cout << "No input files specified." << endl;
            return 1;
        }
        for (int i = 0; i < infiles.size(); i++) {
            if (!mix.extractFile(infiles[i], (i < outfiles.size()) ? outfiles[i] : infiles[i])) {
                cout << "File " << infiles[i] << " not found in the archive." << endl;
            }
        }
        return 0;
    }

    if (vm.count("extract-id")) {
        bool m_is_hex = false;
        if (!infiles.size()) {
            cout << "No input files specified." << endl;
            return 1;
        }
        for (int i = 0; i < infiles.size(); i++) {
            m_is_hex = (infiles[i].length() > 1 && infiles[i][0] == '0' && infiles[i][1] == 'x');
            if (!mix.extractFile(strtol(infiles[i].c_str(), NULL, m_is_hex ? 16 : 10), (i < outfiles.size()) ? outfiles[i] : infiles[i])) {
                cout << "File \"" << infiles[i] << "\" not found in the archive." << endl;
            }

        }
        return 0;
    }

    if (vm.count("extract-all")) {
        if (!mix.extractAll((outfiles.size()) ? outfiles[0] : "", true)) {
            cout << "File " << vm["open-mix"].as< string > () << " could not be extracted." << endl;
        }
        return 0;
    }

    if (vm.count("decrypt")) {
        if (!mix.decrypt((outfiles.size()) ? outfiles[0] : (vm["open-mix"].as< string > () + "_decrypted.mix"))) {
            cout << "File " << vm["open-mix"].as< string > () << " could not be decrypted." << endl;
        }
        return 0;
    }

    if (vm.count("add-file")){
        if(infiles.empty()){
            cout << "No input file specified!" << endl;
            return 1;
        }
        if(!mix.addFile(infiles[0]))
            cout << "File " << infiles[0] << " could not be added." << endl;
        return 0;
    }
    
    if(vm.count("remove-file")){
         if(infiles.empty()){
            cout << "No input file specified!" << endl;
            return 1;
        }
         mix.removeFile(infiles[0]);
    }
    
    return 0;
}



