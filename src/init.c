/*
 * Init.c
 *
 * Game initialisation routines.
 *
 */

#include "stdio.h"
#include "Frame.h"
#include "Init.h"
#include "Mechanics.h"
#include "Objects.h"
#include "Display.h"
#include "AI.h"
#include "astar.h"
#include "Disp2D.h"
#include "HCI.h"
#include "audio.h"
#include "csnap.h"
#include "Wrappers.h"
#include "time.h"
#include "Game.h"
#include "animobj.h"
#include "ani.h"
#include "drive.h"
#include "bucket3d.h"
#include "message.h"
#include "data.h"
#include "RayCast.h"
#include "Text.h"
#include "Console.h"

#include "piedef.h"
#include "pieState.h" 
#ifdef WIN32
#include "config.h"
#include "pieMode.h"
#include "tex.h"
#endif
#include "resource.h"
#include "rendmode.h"
#include "ivi.h"
#include "Group.h"
#include "wrappers.h"
#include "display3D.h"
#ifdef WIN32
#include "Atmos.h"
#include "Environ.h"
#include "warzoneConfig.h"
#include "multiplay.h"
#endif
#include "Script.h"
#include "ScriptTabs.h"
#include "ScriptVals.h"
#include "MiscImd.h"
#include "KeyMap.h"
#include "edit3D.h"
#include "component.h"
#include "Fpath.h"
#include "WinMain.h"
#include "wdg.h"
#include "multiWDG.h"

#ifdef ARROWS
#include "arrow.h"
#endif

#ifdef WIN32
#include "Texture.h"
#endif

#include "Frend.h"

#ifdef PSX
#include "Frend16.h"
#include	"locale.h"
#endif

#include "IntImage.h"
#include "power.h"
#include "deliverance.h"
#include "Radar.h"
#include "Audio_ID.h"
#include "IntDisplay.h"
#include "FormationDef.h"
#include "Formation.h"

#ifdef WIN32
//#include "Glide.h"
#include "cdaudio.h"
#include "mixer.h"
#include "AdvVis.h"
#endif

#include "Mission.h"
#include "Transporter.h"
#include "Projectile.h"
#include "Levels.h"
#include "Loop.h"
#include "CmdDroid.h"
#include "Effects.h"
#include "ScriptExtern.h"
#include "MapGrid.h"
#include "Cluster.h"
#include "Gateway.h"
#include "Lighting.h"

#ifdef PSX
#include "VPad.h"
#include "ctrlpsx.h"
#endif

extern void statsInitVars(void);
extern void	structureInitVars(void);
extern BOOL messageInitVars(void);
extern BOOL researchInitVars(void);
extern void	featureInitVars(void);
extern void radarInitVars(void);
extern void	initMiscVars( void );



#ifdef WIN32

 // the sizes for the game block heap
 #define GAMEBLOCK_INIT		(2*1024*1024)
 #define GAMEBLOCK_EXT		(1024*1024)
 // the sizes for the campaign map block heap
 #define MAPBLOCK_INIT		(1024*1024)
 #define MAPBLOCK_EXT		(32*1024)
 // the sizes for the mission block heap
 #define MISSIONBLOCK_INIT		(2*1024*1024)
 #define MISSIONBLOCK_EXT		(512*1024)
				  
#else

 // the sizes for the game block heap
 
 #ifndef FINALBUILD
 	#define GAMEBLOCK_INIT		(1680000)
 	#define MAPBLOCK_INIT		(512*1024)
 	#define MISSIONBLOCK_INIT	(1656000)
 #else
 	#ifdef COVERMOUNT				   
 		#define GAMEBLOCK_INIT		(700000) 	// 726000
 		#define MAPBLOCK_INIT		(49816)	
 		#define MISSIONBLOCK_INIT	(69000)	   // needs to be 69000
 	#else
 		// New sizes based on max usage in CAM1,CAM2 & CAM3 8/3/99.
 		#define MISSIONBLOCK_INIT	(82000+1024)		//(90000-4096)	//(106476) //(106476)		// 2nd biggest is 66532
 		#define MAPBLOCK_INIT		(120000)	//(132000)	//(119688+1024*14)	//(119688)
   		#define GAMEBLOCK_INIT		(600000+1024)	//(620000-1024)	//(650000-4096)	//(641452)		//(784128)
 		//							________	  _______
 		// Total					(801310)	//(836880)	 	865832
 	#endif
 #endif
 
 #define GAMEBLOCK_EXT		(0)
 #define MISSIONBLOCK_EXT	(0)
 #define MAPBLOCK_EXT		0
 
#endif


// the block heap for the game data
BLOCK_HEAP	*psGameHeap;

// the block heap for the campaign map
BLOCK_HEAP	*psMapHeap;

// the block heap for the pre WRF data
BLOCK_HEAP	*psMissionHeap;

// the block id for the game data
#define GAME_BLOCKID	100



