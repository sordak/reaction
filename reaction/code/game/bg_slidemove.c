//-----------------------------------------------------------------------------
//
// $Id$
//
//-----------------------------------------------------------------------------
//
// $Log$
// Revision 1.18  2005/02/15 16:33:39  makro
// Tons of updates (entity tree attachment system, UI vectors)
//
// Revision 1.17  2002/07/16 04:27:19  niceass
// physics back to 2.0
//
// Revision 1.16  2002/07/08 04:30:19  niceass
// changes to gravity
//
// Revision 1.15  2002/06/16 20:06:14  jbravo
// Reindented all the source files with "indent -kr -ut -i8 -l120 -lc120 -sob -bad -bap"
//
// Revision 1.14  2002/05/18 14:52:16  makro
// Bot stuff. Other stuff. Just... stuff :p
//
// Revision 1.13  2002/05/07 05:06:56  niceass
// physics tweaks
//
// Revision 1.12  2002/04/26 05:29:23  niceass
// PHYSICS, Take 2
//
// Revision 1.11  2002/03/31 03:31:24  jbravo
// Compiler warning cleanups
//
// Revision 1.10  2002/02/04 02:05:28  jbravo
// removed an extraneous return value from a void routine
//
// Revision 1.9  2002/02/04 00:31:40  niceass
// oops =D
//
// Revision 1.8  2002/02/04 00:30:35  niceass
// New physics
//
// Revision 1.7  2002/01/11 19:48:30  jbravo
// Formatted the source in non DOS format.
//
// Revision 1.6  2001/12/31 16:28:42  jbravo
// I made a Booboo with the Log tag.
//
//
//-----------------------------------------------------------------------------
// Copyright (C) 1999-2000 Id Software, Inc.
//
// bg_slidemove.c -- part of bg_pmove functionality

#include "q_shared.h"
#include "bg_public.h"
#include "bg_local.h"

/*

input: origin, velocity, bounds, groundPlane, trace function

output: origin, velocity, impacts, stairup boolean

*/

/*
==================
PM_SlideMove

Returns qtrue if the velocity was clipped in some way
==================
*/
#define	MAX_CLIP_PLANES	5
qboolean PM_SlideMove(qboolean gravity)
{
	int bumpcount, numbumps;
	vec3_t dir;
	float d;
	int numplanes;
	vec3_t planes[MAX_CLIP_PLANES];
	vec3_t primal_velocity;
	vec3_t clipVelocity;
	int i, j, k;
	trace_t trace;
	vec3_t end;
	float time_left;
	float into;
	vec3_t endVelocity;
	vec3_t endClipVelocity;

	numbumps = 4;

	VectorCopy(pm->ps->velocity, primal_velocity);

	if (gravity) {
		VectorCopy(pm->ps->velocity, endVelocity);
		endVelocity[2] -= pm->ps->gravity * pml.frametime;
		pm->ps->velocity[2] = (pm->ps->velocity[2] + endVelocity[2]) * 0.5;
		primal_velocity[2] = endVelocity[2];

		/*if ( pml.groundPlane ) {
		   // slide along the ground plane
		   PM_ClipVelocity (pm->ps->velocity, pml.groundTrace.plane.normal, 
		   pm->ps->velocity, OVERCLIP );
		   } */
	}

	time_left = pml.frametime;

	// never turn against the ground plane
	if (pml.groundPlane) {
		numplanes = 1;
		VectorCopy(pml.groundTrace.plane.normal, planes[0]);
	} else {
		numplanes = 0;
	}

	// never turn against original velocity
	VectorNormalize2(pm->ps->velocity, planes[numplanes]);
	numplanes++;

	for (bumpcount = 0; bumpcount < numbumps; bumpcount++) {

		// calculate position we are trying to move to
		VectorMA(pm->ps->origin, time_left, pm->ps->velocity, end);

		// see if we can make it there
		pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, end, pm->ps->clientNum, pm->tracemask);

		if (trace.allsolid) {
			// entity is completely trapped in another solid
			pm->ps->velocity[2] = 0;	// don't build up falling damage, but allow sideways acceleration
			return qtrue;
		}

		if (trace.fraction > 0) {
			// actually covered some distance
			VectorCopy(trace.endpos, pm->ps->origin);
		}

		if (trace.fraction == 1) {
			break;	// moved the entire distance
		}
		// save entity for contact
		PM_AddTouchEnt(trace.entityNum);

		time_left -= time_left * trace.fraction;

		if (numplanes >= MAX_CLIP_PLANES) {
			// this shouldn't really happen
			// Makro - yet it does ! - added ifdef
#ifdef DEBUG
			Com_Printf("DEBUG: MAX_CLIP_PLANES\n");
#endif
			VectorClear(pm->ps->velocity);
			return qtrue;
		}
		//
		// if this is the same plane we hit before, nudge velocity
		// out along it, which fixes some epsilon issues with
		// non-axial planes
		//
		for (i = 0; i < numplanes; i++) {
			if (DotProduct(trace.plane.normal, planes[i]) > 0.99) {
				VectorAdd(trace.plane.normal, pm->ps->velocity, pm->ps->velocity);
				break;
			}
		}
		if (i < numplanes) {
			continue;
		}
		VectorCopy(trace.plane.normal, planes[numplanes]);
		numplanes++;

		//
		// modify velocity so it parallels all of the clip planes
		//

		// find a plane that it enters
		for (i = 0; i < numplanes; i++) {
			into = DotProduct(pm->ps->velocity, planes[i]);
			if (into >= 0.1) {
				continue;	// move doesn't interact with the plane
			}
			// see how hard we are hitting things
			if (-into > pml.impactSpeed) {
				pml.impactSpeed = -into;
			}
			// slide along the plane
			PM_ClipVelocity(pm->ps->velocity, planes[i], clipVelocity, OVERCLIP);

			// slide along the plane
			PM_ClipVelocity(endVelocity, planes[i], endClipVelocity, OVERCLIP);

			// see if there is a second plane that the new move enters
			for (j = 0; j < numplanes; j++) {
				if (j == i) {
					continue;
				}
				if (DotProduct(clipVelocity, planes[j]) >= 0.1) {
					continue;	// move doesn't interact with the plane
				}
				// try clipping the move to the plane
				PM_ClipVelocity(clipVelocity, planes[j], clipVelocity, OVERCLIP);
				PM_ClipVelocity(endClipVelocity, planes[j], endClipVelocity, OVERCLIP);

				// see if it goes back into the first clip plane
				if (DotProduct(clipVelocity, planes[i]) >= 0) {
					continue;
				}
				// slide the original velocity along the crease
				CrossProduct(planes[i], planes[j], dir);
				VectorNormalize(dir);
				d = DotProduct(dir, pm->ps->velocity);
				VectorScale(dir, d, clipVelocity);

				CrossProduct(planes[i], planes[j], dir);
				VectorNormalize(dir);
				d = DotProduct(dir, endVelocity);
				VectorScale(dir, d, endClipVelocity);

				// see if there is a third plane the the new move enters
				for (k = 0; k < numplanes; k++) {
					if (k == i || k == j) {
						continue;
					}
					if (DotProduct(clipVelocity, planes[k]) >= 0.1) {
						continue;	// move doesn't interact with the plane
					}
					// stop dead at a tripple plane interaction
					VectorClear(pm->ps->velocity);
					return qtrue;
				}
			}

			// if we have fixed all interactions, try another move
			VectorCopy(clipVelocity, pm->ps->velocity);
			VectorCopy(endClipVelocity, endVelocity);
			break;
		}
	}

	if (gravity) {
		VectorCopy(endVelocity, pm->ps->velocity);
	}
	// don't change velocity if in a timer (FIXME: is this correct?)
	if (pm->ps->pm_time) {
		VectorCopy(primal_velocity, pm->ps->velocity);
	}

	return (bumpcount != 0);
}

