/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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
*/

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/bitimage.h"//bitmap routines

#include "hci.h"
#include "intdisplay.h"
#include "intorder.h"
#include "objects.h"
#include "order.h"
#include "scriptextern.h"

#include <set>


#define ORDER_X			6
#define ORDER_Y			45
#define ORDER_WIDTH		RET_FORMWIDTH
#define ORDER_HEIGHT	273
#define ORDER_BUTX		8
#define ORDER_BUTY		16
#define ORDER_BUTGAP	4
#define ORDER_BOTTOMY	318	+ E_H

#define MAX_DISPLAYABLE_ORDERS 11	// Max number of displayable orders.
#define MAX_ORDER_BUTS 5		// Max number of buttons for a given order.
#define NUM_ORDERS 12			// Number of orders in OrderButtons list.

#define IDORDER_REPAIR_LEVEL				8020
#define IDORDER_ATTACK_LEVEL				8030
#define IDORDER_PATROL						8040
#define IDORDER_RETURN						8060
#define IDORDER_RECYCLE						8070
#define IDORDER_ASSIGN_PRODUCTION			8080
#define IDORDER_ASSIGN_CYBORG_PRODUCTION	8090
#define IDORDER_FIRE_DESIGNATOR				8100
#define IDORDER_ASSIGN_VTOL_PRODUCTION		8110
#define IDORDER_CIRCLE						8120

enum ORDBUTTONTYPE
{
	ORD_BTYPE_RADIO,			// Only one state allowed.
	ORD_BTYPE_BOOLEAN,			// Clicking on a button toggles it's state.
	ORD_BTYPE_BOOLEAN_DEPEND,	// Clicking on a button toggles it's state, button
	// is only enabled if previous button is down.
	ORD_BTYPE_BOOLEAN_COMBINE,	// Clicking on a button toggles it's state, all
	// the buttons states are OR'ed together to get the order state
};

enum ORDBUTTONCLASS
{
	ORDBUTCLASS_NORMAL,			// A normal button, one order type per line.
	ORDBUTCLASS_FACTORY,		// A factory assignment button.
	ORDBUTCLASS_CYBORGFACTORY,	// A cyborg factory assignment button.
	ORDBUTCLASS_VTOLFACTORY, 	// A VTOL factory assignment button.
};


/*
 NOTE:
	ORD_BTYPE_BOOLEAN_DEPEND only supports two buttons
	ie button 1 = enable destruct, button 2 = destruct
*/

enum ORDBUTTONJUSTIFY
{
	ORD_JUSTIFY_LEFT,			// Pretty self explanatory really.
	ORD_JUSTIFY_RIGHT,
	ORD_JUSTIFY_CENTER,
	ORD_JUSTIFY_COMBINE,		// allow the button to be put on the same line
	// as other orders with the same justify type
	ORD_NUM_JUSTIFY_TYPES,
};

// maximum number of orders on one line
#define ORD_MAX_COMBINE_BUTS	3

#define ORD_JUSTIFY_MASK	0x0f
#define ORD_JUSTIFY_NEWLINE	0x10	// Or with ORDBUTTONJUSTIFY enum type to specify start on new line.

struct ORDERBUTTONS
{
	ORDBUTTONCLASS Class;					// The class of button.
	SECONDARY_ORDER Order;					// The droid order.
	UDWORD StateMask;						// It's state mask.
	ORDBUTTONTYPE ButType;					// The group type.
	unsigned ButJustify;                            // Button justification. Type ORDBUTTONJUSTIFY, possibly ored with ORD_JUSTIFY_NEWLINE.
	UDWORD ButBaseID;						// Starting widget ID for buttons
	UWORD NumButs;							// Number of buttons ( = number of states )
	UWORD AcNumButs;						// Actual bumber of buttons enabled.
	UWORD ButImageID[MAX_ORDER_BUTS];		// Image ID's for each button ( normal ).
	UWORD ButGreyID[MAX_ORDER_BUTS];		// Image ID's for each button ( greyed ).
	UWORD ButHilightID[MAX_ORDER_BUTS];		// Image ID's for each button ( hilight overlay ).
	UWORD ButTips[MAX_ORDER_BUTS];			// Tip string id for each button.
	unsigned States[MAX_ORDER_BUTS];                // Order state relating to each button, combination of SECONDARY_STATEs ored together.
};


struct AVORDER
{
	bool operator <(AVORDER const &b) const
	{
		return OrderIndex < b.OrderIndex;
	}
	bool operator ==(AVORDER const &b) const
	{
		return OrderIndex == b.OrderIndex;
	}

	UWORD OrderIndex;		// Index into ORDERBUTTONS array of available orders.
};


enum
{
	STR_DORD_REPAIR1,
	STR_DORD_REPAIR2,
	STR_DORD_REPAIR3,
	STR_DORD_FIRE1,
	STR_DORD_FIRE2,
	STR_DORD_FIRE3,
	STR_DORD_PATROL,
	STR_DORD_RETREPAIR,
	STR_DORD_RETBASE,
	STR_DORD_EMBARK,
	STR_DORD_ARMRECYCLE,
	STR_DORD_RECYCLE,
	STR_DORD_FACTORY,
	STR_DORD_CYBORG_FACTORY,
	STR_DORD_FIREDES,
	STR_DORD_VTOL_FACTORY,
	STR_DORD_CIRCLE,
};

