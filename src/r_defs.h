// Emacs style mode select	 -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// DESCRIPTION:
//		Refresh/rendering module, shared data struct definitions.
//
//-----------------------------------------------------------------------------


#ifndef __R_DEFS_H__
#define __R_DEFS_H__

#include "doomdef.h"

// Some more or less basic data types
// we depend on.
#include "m_fixed.h"

// We rely on the thinker data struct
// to handle sound origins in sectors.
// SECTORS do store MObjs anyway.
#include "actor.h"
struct FLightNode;

#include "dthinker.h"

#define MAXWIDTH 2560
#define MAXHEIGHT 1600

const WORD NO_INDEX = 0xffffu;
const DWORD NO_SIDE = 0xffffffffu;

// [BC] This is the maximum length a skin name can be.
#define	MAX_SKIN_NAME					24

// Silhouette, needed for clipping Segs (mainly)
// and sprites representing things.
enum
{
	SIL_NONE,
	SIL_BOTTOM,
	SIL_TOP,
	SIL_BOTH
};

extern size_t MaxDrawSegs;





//
// INTERNAL MAP TYPES
//	used by play and refresh
//

//
// Your plain vanilla vertex.
// Note: transformed values not buffered locally,
//	like some DOOM-alikes ("wt", "WebView") did.
//
struct vertex_t
{
	fixed_t x, y;

	bool operator== (const vertex_t &other)
	{
		return x == other.x && y == other.y;
	}
};

// Forward of LineDefs, for Sectors.
struct line_t;

class player_t;
class FScanner;
class FBitmap;
struct FCopyInfo;
class DInterpolation;

//
// The SECTORS record, at runtime.
// Stores things/mobjs.
//
class DSectorEffect;
struct sector_t;
struct FRemapTable;

enum
{
	SECSPAC_Enter		= 1,	// Trigger when player enters
	SECSPAC_Exit		= 2,	// Trigger when player exits
	SECSPAC_HitFloor	= 4,	// Trigger when player hits floor
	SECSPAC_HitCeiling	= 8,	// Trigger when player hits ceiling
	SECSPAC_Use			= 16,	// Trigger when player uses
	SECSPAC_UseWall		= 32,	// Trigger when player uses a wall
	SECSPAC_EyesDive	= 64,	// Trigger when player eyes go below fake floor
	SECSPAC_EyesSurface = 128,	// Trigger when player eyes go above fake floor
	SECSPAC_EyesBelowC	= 256,	// Trigger when player eyes go below fake ceiling
	SECSPAC_EyesAboveC	= 512,	// Trigger when player eyes go above fake ceiling
	SECSPAC_HitFakeFloor= 1024,	// Trigger when player hits fake floor
};

class ASectorAction : public AActor
{
	DECLARE_ACTOR (ASectorAction, AActor)
public:
	void Destroy ();
	void BeginPlay ();
	void Activate (AActor *source);
	void Deactivate (AActor *source);
	virtual bool TriggerAction (AActor *triggerer, int activationType);
protected:
	bool CheckTrigger (AActor *triggerer) const;
};

class ASkyViewpoint;

struct secplane_t
{
	// the plane is defined as a*x + b*y + c*z + d = 0
	// ic is 1/c, for faster Z calculations

	fixed_t a, b, c, d, ic;

	// Returns the value of z at (x,y)
	fixed_t ZatPoint (fixed_t x, fixed_t y) const
	{
		return FixedMul (ic, -d - DMulScale16 (a, x, b, y));
	}

	// Returns the value of z at (x,y) as a double
	double ZatPoint (double x, double y) const
	{
		return (d + a*x + b*y) * ic / (-65536.0 * 65536.0);
	}

	// Returns the value of z at vertex v
	fixed_t ZatPoint (const vertex_t *v) const
	{
		return FixedMul (ic, -d - DMulScale16 (a, v->x, b, v->y));
	}

	// Returns the value of z at (x,y) if d is equal to dist
	fixed_t ZatPointDist (fixed_t x, fixed_t y, fixed_t dist) const
	{
		return FixedMul (ic, -dist - DMulScale16 (a, x, b, y));
	}

	// Returns the value of z at vertex v if d is equal to dist
	fixed_t ZatPointDist (const vertex_t *v, fixed_t dist)
	{
		return FixedMul (ic, -dist - DMulScale16 (a, v->x, b, v->y));
	}

	// Flips the plane's vertical orientiation, so that if it pointed up,
	// it will point down, and vice versa.
	void FlipVert ()
	{
		a = -a;
		b = -b;
		c = -c;
		d = -d;
		ic = -ic;
	}

	// Returns true if 2 planes are the same
	bool operator== (const secplane_t &other) const
	{
		return a == other.a && b == other.b && c == other.c && d == other.d;
	}

	// Returns true if 2 planes are different
	bool operator!= (const secplane_t &other) const
	{
		return a != other.a || b != other.b || c != other.c || d != other.d;
	}

	// Moves a plane up/down by hdiff units
	void ChangeHeight (fixed_t hdiff)
	{
		d = d - FixedMul (hdiff, c);
	}

