

#ifdef __cplusplus

struct ZoneMapHeader {
	UWORD version;
	UWORD numStrips;
	UWORD numZones;
	UWORD pad;
};

void giSetMapData(CHeightMap *MapData);
BOOL giCreateZones(void);
void giDeleteZones(void);
void giUpdateMapZoneIDs(void);
void giClearGatewayFlags(void);
BOOL giWriteZones(FILE *Stream);

#else

int giGetMapWidth(void);
int giGetMapHeight(void);
void giSetGatewayFlag(int x,int y,BOOL IsGateway);
BOOL giIsClifface(int x,int y);
BOOL giIsWater(int x,int y);
BOOL giIsGateway(int x,int y);
int giGetNumGateways(void);
BOOL giGetGateway(int Index,int *x0,int *y0,int *x1,int *y1);

#endif
