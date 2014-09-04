/* 
 * File:   gmdedit.cpp
 * Author: aidan
 *
 * Created on 03 September 2014, 19:33
 */

#include "../mix_db_gmd.h"
#include <stdio.h>
#include <iostream>

typedef std::pair<std::string, std::string> t_namepair;
t_game game;
std::vector<t_namepair> names;

/*
 * 
 */
/*bool gameChoice()
{
    std::string choice = "";
    while(choice == ""){
        
        std::cout << "Which game do you want to add entries for?\n"
                  << "td|ra|ts|ra2|quit\n"
                  << ">:";
        std::cin >> choice;
        if(choice == "td"){
            game = game_td;
            return false;
        } else if(choice == "ra") {
            game = game_ra;
            return false;
        } else if(choice == "ts") {
            game = game_ts;
            return false;
        } else if(choice == "ra2") {
            game = game_ra2;
            return false;
        } else if(choice == "quit") {
            return true;
        } else {
            choice = "";
        }
    }
}

bool addEntry()
{
    bool another = true;
    std::string input;
    t_namepair name;
    while(another){
        std::cout << "Filename >: ";
        std::cin >> input;
        name.first = tolower(input);
        std::cout << "Description >: ";
        std::cin >> input;
        name.second = tolower(input);
        names.push_back(name);
        std::cout << "Add Another?(YES|no) >: ";
        std::cin >> input;
        if(tolower(input) == "no" | tolower(input) == "n"){
            another = false;
        }
    }
}

void menu()
{
    bool quit = false
    game = game_td;
    
    while(!quit){
        quit = gameChoice();
    } 
}*/

int main(int argc, char** argv) {
    
    MixGMD gmd;
    std::fstream ifh;
    std::fstream ofh;
    std::vector<t_namepair> names;
    
    ifh.open(argv[1], std::ios_base::in|std::ios_base::binary);
    gmd.readDB(ifh);
    ifh.close();
    
    if(argc < 4){
        //menu();
        std::cout << "Use: gmdedit gmdpath additionspath newgmdpath\n";
        return 1;
    } else {
        game = game_td;
        
        ifh.open(argv[2], std::ios_base::in);

        while(!ifh.eof()){
            t_namepair entry;
            std::getline(ifh, entry.first, ',');
            std::getline(ifh, entry.second);
            std::cout << entry.first << " - " << entry.second << "\n";
            if(entry.first != ""){
                names.push_back(entry);
            }
        }
    }

    for(unsigned int i = 0; i < names.size(); i++) {
        gmd.addName(game, names[i].first, names[i].second);
    }
    
    ofh.open(argv[3], std::ios_base::out|std::ios_base::binary);
    gmd.writeDB(ofh);
    
    return 0;
}

