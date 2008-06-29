//-----------------------------------------------------------------------------
//
// Skulltag Source
// Copyright (C) 2002 Brad Carney
// Copyright (C) 2007-2012 Skulltag Development Team
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 3. Neither the name of the Skulltag Development Team nor the names of its
//    contributors may be used to endorse or promote products derived from this
//    software without specific prior written permission.
// 4. Redistributions in any form must be accompanied by information on how to
//    obtain complete source code for the software and any accompanying
//    software that uses the software. The source code must either be included
//    in the distribution or be available for no more than the cost of
//    distribution plus a nominal fee, and must be freely redistributable
//    under reasonable conditions. For an executable file, complete source
//    code means the source code for all modules it contains. It does not
//    include source code for modules or files that typically accompany the
//    major components of the operating system on which the executable file
//    runs.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//
//
// Filename: medal.cpp
//
// Description: Contains medal routines and globals
//
//-----------------------------------------------------------------------------

#include "a_sharedglobal.h"
#include "announcer.h"
#include "chat.h"
#include "cl_demo.h"
#include "deathmatch.h"
#include "duel.h"
#include "gi.h"
#include "gamemode.h"
#include "info.h"
#include "medal.h"
#include "p_local.h"
#include "d_player.h"
#include "network.h"
#include "s_sound.h"
#include "st_stuff.h"
#include "sv_commands.h"
#include "team.h"
#include "templates.h"
#include "v_text.h"
#include "v_video.h"
#include "w_wad.h"
#include "scoreboard.h"

//*****************************************************************************
//	VARIABLES

static	MEDAL_t	g_Medals[NUM_MEDALS] =
{	
	{ "EXCLA0", S_EXCELLENT, "Excellent!", CR_GRAY, "Excellent", NUM_MEDALS, {NULL},	},
	{ "INCRA0", S_INCREDIBLE, "Incredible!", CR_RED, "Incredible", MEDAL_EXCELLENT, {NULL}, },	

	{ "IMPRA0", S_IMPRESSIVE, "Impressive!", CR_GRAY, "Impressive", NUM_MEDALS, {NULL}, },
	{ "MIMPA0", S_MOST_IMPRESSIVE, "Most impressive!", CR_RED, "MostImpressive", MEDAL_IMPRESSIVE, {NULL}, },
	
	{ "DOMNA0", S_DOMINATION, "Domination!", CR_GRAY, "Domination", NUM_MEDALS, {NULL}, },	
	{ "TDOMA0", S_TOTAL_DOMINATION, "Total domination!", CR_RED, "TotalDomination", MEDAL_DOMINATION, {NULL}, },	
	
	{ "ACCUA0", S_ACCURACY, "Accuracy!", CR_GRAY, "Accuracy", NUM_MEDALS, {NULL}, },
	{ "PRECA0", S_PRECISION, "Precision!", CR_RED, "Precision", MEDAL_ACCURACY, {NULL}, },

	{ "FAILA0", S_YOUFAILIT, "You fail it!", CR_GREEN, "YouFailIt", NUM_MEDALS, {NULL}, },	
	{ "SKILA0", S_YOURSKILLISNOTENOUGH, "Your skill is not enough!", CR_ORANGE, "YourSkillIsNotEnough", MEDAL_YOUFAILIT, {NULL}, },

	{ "LLAMA0", S_LLAMA, "Llama!", CR_GREEN, "Llama", NUM_MEDALS, "misc/llama", },
	{ "SPAMA0", S_SPAM, "Spam!", CR_GREEN, "Spam", MEDAL_LLAMA, "misc/spam", },	

	{ "VICTA0", S_VICTORY, "Victory!", CR_GRAY, "Victory", NUM_MEDALS, {NULL}, },
	{ "PFCTA0", S_PERFECT, "Perfect!", CR_RED, "Perfect", MEDAL_VICTORY, {NULL}, },	

	{ "TRMAA0", S_TERMINATION, "Termination!", CR_GRAY, "Termination", NUM_MEDALS, {NULL}, },	
	{ "FFRGA0", S_FIRSTFRAG, "First frag!", CR_GRAY, "FirstFrag", NUM_MEDALS, {NULL}, },	
	{ "CAPTA0", S_CAPTURE, "Capture!", CR_GRAY, "Capture", NUM_MEDALS, {NULL}, },	
	{ "STAGA0", S_TAG, "Tag!", CR_GRAY, "Tag", NUM_MEDALS, {NULL}, },	
	{ "ASSTA0", S_ASSIST, "Assist!", CR_GRAY, "Assist", NUM_MEDALS, {NULL}, },	
	{ "DFNSA0", S_DEFENSE, "Defense!", CR_GRAY, "Defense", NUM_MEDALS, {NULL}, },	
	{ "FISTA0", S_FISTING, "Fisting!", CR_GRAY, "Fisting", NUM_MEDALS, {NULL}, },
};

static	MEDALQUEUE_t	g_MedalQueue[MAXPLAYERS][MEDALQUEUE_DEPTH];

// Has the first frag medal been awarded this round?
static	bool			g_bFirstFragAwarded;

//*****************************************************************************
//	CONSOLE VARIABLES

CVAR( Bool, cl_medals, true, CVAR_ARCHIVE )
CVAR( Bool, cl_icons, true, CVAR_ARCHIVE )

//*****************************************************************************
//	PROTOTYPES

ULONG	medal_AddToQueue( ULONG ulPlayer, ULONG ulMedal );
void	medal_PopQueue( ULONG ulPlayer );
void	medal_TriggerMedal( ULONG ulPlayer, ULONG ulMedal );
void	medal_SelectIcon( ULONG ulPlayer );
void	medal_GiveMedal( ULONG ulPlayer, ULONG ulMedal );
void	medal_CheckForFirstFrag( ULONG ulPlayer );
void	medal_CheckForDomination( ULONG ulPlayer );
void	medal_CheckForFisting( ULONG ulPlayer );
void	medal_CheckForExcellent( ULONG ulPlayer );
void	medal_CheckForTermination( ULONG ulDeadPlayer, ULONG ulPlayer );
void	medal_CheckForLlama( ULONG ulDeadPlayer, ULONG ulPlayer );
void	medal_CheckForYouFailIt( ULONG ulPlayer );
bool	medal_PlayerHasCarrierIcon( ULONG ulPlayer );

//*****************************************************************************
//	FUNCTIONS

void MEDAL_Construct( void )
{
	ULONG	ulIdx;
	ULONG	ulQueueIdx;

	for ( ulIdx = 0; ulIdx < MAXPLAYERS; ulIdx++ )
	{
		for ( ulQueueIdx = 0; ulQueueIdx < MEDALQUEUE_DEPTH; ulQueueIdx++ )
		{
			g_MedalQueue[ulIdx][ulQueueIdx].ulMedal = 0;
			g_MedalQueue[ulIdx][ulQueueIdx].ulTick = 0;
		}
	}

	g_bFirstFragAwarded = false;
}