	// Moves a plane up/down by hdiff units
	fixed_t GetChangedHeight (fixed_t hdiff)
	{
		return d - FixedMul (hdiff, c);
	}

	// Returns how much this plane's height would change if d were set to oldd
	fixed_t HeightDiff (fixed_t oldd) const
	{
		return FixedMul (oldd - d, ic);
	}

	// Returns how much this plane's height would change if d were set to oldd
	fixed_t HeightDiff (fixed_t oldd, fixed_t newd) const
	{
		return FixedMul (oldd - newd, ic);
	}

	fixed_t PointToDist (fixed_t x, fixed_t y, fixed_t z) const
	{
		return -TMulScale16 (a, x, y, b, z, c);
	}

	fixed_t PointToDist (const vertex_t *v, fixed_t z) const
	{
		return -TMulScale16 (a, v->x, b, v->y, z, c);
	}
};

inline FArchive &operator<< (FArchive &arc, secplane_t &plane)
{
	arc << plane.a << plane.b << plane.c << plane.d;
	//if (plane.c != 0)
	{	// plane.c should always be non-0. Otherwise, the plane
		// would be perfectly vertical.
		plane.ic = DivScale32 (1, plane.c);
	}
	return arc;
}

#include "p_3dfloors.h"
struct subsector_t;

// Ceiling/floor flags
enum
{
	SECF_ABSLIGHTING	= 1,	// floor/ceiling light is absolute, not relative
	SECF_SPRINGPAD		= 2,	// [BC] Floor bounces actors up at the same velocity they landed on it with.	
};

// Internal sector flags
enum
{
	SECF_FAKEFLOORONLY	= 2,	// when used as heightsec in R_FakeFlat, only copies floor
	SECF_CLIPFAKEPLANES = 4,	// as a heightsec, clip planes to target sector's planes
	SECF_NOFAKELIGHT	= 8,	// heightsec does not change lighting
	SECF_IGNOREHEIGHTSEC= 16,	// heightsec is only for triggering sector actions
	SECF_UNDERWATER		= 32,	// sector is underwater
	SECF_FORCEDUNDERWATER= 64,	// sector is forced to be underwater
	SECF_UNDERWATERMASK	= 32+64,
	SECF_DRAWN			= 128,	// sector has been drawn at least once
	SECF_RETURNZONE		= 256,	// [BC] Flags should be immediately returned if they're dropped within this sector (lava sectors, unreachable sectors, etc.).
};

enum
{
	SECF_SILENT			= 1,	// actors in sector make no noise
	SECF_NOFALLINGDAMAGE= 2,	// No falling damage in this sector
	SECF_FLOORDROP		= 4,	// all actors standing on this floor will remain on it when it lowers very fast.
};

struct FDynamicColormap;

struct FLightStack
{
	secplane_t Plane;		// Plane above this light (points up)
	sector_t *Master;		// Sector to get light from (NULL for owner)
	BITFIELD bBottom:1;		// Light is from the bottom of a block?
	BITFIELD bFlooder:1;	// Light floods lower lights until another flooder is reached?
	BITFIELD bOverlaps:1;	// Plane overlaps the next one
};

struct FExtraLight
{
	short Tag;
	WORD NumLights;
	WORD NumUsedLights;
	FLightStack *Lights;	// Lights arranged from top to bottom

	void InsertLight (const secplane_t &plane, line_t *line, int type);
};

// this substructure contains a few sector properties that are stored in dynamic arrays
// These must not be copied by R_FakeFlat etc. or bad things will happen.
struct sector_t;

struct FLinkedSector
{
	sector_t *Sector;
	int Type;
};


struct extsector_t
{
	// Boom sector transfer information
	struct fakefloor
	{
		TArray<sector_t *> Sectors;
	} FakeFloor;

	// 3DMIDTEX information
	struct midtex
	{
		struct plane
		{
			TArray<sector_t *> AttachedSectors;		// all sectors containing 3dMidtex lines attached to this sector
			TArray<line_t *> AttachedLines;			// all 3dMidtex lines attached to this sector
		} Floor, Ceiling;
	} Midtex;

	// Linked sector information
	struct linked
	{
		struct plane
		{
			TArray<FLinkedSector> Sectors;
		} Floor, Ceiling;
	} Linked;

	// 3D floors
	struct xfloor
	{
		TDeletingArray<F3DFloor *>		ffloors;		// 3D floors in this sector
		TArray<lightlist_t>				lightlist;		// 3D light list
		TArray<sector_t*>				attached;		// 3D floors attached to this sector
	} XFloor;
	
	void Serialize(FArchive &arc);
};