// Ascii to font image id lookup table for frontend font.
// Same for WIN32 and PSX.
//
#ifdef WIN32
UWORD FEAsciiLookup[256] = 
//#else
//// We can use bytes as long as we ensure the font images are the 1st 256 in the image file.
//UBYTE FEAsciiLookup[256] = 
//#endif
{
	IMAGE_TFONT_63,	//0		0..32 are all mapped to question marks
	IMAGE_TFONT_63,	//1
	IMAGE_TFONT_63,	//2
	IMAGE_TFONT_63,	//3
	IMAGE_TFONT_63,	//4
	IMAGE_TFONT_63,	//5
	IMAGE_TFONT_63,	//6
	IMAGE_TFONT_63,	//7
	IMAGE_TFONT_63,	//8
	IMAGE_TFONT_63,	//9
	IMAGE_TFONT_63,	//10
	IMAGE_TFONT_63,	//11
	IMAGE_TFONT_63,	//12
	IMAGE_TFONT_63,	//13
	IMAGE_TFONT_63,	//14
	IMAGE_TFONT_63,	//15
	IMAGE_TFONT_63,	//16
	IMAGE_TFONT_63,	//17
	IMAGE_TFONT_63,	//18
	IMAGE_TFONT_63,	//19
	IMAGE_TFONT_63,	//20
	IMAGE_TFONT_63,	//21
	IMAGE_TFONT_63,	//22
	IMAGE_TFONT_63,	//23
	IMAGE_TFONT_63,	//24
	IMAGE_TFONT_63,	//25
	IMAGE_TFONT_63,	//26
	IMAGE_TFONT_63,	//27
	IMAGE_TFONT_63,	//28
	IMAGE_TFONT_63,	//29
	IMAGE_TFONT_63,	//30
	IMAGE_TFONT_63,	//31
	IMAGE_TFONT_63,	//32	// space is skipped and not drawn anyway
	IMAGE_TFONT_33,	//33	!
	IMAGE_TFONT_34,	//34	"
	IMAGE_TFONT_35,	//35	#
	IMAGE_TFONT_36,	//36	$
	IMAGE_TFONT_37,	//37	
	IMAGE_TFONT_38,	//38	&
	IMAGE_TFONT_39,	//39	'
	IMAGE_TFONT_40,	//40	(
	IMAGE_TFONT_41,	//41	)
	IMAGE_TFONT_42,	//42	*
	IMAGE_TFONT_43,	//43	+
	IMAGE_TFONT_44,	//44	,
	IMAGE_TFONT_45,	//45	-
	IMAGE_TFONT_46,	//46	.
	IMAGE_TFONT_47,	//47	/
	IMAGE_TFONT_48,	//48	0
	IMAGE_TFONT_49,	//49	1
	IMAGE_TFONT_50,	//50	2
	IMAGE_TFONT_51,	//51	3
	IMAGE_TFONT_52,	//52	4
	IMAGE_TFONT_53,	//53	5
	IMAGE_TFONT_54,	//54	6
	IMAGE_TFONT_55,	//55	7
	IMAGE_TFONT_56,	//56	8
	IMAGE_TFONT_57,	//57	9
	IMAGE_TFONT_58,	//58	:
	IMAGE_TFONT_59,	//59	;
	IMAGE_TFONT_60,	//60	<
	IMAGE_TFONT_61,	//61	=
	IMAGE_TFONT_62,	//62	>
	IMAGE_TFONT_63,	//63	?
	IMAGE_TFONT_64,	//64	@
	IMAGE_TFONT_65,	//65	A
	IMAGE_TFONT_66,	//66	B
	IMAGE_TFONT_67,	//67	C
	IMAGE_TFONT_68,	//68	D
	IMAGE_TFONT_69,	//69	E
	IMAGE_TFONT_70,	//70	F
	IMAGE_TFONT_71,	//71	G
	IMAGE_TFONT_72,	//72	H
	IMAGE_TFONT_73,	//73	I
	IMAGE_TFONT_74,	//74	J
	IMAGE_TFONT_75,	//75	K
	IMAGE_TFONT_76,	//76	L
	IMAGE_TFONT_77,	//77	M
	IMAGE_TFONT_78,	//78	N
	IMAGE_TFONT_79,	//79	O
	IMAGE_TFONT_80,	//80	P
	IMAGE_TFONT_81,	//81	Q
	IMAGE_TFONT_82,	//82	R
	IMAGE_TFONT_83,	//83	S
	IMAGE_TFONT_84,	//84	T
	IMAGE_TFONT_85,	//85	U
	IMAGE_TFONT_86,	//86	V
	IMAGE_TFONT_87,	//87	W
	IMAGE_TFONT_88,	//88	X
	IMAGE_TFONT_89,	//89	Y
	IMAGE_TFONT_90,	//90	Z
	IMAGE_TFONT_91,	//91	[
	IMAGE_TFONT_92,	//92	slash top left to bot right
	IMAGE_TFONT_93,	//93	]
	IMAGE_TFONT_94,	//94	hat
	IMAGE_TFONT_95,	//95	_
	IMAGE_TFONT_96,	//96	`
	IMAGE_TFONT_97,	//97	a
	IMAGE_TFONT_98,	//98	b
	IMAGE_TFONT_99,	//99	c
	IMAGE_TFONT_100,	//100	d
	IMAGE_TFONT_101,	//101	e
	IMAGE_TFONT_102,	//102	f
	IMAGE_TFONT_103,	//103	g
	IMAGE_TFONT_104,	//104	h
	IMAGE_TFONT_105,	//105	i
	IMAGE_TFONT_106,	//106	j
	IMAGE_TFONT_107,	//107	k
	IMAGE_TFONT_108,	//108	l
	IMAGE_TFONT_109,	//109	m
	IMAGE_TFONT_110,	//110	n
	IMAGE_TFONT_111,	//111	o
	IMAGE_TFONT_112,	//112	p
	IMAGE_TFONT_113,	//113	q
	IMAGE_TFONT_114,	//114	r
	IMAGE_TFONT_115,	//115	s
	IMAGE_TFONT_116,	//116	t
	IMAGE_TFONT_117,	//117	u
	IMAGE_TFONT_118,	//118	v
	IMAGE_TFONT_119,	//119	w
	IMAGE_TFONT_120,	//120	x
	IMAGE_TFONT_121,	//121	y
	IMAGE_TFONT_122,	//122	z
	IMAGE_TFONT_123,	//123	{
	IMAGE_TFONT_124,	//124	|
	IMAGE_TFONT_125,	//125	}
	IMAGE_TFONT_126,	//126	~
	IMAGE_TFONT_63,	//127	box1
	IMAGE_TFONT_63,	//128	box2
	IMAGE_TFONT_63,	//129	box3
	IMAGE_TFONT_63,	//130	,
	IMAGE_TFONT_63,	//131	Fn
	IMAGE_TFONT_63,	//132	lower quotes
	IMAGE_TFONT_63,	//133	ellipsis
	IMAGE_TFONT_63,	//134	cross
	IMAGE_TFONT_63,	//135	double cross
	IMAGE_TFONT_63,	//136	power of
	IMAGE_TFONT_63,	//137	zero over double zero
	IMAGE_TFONT_63,	//138	big S with a crown on it
	IMAGE_TFONT_63,	//139	weird <
	IMAGE_TFONT_63,	//140	OE or is it CE
	IMAGE_TFONT_63,	//141	box4
	IMAGE_TFONT_63,	//142	box5
	IMAGE_TFONT_63,	//143	box6
	IMAGE_TFONT_63,	//144	box7
	IMAGE_TFONT_63,	//145	left single quote
	IMAGE_TFONT_63,	//146	right single quote
	IMAGE_TFONT_63,	//147	left double quote
	IMAGE_TFONT_63,	//148	right double quote
	IMAGE_TFONT_63,	//149	big full stop
	IMAGE_TFONT_45,	//150	big minus sign
	IMAGE_TFONT_63,	//151	even bigger minus sign
	IMAGE_TFONT_63,	//152	tilda
	IMAGE_TFONT_63,	//153	TM
	IMAGE_TFONT_63,	//154	little s with a crown on it
	IMAGE_TFONT_63,	//155	weird >
	IMAGE_TFONT_63,	//156	oe
	IMAGE_TFONT_63,	//157	box8
	IMAGE_TFONT_63,	//158	box9
	IMAGE_TFONT_63,	//159   Big Y with umlaut
	IMAGE_TFONT_63,	//160  
	IMAGE_TFONT_63,	//161   upside down !
	IMAGE_TFONT_63,	//162   little c with a slash thru it
	IMAGE_TFONT_63,	//163  pound sign	
	IMAGE_TFONT_63,	//164   circle with crosses thru it	
	IMAGE_TFONT_63,	//165   Big Y with two lines thru it	
	IMAGE_TFONT_63,	//166   Broken 'pipe' sign	
	IMAGE_TFONT_63,	//167   Ornate 's'	
	IMAGE_TFONT_63,	//168   umlaut	
	IMAGE_TFONT_63,	//169   copyright	
	IMAGE_TFONT_63,	//170   little tiny 'a'	
	IMAGE_TFONT_63,	//171   double <<	
	IMAGE_TFONT_63,	//172   	
	IMAGE_TFONT_63,	//173   
	IMAGE_TFONT_63,	//174   registered
	IMAGE_TFONT_63,	//175   
	IMAGE_TFONT_63,	//176   
	IMAGE_TFONT_63,	//177   
	IMAGE_TFONT_63,	//178   
	IMAGE_TFONT_63,	//179   
	IMAGE_TFONT_63,	//180   
	IMAGE_TFONT_63,	//181   
	IMAGE_TFONT_63,	//182  
	IMAGE_TFONT_63,	//183   
	IMAGE_TFONT_63,	//184   
	IMAGE_TFONT_63,	//185   
	IMAGE_TFONT_63,	//186   
	IMAGE_TFONT_63,	//187   
	IMAGE_TFONT_63,	//188   
	IMAGE_TFONT_63,	//189   
	IMAGE_TFONT_63,	//190   
	IMAGE_TFONT_63,	//191   
	IMAGE_TFONT_192,	//192   
	IMAGE_TFONT_193,	//193   
	IMAGE_TFONT_194,	//194   
	IMAGE_TFONT_195,	//195   
	IMAGE_TFONT_196,	//196   
	IMAGE_TFONT_197,	//197   
	IMAGE_TFONT_198,	//198   
	IMAGE_TFONT_199,	//199   
	IMAGE_TFONT_200,	//200   
	IMAGE_TFONT_201,	//201   
	IMAGE_TFONT_202,	//202   
	IMAGE_TFONT_203,	//203   
	IMAGE_TFONT_204,	//204   
	IMAGE_TFONT_205,	//205   
	IMAGE_TFONT_206,	//206   
	IMAGE_TFONT_207,	//207   
	IMAGE_TFONT_208,	//208   
	IMAGE_TFONT_209,	//209   
	IMAGE_TFONT_210,	//210   
	IMAGE_TFONT_211,	//211   
	IMAGE_TFONT_212,	//212   
	IMAGE_TFONT_213,	//213   
	IMAGE_TFONT_214,	//214   
	IMAGE_TFONT_215,	//215   
	IMAGE_TFONT_216,	//216   
	IMAGE_TFONT_217,	//217   
	IMAGE_TFONT_218,	//218   
	IMAGE_TFONT_219,	//219   
	IMAGE_TFONT_220,	//220   
	IMAGE_TFONT_221,	//221   
	IMAGE_TFONT_63,	//222   
	IMAGE_TFONT_63,	//223   
	IMAGE_TFONT_224,	//224   
	IMAGE_TFONT_225,	//225   
	IMAGE_TFONT_226,	//226   
	IMAGE_TFONT_227,	//227   
	IMAGE_TFONT_228,	//228   
	IMAGE_TFONT_229,	//229   
	IMAGE_TFONT_230,	//230   
	IMAGE_TFONT_63,	//231   
	IMAGE_TFONT_232,	//232   
	IMAGE_TFONT_233,	//233   
	IMAGE_TFONT_234,	//234   
	IMAGE_TFONT_235,	//235   
	IMAGE_TFONT_236,	//236   
	IMAGE_TFONT_237,	//237   
	IMAGE_TFONT_238,	//238   
	IMAGE_TFONT_239,	//239   
	IMAGE_TFONT_240,	//240   
	IMAGE_TFONT_241,	//241   
	IMAGE_TFONT_242,	//242   
	IMAGE_TFONT_243,	//243   
	IMAGE_TFONT_244,	//244   
	IMAGE_TFONT_245,	//245   
	IMAGE_TFONT_246,	//246   
	IMAGE_TFONT_63,	//247   
	IMAGE_TFONT_248,	//248   
	IMAGE_TFONT_249,	//249   
	IMAGE_TFONT_250,	//250   
	IMAGE_TFONT_251,	//251   
	IMAGE_TFONT_252,	//252   
	IMAGE_TFONT_253,	//253   
	IMAGE_TFONT_63,	//254   
	IMAGE_TFONT_63	//255
};
#endif

