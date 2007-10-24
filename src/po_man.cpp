
//**************************************************************************
//**
//** PO_MAN.C : Heretic 2 : Raven Software, Corp.
//**
//** $RCSfile: po_man.c,v $
//** $Revision: 1.22 $
//** $Date: 95/09/28 18:20:56 $
//** $Author: cjr $
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "doomdef.h"
#include "p_local.h"
#include "r_local.h"
#include "i_system.h"
#include "w_wad.h"
#include "m_swap.h"
#include "m_bbox.h"
#include "tables.h"
#include "s_sndseq.h"
#include "a_sharedglobal.h"
#include "r_main.h"
// [BC] New #includes.
#include "cl_demo.h"
#include "network.h"
#include "sv_commands.h"

// MACROS ------------------------------------------------------------------

#define PO_MAXPOLYSEGS 64

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

bool PO_RotatePolyobj (int num, angle_t angle);
void PO_Init (void);

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

/*static*/ polyobj_t *GetPolyobj (int polyNum);
// [BC]
polyobj_t *GetPolyobjByIndex( ULONG ulPolyIdx );
static int GetPolyobjMirror (int poly);
static void UpdateSegBBox (seg_t *seg);
static void RotatePt (int an, fixed_t *x, fixed_t *y, fixed_t startSpotX,
	fixed_t startSpotY);
static void UnLinkPolyobj (polyobj_t *po);
static void LinkPolyobj (polyobj_t *po);
static bool CheckMobjBlocking (seg_t *seg, polyobj_t *po);
static void InitBlockMap (void);
static void IterFindPolySegs (vertex_t *v1, vertex_t *v2, seg_t **segList);
static void SpawnPolyobj (int index, int tag, int type);
static void TranslateToStartSpot (int tag, int originX, int originY);
static void DoMovePolyobj (polyobj_t *po, int x, int y);
static void InitSegLists ();
static void KillSegLists ();

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

extern seg_t *segs;

// PUBLIC DATA DEFINITIONS -------------------------------------------------

polyblock_t **PolyBlockMap;
polyobj_t *polyobjs; // list of all poly-objects on the level
int po_NumPolyobjs;
polyspawns_t *polyspawns; // [RH] Let P_SpawnMapThings() find our thingies for us

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static int PolySegCount;

static SDWORD *SegListHead;	// contains numvertexes elements
static TArray<SDWORD> KnownPolySegs;

// CODE --------------------------------------------------------------------

IMPLEMENT_CLASS (DPolyAction)
IMPLEMENT_CLASS (DRotatePoly)
IMPLEMENT_CLASS (DMovePoly)
IMPLEMENT_CLASS (DPolyDoor)

DPolyAction::DPolyAction ()
{
}

void DPolyAction::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	arc << m_PolyObj << m_Speed << m_Dist;
}

DPolyAction::DPolyAction (int polyNum)
{
	m_PolyObj = polyNum;
	m_Speed = 0;
	m_Dist = 0;
	SetInterpolation ();
}

DPolyAction::~DPolyAction ()
{
	polyobj_t *poly = GetPolyobj (m_PolyObj);

	if (poly->specialdata == NULL || poly->specialdata == this)
	{
		poly->specialdata = NULL;
		StopInterpolation ();
	}
}

void DPolyAction::SetInterpolation ()
{
	polyobj_t *poly = GetPolyobj (m_PolyObj);
	for (int i = 0; i < poly->numsegs; ++i)
	{
		setinterpolation (INTERP_Vertex, poly->segs[i]->v1);
		setinterpolation (INTERP_Vertex, poly->segs[i]->v2);
	}
}

void DPolyAction::StopInterpolation ()
{
	polyobj_t *poly = GetPolyobj (m_PolyObj);
	for (int i = 0; i < poly->numsegs; ++i)
	{
		stopinterpolation (INTERP_Vertex, poly->segs[i]->v1);
		stopinterpolation (INTERP_Vertex, poly->segs[i]->v2);
	}
}

LONG DPolyAction::GetSpeed ()
{
	return ( m_Speed );
}

void DPolyAction::SetSpeed (LONG lSpeed)
{
	m_Speed = lSpeed;
}

LONG DPolyAction::GetDist ()
{
	return ( m_Dist );
}

void DPolyAction::SetDist (LONG lDist)
{
	m_Dist = lDist;
}

LONG DPolyAction::GetPolyObj ()
{
	return ( m_PolyObj );
}

DRotatePoly::DRotatePoly ()
{
}

DRotatePoly::DRotatePoly (int polyNum)
	: Super (polyNum)
{
}

// [BC]
void DRotatePoly::UpdateToClient( ULONG ulClient )
{
	SERVERCOMMANDS_DoRotatePoly( m_Speed, m_PolyObj, ulClient, SVCF_ONLYTHISCLIENT );
}

DMovePoly::DMovePoly ()
{
}

void DMovePoly::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	arc << m_Angle << m_xSpeed << m_ySpeed;
}

DMovePoly::DMovePoly (int polyNum)
	: Super (polyNum)
{
	m_Angle = 0;
	m_xSpeed = 0;
	m_ySpeed = 0;
}

void DMovePoly::UpdateToClient( ULONG ulClient )
{
	SERVERCOMMANDS_DoMovePoly( m_xSpeed, m_ySpeed, m_PolyObj, ulClient, SVCF_ONLYTHISCLIENT );
}

LONG DMovePoly::GetXSpeed ()
{
	return ( m_xSpeed );
}

void DMovePoly::SetXSpeed (LONG lSpeed)
{
	m_xSpeed = lSpeed;
}

LONG DMovePoly::GetYSpeed ()
{
	return ( m_ySpeed );
}

void DMovePoly::SetYSpeed (LONG lSpeed)
{
	m_ySpeed = lSpeed;
}

LONG DMovePoly::GetAngle ()
{
	return ( m_Angle );
}

void DMovePoly::SetAngle (LONG lAngle)
{
	m_Angle = lAngle;
}

DPolyDoor::DPolyDoor ()
{
}

void DPolyDoor::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	arc << m_Direction << m_TotalDist << m_Tics << m_WaitTics << m_Type << m_Close;
}

DPolyDoor::DPolyDoor (int polyNum, podoortype_t type)
	: Super (polyNum), m_Type (type)
{
	m_Direction = 0;
	m_TotalDist = 0;
	m_Tics = 0;
	m_WaitTics = 0;
	m_Close = false;
}

void DPolyDoor::UpdateToClient( ULONG ulClient )
{
	SERVERCOMMANDS_DoPolyDoor( m_Type, m_xSpeed, m_ySpeed, m_Speed, m_PolyObj, ulClient, SVCF_ONLYTHISCLIENT );
}

LONG DPolyDoor::GetDirection ()
{
	return ( m_Direction );
}

void DPolyDoor::SetDirection (LONG lDirection)
{
	m_Direction = lDirection;
}

LONG DPolyDoor::GetTotalDist ()
{
	return ( m_TotalDist );
}

void DPolyDoor::SetTotalDist (LONG lDist)
{
	m_TotalDist = lDist;
}

bool DPolyDoor::GetClose ()
{
	return ( m_Close );
}