// return translated text
static const char *getDORDDescription(int id)
{
	switch (id)
	{
	case STR_DORD_REPAIR1        : return _("Retreat at Medium Damage");
	case STR_DORD_REPAIR2        : return _("Retreat at Heavy Damage");
	case STR_DORD_REPAIR3        : return _("Do or Die!");
	case STR_DORD_FIRE1          : return _("Fire-At-Will");
	case STR_DORD_FIRE2          : return _("Return Fire");
	case STR_DORD_FIRE3          : return _("Hold Fire");
	case STR_DORD_PATROL         : return _("Patrol");
	case STR_DORD_RETREPAIR      : return _("Return For Repair");
	case STR_DORD_RETBASE        : return _("Return To HQ");
	case STR_DORD_EMBARK         : return _("Go to Transport");
	case STR_DORD_ARMRECYCLE     : return _("Return for Recycling");
	case STR_DORD_RECYCLE        : return _("Recycle");
	case STR_DORD_FACTORY        : return _("Assign Factory Production");
	case STR_DORD_CYBORG_FACTORY : return _("Assign Cyborg Factory Production");
	case STR_DORD_FIREDES        : return _("Assign Fire Support");
	case STR_DORD_VTOL_FACTORY   : return _("Assign VTOL Factory Production");
	case STR_DORD_CIRCLE         : return _("Circle");

	default : return "";  // make compiler shut up
	}
};

