//-----------------------------------------------------------------------------
//
// Skulltag Source
// Copyright (C) 2002-2006 Brad Carney
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
// Date created:  7/5/06
//
//
// Filename: a_returnzone.cpp
//
// Description: Contains the structure for the flag/skull return zone
//
//-----------------------------------------------------------------------------

#include "info.h"
#include "r_defs.h"

// Skull/flag return zone ---------------------------------------------------

class AReturnZone : public AActor
{
	DECLARE_STATELESS_ACTOR( AReturnZone, AActor )
public:
	void	PostBeginPlay( );
};

IMPLEMENT_STATELESS_ACTOR( AReturnZone, Any, 5067, 0 )
	PROP_Flags( MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY )
	PROP_Flags3( MF3_DONTSPLASH )
END_DEFAULTS

void AReturnZone::PostBeginPlay( )
{
	Super::PostBeginPlay( );

	// Make this sector instantly return any flags/skulls that are dropped in it.
	Sector->MoreFlags |= SECF_RETURNZONE;

	// This object is no longer needed, so just kill it.
	Destroy( );
}