// Ascii to font image id lookup table for in game font.
// Same for WIN32 and PSX.
//
//#ifdef WIN32
//UWORD AsciiLookup[256] = 
//#else
//// We can use bytes as long as we ensure the font images are the 1st 256 in the image file.
UWORD AsciiLookup[256] = 
//#endif
{
	IMAGE_ASCII63,	//0		0..32 are all mapped to question marks
	IMAGE_ASCII63,	//1
	IMAGE_ASCII63,	//2
	IMAGE_ASCII63,	//3
	IMAGE_ASCII63,	//4
	IMAGE_ASCII63,	//5
	IMAGE_ASCII63,	//6
#ifdef WIN32
	IMAGE_ASCII42,	//7		//weird mapping - an asterisk mapping
#else
	IMAGE_ASCII63,	//8
#endif
	IMAGE_ASCII63,	//8
	IMAGE_ASCII63,	//9
	IMAGE_ASCII63,	//10
	IMAGE_ASCII63,	//11
	IMAGE_ASCII63,	//12
	IMAGE_ASCII63,	//13
	IMAGE_ASCII63,	//14
	IMAGE_ASCII63,	//15
	IMAGE_ASCII63,	//16
	IMAGE_ASCII63,	//17
	IMAGE_ASCII63,	//18
	IMAGE_ASCII63,	//19
	IMAGE_ASCII63,	//20
	IMAGE_ASCII63,	//21
	IMAGE_ASCII63,	//22
	IMAGE_ASCII63,	//23
	IMAGE_ASCII63,	//24
	IMAGE_ASCII63,	//25
	IMAGE_ASCII63,	//26
	IMAGE_ASCII63,	//27
	IMAGE_ASCII63,	//28
	IMAGE_ASCII63,	//29
	IMAGE_ASCII63,	//30
	IMAGE_ASCII63,	//31
	IMAGE_ASCII63,	//32	// space is skipped and not drawn anyway
	IMAGE_ASCII33,	//33	!
	IMAGE_ASCII34,	//34	"
	IMAGE_ASCII35,	//35	#
	IMAGE_ASCII36,	//36	$
	IMAGE_ASCII37,	//37	
	IMAGE_ASCII38,	//38	&
	IMAGE_ASCII39,	//39	'
	IMAGE_ASCII40,	//40	(
	IMAGE_ASCII41,	//41	)
#ifdef WIN32
	IMAGE_ASTERISK,	//42	*
#else
	IMAGE_ASCII42,	//42	*
#endif
	IMAGE_ASCII43,	//43	+
	IMAGE_ASCII44,	//44	,
	IMAGE_ASCII45,	//45	-
	IMAGE_ASCII46,	//46	.
	IMAGE_ASCII47,	//47	/
	IMAGE_ASCII48,	//48	0
	IMAGE_ASCII49,	//49	1
	IMAGE_ASCII50,	//50	2
	IMAGE_ASCII51,	//51	3
	IMAGE_ASCII52,	//52	4
	IMAGE_ASCII53,	//53	5
	IMAGE_ASCII54,	//54	6
	IMAGE_ASCII55,	//55	7
	IMAGE_ASCII56,	//56	8
	IMAGE_ASCII57,	//57	9
	IMAGE_ASCII58,	//58	:
	IMAGE_ASCII59,	//59	;
	IMAGE_ASCII60,	//60	<
	IMAGE_ASCII61,	//61	=
	IMAGE_ASCII62,	//62	>
	IMAGE_ASCII63,	//63	?
	IMAGE_ASCII64,	//64	@
	IMAGE_ASCII65,	//65	A
	IMAGE_ASCII66,	//66	B
	IMAGE_ASCII67,	//67	C
	IMAGE_ASCII68,	//68	D
	IMAGE_ASCII69,	//69	E
	IMAGE_ASCII70,	//70	F
	IMAGE_ASCII71,	//71	G
	IMAGE_ASCII72,	//72	H
	IMAGE_ASCII73,	//73	I
	IMAGE_ASCII74,	//74	J
	IMAGE_ASCII75,	//75	K
	IMAGE_ASCII76,	//76	L
	IMAGE_ASCII77,	//77	M
	IMAGE_ASCII78,	//78	N
	IMAGE_ASCII79,	//79	O
	IMAGE_ASCII80,	//80	P
	IMAGE_ASCII81,	//81	Q
	IMAGE_ASCII82,	//82	R
	IMAGE_ASCII83,	//83	S
	IMAGE_ASCII84,	//84	T
	IMAGE_ASCII85,	//85	U
	IMAGE_ASCII86,	//86	V
	IMAGE_ASCII87,	//87	W
	IMAGE_ASCII88,	//88	X
	IMAGE_ASCII89,	//89	Y
	IMAGE_ASCII90,	//90	Z
	IMAGE_ASCII91,	//91	[
	IMAGE_ASCII92,	//92	\ slash top left to bot right
	IMAGE_ASCII93,	//93	]
	IMAGE_ASCII94,	//94	^ hat
	IMAGE_ASCII95,	//95	_
	IMAGE_ASCII96,	//96	`
	IMAGE_ASCII97,	//97	a
	IMAGE_ASCII98,	//98	b
	IMAGE_ASCII99,	//99	c
	IMAGE_ASCII100,	//100	d
	IMAGE_ASCII101,	//101	e
	IMAGE_ASCII102,	//102	f
	IMAGE_ASCII103,	//103	g
	IMAGE_ASCII104,	//104	h
	IMAGE_ASCII105,	//105	i
	IMAGE_ASCII106,	//106	j
	IMAGE_ASCII107,	//107	k
	IMAGE_ASCII108,	//108	l
	IMAGE_ASCII109,	//109	m
	IMAGE_ASCII110,	//110	n
	IMAGE_ASCII111,	//111	o
	IMAGE_ASCII112,	//112	p
	IMAGE_ASCII113,	//113	q
	IMAGE_ASCII114,	//114	r
	IMAGE_ASCII115,	//115	s
	IMAGE_ASCII116,	//116	t
	IMAGE_ASCII117,	//117	u
	IMAGE_ASCII118,	//118	v
	IMAGE_ASCII119,	//119	w
	IMAGE_ASCII120,	//120	x
	IMAGE_ASCII121,	//121	y
	IMAGE_ASCII122,	//122	z
	IMAGE_ASCII123,	//123	{
	IMAGE_ASCII124,	//124	|
	IMAGE_ASCII125,	//125	}
	IMAGE_ASCII126,	//126	~
	IMAGE_ASCII63,	//127	box1
	IMAGE_ASCII63,	//128	box2
	IMAGE_ASCII63,	//129	box3
	IMAGE_ASCII63,	//130	,
	IMAGE_ASCII131,	//131	Fn
	IMAGE_ASCII63,	//132	lower quotes
	IMAGE_ASCII63,	//133	ellipsis
	IMAGE_ASCII63,	//134	cross
	IMAGE_ASCII63,	//135	double cross
	IMAGE_ASCII63,	//136	power of
	IMAGE_ASCII63,	//137	zero over double zero
	IMAGE_ASCII63,	//138	big S with a crown on it
	IMAGE_ASCII63,	//139	weird <
	IMAGE_ASCII63,	//140	OE or is it CE
	IMAGE_ASCII63,	//141	box4
	IMAGE_ASCII63,	//142	box5
	IMAGE_ASCII63,	//143	box6
	IMAGE_ASCII63,	//144	box7
	IMAGE_ASCII63,	//145	left single quote
	IMAGE_ASCII63,	//146	right single quote
	IMAGE_ASCII63,	//147	left double quote
	IMAGE_ASCII63,	//148	right double quote
	IMAGE_ASCII63,	//149	big full stop
	IMAGE_ASCII63,	//150	big minus sign
	IMAGE_ASCII63,	//151	even bigger minus sign
	IMAGE_ASCII63,	//152	tilda
	IMAGE_ASCII63,	//153	TM
	IMAGE_ASCII63,	//154	little s with a crown on it
	IMAGE_ASCII63,	//155	weird >
	IMAGE_ASCII63,	//156	oe
	IMAGE_ASCII63,	//157	box8
	IMAGE_ASCII63,	//158	box9
	IMAGE_ASCII63,	//159   Big Y with umlaut
	IMAGE_ASCII63,	//160  
	IMAGE_ASCII161,	//161   upside down !
	IMAGE_ASCII63,	//162   little c with a slash thru it
	IMAGE_ASCII63,	//163  pound sign	
	IMAGE_ASCII63,	//164   circle with crosses thru it	
	IMAGE_ASCII63,	//165   Big Y with two lines thru it	
	IMAGE_ASCII63,	//166   Broken 'pipe' sign	
	IMAGE_ASCII63,	//167   Ornate 's'	
	IMAGE_ASCII63,	//168   umlaut	
	IMAGE_ASCII63,	//169   copyright	
	IMAGE_ASCII63,	//170   little tiny 'a'	
	IMAGE_ASCII63,	//171   double <<	
	IMAGE_ASCII63,	//172   	
	IMAGE_ASCII63,	//173   
	IMAGE_ASCII63,	//174   registered
	IMAGE_ASCII63,	//175   
	IMAGE_ASCII63,	//176   
	IMAGE_ASCII63,	//177   
	IMAGE_ASCII63,	//178   
	IMAGE_ASCII63,	//179   
	IMAGE_ASCII63,	//180   
	IMAGE_ASCII63,	//181   
	IMAGE_ASCII63,	//182  
	IMAGE_ASCII63,	//183   
	IMAGE_ASCII63,	//184   
	IMAGE_ASCII63,	//185   
	IMAGE_ASCII63,	//186   
	IMAGE_ASCII63,	//187   
	IMAGE_ASCII188,	//188   
	IMAGE_ASCII189,	//189   
	IMAGE_ASCII63,	//190   
	IMAGE_ASCII191,	//191   
	IMAGE_ASCII192,	//192   
	IMAGE_ASCII193,	//193   
	IMAGE_ASCII194,	//194   
	IMAGE_ASCII195,	//195   
	IMAGE_ASCII196,	//196   
	IMAGE_ASCII197,	//197   
	IMAGE_ASCII198,	//198   
	IMAGE_ASCII63,	//199   
	IMAGE_ASCII200,	//200   
	IMAGE_ASCII201,	//201   
	IMAGE_ASCII202,	//202   
	IMAGE_ASCII203,	//203   
	IMAGE_ASCII204,	//204   
	IMAGE_ASCII205,	//205   
	IMAGE_ASCII206,	//206   
	IMAGE_ASCII207,	//207   
	IMAGE_ASCII208,	//208   
	IMAGE_ASCII63,	//209   
	IMAGE_ASCII210,	//210   
	IMAGE_ASCII211,	//211   
	IMAGE_ASCII212,	//212   
	IMAGE_ASCII213,	//213   
	IMAGE_ASCII214,	//214   
	IMAGE_ASCII63,	//215   
	IMAGE_ASCII216,	//216   
	IMAGE_ASCII217,	//217   
	IMAGE_ASCII218,	//218   
	IMAGE_ASCII219,	//219   
	IMAGE_ASCII220,	//220   
	IMAGE_ASCII221,	//221   
	IMAGE_ASCII63,	//222   
	IMAGE_ASCII223,	//223   
	IMAGE_ASCII224,	//224   
	IMAGE_ASCII225,	//225   
	IMAGE_ASCII226,	//226   
	IMAGE_ASCII227,	//227   
	IMAGE_ASCII228,	//228   
	IMAGE_ASCII229,	//229   
	IMAGE_ASCII230,	//230   
	IMAGE_ASCII231,	//231   
	IMAGE_ASCII232,	//232   
	IMAGE_ASCII233,	//233   
	IMAGE_ASCII234,	//234   
	IMAGE_ASCII235,	//235   
	IMAGE_ASCII236,	//236   
	IMAGE_ASCII237,	//237   
	IMAGE_ASCII238,	//238   
	IMAGE_ASCII239,	//239   
	IMAGE_ASCII63,	//240   
	IMAGE_ASCII241,	//241   
	IMAGE_ASCII242,	//242   
	IMAGE_ASCII243,	//243   
	IMAGE_ASCII244,	//244   
	IMAGE_ASCII245,	//245   
	IMAGE_ASCII246,	//246   
	IMAGE_ASCII63,	//247   
	IMAGE_ASCII248,	//248   
	IMAGE_ASCII249,	//249   
	IMAGE_ASCII250,	//250   
	IMAGE_ASCII251,	//251   
	IMAGE_ASCII252,	//252   
	IMAGE_ASCII253,	//253   
	IMAGE_ASCII63,	//254   
	IMAGE_ASCII63	//255
};