// Define the order button groups.
static ORDERBUTTONS OrderButtons[NUM_ORDERS] =
{
	{
		ORDBUTCLASS_NORMAL,
		DSO_REPAIR_LEVEL,
		DSS_REPLEV_MASK,
		ORD_BTYPE_RADIO,
		ORD_JUSTIFY_CENTER | ORD_JUSTIFY_NEWLINE,
		IDORDER_REPAIR_LEVEL,
		3, 0,
		{IMAGE_ORD_REPAIR3UP,	IMAGE_ORD_REPAIR2UP,	IMAGE_ORD_REPAIR1UP},
		{IMAGE_ORD_REPAIR3UP,	IMAGE_ORD_REPAIR2UP,	IMAGE_ORD_REPAIR1UP},
		{IMAGE_DES_HILIGHT,		IMAGE_DES_HILIGHT,	IMAGE_DES_HILIGHT},
		{STR_DORD_REPAIR3,	STR_DORD_REPAIR2,	STR_DORD_REPAIR1},
		{DSS_REPLEV_NEVER,	DSS_REPLEV_HIGH,	DSS_REPLEV_LOW}
	},
	{
		ORDBUTCLASS_NORMAL,
		DSO_ATTACK_LEVEL,
		DSS_ALEV_MASK,
		ORD_BTYPE_RADIO,
		ORD_JUSTIFY_CENTER | ORD_JUSTIFY_NEWLINE,
		IDORDER_ATTACK_LEVEL,
		3, 0,
		{IMAGE_ORD_FATWILLUP,	IMAGE_ORD_RETFIREUP,	IMAGE_ORD_HOLDFIREUP},
		{IMAGE_ORD_FATWILLUP,	IMAGE_ORD_RETFIREUP,	IMAGE_ORD_HOLDFIREUP},
		{IMAGE_DES_HILIGHT,		IMAGE_DES_HILIGHT,	IMAGE_DES_HILIGHT},
		{STR_DORD_FIRE1,	STR_DORD_FIRE2,	STR_DORD_FIRE3},
		{DSS_ALEV_ALWAYS,	DSS_ALEV_ATTACKED,	DSS_ALEV_NEVER}
	},
	{
		ORDBUTCLASS_NORMAL,
		DSO_FIRE_DESIGNATOR,
		DSS_FIREDES_MASK,
		ORD_BTYPE_BOOLEAN,
		ORD_JUSTIFY_COMBINE,
		IDORDER_FIRE_DESIGNATOR,
		1, 0,
		{IMAGE_ORD_FIREDES_UP,	0,	0},
		{IMAGE_ORD_FIREDES_UP,	0,	0},
		{IMAGE_DES_HILIGHT,	0,	0},
		{STR_DORD_FIREDES,	0,	0},
		{DSS_FIREDES_SET,	0,	0}
	},
	{
		ORDBUTCLASS_NORMAL,
		DSO_PATROL,
		DSS_PATROL_MASK,
		ORD_BTYPE_BOOLEAN,
		ORD_JUSTIFY_COMBINE,
		IDORDER_PATROL,
		1, 0,
		{IMAGE_ORD_PATROLUP,	0,	0},
		{IMAGE_ORD_PATROLUP,	0,	0},
		{IMAGE_DES_HILIGHT,	0,	0},
		{STR_DORD_PATROL,	0,	0},
		{DSS_PATROL_SET,	0,	0}
	},
	{
		ORDBUTCLASS_NORMAL,
		DSO_CIRCLE,
		DSS_CIRCLE_MASK,
		ORD_BTYPE_BOOLEAN,
		ORD_JUSTIFY_COMBINE,
		IDORDER_CIRCLE,
		1, 0,
		{IMAGE_ORD_CIRCLEUP,	0,	0},
		{IMAGE_ORD_CIRCLEUP,	0,	0},
		{IMAGE_DES_HILIGHT,	0,	0},
		{STR_DORD_CIRCLE,	0,	0},
		{DSS_CIRCLE_SET,	0,	0}
	},
	{
		ORDBUTCLASS_NORMAL,
		DSO_RETURN_TO_LOC,
		DSS_RTL_MASK,
		ORD_BTYPE_RADIO,
		ORD_JUSTIFY_CENTER | ORD_JUSTIFY_NEWLINE,
		IDORDER_RETURN,
		3, 0,

		{IMAGE_ORD_RTRUP,	IMAGE_ORD_GOTOHQUP,	IMAGE_ORD_EMBARKUP},
		{IMAGE_ORD_RTRUP,	IMAGE_ORD_GOTOHQUP,	IMAGE_ORD_EMBARKUP},

		{IMAGE_DES_HILIGHT,		IMAGE_DES_HILIGHT,	IMAGE_DES_HILIGHT},
		{STR_DORD_RETREPAIR,	STR_DORD_RETBASE,	STR_DORD_EMBARK},
		{DSS_RTL_REPAIR,	DSS_RTL_BASE,	DSS_RTL_TRANSPORT},
	},
	{
		ORDBUTCLASS_NORMAL,
		DSO_RECYCLE,
		DSS_RECYCLE_MASK,
		ORD_BTYPE_BOOLEAN_DEPEND,
		ORD_JUSTIFY_CENTER | ORD_JUSTIFY_NEWLINE,
		IDORDER_RECYCLE,
		2, 0,
		{IMAGE_ORD_DESTRUCT1UP,	IMAGE_ORD_DESTRUCT2UP,	0},
		{IMAGE_ORD_DESTRUCT1UP,	IMAGE_ORD_DESTRUCT2GREY,	0},
		{IMAGE_DES_HILIGHT,	IMAGE_DES_HILIGHT,	0},
		{STR_DORD_ARMRECYCLE,	STR_DORD_RECYCLE,	0},
		{DSS_RECYCLE_SET,	DSS_RECYCLE_SET,	0}
	},
	{
		ORDBUTCLASS_FACTORY,
		DSO_ASSIGN_PRODUCTION,
		DSS_ASSPROD_MASK,
		ORD_BTYPE_BOOLEAN_COMBINE,
		ORD_JUSTIFY_CENTER | ORD_JUSTIFY_NEWLINE,
		IDORDER_ASSIGN_PRODUCTION,
		5, 0,
		{IMAGE_ORD_FAC1UP,		IMAGE_ORD_FAC2UP,	IMAGE_ORD_FAC3UP,	IMAGE_ORD_FAC4UP,	IMAGE_ORD_FAC5UP	},
		{IMAGE_ORD_FAC1UP,		IMAGE_ORD_FAC2UP,	IMAGE_ORD_FAC3UP,	IMAGE_ORD_FAC4UP,	IMAGE_ORD_FAC5UP	},
		{IMAGE_ORD_FACHILITE,	IMAGE_ORD_FACHILITE, IMAGE_ORD_FACHILITE, IMAGE_ORD_FACHILITE, IMAGE_ORD_FACHILITE	},
		{STR_DORD_FACTORY,	STR_DORD_FACTORY,	STR_DORD_FACTORY,	STR_DORD_FACTORY,	STR_DORD_FACTORY},
		{
			0x01 << DSS_ASSPROD_SHIFT, 0x02 << DSS_ASSPROD_SHIFT, 0x04 << DSS_ASSPROD_SHIFT,
			0x08 << DSS_ASSPROD_SHIFT, 0x10 << DSS_ASSPROD_SHIFT
		}
	},
	{
		ORDBUTCLASS_CYBORGFACTORY,
		DSO_ASSIGN_CYBORG_PRODUCTION,
		DSS_ASSPROD_MASK,
		ORD_BTYPE_BOOLEAN_COMBINE,
		ORD_JUSTIFY_CENTER | ORD_JUSTIFY_NEWLINE,
		IDORDER_ASSIGN_CYBORG_PRODUCTION,
		5, 0,
		{IMAGE_ORD_FAC1UP,		IMAGE_ORD_FAC2UP,	IMAGE_ORD_FAC3UP,	IMAGE_ORD_FAC4UP,	IMAGE_ORD_FAC5UP	},
		{IMAGE_ORD_FAC1UP,		IMAGE_ORD_FAC2UP,	IMAGE_ORD_FAC3UP,	IMAGE_ORD_FAC4UP,	IMAGE_ORD_FAC5UP	},
		{IMAGE_ORD_FACHILITE,	IMAGE_ORD_FACHILITE, IMAGE_ORD_FACHILITE, IMAGE_ORD_FACHILITE, IMAGE_ORD_FACHILITE	},
		{STR_DORD_CYBORG_FACTORY,	STR_DORD_CYBORG_FACTORY,	STR_DORD_CYBORG_FACTORY,	STR_DORD_CYBORG_FACTORY,	STR_DORD_CYBORG_FACTORY},
		{
			0x01 << DSS_ASSPROD_CYBORG_SHIFT, 0x02 << DSS_ASSPROD_CYBORG_SHIFT, 0x04 << DSS_ASSPROD_CYBORG_SHIFT,
			0x08 << DSS_ASSPROD_CYBORG_SHIFT, 0x10 << DSS_ASSPROD_CYBORG_SHIFT
		}
	},
	{
		ORDBUTCLASS_VTOLFACTORY,
		DSO_ASSIGN_VTOL_PRODUCTION,
		DSS_ASSPROD_MASK,
		ORD_BTYPE_BOOLEAN_COMBINE,
		ORD_JUSTIFY_CENTER | ORD_JUSTIFY_NEWLINE,
		IDORDER_ASSIGN_VTOL_PRODUCTION,
		5, 0,
		{IMAGE_ORD_FAC1UP,		IMAGE_ORD_FAC2UP,	IMAGE_ORD_FAC3UP,	IMAGE_ORD_FAC4UP,	IMAGE_ORD_FAC5UP	},
		{IMAGE_ORD_FAC1UP,		IMAGE_ORD_FAC2UP,	IMAGE_ORD_FAC3UP,	IMAGE_ORD_FAC4UP,	IMAGE_ORD_FAC5UP	},
		{IMAGE_ORD_FACHILITE,	IMAGE_ORD_FACHILITE, IMAGE_ORD_FACHILITE, IMAGE_ORD_FACHILITE, IMAGE_ORD_FACHILITE	},
		{STR_DORD_VTOL_FACTORY,	STR_DORD_VTOL_FACTORY,	STR_DORD_VTOL_FACTORY,	STR_DORD_VTOL_FACTORY,	STR_DORD_VTOL_FACTORY},
		{
			0x01 << DSS_ASSPROD_VTOL_SHIFT, 0x02 << DSS_ASSPROD_VTOL_SHIFT, 0x04 << DSS_ASSPROD_VTOL_SHIFT,
			0x08 << DSS_ASSPROD_VTOL_SHIFT, 0x10 << DSS_ASSPROD_VTOL_SHIFT
		}
	},
};