//*****************************************************************************
//
void MEDAL_Tick( void )
{
	ULONG	ulIdx;

	for ( ulIdx = 0; ulIdx < MAXPLAYERS; ulIdx++ )
	{
		// No need to do anything.
		if ( playeringame[ulIdx] == false )
			continue;

		// Tick down the duration of the medal on the top of the queue.
		if ( g_MedalQueue[ulIdx][0].ulTick )
		{
			// If time has expired on this medal, pop it and potentially trigger a new one.
			if ( --g_MedalQueue[ulIdx][0].ulTick == 0 )
				medal_PopQueue( ulIdx );
		}

		// If we're not currently displaying a medal for the player, potentially display
		// some other type of icon.
		if ( g_MedalQueue[ulIdx][0].ulTick == 0 )
			medal_SelectIcon( ulIdx );

		// [BB] Remove any old carrier icons.
		medal_PlayerHasCarrierIcon ( ulIdx );

		// Don't render icons floating above our own heads.
		if ( players[ulIdx].pIcon )
		{
			if (( players[ulIdx].mo->CheckLocalView( consoleplayer )) && (( players[ulIdx].cheats & CF_CHASECAM ) == false ))
				players[ulIdx].pIcon->renderflags |= RF_INVISIBLE;
			else
				players[ulIdx].pIcon->renderflags &= ~RF_INVISIBLE;
		}
	}
}

//*****************************************************************************
//
void MEDAL_Render( void )
{
	player_s	*pPlayer;
	ULONG		ulPlayer;
	ULONG		ulMedal;
	ULONG		ulTick;
	char		szString[64];
	char		szPatchName[32];

	if ( players[consoleplayer].camera == NULL )
		return;

	pPlayer = players[consoleplayer].camera->player;
	if ( pPlayer == NULL )
		return;

	ulPlayer = ULONG( pPlayer - players );

	// If the player doesn't have a medal to be drawn, don't do anything.
	if ( g_MedalQueue[ulPlayer][0].ulTick == 0 )
		return;

	ulMedal	= g_MedalQueue[ulPlayer][0].ulMedal;
	ulTick	= g_MedalQueue[ulPlayer][0].ulTick;

	// Get the graphic from the global array.
	sprintf( szPatchName, g_Medals[ulMedal].szLumpName );
	if ( szPatchName[0] )
	{
		ULONG	ulCurXPos;
		ULONG	ulCurYPos;
		ULONG	ulNumMedals = pPlayer->ulMedalCount[ulMedal];
		ULONG	ulLength;

		// Determine how much actual screen space it will take to render the amount of
		// medals the player has received up until this point.
		ulLength = ulNumMedals * TexMan[szPatchName]->GetWidth( );

		if ( viewheight <= ST_Y )
			ulCurYPos = ((int)( ST_Y - 11 * CleanYfac ));
		else
			ulCurYPos = ((int)( SCREENHEIGHT - 11 * CleanYfac ));

		// If that length is greater then the screen width, display the medals as "<icon> <name> X <num>"
		if ( ulLength >= 320 )
		{
			ulCurXPos = 160;

			if ( g_Medals[ulMedal].ulTextColor == CR_RED )
				sprintf( szString, TEXTCOLOR_RED "%s " TEXTCOLOR_WHITE "X %d", g_Medals[ulMedal].szStr, static_cast<unsigned int> (ulNumMedals) );
			else if ( g_Medals[ulMedal].ulTextColor == CR_GREEN )
				sprintf( szString, TEXTCOLOR_GREEN "%s " TEXTCOLOR_RED "X %d", g_Medals[ulMedal].szStr, static_cast<unsigned int> (ulNumMedals) );
			else
				sprintf( szString, TEXTCOLOR_WHITE "%s " TEXTCOLOR_RED "X %d", g_Medals[ulMedal].szStr, static_cast<unsigned int> (ulNumMedals) );

			if ( ulTick > ( 1 * TICRATE ))
				screen->DrawTexture( TexMan[szPatchName], ulCurXPos, (LONG)( ulCurYPos / CleanYfac ), DTA_Clean, true, TAG_DONE );
			else
				screen->DrawTexture( TexMan[szPatchName], ulCurXPos, (LONG)( ulCurYPos / CleanYfac ), DTA_Clean, true, DTA_Alpha, (LONG)( OPAQUE * (float)( (float)ulTick / (float)TICRATE )), TAG_DONE );

			ulCurXPos = 160 - ( SmallFont->StringWidth( szString ) / 2 );

			if ( ulTick > ( 1 * TICRATE ))
				screen->DrawText( g_Medals[ulMedal].ulTextColor, ulCurXPos, (LONG)( ulCurYPos / CleanYfac ), szString, DTA_Clean, true, TAG_DONE );
			else
				screen->DrawText( g_Medals[ulMedal].ulTextColor, ulCurXPos, (LONG)( ulCurYPos / CleanYfac ), szString, DTA_Clean, true, DTA_Alpha, (LONG)( OPAQUE * (float)( (float)ulTick / (float)TICRATE )), TAG_DONE );
		}
		// Display the medal icon <usNumMedals> times centered on the screen.
		else
		{
			ULONG	ulIdx;

			ulCurXPos = 160 - ( ulLength / 2 );
			sprintf( szString, "%s", g_Medals[ulMedal].szStr );

			for ( ulIdx = 0; ulIdx < ulNumMedals; ulIdx++ )
			{
				if ( ulTick > ( 1 * TICRATE ))
					screen->DrawTexture( TexMan[szPatchName], ulCurXPos + ( TexMan[szPatchName]->GetWidth( ) / 2 ), (LONG)( ulCurYPos / CleanYfac ), DTA_Clean, true, TAG_DONE );
				else
					screen->DrawTexture( TexMan[szPatchName], ulCurXPos + ( TexMan[szPatchName]->GetWidth( ) / 2 ), (LONG)( ulCurYPos / CleanYfac ), DTA_Clean, true, DTA_Alpha, (LONG)( OPAQUE * (float)( (float)ulTick / (float)TICRATE )), TAG_DONE );

				ulCurXPos += TexMan[szPatchName]->GetWidth( );
			}
				
			ulCurXPos = 160 - ( SmallFont->StringWidth( szString ) / 2 );

			if ( ulTick >  ( 1 * TICRATE ))
				screen->DrawText( g_Medals[ulMedal].ulTextColor, ulCurXPos, (LONG)( ulCurYPos / CleanYfac ), szString, DTA_Clean, true, TAG_DONE );
			else
				screen->DrawText( g_Medals[ulMedal].ulTextColor, ulCurXPos, (LONG)( ulCurYPos / CleanYfac ), szString, DTA_Clean, true, DTA_Alpha, (LONG)( OPAQUE * (float)( (float)ulTick / (float)TICRATE )), TAG_DONE );
		}
	}
}

