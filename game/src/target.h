
#define	TARGET_TYPE_NONE		0x0000
#define	TARGET_TYPE_THREAT		0x0001
#define	TARGET_TYPE_WALL		0x0002
#define	TARGET_TYPE_STRUCTURE	0x0004
#define	TARGET_TYPE_DROID		0x0008
#define	TARGET_TYPE_FEATURE		0x0010
#define	TARGET_TYPE_ARTIFACT	0x0020
#define	TARGET_TYPE_RESOURCE	0x0040
#define TARGET_TYPE_FRIEND		0x0080
#define	TARGET_TYPE_ANY			0xffff

void targetInitialise(void);
void targetOpenList(BASE_OBJECT *psTargeting);
void targetCloseList(void);
void targetSetTargetable(UWORD DroidType);
void targetAdd(BASE_OBJECT *psObj);
void targetKilledObject(BASE_OBJECT *psObj);
BASE_OBJECT *targetAquireNew(void);
BASE_OBJECT *targetAquireNearestScreen(SWORD x,SWORD y,UWORD TargetType);
BASE_OBJECT *targetAquireNearestView(SWORD x,SWORD y,UWORD TargetType);
BASE_OBJECT *targetAquireNearestObj(BASE_OBJECT *psObj,UWORD TargetType);
BASE_OBJECT *targetAquireNearestObjView(BASE_OBJECT *psObj,UWORD TargetType);
BASE_OBJECT *targetAquireNext(UWORD TargetType);
BASE_OBJECT *targetGetCurrent(void);
void targetStartAnim(void);
void targetMarkCurrent(void);