void DPolyDoor::SetClose (bool bClose)
{
	m_Close = bClose;
}


// ===== Polyobj Event Code =====

//==========================================================================
//
// T_RotatePoly
//
//==========================================================================

void DRotatePoly::Tick ()
{
	// [BC] For clients, just tick them and get out.
	if (( NETWORK_GetState( ) == NETSTATE_CLIENT ) ||
		( CLIENTDEMO_IsPlaying( )))
	{
		PO_RotatePolyobj( m_PolyObj, m_Speed );
		return;
	}

	if (PO_RotatePolyobj (m_PolyObj, m_Speed))
	{
		unsigned int absSpeed = abs (m_Speed);

		if (m_Dist == -1)
		{ // perpetual polyobj
			return;
		}
		m_Dist -= absSpeed;
		if (m_Dist == 0)
		{
			polyobj_t *poly = GetPolyobj (m_PolyObj);
			if (poly->specialdata == this)
			{
				poly->specialdata = NULL;
			}

			// [BC] Now that our destination has been reached, tell clients.
			if ( NETWORK_GetState( ) == NETSTATE_SERVER )
				SERVERCOMMANDS_SetPolyobjRotation( m_PolyObj, poly->angle );

			// [BC] Tell clients to stop the sound sequence, and destroy the rotate poly.
			if ( NETWORK_GetState( ) == NETSTATE_SERVER )
			{
				SERVERCOMMANDS_PlayPolyobjSound( m_PolyObj, POLYSOUND_STOPSEQUENCE );
				SERVERCOMMANDS_DestroyRotatePoly( m_PolyObj );
			}

			SN_StopSequence (poly);
			Destroy ();
		}
		else if ((unsigned int)m_Dist < absSpeed)
		{
			m_Speed = m_Dist * (m_Speed < 0 ? -1 : 1);
		}
	}
}

//==========================================================================
//
// EV_RotatePoly
//
//==========================================================================


bool EV_RotatePoly (line_t *line, int polyNum, int speed, int byteAngle,
					int direction, bool overRide)
{
	int mirror;
	DRotatePoly *pe;
	polyobj_t *poly;

	if ( (poly = GetPolyobj(polyNum)) )
	{
		if (poly->specialdata && !overRide)
		{ // poly is already moving
			return false;
		}
	}
	else
	{
		I_Error("EV_RotatePoly: Invalid polyobj num: %d\n", polyNum);
	}
	pe = new DRotatePoly (polyNum);

	if (byteAngle)
	{
		if (byteAngle == 255)
		{
			pe->m_Dist = ~0;
		}
		else
		{
			pe->m_Dist = byteAngle*(ANGLE_90/64); // Angle
		}
	}
	else
	{
		pe->m_Dist = ANGLE_MAX-1;
	}
	pe->m_Speed = (speed*direction*(ANGLE_90/64))>>3;
	poly->specialdata = pe;
	SN_StartSequence (poly, poly->seqType, SEQ_DOOR, 0);
	
	// [BC] If we're the server, tell clients to create the rotate poly.
	if ( NETWORK_GetState( ) == NETSTATE_SERVER )
		SERVERCOMMANDS_DoRotatePoly( pe->m_Speed, pe->m_PolyObj );

	while ( (mirror = GetPolyobjMirror( polyNum)) )
	{
		poly = GetPolyobj(mirror);
		if (poly == NULL)
		{
			I_Error ("EV_RotatePoly: Invalid polyobj num: %d\n", polyNum);
		}
		if (poly && poly->specialdata && !overRide)
		{ // mirroring poly is already in motion
			break;
		}
		pe = new DRotatePoly (mirror);
		poly->specialdata = pe;
		if (byteAngle)
		{
			if (byteAngle == 255)
			{
				pe->m_Dist = ~0;
			}
			else
			{
				pe->m_Dist = byteAngle*(ANGLE_90/64); // Angle
			}
		}
		else
		{
			pe->m_Dist = ANGLE_MAX-1;
		}
		direction = -direction;
		pe->m_Speed = (speed*direction*(ANGLE_90/64))>>3;
		polyNum = mirror;
		SN_StartSequence (poly, poly->seqType, SEQ_DOOR, 0);

		// [BC] If we're the server, tell clients to create the rotate poly.
		if ( NETWORK_GetState( ) == NETSTATE_SERVER )
			SERVERCOMMANDS_DoRotatePoly( pe->m_Speed, pe->m_PolyObj );
	}
	return true;
}

//==========================================================================
//
// T_MovePoly
//
//==========================================================================

void DMovePoly::Tick ()
{
	polyobj_t *poly;

	// [BC] For clients, just tick them and get out.
	if (( NETWORK_GetState( ) == NETSTATE_CLIENT ) ||
		( CLIENTDEMO_IsPlaying( )))
	{
		PO_MovePolyobj( m_PolyObj, m_xSpeed, m_ySpeed );
		return;
	}

	if (PO_MovePolyobj (m_PolyObj, m_xSpeed, m_ySpeed))
	{
		int absSpeed = abs (m_Speed);
		m_Dist -= absSpeed;
		if (m_Dist <= 0)
		{
			poly = GetPolyobj (m_PolyObj);
			if (poly->specialdata == this)
			{
				poly->specialdata = NULL;
			}

			// [BC] Now that our destination has been reached, tell clients.
			if ( NETWORK_GetState( ) == NETSTATE_SERVER )
				SERVERCOMMANDS_SetPolyobjPosition( m_PolyObj, poly->startSpot[0], poly->startSpot[1] );

			// [BC] Tell clients to stop the sound sequence, and destroy the move poly.
			if ( NETWORK_GetState( ) == NETSTATE_SERVER )
			{
				SERVERCOMMANDS_PlayPolyobjSound( m_PolyObj, POLYSOUND_STOPSEQUENCE );
				SERVERCOMMANDS_DestroyMovePoly( m_PolyObj );
			}

			SN_StopSequence (poly);
			Destroy ();
		}
		else if (m_Dist < absSpeed)
		{
			m_Speed = m_Dist * (m_Speed < 0 ? -1 : 1);
			m_xSpeed = FixedMul (m_Speed, finecosine[m_Angle]);
			m_ySpeed = FixedMul (m_Speed, finesine[m_Angle]);
		}
	}
}

//==========================================================================
//
// EV_MovePoly
//
//==========================================================================