//*****************************************************************************
//*****************************************************************************
//
void MEDAL_GiveMedal( ULONG ulPlayer, ULONG ulMedal )
{
	player_s	*pPlayer;
	ULONG		ulWhereToInsertMedal = -1;

	// Make sure all inputs are valid first.
	if (( ulPlayer >= MAXPLAYERS ) ||
		(( deathmatch || teamgame ) == false ) ||
		( players[ulPlayer].mo == NULL ) ||
		( cl_medals == false ) ||
		( ulMedal >= NUM_MEDALS ))
	{
		return;
	}
	
	pPlayer = &players[ulPlayer];

	// Increase the player's count of this type of medal.
	pPlayer->ulMedalCount[ulMedal]++;	

	// The queue is empty, so put this medal first.
	if ( !g_MedalQueue[ulPlayer][0].ulTick )
		ulWhereToInsertMedal = 0;
	else
	{
		// Go through the queue.
		ULONG ulQueueIdx = 0;
		while ( ulQueueIdx < MEDALQUEUE_DEPTH - 1 && g_MedalQueue[ulPlayer][ulQueueIdx].ulTick )
		{
			// Is this a duplicate/suboordinate of the new medal?
			if ( g_MedalQueue[ulPlayer][ulQueueIdx].ulMedal == g_Medals[ulMedal].ulLowerMedal ||
				 g_MedalQueue[ulPlayer][ulQueueIdx].ulMedal == ulMedal )
			{
				// Commandeer its slot!
				if ( ulWhereToInsertMedal == -1 )
				{
					ulWhereToInsertMedal = ulQueueIdx;
					ulQueueIdx++;
				}

				// Or remove it, as it's a duplicate.
				else
				{
					g_MedalQueue[ulPlayer][ulQueueIdx].ulMedal		= g_MedalQueue[ulPlayer][ulQueueIdx + 1].ulMedal;
					g_MedalQueue[ulPlayer][ulQueueIdx].ulTick		= g_MedalQueue[ulPlayer][ulQueueIdx + 1].ulTick;
				}
			}
			else
				ulQueueIdx++;
		}
	}

	// [RC] No special place for the medal, so just queue it.
	if ( ulWhereToInsertMedal == -1 )
		medal_AddToQueue( ulPlayer, ulMedal );
	else
	{
		// Update the slot in line.
		g_MedalQueue[ulPlayer][ulWhereToInsertMedal].ulTick	= MEDAL_ICON_DURATION;
		g_MedalQueue[ulPlayer][ulWhereToInsertMedal].ulMedal	= ulMedal;

		// If it's replacing/is the medal on top, play the sound.
		if ( ulWhereToInsertMedal == 0 )
		{
			medal_TriggerMedal( ulPlayer, ulMedal );

			if ( pPlayer->pIcon )
				pPlayer->pIcon->lTick = MEDAL_ICON_DURATION;
		}
	}

	// If this player is a bot, tell it that it received a medal.
	if ( players[ulPlayer].pSkullBot )
	{
		players[ulPlayer].pSkullBot->m_ulLastMedalReceived = ulMedal;
		players[ulPlayer].pSkullBot->PostEvent( BOTEVENT_RECEIVEDMEDAL );
	}
}

//*****************************************************************************
//
void MEDAL_RenderAllMedals( LONG lYOffset )
{
	player_s	*pPlayer;
	ULONG		ulPlayer;
	ULONG		ulCurXPos;
	ULONG		ulCurYPos;
	ULONG		ulLength;
	ULONG		ulIdx;
	char		szPatchName[32];

	if ( players[consoleplayer].camera == NULL )
		return;

	pPlayer = players[consoleplayer].camera->player;
	if ( pPlayer == NULL )
		return;

	ulPlayer = ULONG( pPlayer - players );

	if ( viewheight <= ST_Y )
		ulCurYPos = (LONG)((( ST_Y - 11 * CleanYfac ) + lYOffset ) / CleanYfac );
	else
		ulCurYPos = (LONG)((( SCREENHEIGHT - 11 * CleanYfac ) + lYOffset ) / CleanYfac );

	// Determine length of all medals strung together.
	ulLength = 0;
	for ( ulIdx = 0; ulIdx < NUM_MEDALS; ulIdx++ )
	{
		if ( pPlayer->ulMedalCount[ulIdx] )
		{
			sprintf( szPatchName, g_Medals[ulIdx].szLumpName );
			if ( szPatchName[0] )
				ulLength += ( TexMan[szPatchName]->GetWidth( ) * pPlayer->ulMedalCount[ulIdx] );
		}
	}

	// Can't fit all the medals on the screen.
	if ( ulLength >= 320 )
	{
		bool	bScale;
		char	szString[8];

		// Recalculate the length of all the medals strung together.
		ulLength = 0;
		for ( ulIdx = 0; ulIdx < NUM_MEDALS; ulIdx++ )
		{
			if ( pPlayer->ulMedalCount[ulIdx] == 0 )
				continue;

			sprintf( szPatchName, g_Medals[ulIdx].szLumpName );
			if ( szPatchName[0] )
				ulLength += TexMan[szPatchName]->GetWidth( );
		}

		// If the length of all our medals goes beyond 320, we cannot scale them.
		if ( ulLength >= 320 )
		{
			bScale = false;
			ulCurYPos = (LONG)( ulCurYPos * CleanYfac );
		}
		else
			bScale = true;

		ulCurXPos = ( bScale ? 160 : ( SCREENWIDTH / 2 )) - ( ulLength / 2 );
		for ( ulIdx = 0; ulIdx < NUM_MEDALS; ulIdx++ )
		{
			if ( pPlayer->ulMedalCount[ulIdx] == 0 )
				continue;

			sprintf( szPatchName, g_Medals[ulIdx].szLumpName );
			if ( szPatchName[0] )
			{
				screen->DrawTexture( TexMan[szPatchName],
					ulCurXPos + ( TexMan[szPatchName]->GetWidth( ) / 2 ),
					ulCurYPos,
					DTA_Clean, bScale, TAG_DONE );
	
				sprintf( szString, "%d", static_cast<unsigned int> (pPlayer->ulMedalCount[ulIdx]) );
				screen->DrawText( CR_RED,
					ulCurXPos - ( SmallFont->StringWidth( szString ) / 2 ) + TexMan[szPatchName]->GetWidth( ) / 2,
					ulCurYPos,
					szString,
					DTA_Clean, bScale, TAG_DONE );
				ulCurXPos += TexMan[szPatchName]->GetWidth( );
			}
		}
	}
	else
	{
		ulCurXPos = 160 - ( ulLength / 2 );
		for ( ulIdx = 0; ulIdx < NUM_MEDALS; ulIdx++ )
		{
			sprintf( szPatchName, g_Medals[ulIdx].szLumpName );
			if ( szPatchName[0] )
			{
				ULONG	ulMedalIdx;

				for ( ulMedalIdx = 0; ulMedalIdx < pPlayer->ulMedalCount[ulIdx]; ulMedalIdx++ )
				{
					screen->DrawTexture( TexMan[szPatchName], ulCurXPos + ( TexMan[szPatchName]->GetWidth( ) / 2 ), ulCurYPos, DTA_Clean, true, TAG_DONE );
					ulCurXPos += TexMan[szPatchName]->GetWidth( );
				}
			}
		}
	}
}

