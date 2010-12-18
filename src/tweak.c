	

#include <stdlib.h>
#include <stdio.h>

//char tweakNames[tweak_num][20];
char tweakNames[tweak_num][20] = 
{ 
	"dummy" ,
	"power_fact", //power
	"power_exp",
	"power_bonus",
	"power_loss",
	"power_start",
	"res_fact", //res
	"res_exp",
	"res_power", 
	"droidbuild_fact", //droid
	"droidpower_fact",
	"droidbody_fact",
	"structbuild_fact", //struct
	"structarmour_fact",
	"structcost_fact",
	"structbody_fact",
	"armour_fact", //misc
	"damage_fact",
	"speed_fact",
	"accel_fact"
};
double tweakDefault[tweak_num] = 
{
	 100, 
	 100, 100, 0, 0, 100, //power
	 100, 100, 100, //res
	 100, 100, 100, //droid
	 100, 100, 100, 100, //struct
	 100, 100, 100 ,100
	};


void initTweakData(void)
{
	
	int x=-1;
	while(++x<MAX_PLAYERS)
	{
		int x2=-1;
		while(++x2<tweak_num){
			tweakData[x][x2]= tweakDefault[x2];
		}
	}
	printf("%s \n",tweakNames[0]);
}
void loadTweakData(void)
	{
	FILE * tweakFile;
	//int tmpstr ="";
	char sTmp [50];
	char * tmpPos;
	char * tmpPos2;
	char * name;
	int value = 0;
	int player = 0;
	int tweakNum;
	int x=-1;
	initTweakData();
	printf("%s \n",tweakNames[0]);
	tweakFile= fopen("./data/base/stats/tweak.txt", "r"); //FIXME Cyp_ told me I should use loadFile don't know how
	if (tweakFile != NULL)
		{
		//tmpstr = fgetc(tweakFile);
		while(fgets (sTmp,100,tweakFile))
		{ //retrieving data from each line
			tmpPos = strchr(sTmp,','); //first coma
			tmpPos2 = strchr(tmpPos+1,','); //second coma
			name= strndup(sTmp,tmpPos-sTmp);
			value=atoi( strndup(tmpPos+1,tmpPos2-tmpPos) );
			player=atoi( strndup(tmpPos+1,2) );
		 	tweakNum=0;
			x=-1;
			while(++x<tweak_num && tweakNum==0 )
			{
				//printf("%d : '%s'='%s'",x,name,tweakNames[x]);
				if(strcmp(name,tweakNames[x])==0)
				{
					tweakNum=x;
					//printf("(Y)");
				}
				//printf("\n");

			}
			if(tweak_num)
			{
			
				if(player>MAX_PLAYERS)
				{
					debug(LOG_MSG,"%s (%d) = %i for everybody\n",name,tweakNum,value);
					x=-1;
					while(++x<MAX_PLAYERS)
					{
						tweakData[x][tweakNum]=value;
					}
				}
				else
				{
					debug(LOG_MSG,"%s (%d) = %i for %d \n",name,tweakNum,value,player);
					tweakData[player][tweakNum]=value;
				}
			}
		}
	}
	else 
	{
		debug(LOG_MSG,"./data/base/stats/tweak.txt is not existing");	
	}
	//some debug to see if everything look fine...
	//printf("test1 = %f  \n",tweakData[0][tweak_res_exp]);	

}
