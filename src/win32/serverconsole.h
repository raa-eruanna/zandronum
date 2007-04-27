//-----------------------------------------------------------------------------
//
// Skulltag Source
// Copyright (C) 2004 Brad Carney
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
// Created:  4/5/04
//
//
// Filename: serverconsole.h
//
// Description: Contains variables and routines related to the GUI server console
//
//-----------------------------------------------------------------------------

#ifndef	__SERVERCONSOLE_H__
#define	__SERVERCONSOLE_H__

//*****************************************************************************
//	DEFINES

#define	GUI_SERVER_CONSOLE
#define	SERVERCONSOLE_TEXTLENGTH	4096

#define	COLUMN_NAME			0
#define	COLUMN_FRAGS		1
#define	COLUMN_PING			2
#define	COLUMN_TIME			3

#define	MAX_SKULLTAG_SERVER_INSTANCES	128

//*****************************************************************************
//	PROTOTYPES

BOOL CALLBACK	SERVERCONSOLE_ServerDialogBoxCallback( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK	SERVERCONSOLE_DMFlagsCallback( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK	SERVERCONSOLE_MapRotationCallback( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK	SERVERCONSOLE_LMSSettingsCallback( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK	SERVERCONSOLE_MessagesCallback( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK	SERVERCONSOLE_AddBotCallback( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK	SERVERCONSOLE_RemoveBotCallback( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK	SERVERCONSOLE_KickPlayerCallback( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK	SERVERCONSOLE_BanPlayerCallback( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK	SERVERCONSOLE_ChangeMapCallback( HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK	SERVERCONSOLE_BanIPCallback( HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK	SERVERCONSOLE_BanListCallback( HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK	SERVERCONSOLE_EditBanCallback( HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK	SERVERCONSOLE_ServerInformationCallback( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK	SERVERCONSOLE_ServerStatisticsCallback( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK	SERVERCONSOLE_GeneralSettingsCallback( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
void			SERVERCONSOLE_UpdateTitleString( char *pszString );
void			SERVERCONSOLE_UpdateScoreboard( void );
void			SERVERCONSOLE_UpdateTotalOutboundDataTransfer( LONG lData );
void			SERVERCONSOLE_UpdateAverageOutboundDataTransfer( LONG lData );
void			SERVERCONSOLE_UpdatePeakOutboundDataTransfer( LONG lData );
void			SERVERCONSOLE_UpdateCurrentOutboundDataTransfer( LONG lData );
void			SERVERCONSOLE_UpdateTotalInboundDataTransfer( LONG lData );
void			SERVERCONSOLE_UpdateAverageInboundDataTransfer( LONG lData );
void			SERVERCONSOLE_UpdatePeakInboundDataTransfer( LONG lData );
void			SERVERCONSOLE_UpdateCurrentInboundDataTransfer( LONG lData );
void			SERVERCONSOLE_UpdateTotalUptime( LONG lData );
void			SERVERCONSOLE_SetCurrentMapname( char *pszString );
void			SERVERCONSOLE_SetupColumns( void );
void			SERVERCONSOLE_AddNewPlayer( LONG lPlayer );
void			SERVERCONSOLE_UpdatePlayerInfo( LONG lPlayer, ULONG ulUpdateFlags );
void			SERVERCONSOLE_RemovePlayer( LONG lPlayer );
void			SERVERCONSOLE_InitializeDMFlagsDisplay( HWND hDlg );
void			SERVERCONSOLE_UpdateDMFlagsDisplay( HWND hDlg );
void			SERVERCONSOLE_UpdateDMFlags( HWND hDlg );
void			SERVERCONSOLE_InitializeLMSSettingsDisplay( HWND hDlg );
void			SERVERCONSOLE_UpdateLMSSettingsDisplay( HWND hDlg );
void			SERVERCONSOLE_UpdateLMSSettings( HWND hDlg );
void			SERVERCONSOLE_InitializeGeneralSettingsDisplay( HWND hDlg );
bool			SERVERCONSOLE_IsRestartNeeded( HWND hDlg );
void			SERVERCONSOLE_UpdateGeneralSettingsDisplay( HWND hDlg );
void			SERVERCONSOLE_UpdateGeneralSettings( HWND hDlg );
void			SERVERCONSOLE_UpdateOperatingSystem( char *pszString );
void			SERVERCONSOLE_UpdateCPUSpeed( char *pszString );
void			SERVERCONSOLE_UpdateVendor( char *pszString );
void			SERVERCONSOLE_Print( char *pszString );

DWORD WINAPI	MainDoomThread( LPVOID );

#endif	// __SERVERCONSOLE_H__