struct sector_t
{
	// Member functions
	bool IsLinked(sector_t *other, bool ceiling) const;
	fixed_t FindLowestFloorSurrounding (vertex_t **v) const;
	fixed_t FindHighestFloorSurrounding (vertex_t **v) const;
	fixed_t FindNextHighestFloor (vertex_t **v) const;
	fixed_t FindNextLowestFloor (vertex_t **v) const;
	fixed_t FindLowestCeilingSurrounding (vertex_t **v) const;			// jff 2/04/98
	fixed_t FindHighestCeilingSurrounding (vertex_t **v) const;			// jff 2/04/98
	fixed_t FindNextLowestCeiling (vertex_t **v) const;					// jff 2/04/98
	fixed_t FindNextHighestCeiling (vertex_t **v) const;				// jff 2/04/98
	fixed_t FindShortestTextureAround () const;							// jff 2/04/98
	fixed_t FindShortestUpperAround () const;							// jff 2/04/98
	sector_t *FindModelFloorSector (fixed_t floordestheight) const;		// jff 2/04/98
	sector_t *FindModelCeilingSector (fixed_t floordestheight) const;	// jff 2/04/98
	int FindMinSurroundingLight (int max) const;
	sector_t *NextSpecialSector (int type, sector_t *prev) const;		// [RH]
	fixed_t FindLowestCeilingPoint (vertex_t **v) const;
	fixed_t FindHighestFloorPoint (vertex_t **v) const;
	void AdjustFloorClip () const;
	void SetColor(int r, int g, int b, int desat);
	void SetFade(int r, int g, int b);

	DInterpolation *SetInterpolation(int position, bool attach);
	void StopInterpolation(int position);

	// Member variables
	fixed_t		CenterFloor () const { return floorplane.ZatPoint (soundorg[0], soundorg[1]); }
	fixed_t		CenterCeiling () const { return ceilingplane.ZatPoint (soundorg[0], soundorg[1]); }

	// [RH] store floor and ceiling planes instead of heights
	secplane_t	floorplane, ceilingplane;
	fixed_t		floortexz, ceilingtexz;	// [RH] used for wall texture mapping

	// [RH] give floor and ceiling even more properties
	FDynamicColormap *ColorMap;	// [RH] Per-sector colormap

	// killough 3/7/98: floor and ceiling texture offsets
	fixed_t		  floor_xoffs,   floor_yoffs;
	fixed_t		ceiling_xoffs, ceiling_yoffs;

	// [RH] floor and ceiling texture scales
	fixed_t		  floor_xscale,   floor_yscale;
	fixed_t		ceiling_xscale, ceiling_yscale;

	// [RH] floor and ceiling texture rotation
	angle_t		floor_angle, ceiling_angle;

	fixed_t		base_ceiling_angle, base_ceiling_yoffs;
	fixed_t		base_floor_angle, base_floor_yoffs;

	BYTE		FloorLight, CeilingLight;
	BYTE		FloorFlags, CeilingFlags;
	int			floorpic, ceilingpic;
	BYTE		lightlevel;

	TObjPtr<AActor> SoundTarget;
	BYTE 		soundtraversed;	// 0 = untraversed, 1,2 = sndlines -1

	short		special;
	short		tag;
	int			nexttag,firsttag;	// killough 1/30/98: improves searches for tags.

	int			sky;
	short		seqType;		// this sector's sound sequence

	fixed_t		soundorg[3];	// origin for any sounds played by the sector
	int 		validcount;		// if == validcount, already checked
	AActor* 	thinglist;		// list of mobjs in sector

	// killough 8/28/98: friction is a sector property, not an mobj property.
	// these fields used to be in AActor, but presented performance problems
	// when processed as mobj properties. Fix is to make them sector properties.
	fixed_t		friction, movefactor;

	// thinker_t for reversable actions
	TObjPtr<DSectorEffect> floordata;			// jff 2/22/98 make thinkers on
	TObjPtr<DSectorEffect> ceilingdata;			// floors, ceilings, lighting,
	TObjPtr<DSectorEffect> lightingdata;		// independent of one another

	enum
	{
		CeilingMove,
		FloorMove,
		CeilingScroll,
		FloorScroll
	};
	TObjPtr<DInterpolation> interpolations[4];

	// jff 2/26/98 lockout machinery for stairbuilding
	SBYTE stairlock;	// -2 on first locked -1 after thinker done 0 normally
	SWORD prevsec;		// -1 or number of sector for previous step
	SWORD nextsec;		// -1 or number of next step sector

	short linecount;
	struct line_t **lines;		// [linecount] size

	// killough 3/7/98: support flat heights drawn at another sector's heights
	sector_t *heightsec;		// other sector, or NULL if no other sector

	DWORD bottommap, midmap, topmap;	// killough 4/4/98: dynamic colormaps
										// [RH] these can also be blend values if
										//		the alpha mask is non-zero

	// list of mobjs that are at least partially in the sector
	// thinglist is a subset of touching_thinglist
	struct msecnode_t *touching_thinglist;				// phares 3/14/98

	float gravity;		// [RH] Sector gravity (1.0 is normal)
	short damage;		// [RH] Damage to do while standing on floor
	short mod;			// [RH] Means-of-death for applied damage

	WORD ZoneNumber;	// [RH] Zone this sector belongs to
	WORD MoreFlags;		// [RH] Internal sector flags
	DWORD Flags;		// Sector flags

	// [RH] Action specials for sectors. Like Skull Tag, but more
	// flexible in a Bloody way. SecActTarget forms a list of actors
	// joined by their tracer fields. When a potential sector action
	// occurs, SecActTarget's TriggerAction method is called.
	TObjPtr<ASectorAction> SecActTarget;

	// [RH] The sky box to render for this sector. NULL means use a
	// regular sky.
	TObjPtr<ASkyViewpoint> FloorSkyBox, CeilingSkyBox;