//*****************************************************************************
//
void MEDAL_RenderAllMedalsFullscreen( player_s *pPlayer )
{
	bool		bScale;
	ULONG		ulCurXPos;
	ULONG		ulCurYPos;
	ULONG		ulIdx;
	char		szString[8];
	UCVarValue	ValWidth;
	UCVarValue	ValHeight;
	float		fXScale;
	float		fYScale;
	ULONG		ulMaxMedalHeight;
	ULONG		ulNumMedal;
	ULONG		ulLastMedal;

	ValWidth = con_virtualwidth.GetGenericRep( CVAR_Int );
	ValHeight = con_virtualheight.GetGenericRep( CVAR_Int );

	if (( con_scaletext ) && ( con_virtualwidth > 0 ) && ( con_virtualheight > 0 ))
	{
		fXScale =  (float)ValWidth.Int / 320.0f;
		fYScale =  (float)ValHeight.Int / 200.0f;
		bScale = true;
	}
	else
		bScale = false;

	// Start by drawing "MEDALS" 4 pixels from the top.
	ulCurYPos = 4;

	screen->SetFont( BigFont );

	if ( bScale )
	{
		screen->DrawText( gameinfo.gametype == GAME_Doom ? CR_RED : CR_UNTRANSLATED,
			(LONG)(( ValWidth.Int / 2 ) - ( BigFont->StringWidth( "MEDALS" ) / 2 )),
			ulCurYPos,
			"MEDALS",
			DTA_VirtualWidth, ValWidth.Int,
			DTA_VirtualHeight, ValHeight.Int,
			TAG_DONE );
	}
	else
	{
		screen->DrawText( gameinfo.gametype == GAME_Doom ? CR_RED : CR_UNTRANSLATED,
			( SCREENWIDTH / 2 ) - ( BigFont->StringWidth( "MEDALS" ) / 2 ),
			ulCurYPos,
			"MEDALS",
			TAG_DONE );
	}

	screen->SetFont( SmallFont );

	ulCurYPos += 42;

	ulNumMedal = 0;
	for ( ulIdx = 0; ulIdx < NUM_MEDALS; ulIdx++ )
	{
		if ( pPlayer->ulMedalCount[ulIdx] == 0 )
			continue;

		if (( ulNumMedal % 2 ) == 0 )
		{
			ulCurXPos = 40;
			if ( bScale )
				ulCurXPos = (ULONG)( (float)ulCurXPos * fXScale );
			else
				ulCurXPos = (ULONG)( (float)ulCurXPos * CleanXfac );

			ulLastMedal = ulIdx;
		}
		else
		{
			if ( bScale )
				ulCurXPos = (ULONG)(( ValWidth.Int / 2 ) + ( 40 * fXScale ));
			else
				ulCurXPos = (ULONG)(( SCREENWIDTH / 2 ) + ( 40 * CleanXfac ));

			ulMaxMedalHeight = MAX( TexMan[g_Medals[ulIdx].szLumpName]->GetHeight( ), TexMan[g_Medals[ulLastMedal].szLumpName]->GetHeight( ));
		}

		if ( bScale )
		{
			screen->DrawTexture( TexMan[g_Medals[ulIdx].szLumpName],
				ulCurXPos + ( TexMan[g_Medals[ulIdx].szLumpName]->GetWidth( ) / 2 ),
				ulCurYPos + ( TexMan[g_Medals[ulIdx].szLumpName]->GetHeight( )),
				DTA_VirtualWidth, ValWidth.Int,
				DTA_VirtualHeight, ValHeight.Int,
				TAG_DONE );
		}
		else
		{
			screen->DrawTexture( TexMan[g_Medals[ulIdx].szLumpName],
				ulCurXPos + ( TexMan[g_Medals[ulIdx].szLumpName]->GetWidth( ) / 2 ),
				ulCurYPos + ( TexMan[g_Medals[ulIdx].szLumpName]->GetHeight( )),
				TAG_DONE );
		}

		if ( bScale )
		{
			screen->DrawText( CR_RED,
				ulCurXPos + 48,
				ulCurYPos + ( TexMan[g_Medals[ulIdx].szLumpName]->GetHeight( )) / 2 - SmallFont->GetHeight( ) / 2,
				"X",
				DTA_VirtualWidth, ValWidth.Int,
				DTA_VirtualHeight, ValHeight.Int,
				TAG_DONE );
		}
		else
		{
			screen->DrawText( CR_RED,
				ulCurXPos + 48,
				ulCurYPos + ( TexMan[g_Medals[ulIdx].szLumpName]->GetHeight( )) / 2 - SmallFont->GetHeight( ) / 2,
				"X",
				TAG_DONE );
		}

		sprintf( szString, "%d", static_cast<unsigned int> (pPlayer->ulMedalCount[ulIdx]) );
		screen->SetFont( BigFont );
		if ( bScale )
		{
			screen->DrawText( CR_RED,
				ulCurXPos + 64,
				ulCurYPos + ( TexMan[g_Medals[ulIdx].szLumpName]->GetHeight( )) / 2 - BigFont->GetHeight( ) / 2,
				szString,
				DTA_VirtualWidth, ValWidth.Int,
				DTA_VirtualHeight, ValHeight.Int,
				TAG_DONE );
		}
		else
		{
			screen->DrawText( CR_RED,
				ulCurXPos + 64,
				ulCurYPos + ( TexMan[g_Medals[ulIdx].szLumpName]->GetHeight( )) / 2 - BigFont->GetHeight( ) / 2,
				szString,
				TAG_DONE );
		}
		screen->SetFont( SmallFont );

		if ( ulNumMedal % 2 )
			ulCurYPos += ulMaxMedalHeight;

		ulNumMedal++;
	}

	// The player has not earned any medals, so nothing was drawn.
	if ( ulNumMedal == 0 )
	{
		if ( bScale )
		{
			screen->DrawText( CR_WHITE,
				(LONG)(( ValWidth.Int / 2 ) - ( SmallFont->StringWidth( "YOU HAVE NOT YET EARNED ANY MEDALS." ) / 2 )),
				26,
				"YOU HAVE NOT YET EARNED ANY MEDALS.",
				DTA_VirtualWidth, ValWidth.Int,
				DTA_VirtualHeight, ValHeight.Int,
				TAG_DONE );
		}
		else
		{
			screen->DrawText( CR_WHITE,
				( SCREENWIDTH / 2 ) - ( SmallFont->StringWidth( "YOU HAVE NOT YET EARNED ANY MEDALS." ) / 2 ),
				26,
				"YOU HAVE NOT YET EARNED ANY MEDALS.",
				TAG_DONE );
		}
	}
	else
	{
		if ( bScale )
		{
			screen->DrawText( CR_WHITE,
				(LONG)(( ValWidth.Int / 2 ) - ( SmallFont->StringWidth( "YOU HAVE EARNED THE FOLLOWING MEDALS:" ) / 2 )),
				26,
				"YOU HAVE EARNED THE FOLLOWING MEDALS:",
				DTA_VirtualWidth, ValWidth.Int,
				DTA_VirtualHeight, ValHeight.Int,
				TAG_DONE );
		}
		else
		{
			screen->DrawText( CR_WHITE,
				( SCREENWIDTH / 2 ) - ( SmallFont->StringWidth( "YOU HAVE EARNED THE FOLLOWING MEDALS:" ) / 2 ),
				26,
				"YOU HAVE EARNED THE FOLLOWING MEDALS:",
				TAG_DONE );
		}
	}
}