bool EV_MovePoly (line_t *line, int polyNum, int speed, angle_t angle,
				  fixed_t dist, bool overRide)
{
	int mirror;
	DMovePoly *pe;
	polyobj_t *poly;
	angle_t an;

	if ( (poly = GetPolyobj(polyNum)) )
	{
		if (poly->specialdata && !overRide)
		{ // poly is already moving
			return false;
		}
	}
	else
	{
		I_Error("EV_MovePoly: Invalid polyobj num: %d\n", polyNum);
	}
	pe = new DMovePoly (polyNum);
	pe->m_Dist = dist; // Distance
	pe->m_Speed = speed;
	poly->specialdata = pe;

	an = angle;

	pe->m_Angle = an>>ANGLETOFINESHIFT;
	pe->m_xSpeed = FixedMul (pe->m_Speed, finecosine[pe->m_Angle]);
	pe->m_ySpeed = FixedMul (pe->m_Speed, finesine[pe->m_Angle]);
	SN_StartSequence (poly, poly->seqType, SEQ_DOOR, 0);

	// [BC] If we're the server, tell clients to create the move poly.
	if ( NETWORK_GetState( ) == NETSTATE_SERVER )
		SERVERCOMMANDS_DoMovePoly( pe->m_xSpeed, pe->m_ySpeed, pe->m_PolyObj );

	// Do not interpolate very fast moving polyobjects. The minimum tic count is
	// 3 instead of 2, because the moving crate effect in Massmouth 2, Hostitality
	// that this fixes isn't quite fast enough to move the crate back to its start
	// in just 1 tic.
	if (dist/speed <= 2)
	{
		pe->StopInterpolation ();
	}

	while ( (mirror = GetPolyobjMirror(polyNum)) )
	{
		poly = GetPolyobj(mirror);
		if (poly && poly->specialdata && !overRide)
		{ // mirroring poly is already in motion
			break;
		}
		pe = new DMovePoly (mirror);
		poly->specialdata = pe;
		pe->m_Dist = dist; // Distance
		pe->m_Speed = speed;
		an = an+ANGLE_180; // reverse the angle
		pe->m_Angle = an>>ANGLETOFINESHIFT;
		pe->m_xSpeed = FixedMul (pe->m_Speed, finecosine[pe->m_Angle]);
		pe->m_ySpeed = FixedMul (pe->m_Speed, finesine[pe->m_Angle]);
		polyNum = mirror;
		SN_StartSequence (poly, poly->seqType, SEQ_DOOR, 0);
		if (dist/speed <= 2)
		{
			pe->StopInterpolation ();
		}

		// [BC] If we're the server, tell clients to create the move poly.
		if ( NETWORK_GetState( ) == NETSTATE_SERVER )
			SERVERCOMMANDS_DoMovePoly( pe->m_xSpeed, pe->m_ySpeed, pe->m_PolyObj );
	}
	return true;
}

//==========================================================================
//
// T_PolyDoor
//
//==========================================================================

void DPolyDoor::Tick ()
{
	int absSpeed;
	polyobj_t *poly;
	bool	bBlocked;

	// [BC] For clients, just tick them and get out.
	if (( NETWORK_GetState( ) == NETSTATE_CLIENT ) ||
		( CLIENTDEMO_IsPlaying( )))
	{
		switch ( m_Type )
		{
		case PODOOR_SLIDE:

			PO_MovePolyobj( m_PolyObj, m_xSpeed, m_ySpeed );
			break;
		case PODOOR_SWING:

			PO_RotatePolyobj( m_PolyObj, m_Speed );
			break;
		default:

			break;
		}

		return;
	}

	// The door is currently paused after opening all the way.
	if (m_Tics)
	{
		// Pause time is over! Time to set the door back in motion.
		if (!--m_Tics)
		{
			poly = GetPolyobj (m_PolyObj);

			// [BC] Tell clients to start the sound sequence, and to set the polyobject
			// back in motion
			if ( NETWORK_GetState( ) == NETSTATE_SERVER )
			{
				SERVERCOMMANDS_PlayPolyobjSound( m_PolyObj, POLYSOUND_SEQ_DOOR );
				if ( m_Type == PODOOR_SLIDE )
				{
					SERVERCOMMANDS_SetPolyDoorSpeedPosition( m_PolyObj,
																m_xSpeed,
																m_ySpeed,
																poly->startSpot[0],
																poly->startSpot[1] );
				}
			}

			SN_StartSequence (poly, poly->seqType, SEQ_DOOR, m_Close);
		}
		return;
	}

	switch (m_Type)
	{
	case PODOOR_SLIDE:

		poly = GetPolyobj( m_PolyObj );
		bBlocked = ( PO_MovePolyobj (m_PolyObj, m_xSpeed, m_ySpeed) == false );

		// [BC] If the polyobject had NOT been blocked last frame, but is now, tell clients
		// to set stop the polyobject.
		if (( NETWORK_GetState( ) == NETSTATE_SERVER ) && ( bBlocked ) && ( poly->bBlocked == false ))
		{
			poly->bBlocked = true;
			SERVERCOMMANDS_SetPolyDoorSpeedPosition( m_PolyObj,
														m_xSpeed,
														m_ySpeed,
														poly->startSpot[0],
														poly->startSpot[1] );
		}

		if (m_Dist <= 0 || ( bBlocked == false ))
		{
			// [BC] If the polyobject had been blocked last frame, but isn't now, tell clients
			// to set the polyobject back in motion.
			if (( NETWORK_GetState( ) == NETSTATE_SERVER ) && ( poly->bBlocked ))
			{
				poly->bBlocked = false;
				SERVERCOMMANDS_SetPolyDoorSpeedPosition( m_PolyObj,
															m_xSpeed,
															m_ySpeed,
															poly->startSpot[0],
															poly->startSpot[1] );
			}

			absSpeed = abs (m_Speed);
			m_Dist -= absSpeed;
			if (m_Dist <= 0)
			{
				// [BC] Tell clients to stop the sound sequence.
				if ( NETWORK_GetState( ) == NETSTATE_SERVER )
					SERVERCOMMANDS_PlayPolyobjSound( m_PolyObj, POLYSOUND_STOPSEQUENCE );

				poly = GetPolyobj (m_PolyObj);
				SN_StopSequence (poly);
				if (!m_Close)
				{
					m_Dist = m_TotalDist;
					m_Close = true;
					m_Tics = m_WaitTics;
					m_Direction = (ANGLE_MAX>>ANGLETOFINESHIFT)-
						m_Direction;
					m_xSpeed = -m_xSpeed;
					m_ySpeed = -m_ySpeed;					

					// [BC] Tell clients that the door is now paused.
					if ( NETWORK_GetState( ) == NETSTATE_SERVER )
					{
						SERVERCOMMANDS_SetPolyDoorSpeedPosition( m_PolyObj,
																	0,
																	0,
																	poly->startSpot[0],
																	poly->startSpot[1] );
					}
				}
				else
				{
					if (poly->specialdata == this)
					{
						poly->specialdata = NULL;
					}

					// [BC] If we're the server, tell clients to destroy the poly door.
					if ( NETWORK_GetState( ) == NETSTATE_SERVER )
						SERVERCOMMANDS_DestroyPolyDoor( m_PolyObj );

					Destroy ();
				}
			}
		}
		else
		{
			poly = GetPolyobj (m_PolyObj);
			if (poly->crush || !m_Close)
			{ // continue moving if the poly is a crusher, or is opening
				return;
			}
			else
			{ // open back up
				m_Dist = m_TotalDist - m_Dist;
				m_Direction = (ANGLE_MAX>>ANGLETOFINESHIFT)-
					m_Direction;
				m_xSpeed = -m_xSpeed;
				m_ySpeed = -m_ySpeed;
				m_Close = false;
				SN_StartSequence (poly, poly->seqType, SEQ_DOOR, 0);

				// [BC] Tell clients to play the sound sequence.
				if ( NETWORK_GetState( ) == NETSTATE_SERVER )
					SERVERCOMMANDS_PlayPolyobjSound( m_PolyObj, POLYSOUND_SEQ_DOOR );
			}
		}
		break;

	case PODOOR_SWING:
		if (PO_RotatePolyobj (m_PolyObj, m_Speed))
		{
			absSpeed = abs (m_Speed);
			if (m_Dist == -1)
			{ // perpetual polyobj
				return;
			}
			m_Dist -= absSpeed;
			if (m_Dist <= 0)
			{
				poly = GetPolyobj (m_PolyObj);
				SN_StopSequence (poly);

				// [BC] Tell clients to stop the sound sequence.
				if ( NETWORK_GetState( ) == NETSTATE_SERVER )
					SERVERCOMMANDS_PlayPolyobjSound( m_PolyObj, POLYSOUND_STOPSEQUENCE );

				if (!m_Close)
				{
					m_Dist = m_TotalDist;
					m_Close = true;
					m_Tics = m_WaitTics;
					m_Speed = -m_Speed;
				}
				else
				{
					if (poly->specialdata == this)
					{
						poly->specialdata = NULL;
					}

					// [BC] Tell clients to destroy the poly door.
					if ( NETWORK_GetState( ) == NETSTATE_SERVER )
						SERVERCOMMANDS_DestroyPolyDoor( m_PolyObj );

					Destroy ();
				}
			}
		}
		else
		{
			poly = GetPolyobj (m_PolyObj);
			if(poly->crush || !m_Close)
			{ // continue moving if the poly is a crusher, or is opening
				return;
			}
			else
			{ // open back up and rewait
				m_Dist = m_TotalDist - m_Dist;
				m_Speed = -m_Speed;
				m_Close = false;
				SN_StartSequence (poly, poly->seqType, SEQ_DOOR, 0);

				// [BC] Tell clients to play the sound sequence.
				if ( NETWORK_GetState( ) == NETSTATE_SERVER )
					SERVERCOMMANDS_PlayPolyobjSound( m_PolyObj, POLYSOUND_SEQ_DOOR );
			}
		}			
		break;

	default:
		break;
	}
}

