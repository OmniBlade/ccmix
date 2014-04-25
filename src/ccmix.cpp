/* 
 * File:   ccmix.cpp
 * Author: fbsagr
 *
 * Created on April 10, 2014, 1:43 PM
 */

#include "SimpleOpt.h"
#include "mix_file.h"
//#include "SimpleGlob.h"
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

#ifdef _MSC_VER

#include <windows.h>
#include <tchar.h>
#include "win32/dirent.h"

#else

#include <unistd.h>
#include <pwd.h>
#include <dirent.h>
#define TCHAR		char
#define _T(x)		x
#define _tprintf	printf
#define _tmain		main

#endif

#ifdef WINDOWS
#define DIR_SEPARATOR '\\'
#else
#define DIR_SEPARATOR '/'
#endif

using namespace std;

enum { OPT_HELP, OPT_EXTRACT, OPT_CREATE, OPT_GAME, OPT_FILES, OPT_DIR,
       OPT_LIST, OPT_MIX, OPT_ID, OPT_LMD, OPT_ENC};
typedef enum { NONE, EXTRACT, CREATE, ADD, REMOVE, LIST } t_mixmode;

const string games[] = {"td", "ra", "ts"};

//get program directory from argv[0]
static const string getProgramDir(const char* const program_location) {
    register int i;
    const char* const loc = program_location;

    size_t size = strlen(loc);

    for (i = (size - 1); (i >= 0) && (loc[i] != DIR_SEPARATOR); --i);

    if (loc[i] == DIR_SEPARATOR) {
        char* curdir = (char*) malloc((i + 1) * sizeof (char));
        if (curdir != NULL) {
            strncpy(curdir, loc, (size_t) i);
            curdir[i] = '\0';
            return string(curdir);
        }
    }

    const char* curdir = ".";
    return string(curdir);
}

//search and test a few locations for a global mix database
//TODO copy gmd if found but not in a home dir config.
string findGMD(const string program_dir, const string home_dir)
{
    string gmd_loc = "global mix database.dat";
    vector<string> gmd_dir(3);
    gmd_dir[0] = home_dir;
    gmd_dir[1] = "/usr/share/ccmix";
    gmd_dir[2] = program_dir;
    for (int i = 0; i < gmd_dir.size(); i++) {
        string gmd_test = gmd_dir[i] + DIR_SEPARATOR + gmd_loc;
        if (FILE *file = fopen(gmd_test.c_str(), "r")) {
            fclose(file);
            return gmd_test;
        }
    }
    return gmd_loc;
}

// This just shows a quick usage guide in case an incorrect parameter was used
// or not all required args were provided.
inline void ShowUsage(TCHAR** argv)
{
    cout << "Usage: " << argv[0] << " [--mode] (--file FILE)"
            "(--directory DIR) [--mix MIXFILE]" << endl;
    cout << "Try `" << argv[0] << " -?` or `" << argv[0] << 
            " --help` for more information." << endl;
}

// Shows more detailed help if help flags were used in invocation.
void ShowHelp(TCHAR** argv)
{
    cout << "/n***ccmix program usage***\n" << endl;
    cout << "Usage: " << argv[0] << 
            " [--mode] (--file FILE) (--directory DIR) (--game "
            "[td|ra|ts]) [--mix MIXFILE]\n" << endl;
    cout << "Modes:\n" << endl;
    cout << "--extract\n"
            "Extracts the contents of the specified mix file to the current "
            "directory.\n"
            "--file specifies a single file to extract.\n"
            "--directory specifies an alternative directory to extract to.\n"
            "--game specified the game the mix is from, game_td covers the\n"
            "orignal C&C and Sole Survivor. game_ra covers Redalert and its\n"
            "expansions. game_ts covers Tiberian Sun and Red Alert 2/Yuri's "
            "Revenge.\n" << endl;
    cout << "--create\n"
            "Creates a new mix file from the contents of the current folder.\n"
            "--file specifies a single file as the initial file to add to the\n"
            "new mix.\n"
            "--directory specifies an alternative directory to create mix from.\n"
            "--game specified the game the mix is from, game_td covers the\n"
            "orignal C&C and Sole Survivor. game_ra covers Redalert and its\n"
            "expansions. game_ts covers Tiberian Sun and Red Alert 2/Yuri's "
            "Revenge.\n" << endl;
    cout << "--list\n"
            "Lists the contents of the specified mix file.\n" 
            "--game specified the game the mix is from, td covers the\n"
            "orignal C&C and Sole Survivor. ra covers Redalert and its\n"
            "expansions. ts covers Tiberian Sun and Red Alert 2/Yuri's "
            "Revenge.\n" << endl;
}

