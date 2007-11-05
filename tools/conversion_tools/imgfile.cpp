/*
	This file is part of Warzone 2100.
	Copyright (C) 2007  Giel van Schijndel
	Copyright (C) 2007  Warzone Resurrection Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

	$Revision$
	$Id$
	$HeadURL$
*/

#include <iostream>
#include <vector>

struct IMAGEHEADER
{
    uint16_t Version;
    uint16_t NumImages;
    uint16_t BitDepth;
    uint16_t NumTPages;
    char     TPageFiles[16][16];
};

struct IMAGEDEF
{
    uint16_t TPageID;
    uint16_t Tu;
    uint16_t Tv;
    uint16_t Width;
    uint16_t Height;
    int16_t XOffset;
    int16_t YOffset;
};

struct iTexture
{
    iTexture(unsigned short PageID_, const std::string& fileName_) :
        PageID(PageID_),
        fileName(fileName_)
    {}

    unsigned short PageID;
    std::string fileName;
};

/** This template class makes it possible to output subclasses to std::ostream
 *  streams. It will call the member function write_text(std::ostream&) of the
 *  derived class for that purpose.
 *
 *  This technique is also known as the Barton-Nackman trick. See the
 *  Wikipedia article for more information:
 *  http://en.wikipedia.org/wiki/Barton-Nackman_trick
 */
template <typename T>
class text_output_stream
{
    friend inline std::ostream& operator<<(std::ostream& output, const T& to_output)
    {
        to_output.write_text(output);

        return output;
    }
};

/** This class reads in a binary IMAGEFILE and allows it to be written out
 *  again as XML.
 */
struct IMAGEFILE : private text_output_stream<IMAGEFILE>
{
    IMAGEFILE(std::istream& file)
    {
        // Read header from file
        file.read(reinterpret_cast<std::istream::char_type*>(&Header.Version), sizeof(Header.Version));
        file.read(reinterpret_cast<std::istream::char_type*>(&Header.NumImages), sizeof(Header.NumImages));
        file.read(reinterpret_cast<std::istream::char_type*>(&Header.BitDepth), sizeof(Header.BitDepth));
        file.read(reinterpret_cast<std::istream::char_type*>(&Header.NumTPages), sizeof(Header.NumTPages));
        file.read(reinterpret_cast<std::istream::char_type*>(&Header.TPageFiles), sizeof(Header.TPageFiles));

        TexturePages.reserve(Header.NumTPages);
        ImageDefs.resize(Header.NumImages);

        // Load the texture pages
        for (unsigned int i = 0; i < Header.NumTPages; ++i)
        {
            TexturePages.push_back(iTexture(i, Header.TPageFiles[i]));
        }

        for (std::vector<IMAGEDEF>::iterator ImageDef = ImageDefs.begin(); ImageDef != ImageDefs.end(); ++ImageDef)
        {
            // Read image definition from file
            file.read(reinterpret_cast<std::istream::char_type*>(&ImageDef->TPageID), sizeof(ImageDef->TPageID));
            file.read(reinterpret_cast<std::istream::char_type*>(&ImageDef->Tu), sizeof(ImageDef->Tu));
            file.read(reinterpret_cast<std::istream::char_type*>(&ImageDef->Tv), sizeof(ImageDef->Tv));
            file.read(reinterpret_cast<std::istream::char_type*>(&ImageDef->Width), sizeof(ImageDef->Width));
            file.read(reinterpret_cast<std::istream::char_type*>(&ImageDef->Height), sizeof(ImageDef->Height));
            file.read(reinterpret_cast<std::istream::char_type*>(&ImageDef->XOffset), sizeof(ImageDef->XOffset));
            file.read(reinterpret_cast<std::istream::char_type*>(&ImageDef->YOffset), sizeof(ImageDef->YOffset));

            if (ImageDef->Width <= 0
             || ImageDef->Height <= 0)
            {
                throw std::string("IMAGEFILE::IMAGEFILE(std::istream& file): Illegal image size");
            }
        }
    }


    IMAGEHEADER Header;
    std::vector<iTexture> TexturePages;
    unsigned short NumCluts;
    //unsigned short TPageIDs[16];
    unsigned short ClutIDs[48];
    std::vector<IMAGEDEF> ImageDefs;

    void write_text(std::ostream& file) const
    {
        file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
             << "<image version=\"" << Header.Version << "\" bitdepth=\"" << Header.BitDepth << "\">\n";

        for (std::vector<iTexture>::const_iterator texPage = TexturePages.begin(); texPage != TexturePages.end(); ++texPage)
        {
            file << "    <texturepage src=\"" << texPage->fileName << "\" id=\"" << texPage->PageID << "\" />\n";
        }

        file << "\n";

        for (std::vector<IMAGEDEF>::const_iterator ImageDef = ImageDefs.begin(); ImageDef != ImageDefs.end(); ++ImageDef)
        {
            file << "    <imagedefinition pageid=\"" << ImageDef->TPageID << "\" width=\"" << ImageDef->Width << "\" height=\"" << ImageDef->Height << "\"  xoffset=\"" << ImageDef->XOffset << "\"   yoffset=\"" << ImageDef->YOffset << "\">\n"
                 << "        <u>" << ImageDef->Tu << "</u>\n"
                 << "        <v>" << ImageDef->Tv << "</v>\n"
                 << "    </imagedefinition>\n";
        }

        file << "</image>" << std::endl;
    }
};

int main(int argc, const char** argv)
{
    try
    {
        // Read in a binary image file from standard input and write it out as XML to standard output
        std::cout << IMAGEFILE(std::cin);
    }
    catch (std::string& e)
    {
        std::cerr << "unhandled exception (error): " << e << std::endl;
    }
    catch (...)
    {
        std::cerr << "unhandled and unknown exception (error) occurred" << std::endl;

        // rethrow exception
        throw;
    }

    return 0;
}