	// Planes that partition this sector into different light zones.
	FExtraLight *ExtraLights;

	vertex_t *Triangle[3];	// Three points that can define a plane
	short						oldspecial;			//jff 2/16/98 remembers if sector WAS secret (automap)

	// [GZDoom]
	extsector_t	*				e;		// This stores data that requires construction/destruction. Such data must not be copied by R_FakeFlat.
	float						ceiling_reflect, floor_reflect;
	int							sectornum;			// for comparing sector copies

	bool						transdoor;			// For transparent door hacks
	fixed_t						transdoorheight;	// for transparent door hacks
	int							subsectorcount;		// list of subsectors
	subsector_t **				subsectors;

	// [BC] Is this sector a floor or ceiling?
	int		floorOrCeiling;

	// [BC] Has the height changed during the course of the level?
	bool		bCeilingHeightChange;
	bool		bFloorHeightChange;
	secplane_t	SavedCeilingPlane;
	secplane_t	SavedFloorPlane;
	fixed_t		SavedCeilingTexZ;
	fixed_t		SavedFloorTexZ;

	// [BC] Has the flat changed?
	bool	bFlatChange;
	short	SavedFloorPic;
	short	SavedCeilingPic;

	// [BC] Has the light level changed?
	bool	bLightChange;
	BYTE	SavedLightLevel;

	// [BC] Backup other numberous elements for resetting the map.
	FDynamicColormap	*SavedColorMap;
	float				SavedGravity;
	fixed_t				SavedFloorXOffset;
	fixed_t				SavedFloorYOffset;
	fixed_t				SavedCeilingXOffset;
	fixed_t				SavedCeilingYOffset;
	fixed_t				SavedFloorXScale;
	fixed_t				SavedFloorYScale;
	fixed_t				SavedCeilingXScale;
	fixed_t				SavedCeilingYScale;
	fixed_t				SavedFloorAngle;
	fixed_t				SavedCeilingAngle;
	fixed_t				SavedBaseFloorAngle;
	fixed_t				SavedBaseFloorYOffset;
	fixed_t				SavedBaseCeilingAngle;
	fixed_t				SavedBaseCeilingYOffset;
	fixed_t				SavedFriction;
	fixed_t				SavedMoveFactor;
	short				SavedSpecial;
	short				SavedDamage;
	short				SavedMOD;
	float				SavedCeilingReflect;
	float				SavedFloorReflect;
};

struct ReverbContainer;
struct zone_t
{
	ReverbContainer *Environment;
};


//
// The SideDef.
//

class DBaseDecal;

enum
{
	WALLF_ABSLIGHTING	= 1,	// Light is absolute instead of relative
	WALLF_NOAUTODECALS	= 2,	// Do not attach impact decals to this wall
	WALLF_AUTOCONTRAST	= 4,	// Automatically handle fake contrast in side_t::GetLightLevel
};

struct side_t
{
	enum ETexpart
	{
		top=0,
		mid=1,
		bottom=2
	};
	struct part
	{
		fixed_t xoffset;
		fixed_t yoffset;
		int texture;
		TObjPtr<DInterpolation> interpolation;
		//int Light;
	};

	sector_t*	sector;			// Sector the SideDef is facing.
	DBaseDecal*	AttachedDecals;	// [RH] Decals bound to the wall
	part		textures[3];
	DWORD		linenum;
	DWORD		LeftSide, RightSide;	// [RH] Group walls into loops
	WORD		TexelLength;
	SWORD		Light;
	BYTE		Flags;

	// [BC] Saved properties for when a map resets, or when we need to give updates
	// to new clients connecting.
	BYTE		SavedFlags;
	short		SavedTopTexture;
	short		SavedMidTexture;
	short		SavedBottomTexture;

	int GetLightLevel (bool foggy, int baselight) const;

	int GetTexture(int which) const
	{
		return textures[which].texture;
	}
	void SetTexture(int which, int tex)
	{
		textures[which].texture = tex;
	}

	void SetTextureXOffset(int which, fixed_t offset)
	{
		textures[which].xoffset = offset;
	}
	void SetTextureXOffset(fixed_t offset)
	{
		textures[top].xoffset =
		textures[mid].xoffset =
		textures[bottom].xoffset = offset;
	}
	fixed_t GetTextureXOffset(int which) const
	{
		return textures[which].xoffset;
	}
	void AddTextureXOffset(int which, fixed_t delta)
	{
		textures[which].xoffset += delta;
	}

	void SetTextureYOffset(int which, fixed_t offset)
	{
		textures[which].yoffset = offset;
	}
	void SetTextureYOffset(fixed_t offset)
	{
		textures[top].yoffset =
		textures[mid].yoffset =
		textures[bottom].yoffset = offset;
	}
	fixed_t GetTextureYOffset(int which) const
	{
		return textures[which].yoffset;
	}
	void AddTextureYOffset(int which, fixed_t delta)
	{
		textures[which].yoffset += delta;
	}

	DInterpolation *SetInterpolation(int position);
	void StopInterpolation(int position);
	//For GL
	FLightNode * lighthead[2];				// all blended lights that may affect this wall
};

