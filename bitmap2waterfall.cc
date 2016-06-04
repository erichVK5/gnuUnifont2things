// bitmap2waterfall.cc v1.0
// Copyright (C) 2016 Erich S. Heinzle, a1039181@gmail.com
//
//  A utility for turning X window .bdf font description files into
//  - ASCII depictions with up/down/left rotated/right rotated
//    orientations
//  - Hellschreiber audio for viewing on wterfall displays
//  - in due course, gEDA PCB dot matrix style PCB footprints from
//    glyphs
//  - this allows the entire UniCode code space in the gnu Unifont
//    to be depicted
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

#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <string>
#include <sstream>
#include <map>
#include <list>
#include <vector>
#include <cmath>

#define VERBOSE 0

using namespace std;

class Glyph {
public:

    Glyph(std::list<std::string> data,
          int ascent = 14, 
          int descent = 2) {
        fontAscent = ascent;
        fontDescent = descent;
        glyphDef = data;
        parsed = 0;
        floorFreq = 800;
        freqSpacing = 17;
        charLineDurationMS = 200; // was 10
        bitRate = 8000;
        nbits = 16; // not used currently 
        amplitude = 127; //pow(2, nbits-1) - 1;
        tor = 16;//4; // time constant for gaussian error function, ms 
        // but for now being used to ramp tone on/off
        pulseDuration = 0; // for gaussian error function in due course
    }

    ~Glyph() {
        cleanUp();
    }

    void printSymSummary() {
        glyphInit();
        std::cout << "Summary: " << std::endl << " BBXx: "
                  << BBXx << ", BBXy: " << BBXy << std::endl
                  << "BBXxOffset: " << BBXxOffset
                  << ", BBXyOffset: " << BBXyOffset << std::endl
                  << "dispWidth:" << dispWidth << std::endl;
    }
 
    void printSym(char dir) {
        switch (dir) {
        case 'U':
            vertSymAscii();
            break;
        case 'D':
            piRotatedSymAscii();
            break;
        case 'L':
            leftRotSymAscii();
            break;
        case 'R':
            rightRotSymAscii();
            break;            
        }
    }

    vector<int8_t> audioSym(char dir) {
        switch (dir) {
        case 'U':
            return vertSymAudio();
        case 'D':
            return piRotatedSymAudio();
        case 'L':
            return leftRotSymAudio();
        case 'R':
            return rightRotSymAudio();
        }
    }

    friend ostream& operator<<(ostream& output, const Glyph& g) {
        std::list<string>::const_iterator iter;
        //output << "Glyph definition: " << g.glyphDef.front() ;
        for (iter = g.glyphDef.begin();
             iter != g.glyphDef.end(); ++iter) {
            output << *iter << std::endl;
        }
        return output;
    }

    std::list<std::string> glyphDef;
    std::vector<std::string> asciiBitmap;
    std::vector<string> outputRows;
    std::vector<int8_t> symbolAudio;
    std::vector<int8_t> tempAudio;

    int fontAscent;
    int fontDescent;

    int BBXx;
    int BBXy;
    int BBXxOffset;
    int BBXyOffset;

    int dispWidth;
    int paddingLineWidth;

    int floorFreq;
    int freqSpacing;
    int charLineDurationMS;
    int bitRate;
    int nbits;
    int amplitude;

    bool parsed;

    string whitespace;
    string currentLine; 
    string currentRow;
    string output;

    int tor; // time constant for gaussian error function, ms 
    int pulseDuration;

    static const double envelope[];
    
private:
    
   void cleanUp() {
        glyphDef.clear();
        asciiBitmap.clear();
        outputRows.clear();
    }

    static string padding(int num, string pad) {
        if (num < 1 ) { // i.e. if BBXxOffset < 1, we do nothing
            return "";
        } else {
            return pad + padding(num-1, pad);
        }
    }

    string paddingLine(int num, string pad) {
        whitespace = "";
        if (BBXx < dispWidth) {
            num = dispWidth;
        }
        if (BBXxOffset > 0) {
            num += BBXxOffset;
        }
        for (int count = 0; count < num; count++) {
            whitespace = whitespace + pad;
        }
        return whitespace;
    }

