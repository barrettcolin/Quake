#include "quakedef.h"
#include "r_local.h"

extern int lightleft, sourcesstep, blocksize, sourcetstep;
extern int lightright, lightleftstep, lightrightstep, blockdivshift;
extern void *prowdestbase;
extern unsigned char *pbasesource;
extern int surfrowbytes;
extern unsigned *r_lightptr;
extern int r_stepback;
extern int r_lightwidth;
extern int r_numvblocks;
extern unsigned char *r_sourcemax;

// armv6 C sandbox for surf8.S routines
#if idarm

/*
================
R_DrawSurfaceBlock8_mip0
================
*/
void R_DrawSurfaceBlock8_mip0 (void)
{
	int				v, i, b, lightstep, lighttemp, light;
	unsigned char	pix, *psource, *prowdest;

	psource = pbasesource;
	prowdest = prowdestbase;

	for (v=0 ; v<r_numvblocks ; v++)
	{
	// FIXME: make these locals?
	// FIXME: use delta rather than both right and left, like ASM?
		lightleft = r_lightptr[0];
		lightright = r_lightptr[1];
		r_lightptr += r_lightwidth;
		lightleftstep = (r_lightptr[0] - lightleft) >> 4;
		lightrightstep = (r_lightptr[1] - lightright) >> 4;

		for (i=0 ; i<16 ; i++)
		{
			lighttemp = lightright - lightleft;
			lightstep = lighttemp >> 4;

			light = lightleft;

			for (b=0; b < 4; b++)
			{
				unsigned int out4 = ((unsigned char *)vid.colormap)[(light & 0xFF00) + psource[b*4]];
				light += lightstep;
				out4 |= (((unsigned char *)vid.colormap)[(light & 0xFF00) + psource[b*4+1]] << 8);
				light += lightstep;
				out4 |= (((unsigned char *)vid.colormap)[(light & 0xFF00) + psource[b*4+2]] << 16);
				light += lightstep;
				out4 |= (((unsigned char *)vid.colormap)[(light & 0xFF00) + psource[b*4+3]] << 24);
				light += lightstep;

				((unsigned int *)prowdest)[b] = out4;
			}
	
			psource += sourcetstep;
			lightright += lightrightstep;
			lightleft += lightleftstep;
			prowdest += surfrowbytes;
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}


/*
================
R_DrawSurfaceBlock8_mip1
================
*/
void R_DrawSurfaceBlock8_mip1 (void)
{
	int				v, i, b, lightstep, lighttemp, light;
	unsigned char	pix, *psource, *prowdest;

	psource = pbasesource;
	prowdest = prowdestbase;

	for (v=0 ; v<r_numvblocks ; v++)
	{
	// FIXME: make these locals?
	// FIXME: use delta rather than both right and left, like ASM?
		lightleft = r_lightptr[0];
		lightright = r_lightptr[1];
		r_lightptr += r_lightwidth;
		lightleftstep = (r_lightptr[0] - lightleft) >> 3;
		lightrightstep = (r_lightptr[1] - lightright) >> 3;

		for (i=0 ; i<8 ; i++)
		{
			lighttemp = lightright - lightleft;
			lightstep = lighttemp >> 3;

			light = lightleft;

			for (b=0; b<2; b++)
			{
				unsigned int out4 = ((unsigned char *)vid.colormap)[(light & 0xFF00) + psource[b*4]];
				light += lightstep;
				out4 |= (((unsigned char *)vid.colormap)[(light & 0xFF00) + psource[b*4+1]] << 8);
				light += lightstep;
				out4 |= (((unsigned char *)vid.colormap)[(light & 0xFF00) + psource[b*4+2]] << 16);
				light += lightstep;
				out4 |= (((unsigned char *)vid.colormap)[(light & 0xFF00) + psource[b*4+3]] << 24);
				light += lightstep;

				((unsigned int *)prowdest)[b] = out4;
			}

			psource += sourcetstep;
			lightright += lightrightstep;
			lightleft += lightleftstep;
			prowdest += surfrowbytes;
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}


/*
================
R_DrawSurfaceBlock8_mip2
================
*/
void R_DrawSurfaceBlock8_mip2 (void)
{
	int				v, i, b, lightstep, lighttemp, light;
	unsigned char	pix, *psource, *prowdest;

	psource = pbasesource;
	prowdest = prowdestbase;

	for (v=0 ; v<r_numvblocks ; v++)
	{
	// FIXME: make these locals?
	// FIXME: use delta rather than both right and left, like ASM?
		lightleft = r_lightptr[0];
		lightright = r_lightptr[1];
		r_lightptr += r_lightwidth;
		lightleftstep = (r_lightptr[0] - lightleft) >> 2;
		lightrightstep = (r_lightptr[1] - lightright) >> 2;

		for (i=0 ; i<4 ; i++)
		{
			unsigned int out4;

			lighttemp = lightright - lightleft;
			lightstep = lighttemp >> 3;

			light = lightleft;

			out4 = ((unsigned char *)vid.colormap)[(light & 0xFF00) + psource[b*4]];
			light += lightstep;
			out4 |= (((unsigned char *)vid.colormap)[(light & 0xFF00) + psource[b*4+1]] << 8);
			light += lightstep;
			out4 |= (((unsigned char *)vid.colormap)[(light & 0xFF00) + psource[b*4+2]] << 16);
			light += lightstep;
			out4 |= (((unsigned char *)vid.colormap)[(light & 0xFF00) + psource[b*4+3]] << 24);
			light += lightstep;

			*((unsigned int *)prowdest) = out4;

			psource += sourcetstep;
			lightright += lightrightstep;
			lightleft += lightleftstep;
			prowdest += surfrowbytes;
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}


/*
================
R_DrawSurfaceBlock8_mip3
================
*/
void R_DrawSurfaceBlock8_mip3 (void)
{
	int				v, i, b, lightstep, lighttemp, light;
	unsigned char	pix, *psource, *prowdest;

	psource = pbasesource;
	prowdest = prowdestbase;

	for (v=0 ; v<r_numvblocks ; v++)
	{
	// FIXME: make these locals?
	// FIXME: use delta rather than both right and left, like ASM?
		lightleft = r_lightptr[0];
		lightright = r_lightptr[1];
		r_lightptr += r_lightwidth;
		lightleftstep = (r_lightptr[0] - lightleft) >> 1;
		lightrightstep = (r_lightptr[1] - lightright) >> 1;

		for (i=0 ; i<2 ; i++)
		{
			lighttemp = lightleft - lightright;
			lightstep = lighttemp >> 1;

			light = lightright;

			for (b=1; b>=0; b--)
			{
				pix = psource[b];
				prowdest[b] = ((unsigned char *)vid.colormap)
						[(light & 0xFF00) + pix];
				light += lightstep;
			}
	
			psource += sourcetstep;
			lightright += lightrightstep;
			lightleft += lightleftstep;
			prowdest += surfrowbytes;
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}


/*
================
R_DrawSurfaceBlock16

FIXME: make this work
================
*/
void R_DrawSurfaceBlock16 (void)
{
	int				k;
	unsigned char	*psource;
	int				lighttemp, lightstep, light;
	unsigned short	*prowdest;

	prowdest = (unsigned short *)prowdestbase;

	for (k=0 ; k<blocksize ; k++)
	{
		unsigned short	*pdest;
		unsigned char	pix;
		int				b;

		psource = pbasesource;
		lighttemp = lightright - lightleft;
		lightstep = lighttemp >> blockdivshift;

		light = lightleft;
		pdest = prowdest;

		for (b=0; b<blocksize; b++)
		{
			pix = *psource;
			*pdest = vid.colormap16[(light & 0xFF00) + pix];
			psource += sourcesstep;
			pdest++;
			light += lightstep;
		}

		pbasesource += sourcetstep;
		lightright += lightrightstep;
		lightleft += lightleftstep;
		prowdest = (unsigned short *)((long)prowdest + surfrowbytes);
	}

	prowdestbase = prowdest;
}

#endif