FArchive &operator<< (FArchive &arc, side_t::part &p);

//
// Move clipping aid for LineDefs.
//
enum slopetype_t
{
	ST_HORIZONTAL,
	ST_VERTICAL,
	ST_POSITIVE,
	ST_NEGATIVE
};

#define	TEXCHANGE_FRONTTOP		1
#define	TEXCHANGE_FRONTMEDIUM	2
#define	TEXCHANGE_FRONTBOTTOM	4
#define	TEXCHANGE_BACKTOP		8
#define	TEXCHANGE_BACKMEDIUM	16
#define	TEXCHANGE_BACKBOTTOM	32

struct line_t
{
	vertex_t	*v1, *v2;	// vertices, from v1 to v2
	fixed_t 	dx, dy;		// precalculated v2 - v1 for side checking
	DWORD		flags;
	DWORD		activation;	// activation type
	int			special;
	fixed_t		Alpha;		// <--- translucency (0-255/255=opaque)
	int			id;			// <--- same as tag or set with Line_SetIdentification
	int			args[5];	// <--- hexen-style arguments (expanded to ZDoom's full width)
	int			firstid, nextid;
	DWORD		sidenum[2];	// sidenum[1] will be NO_SIDE if one sided
	fixed_t		bbox[4];	// bounding box, for the extent of the LineDef.
	slopetype_t	slopetype;	// To aid move clipping.
	sector_t	*frontsector, *backsector;
	int 		validcount;	// if == validcount, already checked

	// [BC] Have any of this line's textures been changed during the course of the level?
	ULONG		ulTexChangeFlags;

	// [BC] Saved properties for when a map resets, or when we need to give updates
	// to new clients connecting.
	int			SavedSpecial;
	int			SavedArgs[5];
	DWORD		SavedFlags;
	fixed_t		SavedAlpha;

};

// phares 3/14/98
//
// Sector list node showing all sectors an object appears in.
//
// There are two threads that flow through these nodes. The first thread
// starts at touching_thinglist in a sector_t and flows through the m_snext
// links to find all mobjs that are entirely or partially in the sector.
// The second thread starts at touching_sectorlist in a AActor and flows
// through the m_tnext links to find all sectors a thing touches. This is
// useful when applying friction or push effects to sectors. These effects
// can be done as thinkers that act upon all objects touching their sectors.
// As an mobj moves through the world, these nodes are created and
// destroyed, with the links changed appropriately.
//
// For the links, NULL means top or end of list.

struct msecnode_t
{
	sector_t			*m_sector;	// a sector containing this object
	AActor				*m_thing;	// this object
	struct msecnode_t	*m_tprev;	// prev msecnode_t for this thing
	struct msecnode_t	*m_tnext;	// next msecnode_t for this thing
	struct msecnode_t	*m_sprev;	// prev msecnode_t for this sector
	struct msecnode_t	*m_snext;	// next msecnode_t for this sector
	bool visited;	// killough 4/4/98, 4/7/98: used in search algorithms
};

//
// A SubSector.
// References a Sector.
// Basically, this is a list of LineSegs indicating the visible walls that
// define (all or some) sides of a convex BSP leaf.
//
struct FPolyObj;
struct subsector_t
{
	sector_t	*sector;
	DWORD		numlines;
	DWORD		firstline;
	FPolyObj	*poly;
	int			validcount;
	fixed_t		CenterX, CenterY;

	// subsector related GL data
	FLightNode *	lighthead[2];	// Light nodes (blended and additive)
	sector_t *		render_sector;	// The sector this belongs to for rendering
	int				firstvertex;	// index into the gl_vertices array
	int				numvertices;
	int				validcount2;	// Second v
	fixed_t			bbox[4];		// Bounding box
	bool			degenerate;
	char			hacked;			// 1: is part of a render hack
									// 2: has one-sided walls
};

//
// The LineSeg.
//
struct seg_t
{
	vertex_t*	v1;
	vertex_t*	v2;
	
	side_t* 	sidedef;
	line_t* 	linedef;

	// Sector references. Could be retrieved from linedef, too.
	sector_t*		frontsector;
	sector_t*		backsector;		// NULL for one-sided lines

	subsector_t*	Subsector;
	seg_t*			PartnerSeg;

	BITFIELD		bPolySeg:1;
};

// ===== Polyobj data =====
struct FPolyObj
{
	int			numsegs;
	seg_t		**segs;
	int			numlines;
	line_t		**lines;
	int			numvertices;
	vertex_t	**vertices;
	fixed_t		startSpot[3];
	vertex_t	*originalPts;	// used as the base for the rotations
	vertex_t	*prevPts; 		// use to restore the old point values
	angle_t		angle;
	int			tag;			// reference tag assigned in HereticEd
	int			bbox[4];
	int			validcount;
	int			crush; 			// should the polyobj attempt to crush mobjs?
	bool		bHurtOnTouch;	// should the polyobj hurt anything it touches?
	int			seqType;
	fixed_t		size;			// polyobj size (area of POLY_AREAUNIT == size of FRACUNIT)
	DThinker	*specialdata;	// pointer to a thinker, if the poly is moving
	TObjPtr<DInterpolation> interpolation;