#ifdef PSX
UWORD SmallAsciiLookup[256] = 
{
	IMAGE_S_ASCII63,	//0		0..32 are all mapped to question marks
	IMAGE_S_ASCII63,	//1
	IMAGE_S_ASCII63,	//2
	IMAGE_S_ASCII63,	//3
	IMAGE_S_ASCII63,	//4
	IMAGE_S_ASCII63,	//5
	IMAGE_S_ASCII63,	//6
	IMAGE_S_ASCII63,	//7
	IMAGE_S_ASCII63,	//8
	IMAGE_S_ASCII63,	//9
	IMAGE_S_ASCII63,	//10
	IMAGE_S_ASCII63,	//11
	IMAGE_S_ASCII63,	//12
	IMAGE_S_ASCII63,	//13
	IMAGE_S_ASCII63,	//14
	IMAGE_S_ASCII63,	//15
	IMAGE_S_ASCII63,	//16
	IMAGE_S_ASCII63,	//17
	IMAGE_S_ASCII63,	//18
	IMAGE_S_ASCII63,	//19
	IMAGE_S_ASCII63,	//20
	IMAGE_S_ASCII63,	//21
	IMAGE_S_ASCII63,	//22
	IMAGE_S_ASCII63,	//23
	IMAGE_S_ASCII63,	//24
	IMAGE_S_ASCII63,	//25
	IMAGE_S_ASCII63,	//26
	IMAGE_S_ASCII63,	//27
	IMAGE_S_ASCII63,	//28
	IMAGE_S_ASCII63,	//29
	IMAGE_S_ASCII63,	//30
	IMAGE_S_ASCII63,	//31
	IMAGE_S_ASCII63,	//32	// space is skipped and not drawn anyway
	IMAGE_S_ASCII33,	//33	!
	IMAGE_S_ASCII34,	//34	"
	IMAGE_S_ASCII35,	//35	#
	IMAGE_S_ASCII63,	//36	$
	IMAGE_S_ASCII37,	//37	
	IMAGE_S_ASCII38,	//38	&
	IMAGE_S_ASCII39,	//39	'
	IMAGE_S_ASCII40,	//40	(
	IMAGE_S_ASCII41,	//41	)
	IMAGE_S_ASCII42,	//42	*
	IMAGE_S_ASCII43,	//43	+
	IMAGE_S_ASCII44,	//44	,
	IMAGE_S_ASCII45,	//45	-
	IMAGE_S_ASCII46,	//46	.
	IMAGE_S_ASCII47,	//47	/
	IMAGE_S_ASCII48,	//48	0
	IMAGE_S_ASCII49,	//49	1
	IMAGE_S_ASCII50,	//50	2
	IMAGE_S_ASCII51,	//51	3
	IMAGE_S_ASCII52,	//52	4
	IMAGE_S_ASCII53,	//53	5
	IMAGE_S_ASCII54,	//54	6
	IMAGE_S_ASCII55,	//55	7
	IMAGE_S_ASCII56,	//56	8
	IMAGE_S_ASCII57,	//57	9
	IMAGE_S_ASCII58,	//58	:
	IMAGE_S_ASCII59,	//59	;
	IMAGE_S_ASCII60,	//60	<
	IMAGE_S_ASCII61,	//61	=
	IMAGE_S_ASCII62,	//62	>
	IMAGE_S_ASCII63,	//63	?
	IMAGE_S_ASCII63,	//64	@
	IMAGE_S_ASCII65,	//65	A
	IMAGE_S_ASCII66,	//66	B
	IMAGE_S_ASCII67,	//67	C
	IMAGE_S_ASCII68,	//68	D
	IMAGE_S_ASCII69,	//69	E
	IMAGE_S_ASCII70,	//70	F
	IMAGE_S_ASCII71,	//71	G
	IMAGE_S_ASCII72,	//72	H
	IMAGE_S_ASCII73,	//73	I
	IMAGE_S_ASCII74,	//74	J
	IMAGE_S_ASCII75,	//75	K
	IMAGE_S_ASCII76,	//76	L
	IMAGE_S_ASCII77,	//77	M
	IMAGE_S_ASCII78,	//78	N
	IMAGE_S_ASCII79,	//79	O
	IMAGE_S_ASCII80,	//80	P
	IMAGE_S_ASCII81,	//81	Q
	IMAGE_S_ASCII82,	//82	R
	IMAGE_S_ASCII83,	//83	S
	IMAGE_S_ASCII84,	//84	T
	IMAGE_S_ASCII85,	//85	U
	IMAGE_S_ASCII86,	//86	V
	IMAGE_S_ASCII87,	//87	W
	IMAGE_S_ASCII88,	//88	X
	IMAGE_S_ASCII89,	//89	Y
	IMAGE_S_ASCII90,	//90	Z
	IMAGE_S_ASCII63,	//91	[
	IMAGE_S_ASCII92,	//92	slash top left to bot right
	IMAGE_S_ASCII63,	//93	]
	IMAGE_S_ASCII94,	//94	hat
	IMAGE_S_ASCII95,	//95	_
	IMAGE_S_ASCII96,	//96	`
	IMAGE_S_ASCII97,	//97	a
	IMAGE_S_ASCII98,	//98	b
	IMAGE_S_ASCII99,	//99	c
	IMAGE_S_ASCII100,	//100	d
	IMAGE_S_ASCII101,	//101	e
	IMAGE_S_ASCII102,	//102	f
	IMAGE_S_ASCII103,	//103	g
	IMAGE_S_ASCII104,	//104	h
	IMAGE_S_ASCII105,	//105	i
	IMAGE_S_ASCII106,	//106	j
	IMAGE_S_ASCII107,	//107	k
	IMAGE_S_ASCII108,	//108	l
	IMAGE_S_ASCII109,	//109	m
	IMAGE_S_ASCII110,	//110	n
	IMAGE_S_ASCII111,	//111	o
	IMAGE_S_ASCII112,	//112	p
	IMAGE_S_ASCII113,	//113	q
	IMAGE_S_ASCII114,	//114	r
	IMAGE_S_ASCII115,	//115	s
	IMAGE_S_ASCII116,	//116	t
	IMAGE_S_ASCII117,	//117	u
	IMAGE_S_ASCII118,	//118	v
	IMAGE_S_ASCII119,	//119	w
	IMAGE_S_ASCII120,	//120	x
	IMAGE_S_ASCII121,	//121	y
	IMAGE_S_ASCII122,	//122	z
	IMAGE_S_ASCII63,	//123	{
	IMAGE_S_ASCII63,	//124	|
	IMAGE_S_ASCII63,	//125	}
	IMAGE_S_ASCII63,	//126	~
	IMAGE_S_ASCII63,	//127	box1
	IMAGE_S_ASCII63,	//128	box2
	IMAGE_S_ASCII63,	//129	box3
	IMAGE_S_ASCII63,	//130	,
	IMAGE_S_ASCII63,	//131	Fn
	IMAGE_S_ASCII63,	//132	lower quotes
	IMAGE_S_ASCII63,	//133	ellipsis
	IMAGE_S_ASCII63,	//134	cross
	IMAGE_S_ASCII63,	//135	double cross
	IMAGE_S_ASCII63,	//136	power of
	IMAGE_S_ASCII63,	//137	zero over double zero
	IMAGE_S_ASCII63,	//138	big S with a crown on it
	IMAGE_S_ASCII63,	//139	weird <
	IMAGE_S_ASCII63,	//140	OE or is it CE
	IMAGE_S_ASCII63,	//141	box4
	IMAGE_S_ASCII63,	//142	box5
	IMAGE_S_ASCII63,	//143	box6
	IMAGE_S_ASCII63,	//144	box7
	IMAGE_S_ASCII63,	//145	left single quote
	IMAGE_S_ASCII63,	//146	right single quote
	IMAGE_S_ASCII63,	//147	left double quote
	IMAGE_S_ASCII63,	//148	right double quote
	IMAGE_S_ASCII63,	//149	big full stop
	IMAGE_S_ASCII63,	//150	big minus sign
	IMAGE_S_ASCII63,	//151	even bigger minus sign
	IMAGE_S_ASCII63,	//152	tilda
	IMAGE_S_ASCII63,	//153	TM
	IMAGE_S_ASCII63,	//154	little s with a crown on it
	IMAGE_S_ASCII63,	//155	weird >
	IMAGE_S_ASCII63,	//156	oe
	IMAGE_S_ASCII63,	//157	box8
	IMAGE_S_ASCII63,	//158	box9
	IMAGE_S_ASCII63,	//159   Big Y with umlaut
	IMAGE_S_ASCII63,	//160  
	IMAGE_S_ASCII161,	//161   upside down !
	IMAGE_S_ASCII63,	//162   little c with a slash thru it
	IMAGE_S_ASCII63,	//163  pound sign	
	IMAGE_S_ASCII63,	//164   circle with crosses thru it	
	IMAGE_S_ASCII63,	//165   Big Y with two lines thru it	
	IMAGE_S_ASCII63,	//166   Broken 'pipe' sign	
	IMAGE_S_ASCII63,	//167   Ornate 's'	
	IMAGE_S_ASCII63,	//168   umlaut	
	IMAGE_S_ASCII63,	//169   copyright	
	IMAGE_S_ASCII63,	//170   little tiny 'a'	
	IMAGE_S_ASCII63,	//171   double <<	
	IMAGE_S_ASCII63,	//172   	
	IMAGE_S_ASCII63,	//173   
	IMAGE_S_ASCII63,	//174   registered
	IMAGE_S_ASCII63,	//175   
	IMAGE_S_ASCII63,	//176   
	IMAGE_S_ASCII63,	//177   
	IMAGE_S_ASCII63,	//178   
	IMAGE_S_ASCII63,	//179   
	IMAGE_S_ASCII63,	//180   
	IMAGE_S_ASCII63,	//181   
	IMAGE_S_ASCII63,	//182  
	IMAGE_S_ASCII63,	//183   
	IMAGE_S_ASCII63,	//184   
	IMAGE_S_ASCII63,	//185   
	IMAGE_S_ASCII63,	//186   
	IMAGE_S_ASCII63,	//187   
	IMAGE_S_ASCII63,	//188   ¼
	IMAGE_S_ASCII63,	//189	½   
	IMAGE_S_ASCII63,	//190   
	IMAGE_S_ASCII191,	//191   
	IMAGE_S_ASCII192,	//192   
	IMAGE_S_ASCII193,	//193   
	IMAGE_S_ASCII194,	//194   
	IMAGE_S_ASCII195,	//195   
	IMAGE_S_ASCII196,	//196   
	IMAGE_S_ASCII197,	//197   
	IMAGE_S_ASCII198,	//198   
	IMAGE_S_ASCII63,	//199   
	IMAGE_S_ASCII200,	//200   
	IMAGE_S_ASCII201,	//201   
	IMAGE_S_ASCII202,	//202   
	IMAGE_S_ASCII203,	//203   
	IMAGE_S_ASCII204,	//204   
	IMAGE_S_ASCII205,	//205   
	IMAGE_S_ASCII206,	//206   
	IMAGE_S_ASCII207,	//207   
	IMAGE_S_ASCII63,	//208   Ð
	IMAGE_S_ASCII63,	//209   
	IMAGE_S_ASCII210,	//210   
	IMAGE_S_ASCII211,	//211   
	IMAGE_S_ASCII212,	//212   
	IMAGE_S_ASCII213,	//213   
	IMAGE_S_ASCII214,	//214   
	IMAGE_S_ASCII63,	//215   
	IMAGE_S_ASCII216,	//216   
	IMAGE_S_ASCII217,	//217   
	IMAGE_S_ASCII218,	//218   
	IMAGE_S_ASCII219,	//219   
	IMAGE_S_ASCII220,	//220   
	IMAGE_S_ASCII221,	//221   
	IMAGE_S_ASCII63,	//222   
	IMAGE_S_ASCII223,	//223   
	IMAGE_S_ASCII224,	//224   
	IMAGE_S_ASCII225,	//225   
	IMAGE_S_ASCII226,	//226   
	IMAGE_S_ASCII227,	//227   
	IMAGE_S_ASCII228,	//228   
	IMAGE_S_ASCII229,	//229   
	IMAGE_S_ASCII230,	//230   
	IMAGE_S_ASCII231,	//231   
	IMAGE_S_ASCII232,	//232   
	IMAGE_S_ASCII233,	//233   
	IMAGE_S_ASCII234,	//234   
	IMAGE_S_ASCII235,	//235   
	IMAGE_S_ASCII236,	//236   
	IMAGE_S_ASCII237,	//237   
	IMAGE_S_ASCII238,	//238   
	IMAGE_S_ASCII239,	//239   
	IMAGE_S_ASCII63,	//240   
	IMAGE_S_ASCII241,	//241   
	IMAGE_S_ASCII242,	//242   
	IMAGE_S_ASCII243,	//243   
	IMAGE_S_ASCII244,	//244   
	IMAGE_S_ASCII245,	//245   
	IMAGE_S_ASCII246,	//246   
	IMAGE_S_ASCII63,	//247   
	IMAGE_S_ASCII248,	//248   
	IMAGE_S_ASCII249,	//249   
	IMAGE_S_ASCII250,	//250   
	IMAGE_S_ASCII251,	//251   
	IMAGE_S_ASCII252,	//252   
	IMAGE_S_ASCII253,	//253   
	IMAGE_S_ASCII63,	//254   
	IMAGE_S_ASCII63		//255
};
#endif

