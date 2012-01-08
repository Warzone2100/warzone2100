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
#include <cmath>


void printUsage(char const *programName)
{
	printf("Usage:\n"
	       "\t%s join add.png blend.png premult.png\n"
	       //"\t%s split premult.png add.png blend.png\n"
	       "\t\t--no-crush: Don't crush modified png files.\n"
	       "\n"
	       "Join  merges  add.png  and  blend.png  into  premult.png.   Split  splits\n"
	       "premult.png  into add.png and blend.png.  Splitting  just guesses  how to\n"
	       "split.\n"
	       "\n", programName/*, programName*/);
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
	system(("optipng -o7 " + pngFilename + " > /dev/null 2> /dev/null").c_str());
	system(("advpng -z4 " + pngFilename + " > /dev/null 2> /dev/null").c_str());
}

struct Pix
{
	Pix() {}
	Pix(unsigned argb)
	{
		unsigned char aa = argb>>24;
		unsigned char rr = argb>>16;
		unsigned char gg = argb>>8;
		unsigned char bb = argb;
		a = aa/255.;
		r = rr/255.;
		g = gg/255.;
		b = bb/255.;
	}
	unsigned val() const
	{
		int aa = std::min(std::max<int>(a*255. + .5, 0), 255);
		int rr = std::min(std::max<int>(r*255. + .5, 0), 255);
		int gg = std::min(std::max<int>(g*255. + .5, 0), 255);
		int bb = std::min(std::max<int>(b*255. + .5, 0), 255);
		return aa<<24 | rr<<16 | gg<<8 | bb;
	}

	double a, r, g, b;
};

int main(int argc, char **argv)
{
	char const *programName = argv[0];

	if (argc >= 2 && std::string(argv[1]) == "--no-crush")
	{
		--argc;
		++argv;
		shouldCrush = false;
	}

	if (argc != 5)
	{
		printUsage(programName);
		return 1;
	}

	std::string action = argv[1];

	if (action == "split")
	{
		printf("split not implemented\n");
	}
	else if (action == "join")
	{
		std::string nAdd = argv[2];
		std::string nBlend = argv[3];
		std::string nPremult = argv[4];

		QImage add(nAdd.c_str());
		QImage blend(nBlend.c_str());
		add = add.convertToFormat(QImage::Format_ARGB32);
		blend = blend.convertToFormat(QImage::Format_ARGB32);
		QImage premult(add.width(), add.height(), QImage::Format_ARGB32);
		QImage premultOnBlack(add.width(), add.height(), QImage::Format_ARGB32);
		QImage premultOnTexture(add.width(), add.height(), QImage::Format_ARGB32);
		if (blend.size() != add.size())
		{
			printf("Input files not same dimensions, %s = (%d, %d), %s = (%d, %d).\n", nAdd.c_str(), add.width(), add.height(), nBlend.c_str(), blend.width(), blend.height());
			return 1;
		}

		//premult = blend;
		//premult.convertToFormat(QImage::Format_ARGB32_Premultiplied);
		for (int y = 0; y < add.height(); ++y)
			for (int x = 0; x < add.width(); ++x)
		{
			Pix a(add.pixel(x, y));
			Pix b(blend.pixel(x, y));
			Pix p;

			b.a = 1 - ((1 - b.a) + (b.r+b.g+b.b)*b.a/3);
			b.r = b.g = b.b = 0;

			p.a = b.a;
			p.r = a.r*(1 - b.a) + b.r*b.a;
			p.g = a.g*(1 - b.a) + b.g*b.a;
			p.b = a.b*(1 - b.a) + b.b*b.a;
			premult.setPixel(x, y, p.val());

			Pix texture;
			/*texture.r = ((x^y)&8)? 0.25 : 0.75;
			texture.g = ((x-y)&8)? 0.25 : 0.75;
			texture.b = ((x+y+4)&8)? 0.25 : 0.75;*/
			texture.r = .5 + .25*sin(x*2*355/113./30)*sin(y*2*355/113./30);
			texture.g = .5 + .25*sin((x+y)*2*355/113./30);
			texture.b = .5 + .25*sin((x-y)*2*355/113./30);
/*
			Pix pOnBlack;
			pOnBlack.a = 1;
			pOnBlack.r = p.r;
			pOnBlack.g = p.g;
			pOnBlack.b = p.b;
			premult.setPixel(x, y, pOnBlack.val());

			Pix pOnTexture;
			pOnTexture.a = 1;
			pOnTexture.r = texture.r*(1 - p.a) + p.r;
			pOnTexture.g = texture.g*(1 - p.a) + p.g;
			pOnTexture.b = texture.b*(1 - p.a) + p.b;
			premult.setPixel(x, y, pOnTexture.val());
			//*/
		}

		premult.save(nPremult.c_str());
		if (shouldCrush)
			crushPng(nPremult.c_str());
	}
	else
	{
		printUsage(programName);
		return 1;
	}
}