    void glyphInit() {
        if (parsed == 0) {
            parseDef();
            parsed = 1;
        }
    }

    void parseDef() {
        list<std::string>::iterator it1, it2;
        it1 = glyphDef.begin();
        it2 = glyphDef.end(); //ignore "ENDCHAR"
        int index1 = 0;
        int index2 = 0;
        for (; it1 != it2; it1++) {
            currentLine = *it1;
            if (!strcmp(currentLine.substr(0,4).c_str(), "BBX ")) {
                index1 = index2 = 4;
                while (currentLine[index2] != ' ') {
                    index2++;
                }
                BBXx = 
                    std::atoi(currentLine.substr(index1, index2-1).c_str());
                index1 = index2;
                index2++;
                while (currentLine[index2] != ' ') {
                    index2++;
                }
                BBXy = 
                    std::atoi(currentLine.substr(index1, index2-1).c_str());
                index1 = index2;
                index2++;
                while (currentLine[index2] != ' ') {
                    index2++;
                }
                BBXxOffset = 
                    std::atoi(currentLine.substr(index1, index2-1).c_str());
                index1 = index2;
                BBXyOffset = 
                    std::atoi(currentLine.substr(index1).c_str());
            } else if (!strcmp(currentLine.substr(0,7).c_str(), "DWIDTH ")) {
                index1 = index2 = 7;
                while (currentLine[index2] != ' ') {
                    index2++;
                }
                dispWidth = 
                    std::atoi(currentLine.substr(index1, index2-1).c_str());
            } else if (!strcmp(currentLine.substr(0,6).c_str(), "BITMAP")) {
                // we move onto the next line, into the bitmapping
                for (it1++; it1 != it2; it1++) {
                    currentLine = *it1;
                    if (strcmp(currentLine.substr(0,7).c_str(), "ENDCHAR")) {
                        asciiBitmap.push_back(currentLine);
                    }
                }
                // now sort out the outer for loop
                if (it1 == it2) {
                    it1--;
                }
            }
        }
        parsed = 1;
        processBitmap();
    }

    void processBitmap() {

        paddingLineWidth = asciiBitmap[0].length()*4;

        if (BBXxOffset < 0) {
            paddingLineWidth -= BBXxOffset;
        } else {
            paddingLineWidth += BBXxOffset;
        }

        // this is helpful for firefly bdf but not crucial
        // for unifont.bdf which has uniform glyph heights
        if (BBXyOffset < 0 ) {
            for (int count = 0; count > BBXyOffset; count--) {
                output = paddingLine(paddingLineWidth, "-");
                outputRows.push_back(output);
            }
        }

        // could do (fontAscent - glyphHeight) padding
        // rows here I think but unifont.bdf
        // is well behaved WRT BBX height=16 for all glyphs 
        for (int index = 0; index < BBXy; index++) {
            currentRow = asciiBitmap[index];
            output = ""; // zero it out
            for (int column = 0;
                 column < currentRow.length(); column++) {
                switch (currentRow[column]) {
                case '0':
                    output = output + "----";
                    break;
                case '1':
                    output = output + "---#";
                    break;
                case '2':
                    output = output + "--#-";
                    break;
                case '3':
                    output = output + "--##";
                    break;
                case '4':
                    output = output + "-#--";
                    break;
                case '5':
                    output = output + "-#-#";
                    break;
                case '6':
                    output = output + "-##-";
                    break;
                case '7':
                    output = output + "-###";
                    break;
                case '8':
                    output = output + "#---";
                    break;
                case '9':
                    output = output + "#--#";
                    break;
                case 'A':
                    output = output + "#-#-";
                    break;
                case 'B':
                    output = output + "#-##";
                    break;
                case 'C':
                    output = output + "##--";
                    break;
                case 'D':
                    output = output + "##-#";
                    break;
                case 'E':
                    output = output + "###-";
                    break;
                case 'F':
                    output = output + "####";
                    break;
                }
            }
            output = padding(BBXxOffset, "-") + output;
            outputRows.push_back(output);
            output = "";
        }
        if (BBXy < 16){// we do this to sort out descending and
            // ascending limbs, such as on l, k, q, p etc...
            // not actually used with unifont.bdf
            for (int count = 0; // (+2 - BBXyOffset) is about right
                 // this is the "FONT_DESCENT" in the bdf
                 count < (2 + BBXyOffset); count++) {
                output = paddingLine(paddingLineWidth, "-");
                outputRows.push_back(output);
            }
        } else if (BBXyOffset > 0 ) { // and now add the missing
            // whitespace for things like the tilde, carat, etc..
            // not actually used with unifont.bdf which has full
            // height glyph defined
            for (int count = 0; count < BBXyOffset; count++) {
                output = paddingLine(paddingLineWidth, "-");
                outputRows.push_back(output);
            }
        }
    }

