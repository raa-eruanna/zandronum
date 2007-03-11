//-----------------------------------------------------------------------------
//
// Skulltag Source
// Copyright (C) 2005 Brad Carney
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
// Created:  10/21/05
//
//
// Filename: OpenGLVideo.h
//
// Description: 
//
//-----------------------------------------------------------------------------

#ifndef __OPENGLVIDEO_H__
#define __OPENGLVIDEO_H__

#include "win32iface.h"
#include <gl/gl.h>
#include <gl/glu.h>
#include "Glext.h"

//*****************************************************************************
//	STRUCTURES

typedef struct _VIDEOMODEINFO_s
{
	_VIDEOMODEINFO_s (ULONG inX, ULONG inY, ULONG inBits, ULONG inRealY)
		: pNext (NULL),
		  ulWidth (inX),
		  ulHeight (inY),
		  ulBPP (inBits),
		  ulRealHeight (inRealY),
          ulHz(0)
		{}


	// Pointer to the next mode in the list.
	struct _VIDEOMODEINFO_s	*pNext;

	// Width, height, bit, and hz information for this mode.
	ULONG	ulWidth;
	ULONG	ulHeight;
	ULONG	ulBPP;
	ULONG	ulHz;
	ULONG	ulRealHeight;

} VIDEOMODEINFO_s;

//*****************************************************************************
class OpenGLVideo : public IVideo
{
public:

	OpenGLVideo( );
	~OpenGLVideo( );

	// Inherited functions from IVideo.
	EDisplayType	GetDisplayType( ) { return ( DISPLAY_Both ); }
	void			SetWindowedScale( float fScale );
	
	DFrameBuffer	*CreateFrameBuffer( int iWidth, int iHeight, bool bFullScreen, DFrameBuffer *pOldFrameBuffer );
	
	bool			FullscreenChanged( bool bFullScreen );
	int				GetModeCount( void );
	void			StartModeIterator( int iBPP );
	bool			NextMode( int *piWidth, int *piHeight, bool *bLetterbox, ULONG *pulHZ );
	bool			NextMode( int *piWidth, int *piHeight, bool *bLetterbox );

	// New functions.
	void			BlankForGDI( void );

	bool			GoFullscreen( bool bYes );
	bool			PaintToWindow( void );
	ULONG			GetRefreshRate( ULONG ulWidth, ULONG ulHeight, ULONG ulBPP );

private:

	// Why is there a pointer back at this class?
	OpenGLVideo			*m_pSelf;

	// List of video modes.
	VIDEOMODEINFO_s		*m_pVideoModeList;

	// Iterators for NextMode().
	VIDEOMODEINFO_s		*m_pIteratorVideoMode;
	ULONG				m_ulIteratorBPP;

	// How wide is our current screen?
	ULONG				m_ulDisplayWidth;

	// How tall is our current screen?
	ULONG				m_ulDisplayHeight;

	// What is our current bits per pixel?
	ULONG				m_ulDisplayBPP;

	// Are we in fullscreen?
	bool				m_bFullScreen;

	// How many times have we tried to create the frame buffer?
	ULONG				m_ulFBCreateAttemptCounter;

	// Create the list of possible video modes.
	void MakeModesList( void );

	// Free that list.
	void FreeModesList( void );

	// Add a possible video mode to the list.
	void AddMode( ULONG ulWidth, ULONG ulHeight, ULONG ulBPP, ULONG ulBaseHeight, ULONG ulHz );

	friend class OpenGLFrameBuffer;
};

//*****************************************************************************
class OpenGLFrameBuffer : public BaseWinFB
{
public:

	OpenGLFrameBuffer( ULONG ulWidth, ULONG ulHeight, bool bFullScreen );
	~OpenGLFrameBuffer( );

	// Inherited functions from BaseWinFB.
	bool	IsFullscreen( );
	void	Blank( );
	bool	PaintToWindow( );