//quick inline to respond to more than one command being specified.
inline void NoMultiMode(TCHAR** argv)
{
    _tprintf(_T("You cannot specify more than one mode at once.\n"));
    ShowUsage(argv);
}

//convert string we got for ID to uint32_t
//This converts a text hex string to a number, its not to hash the filename.
//That is a method of MixFile.
uint32_t StringToID(string in_string)
{
    if (in_string.size() > 8) return 0;
    
    char* p;
    uint32_t n = strtoul( in_string.c_str(), &p, 16 ); 
    if ( * p != 0 ) {  
    	return 0;
    }    else {  
    	return n;
    }
}

//Function handles the different ways to extract files from a mix.
bool Extraction(MixFile& in_file, string filename, string outdir, uint32_t id)
{   
    if (outdir == "") outdir = "./";
    string destination = outdir + DIR_SEPARATOR + filename;
    
    if (filename == "" && id == 0) {
        cout << "No file or ID to extract given" << endl;
        return false;
    } else if (filename != "" && id == 0) {
        if (!in_file.extractFile(filename, destination)) {
            cout << "Extraction failed" << endl;
            return false;
        } else {
            return true;
        }
    } else if (id != 0){
        if (filename == ""){
            cout << "You must specify a filename to extract to when giving "
                    "an ID" << endl;
        }
        if (!in_file.extractFile(id, destination)) {
            cout << "Extraction failed" << endl;
            return false;
        } else {
            return true;
        }
    } else {
        //cout << input_mixfile << "\nExtract to dir " << outdir << endl;
        if (!in_file.extractAll(outdir)) {
            cout << "Extraction failed" << endl;
            return false;
        } else {
            return true;
        }
    }
}

//Return a string of the home dir path
string GetHomeDir()
{
    char* tmp;
    string rv;
    
    #ifdef WINDOWS

    tmp = getenv("HOMEDRIVE");
    if ( tmp == NULL ) {
        return "";
    } else {
        rv = string(tmp);
    }
    
    tmp = getenv("HOMEPATH");
    if ( tmp == NULL ) {
        return "";
    } else {
        rv += string(tmp);
    }

    #else

    tmp = getenv("HOME");
    if ( tmp == NULL ) {
        rv = string(getpwuid(getuid())->pw_dir);
    } else {
        rv = string(tmp);
    }

    #endif
    return rv;
}

//This specifies the various available command line options
CSimpleOpt::SOption g_rgOptions[] = {
    { OPT_EXTRACT,  _T("--extract"),      SO_NONE    },
    { OPT_CREATE,   _T("--create"),       SO_NONE    },
    { OPT_LIST,     _T("--list"),         SO_NONE    },
    { OPT_LMD,      _T("--lmd"),          SO_NONE    },
    { OPT_ENC,      _T("--encrypt"),      SO_NONE    },
    { OPT_FILES,    _T("--file"),         SO_REQ_SEP },
    { OPT_ID,       _T("--id"),           SO_REQ_SEP },
    { OPT_DIR,      _T("--directory"),    SO_REQ_SEP },
    { OPT_MIX,      _T("--mix"),          SO_REQ_SEP },
    { OPT_GAME,     _T("--game"),         SO_REQ_SEP },
    { OPT_HELP,     _T("-?"),             SO_NONE    },
    { OPT_HELP,     _T("--help"),         SO_NONE    },
    SO_END_OF_OPTIONS                       
};
    
