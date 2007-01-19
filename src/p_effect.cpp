/*
** p_effect.cpp
** Particle effects
**
**---------------------------------------------------------------------------
** Copyright 1998-2006 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
** If particles used real sprites instead of blocks, they could be much
** more useful.
*/

#include "doomtype.h"
#include "doomstat.h"
#include "c_cvars.h"
#include "actor.h"
#include "p_effect.h"
#include "p_local.h"
#include "g_level.h"
#include "v_video.h"
#include "m_random.h"
#include "r_defs.h"
#include "r_things.h"
#include "s_sound.h"
#include "templates.h"

CVAR (Int, cl_rockettrails, 1, CVAR_ARCHIVE);

#define FADEFROMTTL(a)	(255/(a))

static int grey1, grey2, grey3, grey4, red, green, blue, yellow, black,
		   red1, green1, blue1, yellow1, purple, purple1, white,
		   rblue1, rblue2, rblue3, rblue4, orange, yorange, dred, grey5,
		   maroon1, maroon2;

static const struct ColorList {
	int *color;
	BYTE r, g, b;
} Colors[] = {
	{&grey1,	85,  85,  85 },
	{&grey2,	171, 171, 171},
	{&grey3,	50,  50,  50 },
	{&grey4,	210, 210, 210},
	{&grey5,	128, 128, 128},
	{&red,		255, 0,   0  },  
	{&green,	0,   200, 0  },  
	{&blue,		0,   0,   255},
	{&yellow,	255, 255, 0  },  
	{&black,	0,   0,   0  },  
	{&red1,		255, 127, 127},
	{&green1,	127, 255, 127},
	{&blue1,	127, 127, 255},
	{&yellow1,	255, 255, 180},
	{&purple,	120, 0,   160},
	{&purple1,	200, 30,  255},
	{&white, 	255, 255, 255},
	{&rblue1,	81,  81,  255},
	{&rblue2,	0,   0,   227},
	{&rblue3,	0,   0,   130},
	{&rblue4,	0,   0,   80 },
	{&orange,	255, 120, 0  },
	{&yorange,	255, 170, 0  },
	{&dred,		80,  0,   0  },
	{&maroon1,	154, 49,  49 },
	{&maroon2,	125, 24,  24 },
	{NULL}
};

void P_InitEffects ()
{
	const struct ColorList *color = Colors;

	while (color->color)
	{
		*(color->color) = ColorMatcher.Pick (color->r, color->g, color->b);
		color++;
	}
}

void P_ThinkParticles ()
{
	int i;
	particle_t *particle, *prev;

	i = ActiveParticles;
	prev = NULL;
	while (i != NO_PARTICLE)
	{
		BYTE oldtrans;

		particle = Particles + i;
		i = particle->tnext;
		oldtrans = particle->trans;
		particle->trans -= particle->fade;
		if (oldtrans < particle->trans || --particle->ttl == 0)
		{ // The particle has expired, so free it
			memset (particle, 0, sizeof(particle_t));
			if (prev)
				prev->tnext = i;
			else
				ActiveParticles = i;
			particle->tnext = InactiveParticles;
			InactiveParticles = (int)(particle - Particles);
			continue;
		}
		particle->x += particle->velx;
		particle->y += particle->vely;
		particle->z += particle->velz;
		particle->velx += particle->accx;
		particle->vely += particle->accy;
		particle->velz += particle->accz;
		prev = particle;
	}
}

//
// P_RunEffects
//
// Run effects on all actors in the world
//
void P_RunEffects ()
{
	if (players[consoleplayer].camera == NULL) return;

	int	pnum = int(players[consoleplayer].camera->Sector - sectors) * numsectors;

	AActor *actor;
	TThinkerIterator<AActor> iterator;

	while ( (actor = iterator.Next ()) )
	{
		if (actor->effects)
		{
			// Only run the effect if the actor is potentially visible
			int rnum = pnum + int(actor->Sector - sectors);
			if (rejectmatrix == NULL || !(rejectmatrix[rnum>>3] & (1 << (rnum & 7))))
				P_RunEffect (actor, actor->effects);
		}
	}
}

//
// AddParticle
//
// Creates a particle with "jitter"
//
particle_t *JitterParticle (int ttl)
{
	particle_t *particle = NewParticle ();

	if (particle) {
		fixed_t *val = &particle->velx;
		int i;

		// Set initial velocities
		for (i = 3; i; i--, val++)
			*val = (FRACUNIT/4096) * (M_Random () - 128);
		// Set initial accelerations
		for (i = 3; i; i--, val++)
			*val = (FRACUNIT/16384) * (M_Random () - 128);

		particle->trans = 255;	// fully opaque
		particle->ttl = ttl;
		particle->fade = FADEFROMTTL(ttl);
	}
	return particle;
}

