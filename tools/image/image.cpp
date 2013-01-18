#include <cstdio>
#include <string>
#include <vector>
#include <cstdlib>
#include <algorithm>
#include <stdint.h>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <sys/stat.h>
#include <sys/types.h>


void printUsage(char const *programName)
{
	printf("Usage:\n"
	       "\t%s split data/base/images/intfac\n"
	       "\t%s join data/base/images/intfac src/intfac.h\n"
	       "\t%s split data/base/images/frontend\n"
	       "\t%s join data/base/images/frontend src/frend.h\n"
	       "\t\t--no-crush: Don't crush modified png files.\n"
	       "\n"
	       "THIS PROGRAM IS NO LONGER USEFUL (except perhaps as part of converting\n"
	       "old mods).\n"
	       "\n"
	       "The header file argument is optional. After splitting data/base/images/X,\n"
	       "you may edit one or more of resulting data/base/images/X/*.png files, and\n"
	       "then join. After splitting, you may also add files. If adding a file\n"
	       "called image_my_file.png, you should add a dummy entry such as\n"
	       "3,0,0,0,0,0,0,\"image my file\"\n"
	       "which will result in the file being placed on page 3 if possible, or any\n"
	       "page with room, if there isn't room on page 3. The last two numbers give\n"
	       "the offset. The remaining numbers are ignored.\n"
	       "\n", programName, programName, programName, programName);
}

bool shouldCrush = true;

void crushPng(std::string const &pngFilename)
{
	if (!shouldCrush)
	{
		return;
	}
	/*
	system(("pngcrush -o9 " + pngFilename + " TEMPORARY_FILE.png > /dev/null 2> /dev/null").c_str());
	system( "advpng -z4 TEMPORARY_FILE.png > /dev/null 2> /dev/null");
	system(("mv TEMPORARY_FILE.png " + pngFilename + " > /dev/null 2> /dev/null").c_str());
	*/
	system(("optipng -i0 -o5 -f0-5 -strip all " + pngFilename + " > /dev/null 2> /dev/null").c_str());
	system(("advpng -z4 " + pngFilename + " > /dev/null 2> /dev/null").c_str());
}

char upper(char ch)
{
	if (ch >= 'a' && ch <= 'z')
	{
		return ch + 'A' - 'a';
	}
	return ch;
}

char lower(char ch)
{
	if (ch >= 'A' && ch <= 'Z')
	{
		return ch + 'a' - 'A';
	}
	return ch;
}


std::string filter(std::string const &in, bool lowerCase = false)
{
	std::string out;
	for (std::string::const_iterator i = in.begin(); i != in.end(); ++i)
	{
		char ch = '_';
		char up = upper(*i);
		if ((up >= '0' && up <= '9') ||
		    (up >= 'A' && up <= 'Z'))
		{
			ch = lowerCase? lower(up) : up;
		}
		out.push_back(ch);
	}

	return out;
}

static const unsigned DEFAULT_PAGEX = 256, DEFAULT_PAGEY = 256;

struct ImgEntry
{
	ImgEntry() : tPage(0), tx(0), ty(0), sx(0), sy(0), ox(0), oy(0), dirty(true) {}

	//bool isValid() const { return tx < PAGEX && sx <= PAGEX - tx && ty < PAGEY && sy <= PAGEY - ty && tPage < 1000; }

	unsigned tPage;    // Page index.
	unsigned tx, ty;   // Location of top left corner on page.
	unsigned sx, sy;   // Width and height.
	int ox, oy;        // Offset, used when blitting.
	std::string name;  // Name of image. Capital letters only.

	QImage data;

	bool dirty;        // If true, tPage, tx and ty are arbitrary.
};

struct ImgPage
{
	QImage data;       // Pixel data.

	bool dirty;        // If true, image data has changed, and the page file needs saving.
};

struct ImgFile
{
	typedef std::vector<ImgEntry> Entries;
	typedef std::vector<ImgPage> Pages;
	Entries entries;
	Pages pages;
};