IMAGEFILE *FrontImages;
#ifdef PSX

BOOL DirectControl = DEFAULT_TO_DRIVE;
IMAGEFILE *FrontImages16;
BOOL EnableVibration = TRUE;

#define	MIN_CDAUDIO_TRACK	1
#define	MAX_CDAUDIO_TRACK	3
UWORD NextCDAudioTrack = MIN_CDAUDIO_TRACK;

#else
BOOL DirectControl = FALSE;

#endif

extern int FEFont;
//extern int FEBigFont;


// Each module in the game should have a call from here to initialise
// any globals and statics to there default values each time the game
// or frontend restarts.
//
BOOL InitialiseGlobals(void)
{
	frontendInitVars();	// Initialise frontend globals and statics.
	statsInitVars();
	structureInitVars();
	if (!messageInitVars())
	{
		return FALSE;
	}
	if (!researchInitVars())
    {
        return FALSE;
    }
	featureInitVars();
	radarInitVars();
	Edit3DInitVars();

	snapInitVars();
	driveInitVars(TRUE);
	
	return TRUE;
}

#ifdef PSX

/*
	cam1.wrf contains vidmem,basic,audio,stats.wrf from the PC
*/
/*
char gamedev_lev[]=
{
"
campaign	CAM_1
data		\"cam1.wrf\"

camstart	CAM_1_START
dataset		CAM_1
game		\"cam1strt.gam\"
data		\"cam1strt.wrf\"

expand		CAM_1A
dataset		CAM_1						   
game		\"cam1a.gam\"				   
data		\"cam1a.wrf\"

expand		CAM_1B
dataset		CAM_1
game		\"cam1b.gam\"
data		\"cam1b.wrf\"

miss_keep	SUB_1_1
dataset		CAM_1
game		\"sub1-1.gam\"
data		\"sub1-1.wrf\"

expand		CAM_1C
dataset		CAM_1
game		\"cam1c.gam\"
data		\"cam1c.wrf\"
"

};
*/
#endif

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// Called once on program startup.
//
BOOL systemInitialise(void)
{
	W_HEAPINIT		sWInit;
	UBYTE			*pBuffer;
	UDWORD			size;
	
	// Setup the sizes of the widget heaps , using code to setup a structure that calls a routine that tells a library how much memory it should allocate (hmmm...)
	memset(&sWInit, 0, sizeof(sWInit));
#ifdef WIN32
	sWInit.barInit = 40;
	sWInit.barExt = 5;
	sWInit.butInit = 50;		// was 30 ... but what about the virtual keyboard
	sWInit.butExt = 5;
	sWInit.edbInit = 2;
	sWInit.edbExt = 1;
	sWInit.formInit = 10;
	sWInit.formExt = 2;
	sWInit.cFormInit = 50;
	sWInit.cFormExt = 5;
	sWInit.tFormInit = 3;
	sWInit.tFormExt = 2;
	sWInit.labInit = 15;
	sWInit.labExt = 3;
	sWInit.sldInit = 2;
	sWInit.sldExt = 1;
#else
	sWInit.barInit = 82;	// 40 in stats window, 40 in object window + 2 for luck.
	sWInit.barExt = 0;
	sWInit.butInit = 70; 	// 70 , order screen uses loads... should be plenty.
	sWInit.butExt = 0;
	sWInit.edbInit = 2;
	sWInit.edbExt = 0;
	sWInit.formInit = 20;
	sWInit.formExt = 0;
	sWInit.cFormInit = 80;	// 40 in stats window, 40 in object window.
	sWInit.cFormExt = 0;
	sWInit.tFormInit = 5;
	sWInit.tFormExt = 0;
	sWInit.labInit = 80;	// 40 in stats window, 40 in object window.
	sWInit.labExt = 0;
	sWInit.sldInit = 4;
	sWInit.sldExt = 0;
#endif
	if (!widgInitialise(&sWInit))
	{
		return FALSE;
	}


#ifdef WIN32
	// load up the level discription file
	// used for the script stuff .... !
	// loop through all addon.lev files loading each one
	{
		WDG_FINDFILE	sFindFile;

		// load the original gamedesc.lev
		if (!loadFile("GameDesc.lev", &pBuffer, &size))
		{
			return FALSE;
		}
		if (!levParse(pBuffer, size))
		{
			return FALSE;
		}
		FREE(pBuffer);

		wdgFindFirstFileRev(HashStringIgnoreCase("MISCDATA"),
							  HashString("MISCDATA"),
							  HashStringIgnoreCase("addon.lev"),
							  &sFindFile);

		while (sFindFile.psCurrCache != NULL)
		{
			if (!loadFileFromWDGCache(&sFindFile, &pBuffer, &size, WDG_ALLOCATEMEM))
			{
				return FALSE;
			}
			if (!levParse(pBuffer, size))
			{
				return FALSE;
			}
			FREE(pBuffer);

			wdgFindNextFileRev(&sFindFile);
		}
	}

#else
		// On the playstation it is statically defined (saves a good 10k)
//	if (!levParse(gamedev_lev, strlen(gamedev_lev) ))
//	{
//		return FALSE;
//	}
#endif


#ifdef PSX
	//This needs to be called for _TEX_PAGE initialisation BEFORE the resources are loaded
	/* Initialise DOMARKS Library and global variables */
	iV_Initialise();

	iV_RenderAssign(REND_PSX,&rendSurface);
//	InitPrimatives();			// now done in	InitialisePSXHardware ... we need it early because we are using the primative buffer for other things
#else
	//initialize render engine
	switch (war_GetRendMode())
	{
	case	REND_MODE_GLIDE:
		if (!pie_Initialise(REND_GLIDE_3DFX))
		{
			ASSERT((FALSE,"Unable to initialise 3DFX hardware"));
			return FALSE;
		}
		break;
	case	REND_MODE_HAL:
		pie_SetDirect3DDeviceName("Direct3D HAL");
		if (!pie_Initialise(REND_D3D_HAL))
		{
			ASSERT((FALSE,"Unable to initialise DirectX HAL, ensure the correct DirectDraw device is selected"));
			return FALSE;
		}
		break;
	case	REND_MODE_RGB:
		pie_SetDirect3DDeviceName("RGB Emulation");
		if (!pie_Initialise(REND_D3D_RGB))
		{
			ASSERT((FALSE,"Unable to initialise DirectX RGB Renderer"));
			return FALSE;
		}
		break;
	case	REND_MODE_REF:
		pie_SetDirect3DDeviceName("Reference Rasterizer");
		if (!pie_Initialise(REND_D3D_REF))
		{
			ASSERT((FALSE,"Unable to initialise DirectX Reference Renderer"));
			return FALSE;
		}
		break;
	case	REND_MODE_SOFTWARE:
	default:
		war_SetFog(FALSE);
		if (!pie_Initialise(iV_MODE_4101))
		{
			ASSERT((FALSE,"Unable to initialise software renderer"));
			return FALSE;
		}
		break;
	}

	pie_SetTranslucent(war_GetTranslucent());
	pie_SetAdditive(war_GetAdditive());
	
//	displayBufferSize = iV_GetDisplayWidth()*iV_GetDisplayHeight()*iV_GetDisplayBytesPP();
	displayBufferSize = DISP_WIDTH*DISP_HEIGHT*2;
	if (displayBufferSize < 1500000)
	{
		displayBufferSize = 1500000;
	}
	DisplayBuffer = MALLOC(displayBufferSize);
	if (DisplayBuffer == NULL)
	{
		DBERROR(("Unable to allocate memory for display buffer"));
		return FALSE;
	}
#endif


#ifdef WIN32
 #ifdef AUDIO_DISABLED
	if (!audio_Init(frameGetWinHandle(), FALSE, droidAudioTrackStopped))	// audio.
 #else
	if (!audio_Init(frameGetWinHandle(), TRUE, droidAudioTrackStopped))
 #endif
	{
		DBERROR( ("Couldn't initialise audio system: continuing without audio\n") );
	}
#else
	if (!audio_Init(frameGetWinHandle(), TRUE))
	{
		DBERROR( ("Couldn't initialise audio system\n") );
		return FALSE;
	}
#endif

#if defined(WIN32) && !defined(I_LIKE_LISTENING_TO_CDS)
	cdAudio_Open();
	mixer_Open();
#endif

#ifdef WIN32
	if (!bDisableLobby && !multiInitialise())			// ajl. Init net stuff
	{
		return FALSE;
	}

#endif

#ifdef PSX
	keyInitMappings(FALSE);

	AllocateRadarArea();

#endif
	
	if (!dataInitLoadFuncs())				// Pass all the data loading functions to the framework library 
	{
		return FALSE;
	}

	if (!rayInitialise())		/* Initialise the ray tables */
	{
		return FALSE;
	}

	if (!fpathInitialise())
	{
		return FALSE;
	}
	if (!astarInitialise())		// Initialise the findpath system
	{
		return FALSE;
	}

#ifdef PSX
	initPlayerColours();
#endif

#ifdef WIN32
	loadConfig(FALSE);			// get favourite settings from the registry
#endif

	// create a block heap for the game data
	if (!BLOCK_CREATE(&psGameHeap, GAMEBLOCK_INIT, GAMEBLOCK_EXT))
	{
		return FALSE;
	}

	// create a block heap for the campaign map
	if (!BLOCK_CREATE(&psMapHeap, MAPBLOCK_INIT, MAPBLOCK_EXT))
	{
		return FALSE;
	}

	// create a block heap for the pre WRF data
	if (!BLOCK_CREATE(&psMissionHeap, MISSIONBLOCK_INIT, MISSIONBLOCK_EXT))
	{
		return FALSE;
	}

#ifdef ARROWS
	arrowInit();
#endif

#ifdef WIN32
	iV_Reset(TRUE);								// Reset the IV library.
	initLoadingScreen(TRUE, FALSE);
#endif

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// Called once at program shutdown.
//
BOOL systemShutdown(void)
{
#ifdef ARROWS
	arrowShutDown();
#endif

	keyClearMappings();
	fpathShutDown();
	
#ifdef PSX	// Just free's up the memory allocated for the radar in systemInitialise().
	ShutdownRadar();
#endif

	// free up all the load functions (all the data should already have been freed)
	resReleaseAll();

	// release the block heaps
	BLOCK_DESTROY(psGameHeap);
	BLOCK_DESTROY(psMapHeap);
	BLOCK_DESTROY(psMissionHeap);

#ifdef WIN32
	if (!bDisableLobby &&	!multiShutdown())		// ajl. init net stuff
	{
		return FALSE;
	}
#endif


#ifdef WIN32
	if ( audio_Disabled() == FALSE && !audio_Shutdown() )
	{
		return FALSE;
	}
#endif

#if defined(WIN32) && !defined(I_LIKE_LISTENING_TO_CDS)
	cdAudio_Stop();
	cdAudio_Close();
	mixer_Close();
#endif

#ifdef WIN32
	FREE(DisplayBuffer);
#endif

	iV_ShutDown();

	levShutDown();

	widgShutDown();

	return TRUE;
}

/***************************************************************************/

BOOL
init_ObjectDead( void * psObj )
{
	BASE_OBJECT	*psBaseObj = (BASE_OBJECT *) psObj;
	DROID		*psDroid;
	STRUCTURE	*psStructure;

	/* check is valid pointer */
	ASSERT( (PTRVALID(psBaseObj, sizeof(BASE_OBJECT)),
			"init_ObjectDead: game object pointer invalid\n") );

	if ( psBaseObj->died == TRUE )
	{
		switch ( psBaseObj->type )
		{
			case OBJ_DROID:
				psDroid = (DROID *) psBaseObj;
				psDroid->psCurAnim = NULL;
				break;

			case OBJ_STRUCTURE:
				psStructure = (STRUCTURE *) psBaseObj;
				psStructure->psCurAnim = NULL;
				break;

			default:
				DBERROR( ("init_ObjectAnimRemoved: unrecognised object type") );
		}
	}

	return psBaseObj->died;
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// WIN32 Version. Called At Frontend Startup.
#ifdef WIN32
BOOL frontendInitialise(char *ResourceFile)
{
	DBPRINTF(("Initialising frontend : %s\n",ResourceFile));

	// allocate memory from the pre data heap
	memSetBlockHeap(psGameHeap);

	// reset the multiple wdg stuff
	wdgEnableAddonWDG();

	if(!InitialiseGlobals())				// Initialise all globals and statics everywhere.
	{
		return FALSE;
	}

	iV_Reset(TRUE);								// Reset the IV library.

	if (!scrTabInitialise())				// Initialise the script system
	{
		return FALSE;
	}

	if (!stringsInitialise())				// Initialise the string system 
	{
		return FALSE;
	}

	if (!objInitialise())					// Initialise the object system 
	{
		return FALSE;
	}

	if ( !anim_Init( anim_GetShapeFunc ) )
	{
		return FALSE;
	}

	if ( !animObj_Init( init_ObjectDead ) )
	{
		return FALSE;
	}

	if (!allocPlayerPower())	 //set up the PlayerPower for each player - this should only be done ONCE now
	{
		return FALSE;
	}

	DBPRINTF(("frontEndInitialise: loading resource file ....."));
	if (!resLoad(ResourceFile, 0,
				 DisplayBuffer, displayBufferSize,
				 psGameHeap))				//need the object heaps to have been set up before loading in the save game
	{
		return FALSE;
	}

	if (!dispInitialise())					// Initialise the display system 
	{
		return FALSE;
	}

#ifdef BUCKET
	if ( !bucketSetupList() )				// reset object list 
	{
		return FALSE;
	}
#endif
	
	FrontImages = (IMAGEFILE*)resGetData("IMG","frend.img");
	FEFont = iV_CreateFontIndirect(FrontImages,FEAsciiLookup,4);

   	/* Shift the interface initialisation here temporarily so that it
   		can pick up the stats after they have been loaded */
	if (!intInitialise())
	{	 	  	
		return FALSE;
	}

	loadConfig(TRUE);			// get favourite settings from the registry

	// keymappings
	// clear out any existing mappings
	keyClearMappings();
	keyInitMappings(FALSE);


#ifdef OLD_PALETTE
	iV_PaletteSelect(iV_PaletteAdd(&gamePal[0]));
#endif

	pie_SetMouse(IntImages,IMAGE_CURSOR_DEFAULT);	// Set the default cursor shape.
	frameSetCursorFromRes(IDC_DEFAULT);

	SetFormAudioIDs(-1,ID_SOUND_WINDOWCLOSE);			// disable the open noise since distorted in 3dfx builds.

	memSetBlockHeap(NULL);
	
	initMiscVars();

    gameTimeInit();

	// hit me with some funky beats....
	cdAudio_PlayTrack(2);	// track 2 = f.e. music,

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// WIN32 version. Called at frontend Shutdown, before game runs.
//
BOOL frontendShutdown(void)
{

	DBPRINTF(("Shuting down frontend\n"));

	saveConfig();			// save settings to registry.

	//	if (!aiShutdown())
//	{
//		return FALSE;
//	}

	if (!mechShutdown())
	{
		return FALSE;
	}

	releasePlayerPower();

	intShutDown();
	scrShutDown();

	//do this before shutting down the iV library
//	D3DFreeTexturePages();
	resReleaseAllData();

	if (!objShutdown())
	{
		return FALSE;
	}

    ResearchRelease();

	if ( !anim_Shutdown() )
	{
		return FALSE;
	}

	if ( !animObj_Shutdown() )
	{
		return FALSE;
	}

/*
	if (!dispShutdown())
	{
		return FALSE;
	}
*/
	pie_TexShutDown();

#ifdef PSX
	FreeAreas();	// Shutdown VRAM allocation system.
#endif

	// reset the block heap
	BLOCK_RESET(psGameHeap);

	return TRUE;
}



#else

void UploadVab(void)
{
	if (GetCurrentLanguage()==LANGUAGE_FRENCH)
	{
		audio_UploadNewVab("main_fre.vab");	   // hardwared sound effects always loaded !
	}
	else
	if (GetCurrentLanguage()==LANGUAGE_GERMAN)
	{
		audio_UploadNewVab("main_ger.vab");	   // hardwared sound effects always loaded !
	}
	else
	{
		audio_UploadNewVab("main.vab");	   // hardwared sound effects always loaded !
	}
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// PSX specific frontend initialisation and shutdown code.

BOOL frontendInitialise(char *ResourceFile)
{
	UBYTE	*pFileBuff;
	DBPRINTF(("Initialising frontend : %s\n",ResourceFile));

	// allocate memory from the pre data heap
	memSetBlockHeap(psGameHeap);


	InitAreas();	// Initialise VRAM allocation system.


// Initialise all globals and statics everywhere.
	if(!InitialiseGlobals()) {
		return FALSE;
	}

// Reset the IV library.
	iV_Reset(TRUE);

/* Initialise the string system */
	if (!stringsInitialise())
	{
		return FALSE;
	}

/* Initialise the display system */
	if (!dispInitialise())
	{
		return FALSE;
	}

/* Initialise the display system */
	if (!disp2DInitialise())
	{
		return FALSE;
	}

	if (!resLoad(ResourceFile, 0, NULL, 0,
				 psGameHeap))				//need the object heaps to have been set up before loading in the save game
	{
		return FALSE;
	}

   	/* Shift the interface initialisation here temporarily so that it
   		can pick up the stats after they have been loaded */

	if (!intInitialise())
	{	 	  	
		return FALSE;
	}

  	FrontImages = (IMAGEFILE*)resGetData("IMG","frend.img");
  	FrontImages16 = (IMAGEFILE*)resGetData("IMG","frend16.img");
	FEFont = iV_CreateFontIndirect(IntImages,AsciiLookup,4);
	WFont = FEFont;
	SmallWFont = iV_CreateFontIndirect(IntImages,SmallAsciiLookup,2);
	iV_SetFont(FEFont);
//	FEBigFont = FEFont;
//	FEBigFont = iV_CreateFontIndirect(FrontImages16,FEAsciiLookup,4);
//	FEBigFont = iV_CreateFont(FrontImages16,AsciiLookup,4);


#ifdef WIN32
	screenSetPalette(0, 256, gamePalette);			// setup game palette.
	iV_PaletteSelect(iV_PaletteAdd(&gamePal[0]));
#endif

#ifdef PSX
	// May need to change this to load frontend specific vab.
	UploadVab();
#endif

//	SetFormAudioIDs(FE_AUDIO_WINDOWOPEN,FE_AUDIO_WINDOWCLOSE);
	SetFormAudioIDs(ID_SOUND_WINDOWOPEN,ID_SOUND_WINDOWCLOSE);

    //if this screws things up on the PSX then I'm sorry alright?
    gameTimeInit();

	// Set the cursor snap max distances.
	SetMaxDist(320,64);

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// PSX Frontend Shutdown.
//
BOOL frontendShutdown(void)
{
	DBPRINTF(("Shuting down frontend\n"));

	intShutDown();

	//do this before shutting down the iV library
	resReleaseAllData();
	BLOCK_RESET(psGameHeap);

	if (!disp2DShutdown())
	{
		return FALSE;
	}

	if (!dispShutdown())
	{
		return FALSE;
	}

	return TRUE;
}

#endif


/******************************************************************************/
/*                       Initialisation before data is loaded                 */



BOOL stageOneInitialise(void)
{
#ifdef WIN32
	BLOCK_HEAP	*psHeap;
#endif

	DBPRINTF(("stageOneInitalise\n"));

#ifndef FINALBUILD
	tpInit();
#endif

	// Initialise all globals and statics everwhere.
	if(!InitialiseGlobals())
	{
		return FALSE;
	}

#ifdef PSX
	NextCDAudioTrack = MIN_CDAUDIO_TRACK;
	InitAreas();				// Initialise VRAM allocation system.
#endif

	iV_Reset(FALSE);			// Reset the IV library. (but not the palette)

	if (!stringsInitialise())	/* Initialise the string system */
	{
		return FALSE;
	}

	if (!objInitialise())		/* Initialise the object system */					 		
	{
		return FALSE;
	}

	if (!droidInit())
	{
		return FALSE;
	}

	if (!initViewData())
	{
		return FALSE;
	}

	if (!grpInitialise())
	{
		return FALSE;
	}

   	if (!aiInitialise())		/* Initialise the AI system */ // pregame
	{
		return FALSE;
	}

#ifdef WIN32
	// debug mode only so use normal MALLOC
	psHeap = memGetBlockHeap();
	memSetBlockHeap(NULL);
#ifdef DISP2D
	if (!disp2DInitialise())
	{
		return FALSE;
	}
#endif
	memSetBlockHeap(psHeap);
#endif

	if ( !anim_Init( anim_GetShapeFunc ) )
	{
		return FALSE;
	}

	if ( !animObj_Init( init_ObjectDead ) )
	{
		return FALSE;
	}

	if (!allocPlayerPower())	/*set up the PlayerPower for each player - this should only be done ONCE now*/
	{
		return FALSE;
	}

	if (!formationInitialise())		// Initialise the formation system
	{
		return FALSE;
	}

	// initialise the visibility stuff
	if (!visInitialise())
	{
		return FALSE;
	}

	/* Initialise the movement system */
	if (!moveInitialise())
	{
		return FALSE;
	}

	if (!proj_InitSystem())
	{
		return FALSE;
	}

	if (!scrTabInitialise())	// Initialise the script system
	{
		return FALSE;
	}

	if (!gridInitialise())
	{
		return FALSE;
	}

#ifdef WIN32
    if (!environInit())
    {
        return FALSE;
    }
	// reset speed limiter
	moveSetFormationSpeedLimiting(TRUE);
#else
	moveSetFormationSpeedLimiting(FALSE);
#endif

	initMission();
	initTransporters();

    //do this here so that the very first mission has it initialised
    initRunData();

	gameTimeInit();
    //need to reset the event timer too - AB 14/01/99
    eventTimeReset(gameTime/SCR_TICKRATE);

	// Set the cursor snap max distances.
	SetMaxDist(64,64);

	return TRUE;
}


/******************************************************************************/
/*                       Shutdown after data is released                      */


BOOL stageOneShutDown(void)
{
	DBPRINTF(("stageOneShutDown\n"));

#ifdef WIN32		// ffs
	//do this before shutting down the iV library
//	D3DFreeTexturePages();

	if ( audio_Disabled() == FALSE )
	{
		audio_CheckAllUnloaded();
	}
#endif

	proj_Shutdown();

    releaseMission();

	if (!aiShutdown())
	{
		return FALSE;
	}

	if (!objShutdown())
	{
		return FALSE;
	}

	grpShutDown();

	formationShutDown();
	releasePlayerPower();
    
    ResearchRelease();

	//free up the gateway stuff?
	gwShutDown();

	if (!mapShutdown())
	{
		return FALSE;
	}

	scrShutDown();

#ifdef WIN32
    environShutDown();
#endif

	gridShutDown();

	if ( !anim_Shutdown() )
	{
		return FALSE;
	}

	if ( !animObj_Shutdown() )
	{
		return FALSE;
	}

#ifdef WIN32
#ifdef DISP2D
	if (!disp2DShutdown())
	{
		return FALSE;
	}
#endif
#endif

#ifdef PSX
	if (!dispShutdown())
	{
		return FALSE;
	}
#endif

	pie_TexShutDown();

#ifdef PSX
	FreeAreas();	// Shutdown VRAM allocation system.

#endif

	viewDataHeapShutDown();

	initMiscVars();

	// reset the multiple wdg stuff
	wdgEnableAddonWDG();

	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////////
// Initialise after the base data is loaded but before final level data is loaded

BOOL stageTwoInitialise(void)
{
	DBPRINTF(("stageTwoInitalise\n"));

#ifdef WIN32
	if(bMultiPlayer)
	{
		if (!multiTemplateSetup())
		{
			return FALSE;
		}
	}
#endif

	if (!dispInitialise())		/* Initialise the display system */
	{
		return FALSE;
	}

	LOADBARCALLBACK();	//	loadingScreenCallback();

	if(!InitRadar()) 	// After resLoad cause it needs the game palette initialised.
	{
		return FALSE;
	}

	LOADBARCALLBACK();	//	loadingScreenCallback();

	if(!initMiscImds())			/* Set up the explosions */
	{
		iV_ShutDown();
		DBERROR(("Can't find all the explosions PCX's"));
		return FALSE;
	}
  
/*
	if (!loadExtraIMDs())
	{
		return FALSE;
	}
*/

	/*if (!mechInitialise())		// Initialise the mechanics system 
	{
		return FALSE;
	}*/

	if (!cmdDroidInit())
	{
		return FALSE;
	}

	LOADBARCALLBACK();	//	loadingScreenCallback();

#ifdef BUCKET
	if ( !bucketSetupList() )	/* reset object list */
	{
		return FALSE;
	}
#endif
	
   	/* Shift the interface initialisation here temporarily so that it
   		can pick up the stats after they have been loaded */

	LOADBARCALLBACK();	//	loadingScreenCallback();

	if (!intInitialise())
	{	 	  	
		return FALSE;
	}

	LOADBARCALLBACK();	//	loadingScreenCallback();

//	if (!initTitle())
//	{
//		return(FALSE);
//	}

	if (!initMessage())			/* Initialise the message heaps */
	{
		return FALSE;
	}

#ifdef WIN32
	if (!gwInitialise())
	{
		return FALSE;
	}
#endif
	
#ifdef WIN32
	// keymappings
	LOADBARCALLBACK();	//	loadingScreenCallback();
	keyClearMappings();
	keyInitMappings(FALSE);
	LOADBARCALLBACK();	//	loadingScreenCallback();
#endif

	pie_SetMouse(IntImages,IMAGE_CURSOR_DEFAULT);	// Set the default cursor shape.
	frameSetCursorFromRes(IDC_DEFAULT);
//	drawRadar3dfx(64,64);
//	iV_DownLoadRadar(radarBuffer3dfx);

#ifdef PSX
	UploadVab();
#endif

	SetFormAudioIDs(ID_SOUND_WINDOWOPEN,ID_SOUND_WINDOWCLOSE);
	
//	mapNew(256,256);	// Generate the largest size of map needed for the game
//	if (!loadGame("final.gam"))
//	if (!loadGame("savetest.gam"))
//	{
//		return FALSE;
//	}

//	intSetMapPos(43 << TILE_SHIFT, 43 << TILE_SHIFT);


#ifdef PSX
	iV_SetScaleFlags_PSX(IV_SCALE_POSITION | IV_SCALE_SIZE);
	InitialisePIECluts();		// clan colouring stuff
#endif

	DBPRINTF(("stageTwoInitialise: done\n"));

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// Free up after level specific data has been released but before base data is released
//
BOOL stageTwoShutDown(void)
{
	DBPRINTF(("stageTwoShutDown\n"));


#if defined(WIN32) && !defined(I_LIKE_LISTENING_TO_CDS)
	cdAudio_Stop();
#endif

	/* in stageThreeSgutDown now
	if (!missionShutDown())
	{
		return FALSE;
	}*/

	freeAllStructs();
	freeAllDroids();																								  
	freeAllFeatures();
	freeAllFlagPositions();

	if (!messageShutdown())
	{
		return FALSE;
	}

	if (!mechShutdown())
	{
		return FALSE;
	}

#ifdef WIN32
	if(!ShutdownRadar()) {
		return FALSE;
	}
#endif

	intShutDown();

	cmdDroidShutDown();

	//free up the gateway stuff?
	gwShutDown();

	if (!mapShutdown())
	{
		return FALSE;
	}

	return TRUE;
}


/*****************************************************************************/
/*      Initialise after all data is loaded                                  */

void SetAllTilesVisible(void)
{
	// Make all the tiles visible
	MAPTILE *psTile = psMapTiles;
	int i;

	for(i=0; i<mapWidth*mapHeight; i++) {
		SET_TILE_VISIBLE(selectedPlayer,psTile);
		psTile++;
	}
}


/*

	Set all tiles within the scroll limits visible
	- this is for the playstation, so that at the end of each level everything is set to visible
	- this means that we don't have to save the visibilty area in the save game (this is good)
*/
void SetScrollLimitsTilesVisible(void)
{
	MAPTILE	*psTile;
	UWORD	MapX,MapY;

	for (MapY=scrollMinY;MapY<scrollMaxY;MapY++)
	{
		psTile=mapTile(scrollMinX,MapY);
		for (MapX=scrollMinX;MapX<scrollMaxX;MapX++)
		{
			SET_TILE_VISIBLE(selectedPlayer,psTile);
			psTile++;
		}
	}
}


// FromLoad flag used to indicate that this is the first call
// to stageThreeInitialise when loading a saved game so don't close the
// loading screen, onlyt relevant on PSX.
//
#ifdef PSX
BOOL stageThreeInitialise(BOOL FromLoad)
#else
BOOL stageThreeInitialise(void)
#endif
{
//MAPTILE	*psTile;
//UDWORD	i,j;

	STRUCTURE *psStr;

	DBPRINTF(("stageThreeInitalise\n"));

	bTrackingTransporter = FALSE;

	loopMissionState = LMS_NORMAL;

#ifdef WIN32
	// reset the clock to normal speed
	gameTimeResetMod();
#endif

	if (!init3DView())	// Initialise 3d view stuff. After resLoad cause it needs the game palette initialised.
	{
		return FALSE;
	}

	effectResetUpdates();
	//initLighting();
    initLighting(0, 0, mapWidth, mapHeight);

#ifdef WIN32

	if(bMultiPlayer)
	{
		multiGameInit();
		cmdDroidMultiExpBoost(TRUE);
	}
	else
	{
		//ensure single player games do not have this set
		game.maxPlayers = 0;
	}

	preProcessVisibility();
	atmosInitSystem();
	closeLoadingScreen();			// reset the loading screen.
#endif


	if (!fpathInitialise())
	{
		return FALSE;
	}

	clustInitialise();

	gridReset();

	//if mission screen is up, close it.
	if(MissionResUp)
	{
		intRemoveMissionResultNoAnim();
	}

//#ifdef WIN32
//	if(bMultiPlayer)
//	{
//		multiGameInit();
//	}
//#endif


	// determine if to use radar
	for(psStr = apsStructLists[selectedPlayer];psStr;psStr=psStr->psNext){
		if(psStr->pStructureType->type == REF_HQ)
		{
			radarOnScreen = TRUE;
			setHQExists(TRUE,selectedPlayer);
			break;
		}
	}
						

	// Re-inititialise some static variables.

#ifdef PSX
	if(GetControllerType(0) == CON_MOUSE) {
		EnableMouseDraw(TRUE);
	} else {
		EnableMouseDraw(FALSE);
	}
	camInitVars();
	SoundEnable(TRUE);				// re-enable the sound - this was disabled in main.c

#endif
	snapInitVars();
	driveInitVars(FALSE);
	displayInitVars();

	setAllPauseStates(FALSE);


#ifdef PSX
	if(!FromLoad) {
		closeLoadingScreen();			// reset the loading screen.
	}

//	DBPRINTF(("Playing XA Music Track %d\n",NextCDAudioTrack));
//	cdAudio_PlayTrack(NextCDAudioTrack);	// Range 1-3
//	NextCDAudioTrack++;
//	if(NextCDAudioTrack > MAX_CDAUDIO_TRACK) {
//		NextCDAudioTrack = MIN_CDAUDIO_TRACK;
//	}
#endif

#ifdef WIN32		// ffs JS   (and its a global!)
	if (getLevelLoadType() != GTYPE_SAVE_MIDMISSION)
#endif
	{
		eventFireCallbackTrigger(CALL_GAMEINIT);
	}

	return TRUE;
}


#ifdef PSX
void LoadFinished(void)
{
//	closeLoadingScreen();			// reset the loading screen.

	DBPRINTF(("Playing XA Music Track %d\n",NextCDAudioTrack));
	cdAudio_PlayTrack(NextCDAudioTrack);	// Range 1-3
	NextCDAudioTrack++;
	if(NextCDAudioTrack > MAX_CDAUDIO_TRACK) {
		NextCDAudioTrack = MIN_CDAUDIO_TRACK;
	}
}
#endif


/*****************************************************************************/
/*      Shutdown before any data is released                                 */

BOOL stageThreeShutDown(void)
{
	DBPRINTF(("stageThreeShutDown\n"));

#ifdef PSX
	cdAudio_StopTrack();
#endif

	// make sure any button tips are gone.
	widgReset();

#ifdef WIN32
	audio_StopAll();

	saveConfig();					// save options to registry (may have changed in game).

	if(bMultiPlayer)
	{
		multiGameShutdown();
	}
//#if 0
//	environShutDown();
//#endif
//	avCloseSystem();

	cmdDroidMultiExpBoost(FALSE);

#endif

	eventReset();

	// reset the script values system
	scrvReset();

	//call this here before mission data is released
	if (!missionShutDown())
	{
		return FALSE;
	}

	/*
		When this line wasn't at this point. The PSX version always failed on the next script after the tutorial ... unexplained why?
	*/
//	bInTutorial=FALSE;
	scrExternReset();

    //reset the run data so that doesn't need to be initialised in the scripts
    initRunData();

	// Remove any remaining enemy objects.
// Now done in mission state loop.
//	missionDestroyObjects();	

#ifdef PSX
	CancelObjectOrbit();
	// the previous game may have left the pause state true ie, if the player died.
	setGamePauseStatus(FALSE);
#endif

	resetVTOLLandingPos();

// Restore player colours since the scripts might of changed them.
#ifdef WIN32
	if(!bMultiPlayer)
	{
		int temp = getPlayerColour(selectedPlayer);
		initPlayerColours();
		setPlayerColour(selectedPlayer,temp);
	}
	else
	{	
		initPlayerColours();		// reset colours leaving multiplayer game.
	}
#else
	initPlayerColours();
#endif

	setScriptWinLoseVideo(PLAY_NONE);

	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// Reset the state between expand maps or sub maps
//
BOOL gameReset(void)
{
	DBPRINTF(("gameReset\n"));

	return TRUE;
}


// Reset the game between campaigns
BOOL campaignReset(void)
{
	DBPRINTF(("campaignReset\n"));
	gwShutDown();
	mapShutdown();
	return TRUE;
}

// Reset the game when loading a save game
BOOL saveGameReset(void)
{
//#ifdef MISSION_S
	DBPRINTF(("saveGameReset\n"));

#if defined(WIN32) && !defined(I_LIKE_LISTENING_TO_CDS)
	cdAudio_Stop();
#endif

	/* in stageThreeSgutDown now
	if (!missionShutDown())
	{
		return FALSE;
	}*/

	freeAllStructs();
	freeAllDroids();																								  
	freeAllFeatures();
	freeAllFlagPositions();
//#ifdef NEW_SAVE added for V12 SAVE safe for all versions
	initMission();
	initTransporters();
//#endif
	//free up the gateway stuff?
	gwShutDown();
	intResetScreen(TRUE);
	intResetPreviousObj();

	if (!mapShutdown())
	{
		return FALSE;
	}

    //clear out any messages
    freeMessages();

	return TRUE;
}


BOOL newMapInitialise(void)
{
	DBPRINTF(("newMapInitialise\n"));

//NEW_SAVE removed for V11 Save removed for all versions
//	initViewPosition();

// initialise the gateway stuff
//#ifdef WIN32
	// this no longer necessary when RLE map zones are loaded
//	gwProcessMap();	// now loaded with map.

	// this is always necessary
/*	if (!gwLinkGateways())
	{
		return FALSE;
	}
*/
//#endif



	return TRUE;
}

// --- Miscellaneous Initialisation stuff that really should be done each restart
void	initMiscVars( void )
{
	selectedPlayer = 0;
#ifndef NON_INTERACT
	godMode = FALSE;
#else
	godMode = TRUE;
#endif

#ifdef WIN32	// ffs am
//#ifdef ALEXM
//   	setBlipDraw(TRUE);
//	setProximityDraw(FALSE);
//#else
	setBlipDraw(TRUE);
	setProximityDraw(TRUE);
//#endif

#endif
	radarOnScreen = TRUE;
	enableConsoleDisplay(TRUE);

#ifdef WIN32
	setEnergyBarDisplay(TRUE);
#endif

	setSelectedGroup(UBYTE_MAX);
	processDebugMappings(FALSE);
}