int _tmain(int argc, TCHAR** argv)
{
    if(argc <= 1) {
        ShowUsage(argv);
        return 0;
    }
    
    //initialise some variables used later
    uint32_t file_id = 0;
    string file = "";
    string dir = "";
    string input_mixfile = "";
    const string program_path(argv[0]);
    string user_home_dir = GetHomeDir();
    t_game game = game_td;
    t_mixmode mode = NONE;
    bool local_db = false;
    bool encrypt = false;
    
    //seed random number generator
    srand(time(NULL));
    
    CSimpleOpt args(argc, argv, g_rgOptions);
    
    //Process the command line args and set the variables
    while (args.Next()) {
        if (args.LastError() != SO_SUCCESS) {
            _tprintf(_T("Invalid argument: %s\n"), args.OptionText());
            ShowUsage(argv);
            return 1;
        }
        
        switch (args.OptionId()) {
            case OPT_HELP:
            {
                ShowHelp(argv);
                return 0;
            }
            case OPT_FILES:
            {
                if (args.OptionArg() != NULL) {
                    file = string(args.OptionArg());
                } else {
                    _tprintf(_T("--filename option requires a filename.\n"));
                    return 1;
                }
                break;
            }
            case OPT_ID:
            {
                if (args.OptionArg() != NULL) {
                    file_id = StringToID(string(args.OptionArg()));
                } else {
                    _tprintf(_T("--id option requires a file id.\n"));
                    return 1;
                }
                break;
            }
            case OPT_MIX:
            {
                if (args.OptionArg() != NULL) {
                    input_mixfile = string(args.OptionArg());
                } else {
                    _tprintf(_T("--mix option requires a mix file.\n"));
                    return 1;
                }
                break;
            }
            case OPT_DIR:
            {
                if (args.OptionArg() != NULL) {
                    dir = string(args.OptionArg());
                } else {
                    _tprintf(_T("--directory option requires a directory name.\n"));
                    return 1;
                }
                break;
            }
            case OPT_LMD:
            {
                local_db = true;
                break;
            }
            case OPT_ENC:
            {
                encrypt = true;
                break;
            }
            case OPT_GAME:
            {
                string gt = string(args.OptionArg());
                if(gt == games[0]){
                    game = game_td;
                } else if(gt == games[1]){
                    game = game_ra;
                } else if(gt == games[2]){
                    game = game_ts;
                } else {
                    _tprintf(_T("--game is either td, ra or ts.\n"));
                }
                    
                break;
            }
            case OPT_CREATE:
            {
                if (mode != NONE) { NoMultiMode(argv); return 1; }
                mode = CREATE;
                break;
            }
            case OPT_EXTRACT:
            {
                if (mode != NONE) { NoMultiMode(argv); return 1; }
                mode = EXTRACT;
                break;
            }
            case OPT_LIST:
            {
                if (mode != NONE) { NoMultiMode(argv); return 1; }
                mode = LIST;
                break;
            }
            default:
            {
                if (args.OptionArg()) {
                    _tprintf(_T("option: %2d, text: '%s', arg: '%s'\n"),
                        args.OptionId(), args.OptionText(), args.OptionArg());
                }
                else {
                    _tprintf(_T("option: %2d, text: '%s'\n"),
                        args.OptionId(), args.OptionText());
                }
                break;
            }
        }
    }
    
    //check if we got told a mix file to do something with.
    if (input_mixfile == ""){
        cout << "You must specify --mix MIXFILE to operate on." << endl;
    }
    
    switch(mode) {
        case EXTRACT:
        {
            MixFile in_file(findGMD(getProgramDir(program_path.c_str()), 
                            user_home_dir));

            if (!in_file.open(input_mixfile, game)){
                cout << "Cannot open specified mix file" << endl;
                return 1;
            }
            
            if (!Extraction(in_file, file, dir, file_id)) {
                return 1;
            }
            return 0;
            break;
        }
        case CREATE:
        {
            MixFile out_file(findGMD(getProgramDir(program_path.c_str()), 
                            user_home_dir));

            if (!out_file.createMix(input_mixfile, dir, game, local_db, 
                 encrypt)){
                cout << "Failed to create new mix file" << endl;
                return 1;
            }
            
            return 0;
            break;
        }
        case LIST:
        {
            MixFile in_file(findGMD(getProgramDir(program_path.c_str()), 
                            user_home_dir));

            if (!in_file.open(input_mixfile, game)){
                cout << "Cannot open specified mix file" << endl;
                return 1;
            }
            
            cout << in_file.printFileList(1);
            return 0;
            break;
        }
        default:
        {
            cout << "command switch default, this shouldn't happen!!" << endl;
            return 1;
        }
    }
    
    return 0;
}