static std::vector<DROID *> SelectedDroids;
static STRUCTURE *psSelectedFactory = NULL;
static std::vector<AVORDER> AvailableOrders;


bool OrderUp = false;

// Build a list of currently selected droids.
// Returns true if droids were selected.
//
static bool BuildSelectedDroidList(void)
{
	DROID *psDroid;

	for (psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		if (psDroid->selected)
		{
			SelectedDroids.push_back(psDroid);
		}
	}

	return !SelectedDroids.empty();
}

// Build a list of orders available for the selected group of droids.
//
static std::vector<AVORDER> buildDroidOrderList(void)
{
	std::set<AVORDER> orders;

	for (unsigned j = 0; j < SelectedDroids.size(); j++)
	{
		for (unsigned OrdIndex = 0; OrdIndex < NUM_ORDERS; ++OrdIndex)
		{
			// Is the order available?
			if (secondarySupported(SelectedDroids[j], OrderButtons[OrdIndex].Order))
			{
				AVORDER avOrder;
				avOrder.OrderIndex = OrdIndex;
				// Sort by Order index, A bubble sort? I know but it's only
				// a small list so what the hell.
				// Herein lies the remains of a bubble sort.
				orders.insert(avOrder);
			}
		}
	}

	return std::vector<AVORDER>(orders.begin(), orders.end());
}

// Build a list of orders available for the selected structure.
static std::vector<AVORDER> buildStructureOrderList(STRUCTURE *psStructure)
{
	ASSERT_OR_RETURN(std::vector<AVORDER>(), StructIsFactory(psStructure), "BuildStructureOrderList: structure is not a factory");

	//this can be hard-coded!
	std::vector<AVORDER> orders(2);
	orders[0].OrderIndex = 0;//DSO_REPAIR_LEVEL;
	orders[1].OrderIndex = 1;//DSO_ATTACK_LEVEL;

	return orders;
}

// return the state for an order for all the units selected
// if there are multiple states then don't return a state
static SECONDARY_STATE GetSecondaryStates(SECONDARY_ORDER sec)
{
	SECONDARY_STATE state, currState;
	bool	bFirst;

	state = (SECONDARY_STATE)0;
	bFirst = true;
	if (psSelectedFactory)
	{
		if (getFactoryState(psSelectedFactory, sec, &currState))
		{
			state = currState;
		}
	}
	else //droids
	{
		for (unsigned i = 0; i < SelectedDroids.size(); ++i)
		{
			currState = secondaryGetState(SelectedDroids[i], sec, ModeQueue);
			if (bFirst)
			{
				state = currState;
				bFirst = false;
			}
			else if (state != currState)
			{
				state = (SECONDARY_STATE)0;
			}
		}
	}

	return state;
}


static UDWORD GetImageWidth(IMAGEFILE *ImageFile, UDWORD ImageID)
{
	return iV_GetImageWidth(ImageFile, (UWORD)ImageID);
}

static UDWORD GetImageHeight(IMAGEFILE *ImageFile, UDWORD ImageID)
{
	return iV_GetImageHeight(ImageFile, (UWORD)ImageID);
}