//*****************************************************************************
//
ULONG MEDAL_GetDisplayedMedal( ULONG ulPlayer )
{
	if( ulPlayer < MAXPLAYERS )
	{
		if ( g_MedalQueue[ulPlayer][0].ulTick )
			return ( g_MedalQueue[ulPlayer][0].ulMedal );
	}

	return ( NUM_MEDALS );
}

//*****************************************************************************
//
void MEDAL_ClearMedalQueue( ULONG ulPlayer )
{
	ULONG	ulQueueIdx;

	for ( ulQueueIdx = 0; ulQueueIdx < MEDALQUEUE_DEPTH; ulQueueIdx++ )
		g_MedalQueue[ulPlayer][ulQueueIdx].ulTick = 0;
}

//*****************************************************************************
//
void MEDAL_PlayerDied( ULONG ulPlayer, ULONG ulSourcePlayer )
{
	if (( ulPlayer >= MAXPLAYERS ) ||
		( playeringame[ulPlayer] == false ) ||
		( players[ulPlayer].mo == NULL ))
	{
		return;
	}

	// Check for domination and first frag medals.
	if (( ulSourcePlayer < MAXPLAYERS ) &&
		( playeringame[ulSourcePlayer] ) &&
		( players[ulSourcePlayer].mo ) &&
		( players[ulSourcePlayer].mo->IsTeammate( players[ulPlayer].mo ) == false ))
	{
		players[ulSourcePlayer].ulFragsWithoutDeath++;
		players[ulSourcePlayer].ulDeathsWithoutFrag = 0;

		medal_CheckForFirstFrag( ulSourcePlayer );
		medal_CheckForDomination( ulSourcePlayer );
		medal_CheckForFisting( ulSourcePlayer );
		medal_CheckForExcellent( ulSourcePlayer );
		medal_CheckForTermination( ulPlayer, ulSourcePlayer );
		medal_CheckForLlama( ulPlayer, ulSourcePlayer );

		players[ulSourcePlayer].ulLastFragTick = level.time;
	}

	players[ulPlayer].ulDeathCount++;
	players[ulPlayer].ulDeathsWithoutFrag++;
	players[ulPlayer].ulFragsWithoutDeath = 0;

	medal_CheckForYouFailIt( ulPlayer );
}

//*****************************************************************************
//
void MEDAL_ResetFirstFragAwarded( void )
{
	g_bFirstFragAwarded = false;
}

//*****************************************************************************
//*****************************************************************************
//
ULONG medal_AddToQueue( ULONG ulPlayer, ULONG ulMedal )
{
	ULONG	ulQueueIdx;

	// Search for a free slot in this player's medal queue.
	for ( ulQueueIdx = 0; ulQueueIdx < MEDALQUEUE_DEPTH; ulQueueIdx++ )
	{
		// Once we've found a free slot, update its info and break.
		if ( g_MedalQueue[ulPlayer][ulQueueIdx].ulTick == 0 )
		{
			g_MedalQueue[ulPlayer][ulQueueIdx].ulTick = MEDAL_ICON_DURATION;
			g_MedalQueue[ulPlayer][ulQueueIdx].ulMedal = ulMedal;

			return ( ulQueueIdx );
		}
		// If this isn't a free slot, maybe it's a medal of the same type that we're trying to add.
		// If so, there's no need to do anything.
		else if ( g_MedalQueue[ulPlayer][ulQueueIdx].ulMedal == ulMedal )
			return ( ulQueueIdx );
	}

	return ( ulQueueIdx );
}

//*****************************************************************************
//
void medal_PopQueue( ULONG ulPlayer )
{
	ULONG	ulQueueIdx;

	// Shift all items in the queue up one notch.
	for ( ulQueueIdx = 0; ulQueueIdx < ( MEDALQUEUE_DEPTH - 1 ); ulQueueIdx++ )
	{
		g_MedalQueue[ulPlayer][ulQueueIdx].ulMedal	= g_MedalQueue[ulPlayer][ulQueueIdx + 1].ulMedal;
		g_MedalQueue[ulPlayer][ulQueueIdx].ulTick	= g_MedalQueue[ulPlayer][ulQueueIdx + 1].ulTick;
	}

	// Also, erase the info in the last slot.
	g_MedalQueue[ulPlayer][MEDALQUEUE_DEPTH - 1].ulMedal	= 0;
	g_MedalQueue[ulPlayer][MEDALQUEUE_DEPTH - 1].ulTick		= 0;

	// If a new medal is now at the top of the queue, trigger it.
	if ( g_MedalQueue[ulPlayer][0].ulTick )
		medal_TriggerMedal( ulPlayer, g_MedalQueue[ulPlayer][0].ulMedal );
	// If there isn't, just delete the medal that has been displaying.
	else if ( players[ulPlayer].pIcon )
	{
		players[ulPlayer].pIcon->Destroy( );
		players[ulPlayer].pIcon = NULL;
	}
}