//==========================================================================
//
// EV_OpenPolyDoor
//
//==========================================================================

bool EV_OpenPolyDoor (line_t *line, int polyNum, int speed, angle_t angle,
					  int delay, int distance, podoortype_t type)
{
	int mirror;
	DPolyDoor *pd;
	polyobj_t *poly;

	if( (poly = GetPolyobj(polyNum)) )
	{
		if (poly->specialdata)
		{ // poly is already moving
			return false;
		}
	}
	else
	{
		I_Error("EV_OpenPolyDoor: Invalid polyobj num: %d\n", polyNum);
	}

	pd = new DPolyDoor (polyNum, type);

	if (type == PODOOR_SLIDE)
	{
		pd->m_WaitTics = delay;
		pd->m_Speed = speed;
		pd->m_Dist = pd->m_TotalDist = distance; // Distance
		pd->m_Direction = angle >> ANGLETOFINESHIFT;
		pd->m_xSpeed = FixedMul (pd->m_Speed, finecosine[pd->m_Direction]);
		pd->m_ySpeed = FixedMul (pd->m_Speed, finesine[pd->m_Direction]);
		SN_StartSequence (poly, poly->seqType, SEQ_DOOR, 0);
	}
	else if (type == PODOOR_SWING)
	{
		pd->m_WaitTics = delay;
		pd->m_Direction = 1; // ADD:  PODOOR_SWINGL, PODOOR_SWINGR
		pd->m_Speed = (speed*pd->m_Direction*(ANGLE_90/64))>>3;
		pd->m_Dist = pd->m_TotalDist = angle;
		SN_StartSequence (poly, poly->seqType, SEQ_DOOR, 0);
	}

	poly->specialdata = pd;

	// [BC] Tell clients to create the poly door, and play a sound.
	if ( NETWORK_GetState( ) == NETSTATE_SERVER )
	{
		SERVERCOMMANDS_DoPolyDoor( pd->m_Type, pd->m_xSpeed, pd->m_ySpeed, pd->m_Speed, pd->m_PolyObj );
		SERVERCOMMANDS_PlayPolyobjSound( polyNum, POLYSOUND_SEQ_DOOR );
	}

	while ( (mirror = GetPolyobjMirror (polyNum)) )
	{
		poly = GetPolyobj (mirror);
		if (poly && poly->specialdata)
		{ // mirroring poly is already in motion
			break;
		}
		pd = new DPolyDoor (mirror, type);
		poly->specialdata = pd;
		if (type == PODOOR_SLIDE)
		{
			pd->m_WaitTics = delay;
			pd->m_Speed = speed;
			pd->m_Dist = pd->m_TotalDist = distance; // Distance
			pd->m_Direction = (angle + ANGLE_180) >> ANGLETOFINESHIFT; // reverse the angle
			pd->m_xSpeed = FixedMul (pd->m_Speed, finecosine[pd->m_Direction]);
			pd->m_ySpeed = FixedMul (pd->m_Speed, finesine[pd->m_Direction]);
			SN_StartSequence (poly, poly->seqType, SEQ_DOOR, 0);
		}
		else if (type == PODOOR_SWING)
		{
			pd->m_WaitTics = delay;
			pd->m_Direction = -1; // ADD:  same as above
			pd->m_Speed = (speed*pd->m_Direction*(ANGLE_90/64))>>3;
			pd->m_Dist = pd->m_TotalDist = angle;
			SN_StartSequence (poly, poly->seqType, SEQ_DOOR, 0);
		}

		// [BC] Tell clients to create the poly door, and play a sound.
		if ( NETWORK_GetState( ) == NETSTATE_SERVER )
		{
			SERVERCOMMANDS_DoPolyDoor( pd->m_Type, pd->m_xSpeed, pd->m_ySpeed, pd->m_Speed, pd->m_PolyObj );
			SERVERCOMMANDS_PlayPolyobjSound( mirror, POLYSOUND_SEQ_DOOR );
		}

		polyNum = mirror;
	}
	return true;
}
	
// ===== Higher Level Poly Interface code =====

//==========================================================================
//
// GetPolyobj
//
//==========================================================================

/*static*/ polyobj_t *GetPolyobj (int polyNum)
{
	int i;

	for (i = 0; i < po_NumPolyobjs; i++)
	{
		if (polyobjs[i].tag == polyNum)
		{
			return &polyobjs[i];
		}
	}
	return NULL;
}

//==========================================================================
//
// [BC] GetPolyobjByIndex
//
//==========================================================================

polyobj_t *GetPolyobjByIndex( ULONG ulPolyIdx )
{
	if ( ulPolyIdx >= (ULONG)po_NumPolyobjs )
		return ( NULL );

	return ( &polyobjs[ulPolyIdx] );
}

//==========================================================================
//
// GetPolyobjMirror
//
//==========================================================================

static int GetPolyobjMirror(int poly)
{
	int i;

	for (i = 0; i < po_NumPolyobjs; i++)
	{
		if (polyobjs[i].tag == poly)
		{
			return (*polyobjs[i].segs)->linedef->args[1];
		}
	}
	return 0;
}