// Add the droid order screen.
// Returns true if the form was displayed ok.
//
//changed to a BASE_OBJECT to accomodate the factories - AB 21/04/99
bool intAddOrder(BASE_OBJECT *psObj)
{
	bool Animate = true;
	SECONDARY_STATE State;
	UWORD OrdIndex;
	UWORD Height, NumDisplayedOrders;
	UWORD NumButs;
	UWORD NumJustifyButs, NumCombineButs, NumCombineBefore;
	bool  bLastCombine, bHidden;
	DROID *Droid;
	STRUCTURE *psStructure;

	if (bInTutorial)
	{
		// No RMB orders in tutorial!!
		return (false);
	}

	// Is the form already up?
	if (widgGetFromID(psWScreen, IDORDER_FORM) != NULL)
	{
		intRemoveOrderNoAnim();
		Animate = false;
	}
	// Is the stats window up?
	if (widgGetFromID(psWScreen, IDSTAT_FORM) != NULL)
	{
		intRemoveStatsNoAnim();
		Animate = false;
	}

	if (psObj)
	{
		if (psObj->type == OBJ_DROID)
		{
			Droid = (DROID *)psObj;
			psStructure =  NULL;
		}
		else if (psObj->type == OBJ_STRUCTURE)
		{
			Droid = NULL;
			psStructure = (STRUCTURE *)psObj;
			psSelectedFactory = psStructure;
			ASSERT_OR_RETURN(false, StructIsFactory(psSelectedFactory), "Trying to select a %s as a factory!",
			                 objInfo((BASE_OBJECT *)psSelectedFactory));
		}
		else
		{
			ASSERT(false, "Invalid object type");
			Droid = NULL;
			psStructure =  NULL;
		}
	}
	else
	{
		Droid = NULL;
		psStructure =  NULL;
	}

	setWidgetsStatus(true);

	AvailableOrders.clear();
	SelectedDroids.clear();

	// Selected droid is a command droid?
	if ((Droid != NULL) && (Droid->droidType == DROID_COMMAND))
	{
		// displaying for a command droid - ignore any other droids
		SelectedDroids.push_back(Droid);
	}
	else if (psStructure != NULL)
	{
		AvailableOrders = buildStructureOrderList(psStructure);
		if (AvailableOrders.empty())
		{
			return false;
		}
	}
	// Otherwise build a list of selected droids.
	else if (!BuildSelectedDroidList())
	{
		// If no droids selected then see if we were given a specific droid.
		if (Droid != NULL)
		{
			// and put it in the list.
			SelectedDroids.push_back(Droid);
		}
	}

	// Build a list of orders available for the list of selected droids. - if a factory has not been selected
	if (psStructure == NULL)
	{
		AvailableOrders = buildDroidOrderList();
		if (AvailableOrders.empty())
		{
			// If no orders then return;
			return false;
		}
	}

	WIDGET *parent = psWScreen->psForm;

	/* Create the basic form */
	IntFormAnimated *orderForm = new IntFormAnimated(parent, Animate);  // Do not animate the opening, if the window was already open.
	orderForm->id = IDORDER_FORM;
	orderForm->setGeometry(ORDER_X, ORDER_Y, ORDER_WIDTH, ORDER_HEIGHT);

	// Add the close button.
	W_BUTINIT sButInit;
	sButInit.formID = IDORDER_FORM;
	sButInit.id = IDORDER_CLOSE;
	sButInit.x = ORDER_WIDTH - CLOSE_WIDTH;
	sButInit.y = 0;
	sButInit.width = CLOSE_WIDTH;
	sButInit.height = CLOSE_HEIGHT;
	sButInit.pTip = _("Close");
	sButInit.pDisplay = intDisplayImageHilight;
	sButInit.UserData = PACKDWORD_TRI(0, IMAGE_CLOSEHILIGHT , IMAGE_CLOSE);
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

	sButInit = W_BUTINIT();
	sButInit.formID = IDORDER_FORM;
	sButInit.id = IDORDER_CLOSE + 1;
	sButInit.pDisplay = intDisplayButtonHilight;
	sButInit.y = ORDER_BUTY;

	Height = 0;
	NumDisplayedOrders = 0;

	for (unsigned j = 0; j < AvailableOrders.size() && NumDisplayedOrders < MAX_DISPLAYABLE_ORDERS; ++j)
	{
		OrdIndex = AvailableOrders[j].OrderIndex;

		// Get current order state.
		State = GetSecondaryStates(OrderButtons[OrdIndex].Order);

		// Get number of buttons.
		NumButs = OrderButtons[OrdIndex].NumButs;
		// Set actual number of buttons.
		OrderButtons[OrdIndex].AcNumButs = NumButs;

		// Handle special case for factory -> command droid assignment buttons.
		switch (OrderButtons[OrdIndex].Class)
		{
		case ORDBUTCLASS_FACTORY:
			NumButs = countAssignableFactories((UBYTE)selectedPlayer, FACTORY_FLAG);
			break;
		case ORDBUTCLASS_CYBORGFACTORY:
			NumButs = countAssignableFactories((UBYTE)selectedPlayer, CYBORG_FLAG);
			break;
		case ORDBUTCLASS_VTOLFACTORY:
			NumButs = countAssignableFactories((UBYTE)selectedPlayer, VTOL_FLAG);
			break;
		default:
			break;
		}

		sButInit.id = OrderButtons[OrdIndex].ButBaseID;

		NumJustifyButs = NumButs;
		bLastCombine = false;

		switch (OrderButtons[OrdIndex].ButJustify & ORD_JUSTIFY_MASK)
		{
		case ORD_JUSTIFY_LEFT:
			sButInit.x = ORDER_BUTX;
			break;

		case ORD_JUSTIFY_RIGHT:
			sButInit.x = orderForm->width() - ORDER_BUTX -
			             (NumJustifyButs * GetImageWidth(IntImages, OrderButtons[OrdIndex].ButImageID[0]) +
			              (NumJustifyButs - 1) * ORDER_BUTGAP);
			break;

		case ORD_JUSTIFY_CENTER:
			sButInit.x = (orderForm->width() -
			              (NumJustifyButs * GetImageWidth(IntImages, OrderButtons[OrdIndex].ButImageID[0]) +
			               (NumJustifyButs - 1) * ORDER_BUTGAP)) / 2;
			break;

		case ORD_JUSTIFY_COMBINE:
			// see how many are on this line before the button
			NumCombineBefore = 0;
			for (unsigned i = 0; i < j; ++i)
			{
				if ((OrderButtons[AvailableOrders[i].OrderIndex].ButJustify & ORD_JUSTIFY_MASK)
				    == ORD_JUSTIFY_COMBINE)
				{
					NumCombineBefore += 1;
				}
			}
			NumCombineButs = (UWORD)(NumCombineBefore + 1);

			// now see how many in total
			for (unsigned i = j + 1; i < AvailableOrders.size(); ++i)
			{
				if ((OrderButtons[AvailableOrders[i].OrderIndex].ButJustify & ORD_JUSTIFY_MASK)
				    == ORD_JUSTIFY_COMBINE)
				{
					NumCombineButs += 1;
				}
			}

			// get position on line
			NumCombineButs = (UWORD)(NumCombineButs - (NumCombineBefore - (NumCombineBefore % ORD_MAX_COMBINE_BUTS)));

			if (NumCombineButs >= ORD_MAX_COMBINE_BUTS)
			{
				// the buttons will fill the line
				sButInit.x = (SWORD)(ORDER_BUTX +
				                     (GetImageWidth(IntImages, OrderButtons[OrdIndex].ButImageID[0]) + ORDER_BUTGAP) * NumCombineBefore);
			}
			else
			{
				// center the buttons
				sButInit.x = orderForm->width() / 2 -
				             (NumCombineButs * GetImageWidth(IntImages, OrderButtons[OrdIndex].ButImageID[0]) +
				              (NumCombineButs - 1) * ORDER_BUTGAP) / 2;
				sButInit.x = (SWORD)(sButInit.x +
				                     (GetImageWidth(IntImages, OrderButtons[OrdIndex].ButImageID[0]) + ORDER_BUTGAP) * NumCombineBefore);
			}

			// see if need to start a new line of buttons
			if ((NumCombineBefore + 1) == (NumCombineButs % ORD_MAX_COMBINE_BUTS))
			{
				bLastCombine = true;
			}

			break;
		}

		for (unsigned i = 0; i < OrderButtons[OrdIndex].AcNumButs; ++i)
		{
			sButInit.pTip = getDORDDescription(OrderButtons[OrdIndex].ButTips[i]);
			sButInit.width = (UWORD)GetImageWidth(IntImages, OrderButtons[OrdIndex].ButImageID[i]);
			sButInit.height = (UWORD)GetImageHeight(IntImages, OrderButtons[OrdIndex].ButImageID[i]);
			sButInit.UserData = PACKDWORD_TRI(OrderButtons[OrdIndex].ButGreyID[i],
			                                  OrderButtons[OrdIndex].ButHilightID[i],
			                                  OrderButtons[OrdIndex].ButImageID[i]);
			if (!widgAddButton(psWScreen, &sButInit))
			{
				return false;
			}

			// Set the default state for the button.
			switch (OrderButtons[OrdIndex].ButType)
			{
			case ORD_BTYPE_RADIO:
			case ORD_BTYPE_BOOLEAN:
			 {
				bool selected = (State & OrderButtons[OrdIndex].StateMask) == (UDWORD)OrderButtons[OrdIndex].States[i];
				widgSetButtonState(psWScreen, sButInit.id, selected? WBUT_CLICKLOCK : 0);
				break;
			 }
			case ORD_BTYPE_BOOLEAN_DEPEND:
			 {
				bool selected = (State & OrderButtons[OrdIndex].StateMask) == (UDWORD)OrderButtons[OrdIndex].States[i];
				bool first = i == 0;
				widgSetButtonState(psWScreen, sButInit.id, selected? WBUT_CLICKLOCK : first? 0 : WBUT_DISABLE);
				break;
			 }
			case ORD_BTYPE_BOOLEAN_COMBINE:
			 {
				bool selected = State & (UDWORD)OrderButtons[OrdIndex].States[i];
				widgSetButtonState(psWScreen, sButInit.id, selected? WBUT_CLICKLOCK : 0);
				break;
			 }
			}

			// may not add a button if the factory doesn't exist
			bHidden = false;
			switch (OrderButtons[OrdIndex].Class)
			{
			case ORDBUTCLASS_FACTORY:
				if (!checkFactoryExists(selectedPlayer, FACTORY_FLAG, i))
				{
					widgHide(psWScreen, sButInit.id);
					bHidden = true;
				}
				break;
			case ORDBUTCLASS_CYBORGFACTORY:
				if (!checkFactoryExists(selectedPlayer, CYBORG_FLAG, i))
				{
					widgHide(psWScreen, sButInit.id);
					bHidden = true;
				}
				break;
			case ORDBUTCLASS_VTOLFACTORY:
				if (!checkFactoryExists(selectedPlayer, VTOL_FLAG, i))
				{
					widgHide(psWScreen, sButInit.id);
					bHidden = true;
				}
				break;
			default:
				break;
			}

			if (!bHidden)
			{

				sButInit.x = (SWORD)(sButInit.x + sButInit.width + ORDER_BUTGAP);
			}
			sButInit.id++;
		}

		if (((OrderButtons[OrdIndex].ButJustify & ORD_JUSTIFY_MASK) != ORD_JUSTIFY_COMBINE) ||
		    bLastCombine)
		{
			sButInit.y = (SWORD)(sButInit.y + sButInit.height + ORDER_BUTGAP);
			Height = (UWORD)(Height + sButInit.height + ORDER_BUTGAP);
		}
		NumDisplayedOrders ++;
	}

	// Now we know how many orders there are we can resize the form accordingly.
	int newHeight = Height + CLOSE_HEIGHT + ORDER_BUTGAP;
	orderForm->setGeometry(orderForm->x(), ORDER_BOTTOMY - newHeight, orderForm->width(), newHeight);

	OrderUp = true;

	return true;
}