    vector<int8_t> generateAudio(string lastRow,
                                 string currentRow,
                                 string nextRow,
                                 int rowNum) {
        vector<int> summedAudio;
        //    int floorFreq;
        //    int freqSpacing;
        //    int charLineDurationMS
        //    int bitRate
        int samples = ((bitRate*charLineDurationMS)/1000);// per Line
        int taperSamples = ((bitRate*tor)/1000);
        for (int index = 0; index < samples; index++) {
            summedAudio.push_back(0); // zero it out
        }
        double deltaPhase;
        double phaseIncrement;
        for (int chan = 0; chan < currentRow.length(); chan++) { 
            int currentFreq = (chan*freqSpacing + floorFreq);
            deltaPhase = currentFreq*2*3.1417/bitRate;
            phaseIncrement = rowNum*samples*deltaPhase;

            char lastChar = '-';
            char currentChar = currentRow[chan];
            char nextChar = '-';

            if (lastRow.length() != 0) {
                lastChar = lastRow[chan];
            }

            if (nextRow.length() != 0) {
                nextChar = nextRow[chan];
            }

            if ((lastChar == '#')
                && (currentChar == '#')
                && (nextChar == '#')) {
                for (int sample = 0; sample < samples; sample++) {
                    phaseIncrement += deltaPhase;
                    summedAudio[sample]
                        += amplitude*sin(phaseIncrement);
                }
            } else if ((lastChar == '-')
                       && (currentChar == '#')
                       && (nextChar != '#')) { // was == '-', hmm...
                for (int sample = 0; sample < samples; sample++) {
                    phaseIncrement += deltaPhase;
                    summedAudio[sample]
                        += amplitudeEnvelope(amplitude,
                                             samples,
                                             taperSamples,
                                             sample,
                                             1,
                                             1)*sin(phaseIncrement);
                }
            } else if ((lastChar == '-')
                && (currentChar == '#')
                && (nextChar == '#')) {
                for (int sample = 0; sample < samples; sample++) {
                    phaseIncrement += deltaPhase;
                    summedAudio[sample]
                        += amplitudeEnvelope(amplitude,
                                             samples,
                                             taperSamples,
                                             sample,
                                             1,
                                             0)*sin(phaseIncrement);
                }
            } else if ((lastChar == '#')
                && (currentChar == '#')
                && (nextChar == '-')) {
                for (int sample = 0; sample < samples; sample++) {
                    phaseIncrement += deltaPhase;
                    summedAudio[sample]
                        += amplitudeEnvelope(amplitude,
                                             samples,
                                             taperSamples,
                                             sample,
                                             0,
                                             1)*sin(phaseIncrement);
                }
            }
        }
        vector<int8_t> returnAudio;
        for (int index = 0; index < samples; index++) {
            returnAudio.push_back((int8_t)(summedAudio[index]/16));
        }
        summedAudio.clear();
        return returnAudio;
    }