//*****************************************************************************
// [BB, RC] Returns whether the player wears a carrier icon (flag/skull/hellstone/etc) and removes any invalid ones.
// 
bool medal_PlayerHasCarrierIcon ( ULONG ulPlayer )
{
	player_s *pPlayer = &players[ulPlayer];
	AInventory	*pInventory = NULL;
	bool bInvalid = false;
	bool bHasIcon = true;

	// Verify that our current icon is valid.
	if ( pPlayer->pIcon )
	{
		switch ( (ULONG)( pPlayer->pIcon->state - pPlayer->pIcon->SpawnState ))
		{
		// Flag/skull icon. Delete it if the player no longer has it.
		case S_BLUESKULL:
		case ( S_BLUESKULL + 1 ):
		case S_BLUEFLAG:
		case ( S_BLUEFLAG + 1 ):
		case ( S_BLUEFLAG + 2 ):
		case ( S_BLUEFLAG + 3 ):
		case ( S_BLUEFLAG + 4 ):
		case ( S_BLUEFLAG + 5 ):
		case S_REDSKULL:
		case ( S_REDSKULL + 1 ):
		case S_REDFLAG:
		case ( S_REDFLAG + 1 ):
		case ( S_REDFLAG + 2 ):
		case ( S_REDFLAG + 3 ):
		case ( S_REDFLAG + 4 ):
		case ( S_REDFLAG + 5 ):
		case S_WHITEFLAG:
		case ( S_WHITEFLAG + 1 ):
		case ( S_WHITEFLAG + 2 ):
		case ( S_WHITEFLAG + 3 ):
		case ( S_WHITEFLAG + 4 ):
		case ( S_WHITEFLAG + 5 ):

			{
				// Delete the icon if teamgame has been turned off, or if the player
				// is not on a team.
				if (( teamgame == false ) ||
					( pPlayer->bOnTeam == false ))
				{
					bInvalid = true;
					break;
				}

				// Delete the white flag if the player no longer has it.
				pInventory = pPlayer->mo->FindInventory( PClass::FindClass( "WhiteFlag" ));
				if (( oneflagctf ) && ( pInventory == NULL ))
				{
					bInvalid = true;
					break;
				}

				// Delete the flag/skull if the player no longer has it.
				pInventory = pPlayer->mo->FindInventory( TEAM_GetFlagItem( !pPlayer->ulTeam ));
				if (( oneflagctf == false ) && ( pInventory == NULL ))
				{
					bInvalid = true;
					break;
				}

			}

			break;
		// Terminator artifact icon. Delete it if the player no longer has it.
		case S_TERMINATORARTIFACT:
		case ( S_TERMINATORARTIFACT + 1 ):
		case ( S_TERMINATORARTIFACT + 2 ):
		case ( S_TERMINATORARTIFACT + 3 ):

			if (( terminator == false ) || (( pPlayer->cheats & CF_TERMINATORARTIFACT ) == false ))
				bInvalid = true;
			break;
		// Possession artifact icon. Delete it if the player no longer has it.
		case S_POSSESSIONARTIFACT:
		case ( S_POSSESSIONARTIFACT + 1 ):
		case ( S_POSSESSIONARTIFACT + 2 ):
		case ( S_POSSESSIONARTIFACT + 3 ):

			if ((( possession == false ) && ( teampossession == false )) || (( pPlayer->cheats & CF_POSSESSIONARTIFACT ) == false ))
				bInvalid = true;
			break;
		default:
			bHasIcon = false;
			break;
		}
	}

	// Remove it.
	if ( bInvalid && bHasIcon )
	{
		players[ulPlayer].pIcon->Destroy( );
		players[ulPlayer].pIcon = NULL;

		medal_TriggerMedal( ulPlayer, g_MedalQueue[ulPlayer][0].ulMedal );
	}

	return bHasIcon && !bInvalid;
}

//*****************************************************************************
//
void medal_TriggerMedal( ULONG ulPlayer, ULONG ulMedal )
{
	player_s	*pPlayer;

	pPlayer = &players[ulPlayer];

	// Servers don't actually spawn medals.
	if ( NETWORK_GetState( ) == NETSTATE_SERVER )
		return;

	// Shouldn't happen...
	if ( pPlayer->mo == NULL )
		return;

	// Make sure this medal is valid.
	if ( ulMedal >= NUM_MEDALS || !g_MedalQueue[ulPlayer][0].ulTick )
		return;

	// Medals don't override carrier symbols.
	if ( !medal_PlayerHasCarrierIcon( ulPlayer) )
	{
		if ( pPlayer->pIcon )
			pPlayer->pIcon->Destroy( );

		// Spawn the medal as an icon above the player and set its properties.
		pPlayer->pIcon = Spawn<AFloatyIcon>( pPlayer->mo->x, pPlayer->mo->y, pPlayer->mo->z, NO_REPLACE );
		if ( pPlayer->pIcon )
		{
			pPlayer->pIcon->SetState( pPlayer->pIcon->SpawnState + g_Medals[ulMedal].usFrame );
			pPlayer->pIcon->lTick = MEDAL_ICON_DURATION;
			pPlayer->pIcon->SetTracer( pPlayer->mo );
		}
	}

	// Also, locally play the announcer sound associated with this medal.
	if ( pPlayer - players == consoleplayer )
	{
		if ( g_Medals[ulMedal].szAnnouncerEntry[0] )
			ANNOUNCER_PlayEntry( cl_announcer, (const char *)g_Medals[ulMedal].szAnnouncerEntry );
	}
	// If a player besides the console player got the medal, play the remote sound.
	else
	{
		// Play the sound effect associated with this medal type.
		if ( g_Medals[ulMedal].szSoundName[0] != '\0' )
			S_Sound( pPlayer->mo, CHAN_AUTO, g_Medals[ulMedal].szSoundName, 1, ATTN_NORM );
	}
}