//==========================================================================
//
// ThrustMobj
//
//==========================================================================

void ThrustMobj (AActor *actor, seg_t *seg, polyobj_t *po)
{
	int thrustAngle;
	int thrustX;
	int thrustY;
	DPolyAction *pe;

	int force;

	if (!(actor->flags&MF_SHOOTABLE) && !actor->player)
	{
		return;
	}
	thrustAngle =
		(R_PointToAngle2 (seg->v1->x, seg->v1->y, seg->v2->x, seg->v2->y)
		 - ANGLE_90) >> ANGLETOFINESHIFT;

	pe = static_cast<DPolyAction *>(po->specialdata);
	if (pe)
	{
		if (pe->IsKindOf (RUNTIME_CLASS (DRotatePoly)))
		{
			force = pe->m_Speed >> 8;
		}
		else
		{
			force = pe->m_Speed >> 3;
		}
		if (force < FRACUNIT)
		{
			force = FRACUNIT;
		}
		else if (force > 4*FRACUNIT)
		{
			force = 4*FRACUNIT;
		}
	}
	else
	{
		force = FRACUNIT;
	}

	thrustX = FixedMul (force, finecosine[thrustAngle]);
	thrustY = FixedMul (force, finesine[thrustAngle]);
	actor->momx += thrustX;
	actor->momy += thrustY;
	if (po->crush &&
		( NETWORK_GetState( ) != NETSTATE_CLIENT ) &&
		( CLIENTDEMO_IsPlaying( ) == false ))
	{
		if (po->bHurtOnTouch || !P_CheckPosition (actor, actor->x + thrustX, actor->y + thrustY))
		{
			P_DamageMobj (actor, NULL, NULL, po->crush, NAME_Crush);
			P_TraceBleed (po->crush, actor);
		}
	}
}

//==========================================================================
//
// UpdateSegBBox
//
//==========================================================================

static void UpdateSegBBox (seg_t *seg)
{
	line_t *line;

	line = seg->linedef;

	if (seg->v1->x < seg->v2->x)
	{
		line->bbox[BOXLEFT] = seg->v1->x;
		line->bbox[BOXRIGHT] = seg->v2->x;
	}
	else
	{
		line->bbox[BOXLEFT] = seg->v2->x;
		line->bbox[BOXRIGHT] = seg->v1->x;
	}
	if (seg->v1->y < seg->v2->y)
	{
		line->bbox[BOXBOTTOM] = seg->v1->y;
		line->bbox[BOXTOP] = seg->v2->y;
	}
	else
	{
		line->bbox[BOXBOTTOM] = seg->v2->y;
		line->bbox[BOXTOP] = seg->v1->y;
	}

	// Update the line's slopetype
	line->dx = line->v2->x - line->v1->x;
	line->dy = line->v2->y - line->v1->y;
	if (!line->dx)
	{
		line->slopetype = ST_VERTICAL;
	}
	else if (!line->dy)
	{
		line->slopetype = ST_HORIZONTAL;
	}
	else
	{
		line->slopetype = ((line->dy ^ line->dx) >= 0) ? ST_POSITIVE : ST_NEGATIVE;
	}
}

//==========================================================================
//
// PO_MovePolyobj
//
//==========================================================================

bool PO_MovePolyobj (int num, int x, int y, bool force)
{
	polyobj_t *po;

	if (!(po = GetPolyobj (num)))
	{
		I_Error ("PO_MovePolyobj: Invalid polyobj number: %d\n", num);
	}

	UnLinkPolyobj (po);
	DoMovePolyobj (po, x, y);

	if (!force)
	{
		seg_t **segList = po->segs;
		bool blocked = false;

		// Don't check if the polyobject was blocked by something in client mode. The server will
		// tell us if something is blocking its path.
		if (( NETWORK_GetState( ) != NETSTATE_CLIENT ) &&
			( CLIENTDEMO_IsPlaying( ) == false ))
		{
			for (int count = po->numsegs; count; count--, segList++)
			{
				if (CheckMobjBlocking(*segList, po))
				{
					blocked = true;
					break;
				}
			}
		}

		if (blocked)
		{
			DoMovePolyobj (po, -x, -y);
			LinkPolyobj(po);
			return false;
		}
	}
	po->startSpot[0] += x;
	po->startSpot[1] += y;
	LinkPolyobj (po);

	// Polyobject has moved.
	po->bMoved = true;

	return true;
}

//==========================================================================
//
// DoMovePolyobj
//
//==========================================================================

void DoMovePolyobj (polyobj_t *po, int x, int y)
{
	int count;
	seg_t **segList;
	seg_t **veryTempSeg;
	vertex_t *prevPts;

	segList = po->segs;
	prevPts = po->prevPts;

	validcount++;
	for (count = po->numsegs; count; count--, segList++, prevPts++)
	{
		line_t *linedef = (*segList)->linedef;
		if (linedef->validcount != validcount)
		{
			linedef->bbox[BOXTOP] += y;
			linedef->bbox[BOXBOTTOM] += y;
			linedef->bbox[BOXLEFT] += x;
			linedef->bbox[BOXRIGHT] += x;
			linedef->validcount = validcount;
		}
		for (veryTempSeg = po->segs; veryTempSeg != segList; veryTempSeg++)
		{
			if ((*veryTempSeg)->v1 == (*segList)->v1)
			{
				break;
			}
		}
		if (veryTempSeg == segList)
		{
			(*segList)->v1->x += x;
			(*segList)->v1->y += y;
		}
		(*prevPts).x += x; // previous points are unique for each seg
		(*prevPts).y += y;
	}
}

//==========================================================================
//
// RotatePt
//
//==========================================================================

static void RotatePt (int an, fixed_t *x, fixed_t *y, fixed_t startSpotX, fixed_t startSpotY)
{
	fixed_t tr_x = *x;
	fixed_t tr_y = *y;

	*x = DMulScale16 (tr_x, finecosine[an], -tr_y, finesine[an])+startSpotX;
	*y = DMulScale16 (tr_x, finesine[an], tr_y, finecosine[an])+startSpotY;
}

//==========================================================================
//
// PO_RotatePolyobj
//
//==========================================================================