	~FPolyObj();
	DInterpolation *SetInterpolation();
	void StopInterpolation();

	// Has this polyobject moved at all? If so, we need to tell connecting clients of its new position.
	bool		bMoved;
	// [BB] Original start stop, necessary for GAME_ResetMap.
	fixed_t		SavedStartSpot[3];

	// Has this polyobject rotated at all? If so, we need to tell connecting clients of its new position.
	bool		bRotated;

	// Was the polyobject blocked the last time it tried to move?
	bool		bBlocked;
};


//
// BSP node.
//
struct node_t
{
	// Partition line.
	fixed_t		x;
	fixed_t		y;
	fixed_t		dx;
	fixed_t		dy;
	fixed_t		bbox[2][4];		// Bounding box for each child.
	union
	{
		void	*children[2];	// If bit 0 is set, it's a subsector.
		int		intchildren[2];	// Used by nodebuilder.
	};
};


struct polyblock_t
{
	FPolyObj *polyobj;
	struct polyblock_t *prev;
	struct polyblock_t *next;
};



// posts are runs of non masked source pixels
struct column_t
{
	BYTE		topdelta;		// -1 is the last post in a column
	BYTE		length; 		// length data bytes follows
};



//
// OTHER TYPES
//

typedef BYTE lighttable_t;	// This could be wider for >8 bit display.

// Patches.
// A patch holds one or more columns.
// Patches are used for sprites and all masked pictures, and we compose
// textures from the TEXTURE1/2 lists of patches.
struct patch_t
{ 
	SWORD			width;			// bounding box size 
	SWORD			height; 
	SWORD			leftoffset; 	// pixels to the left of origin 
	SWORD			topoffset;		// pixels below the origin 
	DWORD 			columnofs[8];	// only [width] used
	// the [0] is &columnofs[width] 
};

class FileReader;

// All FTextures present their data to the world in 8-bit format, but if
// the source data is something else, this is it.
enum FTextureFormat
{
	TEX_Pal,
	TEX_Gray,
	TEX_RGB,		// Actually ARGB
	TEX_DXT1,
	TEX_DXT2,
	TEX_DXT3,
	TEX_DXT4,
	TEX_DXT5,
};

class FNativeTexture;

// Base texture class
class FTexture
{
public:
	static FTexture *CreateTexture(int lumpnum, int usetype);
	virtual ~FTexture ();

	SWORD LeftOffset, TopOffset;

	BYTE WidthBits, HeightBits;

	fixed_t		xScale;
	fixed_t		yScale;

	char Name[9];
	BYTE UseType;	// This texture's primary purpose

	BYTE bNoDecals:1;		// Decals should not stick to texture
	BYTE bNoRemap0:1;		// Do not remap color 0 (used by front layer of parallax skies)
	BYTE bWorldPanning:1;	// Texture is panned in world units rather than texels
	BYTE bMasked:1;			// Texture (might) have holes
	BYTE bAlphaTexture:1;	// Texture is an alpha channel without color information
	BYTE bHasCanvas:1;		// Texture is based off FCanvasTexture
	BYTE bWarped:2;			// This is a warped texture. Used to avoid multiple warps on one texture
	BYTE bComplex:1;		// Will be used to mark extended MultipatchTextures that have to be
							// fully composited before subjected to any kinf of postprocessing instead of
							// doing it per patch.

	WORD Rotations;

	enum // UseTypes
	{
		TEX_Any,
		TEX_Wall,
		TEX_Flat,
		TEX_Sprite,
		TEX_WallPatch,
		TEX_Build,
		TEX_SkinSprite,
		TEX_Decal,
		TEX_MiscPatch,
		TEX_FontChar,
		TEX_Override,	// For patches between TX_START/TX_END
		TEX_Autopage,	// Automap background - used to enable the use of FAutomapTexture
		TEX_Null,
		TEX_FirstDefined,
	};

	struct Span
	{
		WORD TopOffset;
		WORD Length;	// A length of 0 terminates this column
	};

	// Returns a single column of the texture
	virtual const BYTE *GetColumn (unsigned int column, const Span **spans_out) = 0;

	// Returns the whole texture, stored in column-major order
	virtual const BYTE *GetPixels () = 0;
	
	virtual int CopyTrueColorPixels(FBitmap *bmp, int x, int y, int rotate=0, FCopyInfo *inf = NULL);
	int CopyTrueColorTranslated(FBitmap *bmp, int x, int y, int rotate, FRemapTable *remap, FCopyInfo *inf = NULL);
	virtual bool UseBasePalette();
	virtual int GetSourceLump() { return -1; }

	virtual void Unload () = 0;

	// Returns the native pixel format for this image
	virtual FTextureFormat GetFormat();

	// Returns a native 3D representation of the texture
	FNativeTexture *GetNative(bool wrapping);

	// Frees the native 3D representation of the texture
	void KillNative();

	// Fill the native texture buffer with pixel data for this image
	virtual void FillBuffer(BYTE *buff, int pitch, int height, FTextureFormat fmt);

	int GetWidth () { return Width; }
	int GetHeight () { return Height; }