//*****************************************************************************
//
void medal_SelectIcon( ULONG ulPlayer )
{
	AInventory	*pInventory;
	player_s	*pPlayer;
	ULONG		ulActualSprite = 65535;

	if ( ulPlayer >= MAXPLAYERS )
		return;

	pPlayer = &players[ulPlayer];
	if ( pPlayer->mo == NULL )
		return;

	// Allow the user to disable icons.
	if (( cl_icons == false ) || ( NETWORK_GetState( ) == NETSTATE_SERVER ) || pPlayer->bSpectating )
	{
		if ( pPlayer->pIcon )
		{
			pPlayer->pIcon->Destroy( );
			pPlayer->pIcon = NULL;
		}

		return;
	}

	// Verify that our current icon is valid. (i.e. We may have had a chat bubble, then
	// stopped talking, so we need to delete it).
	if ( pPlayer->pIcon )
	{
		switch ( (ULONG)( pPlayer->pIcon->state - pPlayer->pIcon->SpawnState ))
		{
		// Chat icon. Delete it if the player is no longer talking.
		case S_CHAT:

			if ( pPlayer->bChatting == false )
			{
				pPlayer->pIcon->Destroy( );
				pPlayer->pIcon = NULL;
			}
			else
				ulActualSprite = 12;
			break;
		// In console icon. Delete it if the player is no longer in the console.
		case S_INCONSOLE:
		case ( S_INCONSOLE + 1):

			if ( pPlayer->bInConsole == false )
			{
				pPlayer->pIcon->Destroy( );
				pPlayer->pIcon = NULL;
			}
			else
				ulActualSprite = 1;
			break;

		// Ally icon. Delete it if the player is now our enemy or if we're spectating.
		case S_ALLY:

			if ( ( players[ SCOREBOARD_GetViewPlayer() ].bSpectating ) || ( !pPlayer->mo->IsTeammate( players[ SCOREBOARD_GetViewPlayer() ].mo ) ) )
			{
				pPlayer->pIcon->Destroy( );
				pPlayer->pIcon = NULL;
			}
			else
				ulActualSprite = 1;
			break;
		// Flag/skull icon. Delete it if the player no longer has it.
		case S_BLUESKULL:
		case ( S_BLUESKULL + 1 ):
		case S_BLUEFLAG:
		case ( S_BLUEFLAG + 1 ):
		case ( S_BLUEFLAG + 2 ):
		case ( S_BLUEFLAG + 3 ):
		case ( S_BLUEFLAG + 4 ):
		case ( S_BLUEFLAG + 5 ):
		case S_REDSKULL:
		case ( S_REDSKULL + 1 ):
		case S_REDFLAG:
		case ( S_REDFLAG + 1 ):
		case ( S_REDFLAG + 2 ):
		case ( S_REDFLAG + 3 ):
		case ( S_REDFLAG + 4 ):
		case ( S_REDFLAG + 5 ):
		case S_WHITEFLAG:
		case ( S_WHITEFLAG + 1 ):
		case ( S_WHITEFLAG + 2 ):
		case ( S_WHITEFLAG + 3 ):
		case ( S_WHITEFLAG + 4 ):
		case ( S_WHITEFLAG + 5 ):

			{
				bool	bDelete = false;

				// Delete the icon if teamgame has been turned off, or if the player
				// is not on a team.
				if (( teamgame == false ) ||
					( pPlayer->bOnTeam == false ))
				{
					bDelete = true;
				}

				// Delete the white flag if the player no longer has it.
				pInventory = pPlayer->mo->FindInventory( PClass::FindClass( "WhiteFlag" ));
				if (( oneflagctf ) && ( pInventory == NULL ))
					bDelete = true;

				// Delete the flag/skull if the player no longer has it.
				pInventory = pPlayer->mo->FindInventory( TEAM_GetFlagItem( !pPlayer->ulTeam ));
				if (( oneflagctf == false ) && ( pInventory == NULL ))
					bDelete = true;

				// We wish to delete the icon, so do that now.
				if ( bDelete )
				{
					pPlayer->pIcon->Destroy( );
					pPlayer->pIcon = NULL;
				}
				else
					ulActualSprite = 2;
			}

			break;
		// Terminator artifact icon. Delete it if the player no longer has it.
		case S_TERMINATORARTIFACT:
		case ( S_TERMINATORARTIFACT + 1 ):
		case ( S_TERMINATORARTIFACT + 2 ):
		case ( S_TERMINATORARTIFACT + 3 ):

			if (( terminator == false ) || (( pPlayer->cheats & CF_TERMINATORARTIFACT ) == false ))
			{
				pPlayer->pIcon->Destroy( );
				pPlayer->pIcon = NULL;
			}
			else
				ulActualSprite = 3;
			break;
		// Lag icon. Delete it if the player is no longer lagging.
		case S_LAG:

			if ((( NETWORK_GetState( ) != NETSTATE_CLIENT ) &&
				( CLIENTDEMO_IsPlaying( ) == false )) ||
				( pPlayer->bLagging == false ))
			{
				pPlayer->pIcon->Destroy( );
				pPlayer->pIcon = NULL;
			}
			else
				ulActualSprite = 4;
			break;
		// Possession artifact icon. Delete it if the player no longer has it.
		case S_POSSESSIONARTIFACT:
		case ( S_POSSESSIONARTIFACT + 1 ):
		case ( S_POSSESSIONARTIFACT + 2 ):
		case ( S_POSSESSIONARTIFACT + 3 ):

			if ((( possession == false ) && ( teampossession == false )) || (( pPlayer->cheats & CF_POSSESSIONARTIFACT ) == false ))
			{
				pPlayer->pIcon->Destroy( );
				pPlayer->pIcon = NULL;
			}
			else
				ulActualSprite = 5;
			break;
		}
	}

	// Check if we need to have an icon above us, or change the current icon.
	{
		ULONG	ulFrame = 65535;
		ULONG	ulDesiredSprite = 65535;

		// Draw an ally icon if this person is on our team. Would this be useful for co-op, too?
		if ( GAMEMODE_GetFlags( GAMEMODE_GetCurrentMode( )) & GMF_PLAYERSONTEAMS )
		{
			if ( pPlayer->mo->IsTeammate( players[SCOREBOARD_GetViewPlayer()].mo ) && !players[SCOREBOARD_GetViewPlayer()].bSpectating)
			{
				ulFrame = S_ALLY;
				ulDesiredSprite = 2;
			}
		}

		// Draw a chat icon over the player if they're typing.
		if ( pPlayer->bChatting )
		{
			ulFrame = S_CHAT;
			ulDesiredSprite = 12;
		}

		// Draw a console icon over the player if they're in the console.
		if ( pPlayer->bInConsole )
		{
			ulFrame = S_INCONSOLE;
			ulDesiredSprite = 1;
		}

		// Draw a lag icon over their head if they're lagging.
		if ( pPlayer->bLagging )
		{
			ulFrame = S_LAG;
			ulDesiredSprite = 4;
		}

		// Draw a flag/skull above this player if he's carrying one.
		if ( teamgame )
		{
			if ( pPlayer->bOnTeam )
			{
				if ( oneflagctf )
				{
					pInventory = pPlayer->mo->FindInventory( PClass::FindClass( "WhiteFlag" ));
					if ( pInventory )
					{
						ulFrame = S_WHITEFLAG;
						ulDesiredSprite = 2;
					}
				}
				else
				{
					pInventory = pPlayer->mo->FindInventory( TEAM_GetFlagItem( !pPlayer->ulTeam ));
					if ( pInventory )
					{
						if ( pPlayer->ulTeam == TEAM_BLUE )
							ulFrame = ( ctf ) ? (USHORT)S_REDFLAG : (USHORT)S_REDSKULL;
						else
							ulFrame = ( ctf ) ? (USHORT)S_BLUEFLAG : (USHORT)S_BLUESKULL;

						ulDesiredSprite = 2;
					}
				}
			}
		}

		// Draw the terminator artifact over the terminator.
		if ( terminator && ( pPlayer->cheats & CF_TERMINATORARTIFACT ))
		{
			ulFrame = S_TERMINATORARTIFACT;
			ulDesiredSprite = 3;
		}

		// Draw the possession artifact over the player.
		if (( possession || teampossession ) && ( pPlayer->cheats & CF_POSSESSIONARTIFACT ))
		{
			ulFrame = S_POSSESSIONARTIFACT;
			ulDesiredSprite = 5;
		}

		// We have an icon that needs to be spawned.
		if (( ulFrame != 65535 ) && ( ulDesiredSprite != 65535 ))
		{
			if (( pPlayer->pIcon == NULL ) || ( ulDesiredSprite != ulActualSprite ))
			{
				if ( pPlayer->pIcon )
				{
					pPlayer->pIcon->Destroy( );
					pPlayer->pIcon = NULL;
				}

				pPlayer->pIcon = Spawn<AFloatyIcon>( pPlayer->mo->x, pPlayer->mo->y, pPlayer->mo->z + pPlayer->mo->height + ( 4 * FRACUNIT ), NO_REPLACE );
				if ( pPlayer->pIcon )
				{
					pPlayer->pIcon->SetTracer( pPlayer->mo );
					pPlayer->pIcon->SetState( pPlayer->pIcon->SpawnState + ulFrame );
				}
			}
		}
	}
}