// Do any housekeeping for the droid order screen.
// Any droids that die now get set to NULL - John.
// No droids being selected no longer removes the screen,
// this lets the screen work with command droids - John.
void intRunOrder(void)
{
	// Check to see if there all dead or unselected.
	for (unsigned i = 0; i < SelectedDroids.size(); i++)
	{
		if (SelectedDroids[i]->died)
		{
			SelectedDroids[i] = NULL;
		}
	}
	// Remove any NULL pointers from SelectedDroids.
	SelectedDroids.erase(std::remove(SelectedDroids.begin(), SelectedDroids.end(), (DROID *)NULL), SelectedDroids.end());

	if (psSelectedFactory != NULL && psSelectedFactory->died)
	{
		psSelectedFactory = NULL;
	}

	// If all dead then remove the screen.
	// If droids no longer selected then remove screen.
	if (SelectedDroids.empty())
	{
		// might have a factory selected
		if (psSelectedFactory == NULL)
		{
			intRemoveOrder();
			return;
		}
	}
}


// Set the secondary order state for all currently selected droids. And Factory (if one selected)
// Returns true if successful.
//
static bool SetSecondaryState(SECONDARY_ORDER sec, unsigned State)
{
	// This code is similar to kfsf_SetSelectedDroidsState() in keybind.cpp. Unfortunately, it seems hard to un-duplicate the code.
	for (unsigned i = 0; i < SelectedDroids.size(); ++i)
	{
		if (SelectedDroids[i])
		{
			//Only set the state if it's not a transporter.
			if (!isTransporter(SelectedDroids[i]))
			{
				if (!secondarySetState(SelectedDroids[i], sec, (SECONDARY_STATE)State))
				{
					return false;
				}
			}
		}
	}

	// set the Factory settings
	if (psSelectedFactory)
	{
		if (!setFactoryState(psSelectedFactory, sec, (SECONDARY_STATE)State))
		{
			return false;
		}
	}

	return true;
}


