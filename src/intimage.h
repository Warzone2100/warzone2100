#ifndef __INCLUDED_INTIMAGE__
#define __INCLUDED_INTIMAGE__

#include "intfac.h" // Interface image id's.

#define FILLRED 16
#define FILLGREEN 16
#define FILLBLUE 128
#define FILLTRANS 128


// Sprite image structure.
typedef struct {
	UDWORD BMPNum;	// Source bitmap index.
	UDWORD Offset;	// byte offset within source bitmap.
	UDWORD Width;	// Width of image.
	UDWORD Height;	// Height of image.
	UDWORD Modulus;	// Width of source bitmap.
	SDWORD XOffset;	// X offset for drawing.
	SDWORD YOffset;	// Y offset for drawing.
} IMAGE;



enum {
	FR_IGNORE,	// Fill rect is ignored.
	FR_FRAME,	// Fill rect drawn relative to frame.
	FR_LEFT,	// Fill rect drawn relative to left of frame.
	FR_RIGHT,	// Fill rect drawn relative to right of frame.
	FR_TOP,		// Fill rect drawn relative to top of frame.
	FR_BOTTOM,	// Fill rect drawn relative to bottom of frame.
};

enum {
	FR_SOLID,	// Bitmap drawn solid.
	FR_KEYED,	// Bitmap drawn with colour 0 transparent.
};

typedef struct {
	UWORD Type;					// One of the FR_... values.
	SWORD TLXOffset,TLYOffset;	// Offsets for the rect fill.
	SWORD BRXOffset,BRYOffset;
	UBYTE ColourIndex;
} FRAMERECT;

// Frame definition structure.
typedef struct {
	SWORD OffsetX0,OffsetY0;	// Offset top left of frame.
	SWORD OffsetX1,OffsetY1;	// Offset bottom right of frame.
	SWORD TopLeft;				// Image indecies for the corners ( -1 = don't draw).
	SWORD TopRight;
	SWORD BottomLeft;
	SWORD BottomRight;
	SWORD TopEdge,TopType;		// Image indecies for the edges ( -1 = don't draw). Type ie FR_SOLID or FR_KEYED.
	SWORD RightEdge,RightType;
	SWORD BottomEdge,BottomType;
	SWORD LeftEdge,LeftType;
	FRAMERECT FRect[5];			// Fill rectangles.
} IMAGEFRAME;

typedef struct {
	SWORD MajorUp;			// Index of image to use for tab not pressed.
	SWORD MajorDown;		// Index of image to use for tab pressed.
	SWORD MajorHilight;		// Index of image to use for tab hilighted by mouse.
	SWORD MajorSelected;	// Index of image to use for tab selected ( same as pressed ).

	SWORD MinorUp;			// As above but for minot tabs.
	SWORD MinorDown;
	SWORD MinorHilight;
	SWORD MinorSelected;
} TABDEF;

extern IMAGEFILE *IntImages;	// All the 2d graphics for the user interface.


// A few useful defined frames.
extern IMAGEFRAME FrameNormal;
extern IMAGEFRAME FrameDesignComponent;
extern IMAGEFRAME FrameRadar;
extern IMAGEFRAME FrameObject;
extern IMAGEFRAME FrameStats;
extern IMAGEFRAME FrameDesignLeft;
extern IMAGEFRAME FrameDesignRight;
extern IMAGEFRAME FrameDesignCenter;
extern IMAGEFRAME FrameDesignComponent;
extern IMAGEFRAME FrameDesignView;
extern IMAGEFRAME FrameDesignHilight;
extern IMAGEFRAME FrameText;

// A few useful defined tabs.
extern TABDEF StandardTab;
extern TABDEF SystemTab;

extern TABDEF SmallTab;


extern BOOL imageInitBitmaps(void);

extern void imageDeleteBitmaps(void);

// Draws a transparent window
extern void RenderWindowFrame(IMAGEFRAME *Frame,UDWORD x,UDWORD y,UDWORD Width,UDWORD Height);

// Draws a solid window
extern void RenderOpaqueWindow(IMAGEFRAME *Frame,UDWORD x,UDWORD y,UDWORD Width,UDWORD Height);

// Called by RenderWindowFrame and RenderOpaqueWindow but you can call it yourself if you want.
extern void RenderWindow(IMAGEFRAME *Frame,UDWORD x,UDWORD y,UDWORD Width,UDWORD Height,BOOL Opaque);

#endif