void arrangeEntryOnPage(ImgFile &img, ImgEntry &entry, unsigned page)
{
	unsigned PAGEX = img.pages[page].data.width();
	unsigned PAGEY = img.pages[page].data.height();
	if ((PAGEX > DEFAULT_PAGEX ||
	     PAGEY > DEFAULT_PAGEY) &&
	    !(entry.sx > DEFAULT_PAGEX ||
	      entry.sy > DEFAULT_PAGEY))
	{
//printf("Skip page %u = (%u,%u), need (%u,%u)\n", page, PAGEX, PAGEY, entry.sx, entry.sy);
		return;  // This page is special...
	}

	std::vector<bool> pageMap(PAGEX*PAGEY, false);
	for (unsigned e = 0; e < img.entries.size(); ++e)
	{
		if (img.entries[e].tPage == page && !img.entries[e].dirty)
		{
			for (unsigned y = img.entries[e].ty; y < std::min(PAGEY, img.entries[e].ty + img.entries[e].sy); ++y)
				for (unsigned x = img.entries[e].tx; x < std::min(PAGEX, img.entries[e].tx + img.entries[e].sx); ++x)
			{
				pageMap[x + y*PAGEX] = true;
			}
		}
	}
	std::vector<int> cy(PAGEX, entry.sy - 1);
	for (unsigned y = 0; y < PAGEY; ++y)
	{
		int cx = entry.sx - 1;
		for (unsigned x = 0; x < PAGEX; ++x)
		{
			if (pageMap[x + y*PAGEX])
			{
				cx = entry.sx;
			}
			pageMap[x + y*PAGEX] = --cx >= 0 || pageMap[x + y*PAGEX];

			if (pageMap[x + y*PAGEX])
			{
				cy[x] = entry.sy;
			}
			pageMap[x + y*PAGEX] = --cy[x] >= 0 || pageMap[x + y*PAGEX];
		}
	}
	for (unsigned y = 0; y != PAGEY; ++y)
		for (unsigned x = 0; x != PAGEX; ++x)
	{
		if (!pageMap[x + y*PAGEX])
		{
			entry.tPage = page;
			entry.tx = x + 1 - entry.sx;
			entry.ty = y + 1 - entry.sy;
			entry.dirty = false;
			img.pages[entry.tPage].dirty = true;
			return;
		}
	}
}

void arrangeEntry(ImgFile &img, ImgEntry &entry)
{
	if (entry.tPage < img.pages.size())
	{
		arrangeEntryOnPage(img, entry, entry.tPage);
	}

	for (unsigned page = 0; page < img.pages.size() && entry.dirty; ++page)
	{
		arrangeEntryOnPage(img, entry, page);
	}

	if (!entry.dirty)
	{
		return;  // Done.
	}

	ImgPage newPage;
	newPage.data = QImage(DEFAULT_PAGEX, DEFAULT_PAGEY, QImage::Format_ARGB32);
	img.pages.push_back(newPage);

	arrangeEntryOnPage(img, entry, img.pages.size() - 1);
	if (entry.dirty)
	{
		printf("Couldn't find room for entry on a blank page, entry (%u, %u) too big?\n", entry.sx, entry.sy);
		exit(1);
	}
}

void arrangeEntries(ImgFile &img)
{
	std::vector<std::pair<std::pair<int, int>, unsigned> > dirtyEntries;
	for (unsigned i = 0; i < img.entries.size(); ++i)
	{
		if (img.entries[i].dirty)
		{
			int sx = img.entries[i].sx;
			int sy = img.entries[i].sy;
			dirtyEntries.push_back(std::make_pair(std::make_pair(-std::max(sx, sy), -std::min(sx, sy)), i));
		}
	}
	std::sort(dirtyEntries.begin(), dirtyEntries.end());  // Sort by width and then by height.
	for (unsigned i = 0; i < dirtyEntries.size(); ++i)
	{
		arrangeEntry(img, img.entries[dirtyEntries[i].second]);
	}
}