// Process the droid order screen.
//
void intProcessOrder(UDWORD id)
{
	UWORD i;
	UWORD OrdIndex;
	UDWORD BaseID;
	UDWORD StateIndex;
	UDWORD CombineState;

	if (id == IDORDER_CLOSE)
	{
		intRemoveOrder();
		if (intMode == INT_ORDER)
		{
			intMode = INT_NORMAL;
		}
		else
		{
			/* Unlock the stats button */
			widgSetButtonState(psWScreen, objStatID, 0);
			intMode = INT_OBJECT;
		}
		return;
	}

	for (OrdIndex = 0; OrdIndex < NUM_ORDERS; OrdIndex++)
	{
		BaseID = OrderButtons[OrdIndex].ButBaseID;

		switch (OrderButtons[OrdIndex].ButType)
		{
		case ORD_BTYPE_RADIO:
			if ((id >= BaseID) && (id < BaseID + OrderButtons[OrdIndex].AcNumButs))
			{
				StateIndex = id - BaseID;

				for (i = 0; i < OrderButtons[OrdIndex].AcNumButs; i++)
				{
					widgSetButtonState(psWScreen, BaseID + i, 0);
				}
				if (SetSecondaryState(OrderButtons[OrdIndex].Order,
				                      OrderButtons[OrdIndex].States[StateIndex] & OrderButtons[OrdIndex].StateMask))
				{
					widgSetButtonState(psWScreen, id, WBUT_CLICKLOCK);
				}
			}
			break;

		case ORD_BTYPE_BOOLEAN:
			if ((id >= BaseID) && (id < BaseID + OrderButtons[OrdIndex].AcNumButs))
			{
				StateIndex = id - BaseID;

				if (widgGetButtonState(psWScreen, id) & WBUT_CLICKLOCK)
				{
					widgSetButtonState(psWScreen, id, 0);
					SetSecondaryState(OrderButtons[OrdIndex].Order, 0);
				}
				else
				{
					widgSetButtonState(psWScreen, id, WBUT_CLICKLOCK);
					SetSecondaryState(OrderButtons[OrdIndex].Order,
					                  OrderButtons[OrdIndex].States[StateIndex] & OrderButtons[OrdIndex].StateMask);
				}

			}
			break;

		case ORD_BTYPE_BOOLEAN_DEPEND:
			// Toggle the state of this button.
			if (id == BaseID)
			{
				if (widgGetButtonState(psWScreen, id) & WBUT_CLICKLOCK)
				{
					widgSetButtonState(psWScreen, id, 0);
					// Disable the dependant button.
					widgSetButtonState(psWScreen, id + 1, WBUT_DISABLE);
				}
				else
				{
					widgSetButtonState(psWScreen, id, WBUT_CLICKLOCK);
					// Enable the dependant button.
					widgSetButtonState(psWScreen, id + 1, 0);
				}
			} if ((id > BaseID) && (id < BaseID + OrderButtons[OrdIndex].AcNumButs))
			{
				// If the previous button is down ( armed )..
				if (widgGetButtonState(psWScreen, id - 1) & WBUT_CLICKLOCK)
				{
					// Toggle the state of this button.
					if (widgGetButtonState(psWScreen, id) & WBUT_CLICKLOCK)
					{
						widgSetButtonState(psWScreen, id, 0);
						SetSecondaryState(OrderButtons[OrdIndex].Order, 0);
					}
					else
					{
						StateIndex = id - BaseID;

						widgSetButtonState(psWScreen, id, WBUT_CLICKLOCK);
						SetSecondaryState(OrderButtons[OrdIndex].Order,
						                  OrderButtons[OrdIndex].States[StateIndex] & OrderButtons[OrdIndex].StateMask);
					}
				}
			}
			break;
		case ORD_BTYPE_BOOLEAN_COMBINE:
			if ((id >= BaseID) && (id < BaseID + OrderButtons[OrdIndex].AcNumButs))
			{
				// Toggle the state of this button.
				if (widgGetButtonState(psWScreen, id) & WBUT_CLICKLOCK)
				{
					widgSetButtonState(psWScreen, id, 0);
				}
				else
				{
					widgSetButtonState(psWScreen, id, WBUT_CLICKLOCK);
				}

				// read the state of all the buttons to get the final state
				CombineState = 0;
				for (StateIndex = 0; StateIndex < OrderButtons[OrdIndex].AcNumButs; StateIndex++)
				{
					if (widgGetButtonState(psWScreen, BaseID + StateIndex) & WBUT_CLICKLOCK)
					{
						CombineState |= OrderButtons[OrdIndex].States[StateIndex];
					}
				}

				// set the final state
				SetSecondaryState(OrderButtons[OrdIndex].Order,
				                  CombineState & OrderButtons[OrdIndex].StateMask);
			}
			break;
		}
	}
}