    // we started with a simple linear ramp up and down of tone
    // starts and tone stops in an effort to reduce splatter,
    // now have gaussian envelope
    static int amplitudeEnvelope(int ceiling,
                          int totalSamples,
                          int taperSamples,
                          int count,
                          int ascending,
                          int descending) {
        if (ascending && !descending && (count > taperSamples)) {
            return ceiling;
        } else if (!ascending && descending
                   && (count < (totalSamples - taperSamples))) {
            return ceiling;
        } else if (ascending && !descending  
                   && (count < taperSamples)) {
            return (int)(ceiling*envelope[(int)((count*126)/taperSamples)]);
        } else if (!ascending && descending
                   && (count > (totalSamples - taperSamples))) {
            return (int)(ceiling*envelope[((int)(((totalSamples-count)*126)/taperSamples))]);
        } else if (ascending && descending) {
            //            return ceiling;
            if (count < taperSamples) {
                return (int)(ceiling*envelope[(int)((count*126)/taperSamples)]);
                //                return (count*ceiling)/taperSamples;
            } else if (count < (totalSamples - taperSamples)) {
                return ceiling;
            } else {
                return (int)(ceiling*envelope[((int)(((totalSamples-count)*126)/taperSamples))]);
                //return ((totalSamples-count)*ceiling)/taperSamples;
            }
        } else {
            return ceiling;
        }
    }

    vector<int8_t> vertSymAudio() {
        glyphInit();
        // we ramp audio up and down into/out of the pixel(s)
        // to do this, we need to send the previous and next line
        string thisRow;// = outputRows[row];
        string lastRow;// = "";
        string nextRow;// = "";
        symbolAudio.clear();
        for (int row = 0; row < outputRows.size(); row++) {
            lastRow = "";
            thisRow = outputRows[row];
            nextRow = "";
            if (row > 0) {
                lastRow = outputRows[row - 1];
            }
            if (row < (outputRows.size() - 1)) {
                nextRow = outputRows[row + 1];
            }
            tempAudio = generateAudio(lastRow, thisRow, nextRow, row);
            symbolAudio.insert(symbolAudio.end(),
                               tempAudio.begin(),
                               tempAudio.end());
            tempAudio.clear();
        }
        return symbolAudio;
    }
    // other sym-> audio not implemented properly yet
    vector<int8_t> leftRotSymAudio() {
        glyphInit();
        for (int col = paddingLineWidth; col > 0; col--) {
            for (int row = 0; row < outputRows.size(); row++) {
                std::cout << outputRows[row][col-1];
            }
            std::cout << std::endl;
        }
        return symbolAudio;
    }

    vector<int8_t> rightRotSymAudio() {
        glyphInit();
        for (int col = 0; col < paddingLineWidth; col++) {
            for (int row = outputRows.size(); row > 0; row--) {
                std::cout << outputRows[row-1][col];
            }
            std::cout << std::endl;
        }
        return symbolAudio;
    }

    vector<int8_t> piRotatedSymAudio() {// different directions etc..
        glyphInit();
        for (int row = outputRows.size(); row > 0; row--) {
            for (int col = paddingLineWidth; col > 0; col--) {
                std::cout << outputRows[row-1][col-1];
            }
            std::cout << std::endl;
        }
        return symbolAudio;
    }


    void vertSymAscii() {
        glyphInit();
        for (int row = 0; row < outputRows.size(); row++) {
            std::cout << outputRows[row] << std::endl;
        }
    }

    void leftRotSymAscii() {
        glyphInit();
        for (int col = paddingLineWidth; col > 0; col--) {
            for (int row = 0; row < outputRows.size(); row++) {
                std::cout << outputRows[row][col-1];
            }
            std::cout << std::endl;
        }
    }

    void rightRotSymAscii() {
        glyphInit();
        for (int col = 0; col < paddingLineWidth; col++) {
            for (int row = outputRows.size(); row > 0; row--) {
                std::cout << outputRows[row-1][col];
            }
            std::cout << std::endl;
        }
    }

    void piRotatedSymAscii() {// different waterfall directions etc..
        glyphInit();
        for (int row = outputRows.size(); row > 0; row--) {
            for (int col = paddingLineWidth; col > 0; col--) {
                std::cout << outputRows[row-1][col-1];
            }
            std::cout << std::endl;
        }
    }

};