static void MakeFountain (AActor *actor, int color1, int color2)
{
	particle_t *particle;

	if (!(level.time & 1))
		return;

	particle = JitterParticle (51);

	if (particle)
	{
		angle_t an = M_Random()<<(24-ANGLETOFINESHIFT);
		fixed_t out = FixedMul (actor->radius, M_Random()<<8);

		particle->x = actor->x + FixedMul (out, finecosine[an]);
		particle->y = actor->y + FixedMul (out, finesine[an]);
		particle->z = actor->z + actor->height + FRACUNIT;
		if (out < actor->radius/8)
			particle->velz += FRACUNIT*10/3;
		else
			particle->velz += FRACUNIT*3;
		particle->accz -= FRACUNIT/11;
		if (M_Random() < 30) {
			particle->size = 4;
			particle->color = color2;
		} else {
			particle->size = 6;
			particle->color = color1;
		}
	}
}

void P_RunEffect (AActor *actor, int effects)
{
	angle_t moveangle = R_PointToAngle2(0,0,actor->momx,actor->momy);
	particle_t *particle;
	int i;

	if ((effects & FX_ROCKET) && cl_rockettrails)
	{
		// Rocket trail

		fixed_t backx = actor->x - FixedMul (finecosine[(moveangle)>>ANGLETOFINESHIFT], actor->radius*2);
		fixed_t backy = actor->y - FixedMul (finesine[(moveangle)>>ANGLETOFINESHIFT], actor->radius*2);
		fixed_t backz = actor->z - (actor->height>>3) * (actor->momz>>16) + (2*actor->height)/3;

		angle_t an = (moveangle + ANG90) >> ANGLETOFINESHIFT;
		int speed;

		particle = JitterParticle (3 + (M_Random() & 31));
		if (particle) {
			fixed_t pathdist = M_Random()<<8;
			particle->x = backx - FixedMul(actor->momx, pathdist);
			particle->y = backy - FixedMul(actor->momy, pathdist);
			particle->z = backz - FixedMul(actor->momz, pathdist);
			speed = (M_Random () - 128) * (FRACUNIT/200);
			particle->velx += FixedMul (speed, finecosine[an]);
			particle->vely += FixedMul (speed, finesine[an]);
			particle->velz -= FRACUNIT/36;
			particle->accz -= FRACUNIT/20;
			particle->color = yellow;
			particle->size = 2;
		}
		for (i = 6; i; i--) {
			particle_t *particle = JitterParticle (3 + (M_Random() & 31));
			if (particle) {
				fixed_t pathdist = M_Random()<<8;
				particle->x = backx - FixedMul(actor->momx, pathdist);
				particle->y = backy - FixedMul(actor->momy, pathdist);
				particle->z = backz - FixedMul(actor->momz, pathdist) + (M_Random() << 10);
				speed = (M_Random () - 128) * (FRACUNIT/200);
				particle->velx += FixedMul (speed, finecosine[an]);
				particle->vely += FixedMul (speed, finesine[an]);
				particle->velz += FRACUNIT/80;
				particle->accz += FRACUNIT/40;
				if (M_Random () & 7)
					particle->color = grey2;
				else
					particle->color = grey1;
				particle->size = 3;
			} else
				break;
		}
	}
	if ((effects & FX_GRENADE) && (cl_rockettrails))
	{
		// Grenade trail

		P_DrawSplash2 (6,
			actor->x - FixedMul (finecosine[(moveangle)>>ANGLETOFINESHIFT], actor->radius*2),
			actor->y - FixedMul (finesine[(moveangle)>>ANGLETOFINESHIFT], actor->radius*2),
			actor->z - (actor->height>>3) * (actor->momz>>16) + (2*actor->height)/3,
			moveangle + ANG180, 2, 2);
	}
	if (effects & FX_FOUNTAINMASK)
	{
		// Particle fountain

		static const int *fountainColors[16] = 
			{ &black,	&black,
			  &red,		&red1,
			  &green,	&green1,
			  &blue,	&blue1,
			  &yellow,	&yellow1,
			  &purple,	&purple1,
			  &black,	&grey3,
			  &grey4,	&white
			};
		int color = (effects & FX_FOUNTAINMASK) >> 15;
		MakeFountain (actor, *fountainColors[color], *fountainColors[color+1]);
	}
	if (effects & FX_RESPAWNINVUL)
	{
		// Respawn protection

		static const int *protectColors[2] = { &yellow1, &white };

		for (i = 3; i > 0; i--)
		{
			particle = JitterParticle (16);
			if (particle != NULL)
			{
				angle_t ang = M_Random () << (32-ANGLETOFINESHIFT-8);
				particle->x = actor->x + FixedMul (actor->radius, finecosine[ang]);
				particle->y = actor->y + FixedMul (actor->radius, finesine[ang]);
				particle->color = *protectColors[M_Random() & 1];
				particle->z = actor->z;
				particle->velz = FRACUNIT;
				particle->accz = M_Random () << 7;
				particle->size = 1;
				if (M_Random () < 128)
				{ // make particle fall from top of actor
					particle->z += actor->height;
					particle->velz = -particle->velz;
					particle->accz = -particle->accz;
				}
			}
		}
	}
}

