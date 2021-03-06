//-----------------------------------------------------------------------------
//
// $Id$
//
//-----------------------------------------------------------------------------
//
// $Log$
// Revision 1.5  2002/06/16 20:06:13  jbravo
// Reindented all the source files with "indent -kr -ut -i8 -l120 -lc120 -sob -bad -bap"
//
// Revision 1.4  2002/01/11 19:48:29  jbravo
// Formatted the source in non DOS format.
//
// Revision 1.3  2001/12/31 16:28:41  jbravo
// I made a Booboo with the Log tag.
//
//
//-----------------------------------------------------------------------------
// cg_rankings.c -- client global rankings system

#include "cg_local.h"

/*
================
CG_RankRunFrame
================
*/

void CG_RankRunFrame(void)
{
	grank_status_t status;

	if (!cgs.localServer)
		trap_CL_RankPoll();

	status = trap_CL_RankUserStatus();

	if (cgs.client_status != status) {
		// GRank status changed

		// inform UI of current status
		trap_Cvar_Set("client_status", va("%i", (int) (status)));

		// show rankings status dialog if error
		trap_CL_RankShowStatus((int) status);

		cgs.client_status = status;
	}
	return;
}
