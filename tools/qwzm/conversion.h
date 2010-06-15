#ifndef CONVERSION_H
#define CONVERSION_H
#include <GL/gl.h>

#include "ui_pieexport.h"

#define VERTICES_PER_TRIANGLE 3

// Load PIE files
#define iV_IMD_TEX	0x00000200
#define iV_IMD_XNOCUL	0x00002000
#define iV_IMD_TEXANIM	0x00004000
#define iV_IMD_TCMASK	0x00010000
#define MAX_POLYGON_SIZE 16

#define OLD_TEXTURE_SIZE_FIX 256.0f

#define pie_MAX_VERTICES		768
#define pie_MAX_POLYGONS		512
#define pie_MAX_VERTICES_PER_POLYGON	6

typedef struct
{
	int index[MAX_POLYGON_SIZE];
	GLfloat texCoord[MAX_POLYGON_SIZE][2];
	int vertices;
	int frames, rate; // animation data
	GLfloat  width, height; // animation data
	bool cull;
} WZ_FACE;

typedef struct
{
	GLfloat x, y, z;
} WZ_POSITION;

class pie_Point
{
private:
	GLfloat x, y, z;
public:

	pie_Point(GLfloat x, GLfloat y, GLfloat z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}

	void getXYZ(GLfloat &x, GLfloat &y, GLfloat &z) const
	{
		x = this->x;
		y = this->y;
		z = this->z;
	}

	bool operator < (const pie_Point& rhs) const
	{
		if (this->x == rhs.x && this->y == rhs.y)
		{
			return (this->z < rhs.z);
		}
		else if (this->x == rhs.x)
		{
			return (this->y < rhs.y);
		}
		else
		{
			return (this->x < rhs.x);
		}
	}

	bool operator == (const pie_Point& rhs) const
	{
		return (this->x == rhs.x && this->x == rhs.x && this->x == rhs.x);
	}
};

struct textCoords
{
	GLfloat u, v;
	textCoords(GLfloat u, GLfloat v)
	{
		this->u = u;
		this->v = v;
	}
};

/** PIE Export dialog*/
class QPieExportDialog : public QDialog, private Ui_PieExportDialog
{
public:
	QPieExportDialog(QWidget *parent = 0);
	~QPieExportDialog();
	int getPieVersion();
	int getFlags();
};
#endif // CONVERSION_H