void writeEntry(ImgFile &img, ImgEntry const &entry)
{
	QPainter paint(&img.pages[entry.tPage].data);
	paint.setCompositionMode(QPainter::CompositionMode_Source);
	paint.drawImage(entry.tx, entry.ty, entry.data);
}

void writePages(ImgFile &img)
{
	for (unsigned i = 0; i < img.pages.size(); ++i)
	{
		//img.pages[i].data.fill(0x80FF0080);  // Make empty areas pink semitransparent.
		img.pages[i].data.fill(0x00FFFFFF);  // Make empty areas white transparent.
	}
	for (unsigned i = 0; i < img.entries.size(); ++i)
	{
		writeEntry(img, img.entries[i]);
	}
}

ImgFile parseImgFile(std::string const &fName)
{
	ImgFile img;

	FILE *f = fopen((fName + ".img").c_str(), "rb");
	if (f == NULL)
	{
		printf("Couldn't read file \"%s\".\n", (fName + ".img").c_str());
		exit(1);
	}
	char buf[101];
	ImgEntry e;
	while (fscanf(f, "%u,%u,%u,%u,%u,%d,%d,\"%100[0-9A-Z _]\"\n", &e.tPage, &e.tx, &e.ty, &e.sx, &e.sy, &e.ox, &e.oy, buf) == 8)
	{
		e.name = buf;
		e.dirty = false;
		img.entries.push_back(e);

		if (e.tPage > 100 || img.entries.size() > 100000)
		{
			printf("Invalid entry \"%s\" in file \"%s\".\n", buf, (fName + ".img").c_str());
			exit(1);
		}

		if (e.tPage >= img.pages.size())
		{
			img.pages.resize(e.tPage + 1);
		}
	}
	if (!feof(f))
	{
		printf("Invalid entry in file \"%s\".\n", (fName + ".img").c_str());
		exit(1);
	}
	fclose(f);

	for (unsigned i = 0; i < img.pages.size(); ++i)
	{
		char index[100];
		sprintf(index, "%u.png", i);
		img.pages[i].data = QImage((fName + index).c_str()).convertToFormat(QImage::Format_ARGB32);
		img.pages[i].dirty = false;
		if (img.pages[i].data.isNull())
		{
			printf("Error loading file \"%s\".\n", (fName + index).c_str());
			exit(1);
		}
	}

	for (unsigned i = 0; i < img.entries.size(); ++i)
	{
		ImgEntry &e = img.entries[i];
		e.data = img.pages[e.tPage].data.copy(e.tx, e.ty, e.sx, e.sy);
	}

	return img;
}

void writeImgFile(std::string const &fName, ImgFile const &img)
{
	FILE *f = fopen((fName + ".img").c_str(), "wb");
	if (f == NULL)
	{
		printf("Couldn't write file \"%s\".\n", (fName + ".img").c_str());
		exit(1);
	}

	for (ImgFile::Entries::const_iterator i = img.entries.begin(); i != img.entries.end(); ++i)
	{
		fprintf(f, "%u,%u,%u,%u,%u,%d,%d,\"%s\"\n", i->tPage, i->tx, i->ty, i->sx, i->sy, i->ox, i->oy, i->name.c_str());
	}

	fclose(f);

	for (unsigned i = 0; i < img.pages.size(); ++i)
	{
		if (!img.pages[i].dirty)
		{
			continue;  // Image not changed, no need to resave.
		}
		char index[100];
		sprintf(index, "%u.png", i);
		std::string pageFilename = fName + index;
		if (!img.pages[i].data.save(pageFilename.c_str()))
		{
			printf("Error saving file \"%s\".\n", pageFilename.c_str());
			exit(1);
		}
		crushPng(pageFilename);
	}
}