bool PO_RotatePolyobj (int num, angle_t angle)
{
	int count;
	seg_t **segList;
	vertex_t *originalPts;
	vertex_t *prevPts;
	int an;
	polyobj_t *po;
	bool blocked;

	if(!(po = GetPolyobj(num)))
	{
		I_Error("PO_RotatePolyobj: Invalid polyobj number: %d\n", num);
	}
	an = (po->angle+angle)>>ANGLETOFINESHIFT;

	UnLinkPolyobj(po);

	segList = po->segs;
	originalPts = po->originalPts;
	prevPts = po->prevPts;

	for(count = po->numsegs; count; count--, segList++, originalPts++,
		prevPts++)
	{
		prevPts->x = (*segList)->v1->x;
		prevPts->y = (*segList)->v1->y;
		(*segList)->v1->x = originalPts->x;
		(*segList)->v1->y = originalPts->y;
		RotatePt (an, &(*segList)->v1->x, &(*segList)->v1->y, po->startSpot[0],
			po->startSpot[1]);
	}
	segList = po->segs;
	blocked = false;
	validcount++;
	for (count = po->numsegs; count; count--, segList++)
	{
		if (CheckMobjBlocking(*segList, po))
		{
			blocked = true;
		}
		if ((*segList)->linedef->validcount != validcount)
		{
			UpdateSegBBox(*segList);
			(*segList)->linedef->validcount = validcount;
		}
	}
	if (blocked)
	{
		segList = po->segs;
		prevPts = po->prevPts;
		for (count = po->numsegs; count; count--, segList++, prevPts++)
		{
			(*segList)->v1->x = prevPts->x;
			(*segList)->v1->y = prevPts->y;
		}
		segList = po->segs;
		validcount++;
		for (count = po->numsegs; count; count--, segList++, prevPts++)
		{
			if ((*segList)->linedef->validcount != validcount)
			{
				UpdateSegBBox(*segList);
				(*segList)->linedef->validcount = validcount;
			}
		}
		LinkPolyobj(po);
		return false;
	}
	po->angle += angle;
	LinkPolyobj(po);

	// [BC] Polyobject has rotated.
	po->bRotated = true;

	return true;
}

//==========================================================================
//
// UnLinkPolyobj
//
//==========================================================================

static void UnLinkPolyobj (polyobj_t *po)
{
	polyblock_t *link;
	int i, j;
	int index;

	// remove the polyobj from each blockmap section
	for(j = po->bbox[BOXBOTTOM]; j <= po->bbox[BOXTOP]; j++)
	{
		index = j*bmapwidth;
		for(i = po->bbox[BOXLEFT]; i <= po->bbox[BOXRIGHT]; i++)
		{
			if(i >= 0 && i < bmapwidth && j >= 0 && j < bmapheight)
			{
				link = PolyBlockMap[index+i];
				while(link != NULL && link->polyobj != po)
				{
					link = link->next;
				}
				if(link == NULL)
				{ // polyobj not located in the link cell
					continue;
				}
				link->polyobj = NULL;
			}
		}
	}
}

//==========================================================================
//
// LinkPolyobj
//
//==========================================================================

static void LinkPolyobj (polyobj_t *po)
{
	int leftX, rightX;
	int topY, bottomY;
	seg_t **tempSeg;
	polyblock_t **link;
	polyblock_t *tempLink;
	int i, j;

	// calculate the polyobj bbox
	tempSeg = po->segs;
	rightX = leftX = (*tempSeg)->v1->x;
	topY = bottomY = (*tempSeg)->v1->y;

	for(i = 0; i < po->numsegs; i++, tempSeg++)
	{
		if((*tempSeg)->v1->x > rightX)
		{
			rightX = (*tempSeg)->v1->x;
		}
		if((*tempSeg)->v1->x < leftX)
		{
			leftX = (*tempSeg)->v1->x;
		}
		if((*tempSeg)->v1->y > topY)
		{
			topY = (*tempSeg)->v1->y;
		}
		if((*tempSeg)->v1->y < bottomY)
		{
			bottomY = (*tempSeg)->v1->y;
		}
	}
	po->bbox[BOXRIGHT] = (rightX-bmaporgx)>>MAPBLOCKSHIFT;
	po->bbox[BOXLEFT] = (leftX-bmaporgx)>>MAPBLOCKSHIFT;
	po->bbox[BOXTOP] = (topY-bmaporgy)>>MAPBLOCKSHIFT;
	po->bbox[BOXBOTTOM] = (bottomY-bmaporgy)>>MAPBLOCKSHIFT;
	// add the polyobj to each blockmap section
	for(j = po->bbox[BOXBOTTOM]*bmapwidth; j <= po->bbox[BOXTOP]*bmapwidth;
		j += bmapwidth)
	{
		for(i = po->bbox[BOXLEFT]; i <= po->bbox[BOXRIGHT]; i++)
		{
			if(i >= 0 && i < bmapwidth && j >= 0 && j < bmapheight*bmapwidth)
			{
				link = &PolyBlockMap[j+i];
				if(!(*link))
				{ // Create a new link at the current block cell
					*link = new polyblock_t;
					(*link)->next = NULL;
					(*link)->prev = NULL;
					(*link)->polyobj = po;
					continue;
				}
				else
				{
					tempLink = *link;
					while(tempLink->next != NULL && tempLink->polyobj != NULL)
					{
						tempLink = tempLink->next;
					}
				}
				if(tempLink->polyobj == NULL)
				{
					tempLink->polyobj = po;
					continue;
				}
				else
				{
					tempLink->next = new polyblock_t;
					tempLink->next->next = NULL;
					tempLink->next->prev = tempLink;
					tempLink->next->polyobj = po;
				}
			}
			// else, don't link the polyobj, since it's off the map
		}
	}
}

//==========================================================================
//
// CheckMobjBlocking
//
//==========================================================================

static bool CheckMobjBlocking (seg_t *seg, polyobj_t *po)
{
	static TArray<AActor *> checker;
	FBlockNode *block;
	AActor *mobj;
	int i, j, k;
	int left, right, top, bottom;
	fixed_t tmbbox[4];
	line_t *ld;
	bool blocked;

	if (( NETWORK_GetState( ) == NETSTATE_CLIENT ) ||
		( CLIENTDEMO_IsPlaying( )))
	{
		return ( false );
	}

	ld = seg->linedef;

	top = (ld->bbox[BOXTOP]-bmaporgy+MAXRADIUS)>>MAPBLOCKSHIFT;
	bottom = (ld->bbox[BOXBOTTOM]-bmaporgy-MAXRADIUS)>>MAPBLOCKSHIFT;
	left = (ld->bbox[BOXLEFT]-bmaporgx-MAXRADIUS)>>MAPBLOCKSHIFT;
	right = (ld->bbox[BOXRIGHT]-bmaporgx+MAXRADIUS)>>MAPBLOCKSHIFT;

	blocked = false;
	checker.Clear();

	bottom = bottom < 0 ? 0 : bottom;
	bottom = bottom >= bmapheight ? bmapheight-1 : bottom;
	top = top < 0 ? 0 : top;
	top = top >= bmapheight  ? bmapheight-1 : top;
	left = left < 0 ? 0 : left;
	left = left >= bmapwidth ? bmapwidth-1 : left;
	right = right < 0 ? 0 : right;
	right = right >= bmapwidth ?  bmapwidth-1 : right;

	for (j = bottom*bmapwidth; j <= top*bmapwidth; j += bmapwidth)
	{
		for (i = left; i <= right; i++)
		{
			for (block = blocklinks[j+i]; block != NULL; block = block->NextActor)
			{
				mobj = block->Me;
				for (k = (int)checker.Size()-1; k >= 0; --k)
				{
					if (checker[k] == mobj)
					{
						break;
					}
				}
				if (k < 0)
				{
					checker.Push (mobj);
					if ((mobj->flags&MF_SOLID) && !(mobj->flags&MF_NOCLIP))
					{
						tmbbox[BOXTOP] = mobj->y+mobj->radius;
						tmbbox[BOXBOTTOM] = mobj->y-mobj->radius;
						tmbbox[BOXLEFT] = mobj->x-mobj->radius;
						tmbbox[BOXRIGHT] = mobj->x+mobj->radius;

						if (tmbbox[BOXRIGHT] <= ld->bbox[BOXLEFT]
							|| tmbbox[BOXLEFT] >= ld->bbox[BOXRIGHT]
							|| tmbbox[BOXTOP] <= ld->bbox[BOXBOTTOM]
							|| tmbbox[BOXBOTTOM] >= ld->bbox[BOXTOP])
						{
							continue;
						}
						if (P_BoxOnLineSide(tmbbox, ld) != -1)
						{
							continue;
						}
						ThrustMobj (mobj, seg, po);
						blocked = true;
					}
				}
			}
		}
	}
	return blocked;
}