//*****************************************************************************
//
void medal_GiveMedal( ULONG ulPlayer, ULONG ulMedal )
{
	// Give the player the medal, and if we're the server, tell clients.
	MEDAL_GiveMedal( ulPlayer, ulMedal );
	if ( NETWORK_GetState( ) == NETSTATE_SERVER )
		SERVERCOMMANDS_GivePlayerMedal( ulPlayer, ulMedal );
}

//*****************************************************************************
//
void medal_CheckForFirstFrag( ULONG ulPlayer )
{
	// Only award it once.
	if ( g_bFirstFragAwarded )
		return;

	if (( deathmatch ) &&
		( lastmanstanding == false ) &&
		( teamlms == false ) &&
		( possession == false ) &&
		( teampossession == false ) &&
		(( duel == false ) || ( DUEL_GetState( ) == DS_INDUEL )))
	{
		medal_GiveMedal( ulPlayer, MEDAL_FIRSTFRAG );

		// It's been given.
		g_bFirstFragAwarded = true;
	}
}

//*****************************************************************************
//
void medal_CheckForDomination( ULONG ulPlayer )
{
	// If the player has gotten 5 straight frags without dying, award a medal.
	if (( players[ulPlayer].ulFragsWithoutDeath % 5 ) == 0 )
	{
		// If the player gets 10+ straight frags without dying, award a "Total Domination" medal.
		if ( players[ulPlayer].ulFragsWithoutDeath >= 10 )
			medal_GiveMedal( ulPlayer, MEDAL_TOTALDOMINATION );
		// Otherwise, award a "Domination" medal.
		else
			medal_GiveMedal( ulPlayer, MEDAL_DOMINATION );
	}
}

//*****************************************************************************
//
void medal_CheckForFisting( ULONG ulPlayer )
{
	if ( players[ulPlayer].ReadyWeapon == NULL )
		return;

	// If the player killed him with this fist, award him a "Fisting!" medal.
	if ( players[ulPlayer].ReadyWeapon->GetClass( ) == PClass::FindClass( "Fist" ))
		medal_GiveMedal( ulPlayer, MEDAL_FISTING );

	// If this is the second frag this player has gotten THIS TICK with the
	// BFG9000, award him a "SPAM!" medal.
	if ( players[ulPlayer].ReadyWeapon->GetClass( ) == PClass::FindClass( "BFG9000" ))
	{
		if ( players[ulPlayer].ulLastBFGFragTick == static_cast<unsigned> (level.time) )
		{
			// Award the medal.
			medal_GiveMedal( ulPlayer, MEDAL_SPAM );

			// Also, cancel out the possibility of getting an Excellent/Incredible medal.
			players[ulPlayer].ulLastExcellentTick = 0;
			players[ulPlayer].ulLastFragTick = 0;
		}
		else
			players[ulPlayer].ulLastBFGFragTick = level.time;
	}
}

//*****************************************************************************
//
void medal_CheckForExcellent( ULONG ulPlayer )
{
	// If the player has gotten two Excelents within two seconds, award an "Incredible" medal.
	if (( players[ulPlayer].ulLastExcellentTick + ( 2 * TICRATE )) > (ULONG)level.time )
	{
		// Award the incredible.
		medal_GiveMedal( ulPlayer, MEDAL_INCREDIBLE );

		players[ulPlayer].ulLastExcellentTick = level.time;
		players[ulPlayer].ulLastFragTick = level.time;
	}
	// If this player has gotten two frags within two seconds, award an "Excellent" medal.
	else if (( players[ulPlayer].ulLastFragTick + ( 2 * TICRATE )) > (ULONG)level.time )
	{
		// Award the excellent.
		medal_GiveMedal( ulPlayer, MEDAL_EXCELLENT );

		players[ulPlayer].ulLastExcellentTick = level.time;
		players[ulPlayer].ulLastFragTick = level.time;
	}
}

//*****************************************************************************
//
void medal_CheckForTermination( ULONG ulDeadPlayer, ULONG ulPlayer )
{
	// If the target player is the terminatior, award a "termination" medal.
	if ( players[ulDeadPlayer].cheats & CF_TERMINATORARTIFACT )
		medal_GiveMedal( ulPlayer, MEDAL_TERMINATION );
}

//*****************************************************************************
//
void medal_CheckForLlama( ULONG ulDeadPlayer, ULONG ulPlayer )
{
	// Award a "llama" medal if the victim had been typing, lagging, or in the console.
	if ( players[ulDeadPlayer].bChatting ||	players[ulDeadPlayer].bLagging || players[ulDeadPlayer].bInConsole )
		medal_GiveMedal( ulPlayer, MEDAL_LLAMA );
}

//*****************************************************************************
//
void medal_CheckForYouFailIt( ULONG ulPlayer )
{
	// If the player dies TEN times without getting a frag, award a "Your skill is not enough" medal.
	if (( players[ulPlayer].ulDeathsWithoutFrag % 10 ) == 0 )
		medal_GiveMedal( ulPlayer, MEDAL_YOURSKILLISNOTENOUGH );
	// If the player dies five times without getting a frag, award a "You fail it" medal.
	else if (( players[ulPlayer].ulDeathsWithoutFrag % 5 ) == 0 )
		medal_GiveMedal( ulPlayer, MEDAL_YOUFAILIT );
}

#ifdef	_DEBUG
#include "c_dispatch.h"
CCMD( testgivemedal )
{
	ULONG	ulIdx;

	for ( ulIdx = 0; ulIdx < NUM_MEDALS; ulIdx++ )
		MEDAL_GiveMedal( consoleplayer, ulIdx );
}
#endif	// _DEBUG