void P_DrawSplash (int count, fixed_t x, fixed_t y, fixed_t z, angle_t angle, int kind)
{
	int color1, color2;

	switch (kind)
	{
	case 1:		// Spark
		color1 = orange;
		color2 = yorange;
		break;
	default:
		return;
	}

	for (; count; count--)
	{
		particle_t *p = JitterParticle (10);
		angle_t an;

		if (!p)
			break;

		p->size = 2;
		p->color = M_Random() & 0x80 ? color1 : color2;
		p->velz -= M_Random () * 512;
		p->accz -= FRACUNIT/8;
		p->accx += (M_Random () - 128) * 8;
		p->accy += (M_Random () - 128) * 8;
		p->z = z - M_Random () * 1024;
		an = (angle + (M_Random() << 21)) >> ANGLETOFINESHIFT;
		p->x = x + (M_Random () & 15)*finecosine[an];
		p->y = y + (M_Random () & 15)*finesine[an];
	}
}

void P_DrawSplash2 (int count, fixed_t x, fixed_t y, fixed_t z, angle_t angle, int updown, int kind)
{
	int color1, color2, zvel, zspread, zadd;

	switch (kind)
	{
	case 0:		// Blood
		color1 = red;
		color2 = dred;
		break;
	case 1:		// Gunshot
		color1 = grey3;
		color2 = grey5;
		break;
	case 2:		// Smoke
		color1 = grey3;
		color2 = grey1;
		break;
	default:	// colorized blood
		color1 = ColorMatcher.Pick(RPART(kind), GPART(kind), BPART(kind));
		color2 = ColorMatcher.Pick(RPART(kind)>>1, GPART(kind)>>1, BPART(kind)>>1);
		break;
	}

	zvel = -128;
	zspread = updown ? -6000 : 6000;
	zadd = (updown == 2) ? -128 : 0;

	for (; count; count--)
	{
		particle_t *p = NewParticle ();
		angle_t an;

		if (!p)
			break;

		p->ttl = 12;
		p->fade = FADEFROMTTL(12);
		p->trans = 255;
		p->size = 4;
		p->color = M_Random() & 0x80 ? color1 : color2;
		p->velz = M_Random () * zvel;
		p->accz = -FRACUNIT/22;
		if (kind) {
			an = (angle + ((M_Random() - 128) << 23)) >> ANGLETOFINESHIFT;
			p->velx = (M_Random () * finecosine[an]) >> 11;
			p->vely = (M_Random () * finesine[an]) >> 11;
			p->accx = p->velx >> 4;
			p->accy = p->vely >> 4;
		}
		p->z = z + (M_Random () + zadd - 128) * zspread;
		an = (angle + ((M_Random() - 128) << 22)) >> ANGLETOFINESHIFT;
		p->x = x + ((M_Random () & 31)-15)*finecosine[an];
		p->y = y + ((M_Random () & 31)-15)*finesine[an];
	}
}