//==========================================================================
//
// InitBlockMap
//
//==========================================================================

static void InitBlockMap (void)
{
	int i;

	PolyBlockMap = new polyblock_t *[bmapwidth*bmapheight];
	memset (PolyBlockMap, 0, bmapwidth*bmapheight*sizeof(polyblock_t *));

	for (i = 0; i < po_NumPolyobjs; i++)
	{
		LinkPolyobj(&polyobjs[i]);
	}
}

//==========================================================================
//
// InitSegLists [RH]
//
// Group segs by vertex and collect segs that are known to belong to a
// polyobject so that they can be initialized fast.
//==========================================================================

static void InitSegLists ()
{
	SDWORD i;

	SegListHead = new SDWORD[numvertexes];

	clearbuf (SegListHead, numvertexes, -1);

	for (i = 0; i < numsegs; ++i)
	{
		if (segs[i].linedef != NULL)
		{
			SegListHead[segs[i].v1 - vertexes] = i;
			if ((segs[i].linedef->special == PO_LINE_START ||
				segs[i].linedef->special == PO_LINE_EXPLICIT))
			{
				KnownPolySegs.Push (i);
			}
		}
	}
}

//==========================================================================
//
// KilSegLists [RH]
//
//==========================================================================

static void KillSegLists ()
{
	delete[] SegListHead;
	SegListHead = NULL;
	KnownPolySegs.Clear ();
	KnownPolySegs.ShrinkToFit ();
}

//==========================================================================
//
// IterFindPolySegs
//
// Passing NULL for segList will cause IterFindPolySegs to count the
// number of segs in the polyobj. v1 is the vertex to stop at, and v2
// is the vertex to start at.
//==========================================================================

static void IterFindPolySegs (vertex_t *v1, vertex_t *v2p, seg_t **segList)
{
	SDWORD j;
	int v2 = int(v2p - vertexes);
	int i;

	// This for loop exists solely to avoid infinitely looping on badly
	// formed polyobjects.
	for (i = 0; i < numsegs; i++)
	{
		j = SegListHead[v2];

		if (j < 0)
		{
			break;
		}

		if (segs[j].v1 == v1)
		{
			return;
		}

		if (segs[j].linedef != NULL)
		{
			if (segList == NULL)
			{
				PolySegCount++;
			}
			else
			{
				*segList++ = &segs[j];
				segs[j].bPolySeg = true;
			}
		}
		v2 = int(segs[j].v2 - vertexes);
	}
	I_Error ("IterFindPolySegs: Non-closed Polyobj around (%d,%d).\n",
		v1->x >> FRACBITS, v1->y >> FRACBITS);
}


//==========================================================================
//
// SpawnPolyobj
//
//==========================================================================

static void SpawnPolyobj (int index, int tag, int type)
{
	unsigned int ii;
	int i;
	int j;

	for (ii = 0; ii < KnownPolySegs.Size(); ++ii)
	{
		i = KnownPolySegs[ii];
		if (i < 0)
		{
			continue;
		}
		
		if (segs[i].linedef->special == PO_LINE_START &&
			segs[i].linedef->args[0] == tag)
		{
			if (polyobjs[index].segs)
			{
				I_Error ("SpawnPolyobj: Polyobj %d already spawned.\n", tag);
			}
			segs[i].linedef->special = 0;
			segs[i].linedef->args[0] = 0;
			segs[i].bPolySeg = true;
			PolySegCount = 1;
			IterFindPolySegs(segs[i].v1, segs[i].v2, NULL);

			polyobjs[index].numsegs = PolySegCount;
			polyobjs[index].segs = new seg_t *[PolySegCount];
			polyobjs[index].segs[0] = &segs[i]; // insert the first seg
			IterFindPolySegs (segs[i].v1, segs[i].v2, polyobjs[index].segs+1);
			polyobjs[index].crush = (type != PO_SPAWN_TYPE) ? 3 : 0;
			polyobjs[index].bHurtOnTouch = (type == PO_SPAWNHURT_TYPE);
			polyobjs[index].bMoved = false;
			polyobjs[index].bRotated = false;
			polyobjs[index].bBlocked = false;
			polyobjs[index].tag = tag;
			polyobjs[index].seqType = segs[i].linedef->args[2];
			if (polyobjs[index].seqType < 0 || polyobjs[index].seqType > 63)
			{
				polyobjs[index].seqType = 0;
			}
			break;
		}
	}
	if (!polyobjs[index].segs)
	{ // didn't find a polyobj through PO_LINE_START
		TArray<seg_t *> polySegList;
		unsigned int psIndexOld;
		polyobjs[index].numsegs = 0;
		for (j = 1; j < PO_MAXPOLYSEGS; j++)
		{
			psIndexOld = polySegList.Size();
			for (ii = 0; ii < KnownPolySegs.Size(); ++ii)
			{
				i = KnownPolySegs[ii];

				if (i >= 0 &&
					segs[i].linedef->special == PO_LINE_EXPLICIT &&
					segs[i].linedef->args[0] == tag)
				{
					if (!segs[i].linedef->args[1])
					{
						I_Error ("SpawnPolyobj: Explicit line missing order number (probably %d) in poly %d.\n",
							j+1, tag);
					}
					if (segs[i].linedef->args[1] == j)
					{
						polySegList.Push (&segs[i]);
						polyobjs[index].numsegs++;
					}
				}
			}
			// Clear out any specials for these segs...we cannot clear them out
			// 	in the above loop, since we aren't guaranteed one seg per linedef.
			for (ii = 0; ii < KnownPolySegs.Size(); ++ii)
			{
				i = KnownPolySegs[ii];
				if (i >= 0 &&
					segs[i].linedef->special == PO_LINE_EXPLICIT &&
					segs[i].linedef->args[0] == tag && segs[i].linedef->args[1] == j)
				{
					segs[i].linedef->special = 0;
					segs[i].linedef->args[0] = 0;
					segs[i].bPolySeg = true;
					KnownPolySegs[ii] = -1;
				}
			}
			if (polySegList.Size() == psIndexOld)
			{ // Check if an explicit line order has been skipped.
			  // A line has been skipped if there are any more explicit
			  // lines with the current tag value. [RH] Can this actually happen?
				for (ii = 0; ii < KnownPolySegs.Size(); ++ii)
				{
					i = KnownPolySegs[ii];
					if (i >= 0 &&
						segs[i].linedef->special == PO_LINE_EXPLICIT &&
						segs[i].linedef->args[0] == tag)
					{
						I_Error ("SpawnPolyobj: Missing explicit line %d for poly %d\n",
							j, tag);
					}
				}
			}
		}
		if (polyobjs[index].numsegs)
		{
			PolySegCount = polyobjs[index].numsegs; // PolySegCount used globally
			polyobjs[index].crush = (type != PO_SPAWN_TYPE) ? 3 : 0;
			polyobjs[index].bHurtOnTouch = (type == PO_SPAWNHURT_TYPE);
			polyobjs[index].bMoved = false;
			polyobjs[index].bRotated = false;
			polyobjs[index].bBlocked = false;
			polyobjs[index].tag = tag;
			polyobjs[index].segs = new seg_t *[polyobjs[index].numsegs];
			for (i = 0; i < polyobjs[index].numsegs; i++)
			{
				polyobjs[index].segs[i] = polySegList[i];
			}
			polyobjs[index].seqType = (*polyobjs[index].segs)->linedef->args[3];
			// Next, change the polyobj's first line to point to a mirror
			//		if it exists
			(*polyobjs[index].segs)->linedef->args[1] =
				(*polyobjs[index].segs)->linedef->args[2];
		}
		else
			I_Error ("SpawnPolyobj: Poly %d does not exist\n", tag);
	}
}