void readSplitFiles(std::string const &fName, ImgFile &img)
{
	std::string dir = fName + "/";
	for (ImgFile::Entries::iterator i = img.entries.begin(); i != img.entries.end(); ++i)
	{
		std::string png = dir + filter(i->name, true) + ".png";
		QImage newData = QImage(png.c_str()).convertToFormat(QImage::Format_ARGB32);
		if (newData.isNull())
		{
			printf("Error loading file \"%s\".\n", png.c_str());
			exit(1);
		}
		if (i->data == newData)
		{
			continue;  // Nothing changed.
		}
		i->data = newData;
		if (i->sx != (unsigned)i->data.width() ||
		    i->sy != (unsigned)i->data.height())
		{
			// Image size changed, mark it dirty, since it probably needs to be moved.
			i->sx = i->data.width();
			i->sy = i->data.height();
			i->dirty = true;
		}
		else
		{
			// Image size did not change, so can just write it where it was.
			img.pages[i->tPage].dirty = true;
		}
	}
}

void writeSplitFiles(std::string const &fName, ImgFile const &img)
{
	std::string dir = fName + "/";
	mkdir(dir.c_str(), 0755);

	for (ImgFile::Entries::const_iterator i = img.entries.begin(); i != img.entries.end(); ++i)
	{
		std::string png = dir + filter(i->name, true) + ".png";
		i->data.save(png.c_str());
		crushPng(png);
	}
}

void writeHeaderFile(std::string const &fName, ImgFile const &img)
{
	FILE *f = fopen(fName.c_str(), "wb");
	if (f == NULL)
	{
		printf("Couldn't write file \"%s\".\n", fName.c_str());
		exit(1);
	}

	fprintf(f, "// THIS IS AN AUTOGENERATED HEADER! DO NOT EDIT (since your changes would be lost, anyway)!\n"
	           "\n"
	           "#ifndef __INCLUDED_%s__\n"
	           "#define __INCLUDED_%s__\n"
	           "\n"
	           "enum\n"
	           "{\n", filter(fName).c_str(), filter(fName).c_str());
	for (ImgFile::Entries::const_iterator i = img.entries.begin(); i != img.entries.end(); ++i)
	{
		fprintf(f, "\t%s,\n", filter(i->name).c_str());
	}
	fprintf(f, "};\n"
	           "\n"
	           "#endif //__INCLUDED_%s__\n", filter(fName).c_str());

	fclose(f);
}

int main(int argc, char **argv)
{
	char const *programName = argv[0];

	if (argc >= 2 && std::string(argv[1]) == "--no-crush")
	{
		--argc;
		++argv;
		shouldCrush = false;
	}

	if (argc != 3 && argc != 4)
	{
		printUsage(programName);
		return 1;
	}

	std::string action = argv[1];
	std::string base = argv[2];
	std::string header;
	if (argc == 4)
	{
		header = argv[3];
	}

	if (action == "split")
	{
		ImgFile img = parseImgFile(base);

		writeSplitFiles(base, img);
		/*for (ImgFile::Entries::const_iterator i = img.entries.begin(); i != img.entries.end(); ++i)
		{
			printf("%u,%u,%u,%u,%u,%d,%d,\"%s\"\n", i->tPage, i->tx, i->ty, i->sx, i->sy, i->ox, i->oy, i->name.c_str());
		}*/
	}
	else if (action == "join" || action == "repack")
	{
		ImgFile img = parseImgFile(base);
		readSplitFiles(base, img);

		if (action == "repack")
		{
			for (unsigned i = 0; i < img.entries.size(); ++i)
			{
				img.entries[i].tPage = 0;
				img.entries[i].dirty = true;
			}
		}

		arrangeEntries(img);

		writePages(img);

		writeImgFile(base, img);
		if (!header.empty())
		{
			writeHeaderFile(header, img);
		}
	}
	else
	{
		printUsage(programName);
		return 1;
	}
}
