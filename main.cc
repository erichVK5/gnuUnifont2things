// bitmap2waterfall.cc v1.0
// main.cc v1.0
// comprising gnuUnifont2things
//
// A utility for turning X window .bdf font description files into
//  - ASCII depictions with up/down/left rotated/right rotated
//    orientations
//  - Hellschreiber audio for viewing on wterfall displays
//  - in due course, gEDA PCB dot matrix style PCB footprints from
//    glyphs
//  - this allows the entire UniCode code space in the gnu Unifont
//    to be depicted
//
// Copyright (C) 2016 Erich S. Heinzle, a1039181@gmail.com
//
//    see LICENSE-gpl-v2.txt for software license
//    see README.txt
//    
//    This program is free software; you can redistribute it and/or
//    modify it under the terms of the GNU General Public License
//    as published by the Free Software Foundation; either version 2
//    of the License, or (at your option) any later version.
//    
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//    
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//    
//    bitmap2waterfall.cc (C) 2016 Erich S. Heinzle a1039181@gmail.com
//

#include "bitmap2waterfall.cc"
#include <map>
#include <iostream>
#include <string>

using namespace std;

int main (int argc, char * argv[]) {

    string textToParse;
    vector<int> glyphsToRender;
    int interSymbol32 = 0; // flag to add spacing, or not, between chars
    if (argc > 1) {
        textToParse = argv[1];
    } else {
        textToParse = "";
    }

    //    string defaultBDF = "fireflyR16.bdf";
    string defaultBDF = "unifont-8.0.01.bdf";

    string filename =  "output.raw";
    int outputRawIntegersToScreen = 0;
    int extraSpacesBetweenGlyphs = 0;

    if (textToParse.length() != 0) {
        // std::cout << "about to load: " << defaultBDF << endl;
        std::map<int, Glyph*> glyphMap = load_bdf_file(defaultBDF);
        // std::cout << "loaded: " << defaultBDF  << endl;
        // std::cout << " glyph map size : "
        // << glyphMap.size() << std::endl;
        writeGlyphsToAudio(glyphMap,
                           textToParse,
                           extraSpacesBetweenGlyphs,                   
                           filename,
                           outputRawIntegersToScreen);
        // time to clean up after ourselves
        cleanUpGlyphMap(glyphMap);
        std::cout << "Now use: \n"
                  << "sox -r 8000 -t raw -b 8 -e signed-integer "
                  << "output" << ".raw "
                  << "output" << ".wav && play output.wav"
                  << std::endl;
    }

    return 0;

}