//==========================================================================
//
// TranslateToStartSpot
//
//==========================================================================

static void TranslateToStartSpot (int tag, int originX, int originY)
{
	seg_t **tempSeg;
	seg_t **veryTempSeg;
	vertex_t *tempPt;
	subsector_t *sub;
	polyobj_t *po;
	int deltaX;
	int deltaY;
	vertex_t avg; // used to find a polyobj's center, and hence subsector
	int i;

	po = NULL;
	for (i = 0; i < po_NumPolyobjs; i++)
	{
		if (polyobjs[i].tag == tag)
		{
			po = &polyobjs[i];
			break;
		}
	}
	if (po == NULL)
	{ // didn't match the tag with a polyobj tag
		I_Error("TranslateToStartSpot: Unable to match polyobj tag: %d\n",
			tag);
	}
	if (po->segs == NULL)
	{
		I_Error ("TranslateToStartSpot: Anchor point located without a StartSpot point: %d\n", tag);
	}
	po->originalPts = new vertex_t[po->numsegs];
	po->prevPts = new vertex_t[po->numsegs];
	deltaX = originX-po->startSpot[0];
	deltaY = originY-po->startSpot[1];

	tempSeg = po->segs;
	tempPt = po->originalPts;
	avg.x = 0;
	avg.y = 0;

	validcount++;
	for (i = 0; i < po->numsegs; i++, tempSeg++, tempPt++)
	{
		if ((*tempSeg)->linedef->validcount != validcount)
		{
			(*tempSeg)->linedef->bbox[BOXTOP] -= deltaY;
			(*tempSeg)->linedef->bbox[BOXBOTTOM] -= deltaY;
			(*tempSeg)->linedef->bbox[BOXLEFT] -= deltaX;
			(*tempSeg)->linedef->bbox[BOXRIGHT] -= deltaX;
			(*tempSeg)->linedef->validcount = validcount;
		}
		for (veryTempSeg = po->segs; veryTempSeg != tempSeg; veryTempSeg++)
		{
			if((*veryTempSeg)->v1 == (*tempSeg)->v1)
			{
				break;
			}
		}
		if (veryTempSeg == tempSeg)
		{ // the point hasn't been translated, yet
			(*tempSeg)->v1->x -= deltaX;
			(*tempSeg)->v1->y -= deltaY;
		}
		avg.x += (*tempSeg)->v1->x>>FRACBITS;
		avg.y += (*tempSeg)->v1->y>>FRACBITS;
		// the original Pts are based off the startSpot Pt, and are
		// unique to each seg, not each linedef
		tempPt->x = (*tempSeg)->v1->x-po->startSpot[0];
		tempPt->y = (*tempSeg)->v1->y-po->startSpot[1];
	}
	avg.x /= po->numsegs;
	avg.y /= po->numsegs;
	sub = R_PointInSubsector2 (avg.x<<FRACBITS, avg.y<<FRACBITS);
	if (sub->poly != NULL)
	{
		I_Error ("PO_TranslateToStartSpot: Multiple polyobjs in a single subsector.\n");
	}
	sub->poly = po;
}

//==========================================================================
//
// PO_Init
//
//==========================================================================

void PO_Init (void)
{
	// [RH] Hexen found the polyobject-related things by reloading the map's
	//		THINGS lump here and scanning through it. I have P_SpawnMapThing()
	//		record those things instead, so that in here we simply need to
	//		look at the polyspawns list.
	polyspawns_t *polyspawn, **prev;
	int polyIndex;

	// [RH] Make this faster
	InitSegLists ();

	polyobjs = new polyobj_t[po_NumPolyobjs];
	memset (polyobjs, 0, po_NumPolyobjs*sizeof(polyobj_t));

	polyIndex = 0; // index polyobj number
	// Find the startSpot points, and spawn each polyobj
	for (polyspawn = polyspawns, prev = &polyspawns; polyspawn;)
	{
		// 9301 (3001) = no crush, 9302 (3002) = crushing, 9303 = hurting touch
		if (polyspawn->type == PO_SPAWN_TYPE ||
			polyspawn->type == PO_SPAWNCRUSH_TYPE ||
			polyspawn->type == PO_SPAWNHURT_TYPE)
		{ // Polyobj StartSpot Pt.
			polyobjs[polyIndex].startSpot[0] = polyspawn->x;
			polyobjs[polyIndex].startSpot[1] = polyspawn->y;
			SpawnPolyobj(polyIndex, polyspawn->angle, polyspawn->type);
			polyIndex++;
			*prev = polyspawn->next;
			delete polyspawn;
			polyspawn = *prev;
		} else {
			prev = &polyspawn->next;
			polyspawn = polyspawn->next;
		}
	}
	for (polyspawn = polyspawns; polyspawn;)
	{
		polyspawns_t *next = polyspawn->next;
		if (polyspawn->type == PO_ANCHOR_TYPE)
		{ // Polyobj Anchor Pt.
			TranslateToStartSpot (polyspawn->angle, polyspawn->x, polyspawn->y);
		}
		delete polyspawn;
		polyspawn = next;
	}
	polyspawns = NULL;

	// check for a startspot without an anchor point
	for (polyIndex = 0; polyIndex < po_NumPolyobjs; polyIndex++)
	{
		if (!polyobjs[polyIndex].originalPts)
		{
			I_Error ("PO_Init: StartSpot located without an Anchor point: %d\n",
				polyobjs[polyIndex].tag);
		}
	}
	InitBlockMap();

	// [RH] Don't need the seg lists anymore
	KillSegLists ();
}

//==========================================================================
//
// PO_Busy
//
//==========================================================================

bool PO_Busy (int polyobj)
{
	polyobj_t *poly;

	poly = GetPolyobj (polyobj);
	if (!poly->specialdata)
	{
		return false;
	}
	else
	{
		return true;
	}
}