int hexadecimalToInteger(char textChar, int power) {
    int hexVal;
    switch (textChar) {
    case '0':
        break;
    case '1':
        hexVal += 1*pow(16,power);
        break;
    case '2':
        hexVal += 2*pow(16,power);
        break;
    case '3':
        hexVal += 3*pow(16,power);
        break;
    case '4':
        hexVal += 4*pow(16,power);
        break;
    case '5':
        hexVal += 5*pow(16,power);
        break;
    case '6':
        hexVal += 6*pow(16,power);
        break;
    case '7':
        hexVal += 7*pow(16,power);
        break;
    case '8':
        hexVal += 8*pow(16,power);
        break;
    case '9':
        hexVal += 9*pow(16,power);
        break;
    case 'A':
        hexVal += 10*pow(16,power);
        break;
    case 'a':
        hexVal += 10*pow(16,power);
        break;
    case 'B':
        hexVal += 11*pow(16,power);
        break;
    case 'b':
        hexVal += 11*pow(16,power);
        break;
    case 'C':
        hexVal += 12*pow(16,power);
        break;
    case 'c':
        hexVal += 12*pow(16,power);
        break;
    case 'D':
        hexVal += 13*pow(16,power);
        break;
    case 'd':
        hexVal += 13*pow(16,power);
        break;
    case 'E':
        hexVal += 14*pow(16,power);
        break;
    case 'e':
        hexVal += 14*pow(16,power);
        break;
    case 'F':
        hexVal += 15*pow(16,power);
        break;
    case 'f':
        hexVal += 15*pow(16,power);
        break;
    default:
        hexVal = 0;
    }
    return hexVal;
}

int unicodeToInteger(string text) {
    int hexVal;
    hexVal = 0;
    //std::cout << "Unicode: " << tempString << std::endl;
    for (int textChar = 0; 
         textChar < (text.length()-2); // skip the "U+" start
         textChar++) {
        if (VERBOSE) {
            std::cout << "Processing: " 
                      << text.at(text.length()-textChar-1) 
                      << std::endl;
        }
        char c;
        c = text.at(text.length()-textChar-1);
        hexVal += hexadecimalToInteger(c, textChar);
    }
    return hexVal;
}


map<int, Glyph*> load_bdf_file(std::string filename) {
    std::string latestLine = "";
    std::list<std::string> symbolDef;
    std::ifstream input(filename.c_str());
    int currentID = 0;
    std::map<int, Glyph*> glyphMap;
    while (getline(input, latestLine)) {
        if (!strcmp(latestLine.substr(0,9).c_str(), "STARTCHAR")) {
            symbolDef.clear();
            symbolDef.push_back(latestLine); // store "STARTCHAR line"
            getline(input, latestLine);
            currentID = std::atoi(latestLine.substr(9).c_str());
            while (strcmp(latestLine.substr(0,7).c_str(), "ENDCHAR")
                   && strcmp(latestLine.substr(0,9).c_str(), "STARTCHAR")
                   && strcmp(latestLine.substr(0,7).c_str(), "ENDFONT")) {
                symbolDef.push_back(latestLine);
                getline(input, latestLine);
            }
            if (!strcmp(latestLine.substr(0,7).c_str(), "ENDCHAR")) {
                symbolDef.push_back(latestLine); // append "ENDCHAR"
            }
            Glyph* newGlyph = new Glyph(symbolDef);
            glyphMap.insert(std::pair<int,
                            Glyph*>(currentID, newGlyph));
        }
    }
    input.close();
    symbolDef.clear();
    return glyphMap;
}