	// New functions.
	void	StartFrame();
	void	Update( );
	void	GetFlashedPalette( PalEntry aPal[256] );
	void	UpdatePalette( );
	void	GetFlash( PalEntry &pRGB, int &piAmount );
	void	GetFlash( PalEntry *pRGP, int *piAmount );
	void	SetViewport(int iX, int iY, int iWidth, int iHeight);
	void	ReadPixels(int iX, int iY, int iWidth, int iHeight, BYTE *img);
	void	ResetContext();
	void	InitializeState();
	void	SetNumTris(int iNumTris);

	virtual void Dim( ) const;
	virtual void Dim ( PalEntry Color, float fDAmount, int iX1, int iY1, int iW, int iH ) const;
	virtual void Clear( int iLeft, int iTop, int iRight, int iBottom, int iColor ) const;

	bool	Lock( );
	bool	Lock( bool bBuffer );
	bool	Relock( );

	bool	SetGamma( float fGamma );
	bool	GetGammaRamp( void *pv );

	bool	SetFlash(PalEntry rgb, int amount);
	bool	SwapControl() { return m_bSupportsSwapInterval; }
	bool	CompiledArrays() { return m_bCompiledArrays; }
	bool	MultiTexture() { return m_useMultiTexture; }
	int		NumTexelUnits() { return m_maxTexelUnits; }
	bool	UseVBO();
	bool	SupportsVertexPrograms() { return m_bSupportsVertexProgram; }
	bool	SupportsFragmentPrograms() { return m_supportsFragmentProgram; }
	bool	SupportsRenderTexture() { return m_bSupportsRenderTexture; }
	bool	SupportsPointSprites() { return m_bSupportsPointSprites; }
	bool	SupportsGamma() { return m_bSupportsGamma; }

	int		GetPageCount();
	void	PaletteChanged( );
	int		QueryNewPalette( );
	int		GetBitdepth();
	int		TrueHeight() { return m_ulTrueHeight; }

	BYTE	GetGamma( BYTE input );
	PalEntry *GetPalette();
	HRESULT	GetHR();
	virtual int QueryPalette() { return 0; };

protected:
   bool CreateResources() { return true; }
   void ReleaseResources() {}

private:

	// Handle to the screen's device context.
	HDC m_hDC;

	// Handle to the screen's OpenGL rendering context.
	HGLRC m_hRC;

   HRESULT m_lastHR;
	PalEntry		m_SourcePalette[256];
	PALETTEENTRY m_palEntries[256];
	PalEntry			m_Flash;

	WORD		m_OrigGamma[768];

	bool		m_bNeedPalUpdate;

	bool		m_bSupportsSwapInterval;
	
	bool		m_bCompiledArrays;
	
	bool		m_bSupportsVertexProgram;

	bool		m_bSupportsGamma;
	
	bool		m_supportsFragmentProgram;

	bool		m_supportsVBO;

	bool		m_bSupportsBlendSubtract;

	bool		m_bSupportsRenderTexture;

	bool		m_bSupportsPointSprites;

	LONG		m_lFlashAmount;

	LONG		m_lNumTris;

	float		m_fGamma;
	
	int			m_maxTexelUnits;

	bool		m_useMultiTexture;

	// How wide is our current screen?
	ULONG		m_ulDisplayWidth;

	// How tall is our current screen?
	ULONG		m_ulDisplayHeight;

	// What is our current bits per pixel?
	ULONG		m_ulDisplayBPP;

	// Are we in fullscreen?
	bool		m_bFullScreen;

	// True height of the current screen (letterbox)
	ULONG		m_ulTrueHeight;

	BYTE		m_GammaTable[256];
	WORD		m_OriginalGamma[768];

	void		SetupPixelFormat( );
	void		loadLightInfo();
	bool		loadRequiredExtensions();
	void		loadExtensions();

	// Let the OpenGLVideo class access member variables of this class.
	friend OpenGLVideo;
};

#endif //__OPENGLVIDEO_H__