void P_DrawRailTrail (AActor *source, const FVector3 &start, const FVector3 &end, int color1, int color2, float maxdiff, bool silent)
{
	double length, lengthsquared;
	int steps, i;
	FAngle deg;
	FVector3 step, dir, pos, extend;

	dir = end - start;
	lengthsquared = dir | dir;
	length = sqrt(lengthsquared);
	steps = int(length / 3);

	if (steps)
	{
		if (!silent)
		{
			int sound;
			
			// Allow other sounds than 'weapons/railgf'!
			if (!source->player) sound = source->AttackSound;
			else if (source->player->ReadyWeapon) sound = source->player->ReadyWeapon->AttackSound;
			else sound = 0;
			if (!sound) sound=S_FindSound("weapons/railgf");

			// The railgun's sound is special. It gets played from the
			// point on the slug's trail that is closest to the hearing player.
			AActor *mo = players[consoleplayer].camera;
			FVector3 point;
			double r;
			float dirz;

			if (abs(mo->x - FLOAT2FIXED(start.X)) < 20 * FRACUNIT
				&& (mo->y - FLOAT2FIXED(start.Y)) < 20 * FRACUNIT)
			{ // This player (probably) fired the railgun
				S_SoundID (mo, CHAN_WEAPON, sound, 1, ATTN_NORM);
			}
			else
			{
				// Only consider sound in 2D (for now, anyway)
				r = ((start.Y - FIXED2FLOAT(mo->y)) * (-dir.Y) -
					(start.X - FIXED2FLOAT(mo->x)) * (dir.X)) * length * length;

				dirz = dir.Z;
				dir.Z = 0;
				point = start + r * dir;
				dir.Z = dirz;

				S_SoundID (FLOAT2FIXED(point.X), FLOAT2FIXED(point.Y), mo->z,
					CHAN_WEAPON, sound, 1, ATTN_NORM);
			}
		}
	}
	else
	{
		// line is 0 length, so nothing to do
		return;
	}

	dir /= length;

	//Calculate PerpendicularVector (extend, dir):
	double minelem = 1;
	int epos;
	for (epos = 0, i = 0; i < 3; ++i)
	{
		if (fabs(dir[i]) < minelem)
		{
			epos = i;
			minelem = fabs(dir[i]);
		}
	}
	FVector3 tempvec(0,0,0);
	tempvec[epos] = 1;
	extend = tempvec - (dir | tempvec) * dir;
	//

	extend *= 3;
	step = dir * 3;

	// Create the outer spiral.
	if (color1 != -1)
	{
		color1 = color1 == 0 ? -1 : ColorMatcher.Pick(RPART(color1), GPART(color1), BPART(color1));
		pos = start;
		deg = FAngle(270);
		for (i = steps; i; i--)
		{
			particle_t *p = NewParticle ();
			FVector3 tempvec;

			if (!p)
				return;

			p->trans = 255;
			p->ttl = 35;
			p->fade = FADEFROMTTL(35);
			p->size = 3;

			tempvec = FMatrix3x3(dir, deg) * extend;
			p->velx = FLOAT2FIXED(tempvec.X)>>4;
			p->vely = FLOAT2FIXED(tempvec.Y)>>4;
			p->velz = FLOAT2FIXED(tempvec.Z)>>4;
			tempvec += pos;
			p->x = FLOAT2FIXED(tempvec.X);
			p->y = FLOAT2FIXED(tempvec.Y);
			p->z = FLOAT2FIXED(tempvec.Z);
			pos += step;
			deg += FAngle(14);

			if (color1 == -1)
			{
				int rand = M_Random();

				if (rand < 155)
					p->color = rblue2;
				else if (rand < 188)
					p->color = rblue1;
				else if (rand < 222)
					p->color = rblue3;
				else
					p->color = rblue4;
			}
			else 
			{
				p->color = color1;
			}
		}
	}

	// Create the inner trail.
	if (color2 != -1)
	{
		color2 = color2 == 0 ? -1 : ColorMatcher.Pick(RPART(color2), GPART(color2), BPART(color2));
		FVector3 diff(0, 0, 0);

		pos = start;
		for (i = steps; i; i--)
		{
			particle_t *p = JitterParticle (33);

			if (!p)
				return;

			if (maxdiff > 0)
			{
				int rnd = M_Random ();
				if (rnd & 1)
					diff.X = clamp<float> (diff.X + ((rnd & 8) ? 1 : -1), -maxdiff, maxdiff);
				if (rnd & 2)
					diff.Y = clamp<float> (diff.Y + ((rnd & 16) ? 1 : -1), -maxdiff, maxdiff);
				if (rnd & 4)
					diff.Z = clamp<float> (diff.Z + ((rnd & 32) ? 1 : -1), -maxdiff, maxdiff);
			}

			FVector3 postmp = pos + diff;

			p->size = 2;
			p->x = FLOAT2FIXED(postmp.X);
			p->y = FLOAT2FIXED(postmp.Y);
			p->z = FLOAT2FIXED(postmp.Z);
			if (color1 != -1)
				p->accz -= FRACUNIT/4096;
			pos += step;

			if (color2 == -1)
			{
				int rand = M_Random();

				if (rand < 85)
					p->color = grey4;
				else if (rand < 170)
					p->color = grey2;
				else
					p->color = grey1;
			}
			else 
			{
				p->color = color2;
			}
		}
	}
}

void P_DisconnectEffect (AActor *actor)
{
	int i;

	if (actor == NULL)
		return;

	for (i = 64; i; i--)
	{
		particle_t *p = JitterParticle (TICRATE*2);

		if (!p)
			break;

		p->x = actor->x + ((M_Random()-128)<<9) * (actor->radius>>FRACBITS);
		p->y = actor->y + ((M_Random()-128)<<9) * (actor->radius>>FRACBITS);
		p->z = actor->z + (M_Random()<<8) * (actor->height>>FRACBITS);
		p->accz -= FRACUNIT/4096;
		p->color = M_Random() < 128 ? maroon1 : maroon2;
		p->size = 4;
	}
}