int writeGlyphsToAudio(map<int, Glyph*> glyphMap,
                       vector<int> glyphCodes,
                       string fName,
                       int textNumbers) {

    ofstream fOutput(fName.c_str());
    vector<int8_t> temp;
    char buffer[33];
    for (int index = 0; index < glyphCodes.size(); index ++) {
        if (glyphMap[glyphCodes[index]] != 0) {
            temp = glyphMap[glyphCodes[index]]->audioSym('U'); 
            if (VERBOSE) {
                std::cout << "Generating audio for: " 
                          << glyphCodes[index] << std::endl;
            }
        } else {
            std::cout << "Glyph "<< 
                glyphCodes[index] << 
                " not found in bdf file." << std::endl;
            temp.clear();
        }
        if (VERBOSE) {
            std::cout
                << "About to write audio data to file of length: " 
                << temp.size() << std::endl;
        }
        for (int index2 = temp.size(); index2 > 0; index2--) {
            fOutput << (temp[index2-1]); //  << endl;
            sprintf(buffer, "%d", temp[temp.size()-index2]);
            if (textNumbers) {
                std::cout << buffer << std::endl;
            }
        }
    }
    fOutput.close();
    temp.clear();
    return 0;
}

vector<int> stringToGlyphCodeVector(string textToParse,
                                    int extraSpaces ) {
    string tempString = textToParse;
    string tempString2;
    static vector<int> glyphsToRender;
    while (tempString.length() > 0) {
        if ((tempString.length() < 4) ||
            (tempString.length() == 5)) {
            for (int index = 0; index < tempString.length(); index++) {
                glyphsToRender.push_back((int)tempString.at(index));
                if (VERBOSE) {
                    std::cout << "Processing: "
                              << tempString.at(index) << std::endl;
                }
                if (extraSpaces) {
                    glyphsToRender.push_back(32); // add space
                }
            }
            tempString = "";
            if (VERBOSE) {
                std::cout << "textToParse length: "
                          << tempString.length() << std::endl;
            }
        } else if ((!strcmp(tempString.substr(0,2).c_str(), "U+") )
                   && ((tempString.length() == 4)
                       || (tempString.length() == 6))) {
            glyphsToRender.push_back(unicodeToInteger(tempString));
            tempString = "";
            if (VERBOSE) {
                std::cout << "textToParse length: "
                          << tempString.length() << std::endl;
            }
        } else if ((!strcmp(tempString.substr(0,2).c_str(), "U+") )
                   && (tempString.length() > 5)
                   && (!strcmp(tempString.substr(4,2).c_str(), "U+"))) {
            tempString2 = tempString.substr(0,4);
            //tempString = tempString.substr(4);
            tempString.erase(0,4);
            if (VERBOSE) {
                std::cout << "textToParse length: "
                          << tempString.length() << std::endl;
            }
            glyphsToRender.push_back(unicodeToInteger(tempString2));
        } else if ((!strcmp(tempString.substr(0,2).c_str(), "U+") )
                   && (tempString.length() > 7)
                   && (!strcmp(tempString.substr(6,2).c_str(), "U+"))) {
            tempString2 = tempString.substr(0,6);
            tempString.erase(0,6);
            //tempString = tempString.substr(6);
            glyphsToRender.push_back(unicodeToInteger(tempString2));
        } else if ((!strcmp(tempString.substr(0,2).c_str(), "U+") )
                   && (tempString.length() > 9)
                   && (!strcmp(tempString.substr(8,2).c_str(), "U+"))) {
            tempString2 = tempString.substr(0,8);
            //tempString = tempString.substr(8);
            tempString.erase(0,8);
            if (VERBOSE) {
                std::cout << "textToParse length: "
                          << tempString.length() << std::endl;
            }
            glyphsToRender.push_back(unicodeToInteger(tempString2));
        } else if ((!strcmp(tempString.substr(0,2).c_str(), "U+") )
                   && (tempString.length() > 7)) {
            tempString2 = tempString.substr(0,8);
            //tempString = tempString.substr(8);
            tempString.erase(0,8);
            if (VERBOSE) {
                std::cout << "textToParse length: "
                          << tempString.length() << std::endl;
            }
            glyphsToRender.push_back(unicodeToInteger(tempString2));
        } else {
            glyphsToRender.push_back((int)tempString.at(0));
            if (extraSpaces) {
                glyphsToRender.push_back(32); // add space
            }
            //tempString = tempString.substr(1);
            tempString.erase(0,1);
            if (VERBOSE) {
                std::cout << "TempString now: "
                          << tempString << std::endl;
            }
        }
     }
    if (extraSpaces) {
        glyphsToRender.push_back(32); // add space
    }
    if (VERBOSE) {
        std::cout << "Code vector size: " << glyphsToRender.size()
                  << std::endl;

        for (int index = 0; index < glyphsToRender.size(); index++) {
            std::cout << "ASCII: " << glyphsToRender[index]
                      << std::endl;
        }
    }
    return glyphsToRender;
}