	int GetScaledWidth () { int foo = (Width << 17) / xScale; return (foo >> 1) + (foo & 1); }
	int GetScaledHeight () { int foo = (Height << 17) / yScale; return (foo >> 1) + (foo & 1); }

	int GetScaledLeftOffset () { int foo = (LeftOffset << 17) / xScale; return (foo >> 1) + (foo & 1); }
	int GetScaledTopOffset () { int foo = (TopOffset << 17) / yScale; return (foo >> 1) + (foo & 1); }

	virtual void SetFrontSkyLayer();

	void CopyToBlock (BYTE *dest, int dwidth, int dheight, int x, int y, const BYTE *translation=NULL)
	{
		CopyToBlock(dest, dwidth, dheight, x, y, 0, translation);
	}

	void CopyToBlock (BYTE *dest, int dwidth, int dheight, int x, int y, int rotate, const BYTE *translation=NULL);

	// Returns true if the next call to GetPixels() will return an image different from the
	// last call to GetPixels(). This should be considered valid only if a call to CheckModified()
	// is immediately followed by a call to GetPixels().
	virtual bool CheckModified ();

	static void InitGrayMap();

	void CopySize(FTexture *BaseTexture)
	{
		Width = BaseTexture->GetWidth();
		Height = BaseTexture->GetHeight();
		TopOffset = BaseTexture->TopOffset;
		LeftOffset = BaseTexture->LeftOffset;
		WidthBits = BaseTexture->WidthBits;
		HeightBits = BaseTexture->HeightBits;
		xScale = BaseTexture->xScale;
		yScale = BaseTexture->yScale;
		WidthMask = (1 << WidthBits) - 1;
	}

	void SetScaledSize(int fitwidth, int fitheight)
	{
		xScale = DivScale16(Width, fitwidth);
		yScale = DivScale16(Height,fitheight);
		// compensate for roundoff errors
		if (MulScale16(xScale, fitwidth) != Width) xScale++;
		if (MulScale16(yScale, fitheight) != Height) yScale++;
	}

	virtual void HackHack (int newheight);	// called by FMultipatchTexture to discover corrupt patches.

protected:
	WORD Width, Height, WidthMask;
	static BYTE GrayMap[256];
	FNativeTexture *Native;

	FTexture ();

	Span **CreateSpans (const BYTE *pixels) const;
	void FreeSpans (Span **spans) const;
	void CalcBitSize ();

	static void FlipSquareBlock (BYTE *block, int x, int y);
	static void FlipSquareBlockRemap (BYTE *block, int x, int y, const BYTE *remap);
	static void FlipNonSquareBlock (BYTE *blockto, const BYTE *blockfrom, int x, int y, int srcpitch);
	static void FlipNonSquareBlockRemap (BYTE *blockto, const BYTE *blockfrom, int x, int y, int srcpitch, const BYTE *remap);

	friend class D3DTex;

public:

	struct FBrightmapInfo
	{
		FTexture *Brightmap;
		signed char bIsBrightmap;
		bool bBrightmapDisablesFullbright;

		FBrightmapInfo()
		{
			Brightmap = NULL;
			bIsBrightmap = false;
			bBrightmapDisablesFullbright = false;
		}

		~FBrightmapInfo()
		{
			if (Brightmap) delete Brightmap;
			Brightmap = NULL;
		}
	};

	virtual void PrecacheGL();
	virtual void UncacheGL();
	virtual bool IsSkybox() const { return false; }
	FBrightmapInfo bm_info;
};

// Texture manager
class FTextureManager
{
public:
	FTextureManager ();
	~FTextureManager ();

	// Get texture without translation
	FTexture *operator[] (int texnum)
	{
		if ((size_t)texnum >= Textures.Size()) return NULL;
		return Textures[texnum].Texture;
	}
	FTexture *operator[] (const char *texname)
	{
		int texnum = GetTexture (texname, FTexture::TEX_MiscPatch);
		if (texnum==-1) return NULL;
		return Textures[texnum].Texture;
	}
	FTexture *FindTexture(const char *texname, int usetype = FTexture::TEX_MiscPatch, BITFIELD flags = TEXMAN_TryAny);

	// Get texture with translation
	FTexture *operator() (int texnum)
	{
		if ((size_t)texnum >= Textures.Size()) return NULL;
		return Textures[Translation[texnum]].Texture;
	}
	FTexture *operator() (const char *texname)
	{
		int texnum = GetTexture (texname, FTexture::TEX_MiscPatch);
		if (texnum==-1) return NULL;
		return Textures[Translation[texnum]].Texture;
	}

	void SetTranslation (int fromtexnum, int totexnum)
	{
		if ((size_t)fromtexnum < Translation.Size())
		{
			if ((size_t)totexnum >= Textures.Size())
			{
				totexnum = fromtexnum;
			}
			Translation[fromtexnum] = totexnum;
		}
	}

	enum
	{
		TEXMAN_TryAny = 1,
		TEXMAN_Overridable = 2,
		TEXMAN_ReturnFirst = 4,
	};

	int CheckForTexture (const char *name, int usetype, BITFIELD flags=TEXMAN_TryAny);
	int GetTexture (const char *name, int usetype, BITFIELD flags=0);
	int ListTextures (const char *name, TArray<int> &list);