// check whether the order list has changed
static bool CheckObjectOrderList(void)
{
	std::vector<AVORDER> NewAvailableOrders;

	if (psSelectedFactory != NULL)
	{
		NewAvailableOrders = buildStructureOrderList(psSelectedFactory);
	}
	else
	{
		NewAvailableOrders = buildDroidOrderList();
	}

	// now check that all the orders are the same
	return NewAvailableOrders == AvailableOrders;
}

static bool intRefreshOrderButtons(void)
{
	SECONDARY_STATE State;
	UWORD OrdIndex;
	UDWORD	id;

	for (unsigned j = 0; j < AvailableOrders.size() && j < MAX_DISPLAYABLE_ORDERS; ++j)
	{
		OrdIndex = AvailableOrders[j].OrderIndex;

		// Get current order state.
		State = GetSecondaryStates(OrderButtons[OrdIndex].Order);

		// Set actual number of buttons.
		OrderButtons[OrdIndex].AcNumButs = OrderButtons[OrdIndex].NumButs;

		id = OrderButtons[OrdIndex].ButBaseID;
		for (unsigned i = 0; i < OrderButtons[OrdIndex].AcNumButs; ++i)
		{
			// Set the state for the button.
			switch (OrderButtons[OrdIndex].ButType)
			{
			case ORD_BTYPE_RADIO:
			case ORD_BTYPE_BOOLEAN:
			 {
				bool selected = (State & OrderButtons[OrdIndex].StateMask) == (UDWORD)OrderButtons[OrdIndex].States[i];
				widgSetButtonState(psWScreen, id, selected? WBUT_CLICKLOCK : 0);
				break;
			 }
			case ORD_BTYPE_BOOLEAN_DEPEND:
			 {
				bool selected = (State & OrderButtons[OrdIndex].StateMask) == (UDWORD)OrderButtons[OrdIndex].States[i];
				bool first = i == 0;
				widgSetButtonState(psWScreen, id, selected? WBUT_CLICKLOCK : first? 0 : WBUT_DISABLE);
				break;
			 }
			case ORD_BTYPE_BOOLEAN_COMBINE:
			 {
				bool selected = State & (UDWORD)OrderButtons[OrdIndex].States[i];
				widgSetButtonState(psWScreen, id, selected? WBUT_CLICKLOCK : 0);
				break;
			 }
			}

			id ++;
		}
	}

	return true;
}


// Call to refresh the Order screen, ie when a droids boards it.
//
bool intRefreshOrder(void)
{
	// Is the Order screen up?
	if ((intMode == INT_ORDER) &&
	    (widgGetFromID(psWScreen, IDORDER_FORM) != NULL))
	{
		bool Ret;

		SelectedDroids.clear();

		// check for factory selected first
		if (!psSelectedFactory)
		{
			if (!BuildSelectedDroidList())
			{
				// no units selected - quit
				intRemoveOrder();
				return true;
			}
		}

		// if the orders havn't changed, just reset the state
		if (CheckObjectOrderList())
		{
			Ret = intRefreshOrderButtons();
		}
		else
		{
			// Refresh it by re-adding it.
			Ret = intAddOrder(NULL);
			if (Ret == false)
			{
				intMode = INT_NORMAL;
			}
		}
		return Ret;
	}

	return true;
}


// Remove the droids order screen with animation.
//
void intRemoveOrder(void)
{
	widgDelete(psWScreen, IDORDER_CLOSE);

	// Start the window close animation.
	IntFormAnimated *form = (IntFormAnimated *)widgGetFromID(psWScreen, IDORDER_FORM);
	if (form != nullptr)
	{
		form->closeAnimateDelete();
		OrderUp = false;
		SelectedDroids.clear();
		psSelectedFactory = NULL;
	}
}


// Remove the droids order screen without animation.
//
void intRemoveOrderNoAnim(void)
{
	widgDelete(psWScreen, IDORDER_CLOSE);
	widgDelete(psWScreen, IDORDER_FORM);
	OrderUp = false;
	SelectedDroids.clear();
	psSelectedFactory = NULL;
}

//new function added to bring up the RMB order form for Factories as well as droids
void intAddFactoryOrder(STRUCTURE *psStructure)
{
	if (!OrderUp)
	{
		intResetScreen(false);
		intAddOrder((BASE_OBJECT *)psStructure);
		intMode = INT_ORDER;
	}
	else
	{
		intAddOrder((BASE_OBJECT *)psStructure);
	}
}