/*
==================
PM_StepSlideMove

==================
*/
void PM_StepSlideMove(qboolean gravity)
{
	vec3_t start_o, start_v;
	vec3_t down_o, down_v;
	trace_t trace;
	vec3_t up, down;
	vec3_t tmp;

	VectorCopy(pm->ps->origin, start_o);
	VectorCopy(pm->ps->velocity, start_v);

	if (PM_SlideMove(gravity) == 0)
		return;		// we got exactly where we wanted to go first try       

	VectorCopy(pm->ps->origin, down_o);
	VectorCopy(pm->ps->velocity, down_v);

	VectorCopy(start_o, up);
	up[2] += STEPSIZE;

	// test the player position if they were a stepheight higher
	pm->trace(&trace, up, pm->mins, pm->maxs, up, pm->ps->clientNum, pm->tracemask);
	if (trace.allsolid) {
		return;		// can't step up
	}
	// try slidemove from this position
	VectorCopy(up, pm->ps->origin);
	VectorCopy(start_v, pm->ps->velocity);

	PM_SlideMove(gravity);

	// push down the final amount
	VectorCopy(pm->ps->origin, down);
	down[2] -= STEPSIZE;
	pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, down, pm->ps->clientNum, pm->tracemask);
	if (!trace.allsolid)
		VectorCopy(trace.endpos, pm->ps->origin);

	//Makro - possible fix for the "player stuck on stairs" bug ?
	//if (trace.plane.normal[0] || trace.plane.normal[1] || trace.plane.normal[2])
	//{
		if (trace.plane.normal[2] < 0.7f) {
			VectorCopy(down_o, pm->ps->origin);
			VectorCopy(down_v, pm->ps->velocity);
			return;
		}
	//}

	pm->ps->velocity[2] = down_v[2];

	VectorCopy(pm->ps->origin, tmp);

	// NiceAss: nudge it up a little.
	tmp[2] += 0.1f;
	start_o[2] += 0.1f;

	// if the down trace can trace back to the original position directly, don't step
	pm->trace(&trace, tmp, pm->mins, pm->maxs, start_o, pm->ps->clientNum, pm->tracemask);
	if (trace.fraction != 1.0) {
		// use the step move
		float delta;

		delta = pm->ps->origin[2] - start_o[2];
		if (delta > 2) {
			if (delta < 7) {
				PM_AddEvent(EV_STEP_4);
			} else if (delta < 11) {
				PM_AddEvent(EV_STEP_8);
			} else if (delta < 15) {
				PM_AddEvent(EV_STEP_12);
			} else {
				PM_AddEvent(EV_STEP_16);
			}
		}
		if (pm->debugLevel) {
			Com_Printf("%i:stepped\n", c_pmove);
		}
	}
}