	void WriteTexture (FArchive &arc, int picnum);
	int ReadTexture (FArchive &arc);

	void AddTexturesLump (const void *lumpdata, int lumpsize, int deflumpnum, int patcheslump, int firstdup=0, bool texture1=false);
	void AddTexturesLumps (int lump1, int lump2, int patcheslump);
	void AddGroup(int wadnum, const char * startlump, const char * endlump, int ns, int usetype);
	void AddPatches (int lumpnum);
	void AddTiles (void *tileFile);
	void AddHiresTextures (int wadnum);
	void LoadTextureDefs(int wadnum, const char *lumpname);
	void ParseXTexture(FScanner &sc, int usetype);
	void SortTexturesByType(int start, int end);

	int CreateTexture (int lumpnum, int usetype=FTexture::TEX_Any);	// Also calls AddTexture
	int AddTexture (FTexture *texture);
	int AddPatch (const char *patchname, int namespc=0, bool tryany = false);

	void LoadTextureX(int wadnum);
	void AddTexturesForWad(int wadnum);
	void Init();

	// Replaces one texture with another. The new texture will be assigned
	// the same name, slot, and use type as the texture it is replacing.
	// The old texture will no longer be managed. Set free true if you want
	// the old texture to be deleted or set it false if you want it to
	// be left alone in memory. You will still need to delete it at some
	// point, because the texture manager no longer knows about it.
	// This function can be used for such things as warping textures.
	void ReplaceTexture (int picnum, FTexture *newtexture, bool free);

	void UnloadAll ();

	int NumTextures () const { return (int)Textures.Size(); }


private:
	struct TextureHash
	{
		FTexture *Texture;
		int HashNext;
	};
	enum { HASH_END = -1, HASH_SIZE = 1027 };
	TArray<TextureHash> Textures;
	TArray<int> Translation;
	int HashFirst[HASH_SIZE];
	int DefaultTexture;
};

extern FTextureManager TexMan;


// A vissprite_t is a thing
//	that will be drawn during a refresh.
// I.e. a sprite object that is partly visible.
struct vissprite_t
{
	short			x1, x2;
	fixed_t			cx;				// for line side calculation
	fixed_t			gx, gy;			// for fake floor clipping
	fixed_t			gz, gzt;		// global bottom / top for silhouette clipping
	fixed_t			startfrac;		// horizontal position of x1
	fixed_t			xscale, yscale;
	fixed_t			xiscale;		// negative if flipped
	fixed_t			idepth;			// 1/z
	fixed_t			texturemid;
	DWORD			FillColor;
	lighttable_t	*colormap;
	sector_t		*heightsec;		// killough 3/27/98: height sector for underwater/fake ceiling
	sector_t		*sector;		// [RH] sector this sprite is in
	fixed_t			alpha;
	fixed_t			floorclip;
	FTexture		*pic;
	short 			renderflags;
	DWORD			Translation;	// [RH] for color translation
	FRenderStyle	RenderStyle;
	BYTE			FakeFlatStat;	// [RH] which side of fake/floor ceiling sprite is on
	BYTE			bSplitSprite;	// [RH] Sprite was split by a drawseg
};

enum
{
	FAKED_Center,
	FAKED_BelowFloor,
	FAKED_AboveCeiling
};

//
// Sprites are patches with a special naming convention so they can be
// recognized by R_InitSprites. The base name is NNNNFx or NNNNFxFx, with
// x indicating the rotation, x = 0, 1-7. The sprite and frame specified
// by a thing_t is range checked at run time.
// A sprite is a patch_t that is assumed to represent a three dimensional
// object and may have multiple rotations pre drawn. Horizontal flipping
// is used to save space, thus NNNNF2F5 defines a mirrored patch.
// Some sprites will only have one picture used for all views: NNNNF0
//
struct spriteframe_t
{
	WORD Texture[16];	// texture to use for view angles 0-15
	WORD Flip;			// flip (1 = flip) to use for view angles 0-15.
};

//
// A sprite definition:
//	a number of animation frames.
//
struct spritedef_t
{
	char name[5];
	BYTE numframes;
	WORD spriteframes;
};

extern TArray<spriteframe_t> SpriteFrames;

//
// [RH] Internal "skin" definition.
//
class FPlayerSkin
{
public:
	// [BC] Changed to MAX_SKIN_NAME.
	char		name[MAX_SKIN_NAME+1];	// MAX_SKIN_NAME chars + NULL
	char		face[4];	// 3 chars ([MH] + NULL so can use as a C string)
	BYTE		gender;		// This skin's gender (not really used)
	BYTE		range0start;
	BYTE		range0end;
	bool		othergame;	// [GRB]
	fixed_t		ScaleX;
	fixed_t		ScaleY;
	int			sprite;
	int			crouchsprite;
	int			namespc;	// namespace for this skin

	// [BC] New skin properties for Skulltag.
	// Default color used for this skin.
	char		szColor[16];

	// Can this skin be selected from the menu?
	bool		bRevealed;

	// Is this skin hidden by default?
	bool		bRevealedByDefault;

	// Is this skin a cheat skin?
	bool		bCheat;
	// [BC] End of new skin properties.
};

#endif