int writeGlyphsToAudio(map<int, Glyph*> glyphMap,
                       string glyphString,
                       int extraSpaces,
                       string fName,
                       int textNumbers) {

    return writeGlyphsToAudio(glyphMap,
                              stringToGlyphCodeVector(glyphString,
                                                      extraSpaces),
                              fName,
                              textNumbers);
}


int cleanUpGlyphMap(map<int, Glyph*> theMap) {
    // time to clean up after ourselves
    std::map<int, Glyph*>::iterator iter1, iter2;
    iter1 = theMap.begin();
    iter2 = theMap.end();
    for (; iter1 != iter2; iter1++) {
        //        (*iter1->second).cleanUp(); // not needed
        delete iter1->second;
    }
    return 0;

}


const double Glyph::envelope [127] = {0.003,
                                      0.004,
                                      0.005,
                                      0.007,
                                      0.009,
                                      0.012,
                                      0.013,
                                      0.016,
                                      0.018,
                                      0.020,
                                      0.022,
                                      0.023,
                                      0.026,
                                      0.030,
                                      0.031,
                                      0.035,
                                      0.040,
                                      0.041,
                                      0.046,
                                      0.046,
                                      0.052,
                                      0.058,
                                      0.059,
                                      0.066,
                                      0.073,
                                      0.074,
                                      0.082,
                                      0.084,
                                      0.092,
                                      0.100,
                                      0.103,
                                      0.112,
                                      0.121,
                                      0.125,
                                      0.135,
                                      0.140,
                                      0.150,
                                      0.161,
                                      0.167,
                                      0.178,
                                      0.190,
                                      0.197,
                                      0.209,
                                      0.216,
                                      0.230,
                                      0.243,
                                      0.251,
                                      0.265,
                                      0.280,
                                      0.288,
                                      0.303,
                                      0.313,
                                      0.328,
                                      0.344,
                                      0.354,
                                      0.370,
                                      0.386,
                                      0.397,
                                      0.413,
                                      0.424,
                                      0.441,
                                      0.458,
                                      0.469,
                                      0.486,
                                      0.505,
                                      0.518,
                                      0.531,
                                      0.545,
                                      0.564,
                                      0.577,
                                      0.590,
                                      0.609,
                                      0.616,
                                      0.634,
                                      0.647,
                                      0.659,
                                      0.677,
                                      0.688,
                                      0.700,
                                      0.717,
                                      0.723,
                                      0.739,
                                      0.750,
                                      0.761,
                                      0.776,
                                      0.785,
                                      0.795,
                                      0.809,
                                      0.814,
                                      0.827,
                                      0.834,
                                      0.843,
                                      0.856,
                                      0.862,
                                      0.870,
                                      0.881,
                                      0.883,
                                      0.894,
                                      0.898,
                                      0.905,
                                      0.914,
                                      0.918,
                                      0.924,
                                      0.932,
                                      0.933,
                                      0.941,
                                      0.942,
                                      0.948,
                                      0.955,
                                      0.956,
                                      0.960,
                                      0.966,
                                      0.967,
                                      0.971,
                                      0.971,
                                      0.976,
                                      0.980,
                                      0.981,
                                      0.984,
                                      0.987,
                                      0.989,
                                      0.990,
                                      0.991,
                                      0.994,
                                      0.997,
                                      0.998,
                                      1.000};
