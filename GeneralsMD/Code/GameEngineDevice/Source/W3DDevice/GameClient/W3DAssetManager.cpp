/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

////////////////////////////////////////////////////////////////////////////////
//																																						//
//  (c) 2001-2003 Electronic Arts Inc.																				//
//																																						//
////////////////////////////////////////////////////////////////////////////////

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : W3DAssetManager                                                  *
 *                                                                                             *
 *                     $Archive::                                                             $*
 *                                                                                             *
 *              Original Author:: Hector Yee (Enbassetmgr)
 * 			     Added/Modified:: Mark Wilczynski
 *                                                                                             *
 *                      $Author::                                                             $*
 *                                                                                             *
 *                     $Modtime::                                                             $*
 *                                                                                             *
 *                    $Revision::                                                             $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include <WWLib/always.h>
#include "W3DDevice/GameClient/W3DAssetManager.h"
#include "WW3D2/proto.h"
#include "WW3D2/rendobj.h"
#include <WWMath/vector3.h>
#include "WW3D2/mesh.h"
#include "WW3D2/hlod.h"
#include "WW3D2/matinfo.h"
#include "WW3D2/meshmdl.h"
#include "WW3D2/part_emt.h"
#include "WW3D2/vertmaterial.h"
#include "WW3D2/dx8wrapper.h"
#include "WW3D2/texture.h"
#include "WW3D2/surfaceclass.h"
#include "WW3D2/textureloader.h"
#include "WW3D2/ww3dformat.h"
#include "WW3D2/colorspace.h"
#include <WWDebug/wwprofile.h>
#include "WWDebug/wwmemlog.h"
#include "WWLib/ffactory.h"
#include "WW3D2/font3d.h"
#include "WW3D2/render2dsentence.h"
#include "Common/PerfTimer.h"
#include "Common/GlobalData.h"
#include "Common/GameCommon.h"


//---------------------------------------------------------------------
// Constants
//---------------------------------------------------------------------

const float ident_scale(1.0f);
const float scale_epsilon(0.01f);
const Vector3 ident_HSV(0,0,0);
const float H_epsilon(1.0f);
const float S_epsilon(0.01f);
const float V_epsilon(0.01f);

//---------------------------------------------------------------------
// Externs defined somewhere in W3D.
//---------------------------------------------------------------------

void Convert_Pixel(Vector3 &rgb, const SurfaceClass::SurfaceDescription &sd, const unsigned char * pixel);
void Convert_Pixel(unsigned char * pixel,const SurfaceClass::SurfaceDescription &sd, const Vector3 &rgb);

//---------------------------------------------------------------------
// W3DPrototype
//---------------------------------------------------------------------

//---------------------------------------------------------------------
class W3DPrototypeClass : public MemoryPoolObject, public PrototypeClass
{
	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( W3DPrototypeClass, "W3DPrototypeClass" )

public:
	W3DPrototypeClass(RenderObjClass * proto, const AsciiString& name);

	virtual const char*					Get_Name() const override { return Name.str(); }
	virtual int									Get_Class_ID() const override { return Proto->Class_ID(); }
	virtual RenderObjClass *		Create() override;
	virtual void								DeleteSelf() override							{	deleteInstance(this); }

protected:
	//virtual ~W3DPrototypeClass();

private:
	RenderObjClass *					Proto;
	AsciiString								Name;
};

//---------------------------------------------------------------------
W3DPrototypeClass::W3DPrototypeClass(RenderObjClass * proto, const AsciiString& name) :
	Proto(proto),
	Name(name)
{
	assert(Proto);
	Proto->Add_Ref();
}

//---------------------------------------------------------------------
W3DPrototypeClass::~W3DPrototypeClass()
{
	if (Proto) {
		Proto->Release_Ref();
		Proto = nullptr;
	}
}

//---------------------------------------------------------------------
RenderObjClass * W3DPrototypeClass::Create()
{
	return (RenderObjClass *)( SET_REF_OWNER( Proto->Clone() ) );
}

//---------------------------------------------------------------------
// W3DAssetManager
//---------------------------------------------------------------------

//---------------------------------------------------------------------
W3DAssetManager::W3DAssetManager()
{
}

//---------------------------------------------------------------------
W3DAssetManager::~W3DAssetManager()
{
}

#ifdef DUMP_PERF_STATS
__int64 Total_Get_Texture_Time=0;
#endif

TextureClass *	W3DAssetManager::Get_Texture
	(
		const char * filename,
		MipCountType mip_level_count,
		WW3DFormat texture_format,
		bool allow_compression,
		TextureBaseClass::TexAssetType type,
		bool allow_reduction
	)
{
	//Just call the base implementation after adjusting reduction to deal
	//with our special types.

	if (filename && *filename && _strnicmp(filename,"ZHC",3) == 0)
		allow_reduction = false;	//don't allow reduction on our infantry textures.

	return WW3DAssetManager::Get_Texture(	filename,
		mip_level_count,
		texture_format,
		allow_compression,
		type,
		allow_reduction
	);
}

#if 0	//this function is obsolete in latest C&C3 drop.  Use the one above.
//---------------------------------------------------------------------
TextureClass *W3DAssetManager::Get_Texture(
	const char * filename,
	MipCountType mip_level_count,
	WW3DFormat texture_format,
	bool allow_compression
)
{
	#ifdef DUMP_PERF_STATS
	__int64 startTime64,endTime64;
	GetPrecisionTimer(&startTime64);
	#endif

	WWPROFILE( "WW3DAssetManager::Get_Texture 1" );

	/*
	** Bail if the user isn't really asking for anything
	*/
	if (!filename || !*filename)
	{
		#ifdef DUMP_PERF_STATS
		GetPrecisionTimer(&endTime64);
		Total_Get_Texture_Time += endTime64-startTime64;
		#endif
		return nullptr;
	}

	StringClass lower_case_name(filename,true);
	_strlwr(lower_case_name.str());

	/*
	** See if the texture has already been loaded.
	*/
	TextureClass* tex = TextureHash.Get(lower_case_name);
	if (tex && texture_format != WW3D_FORMAT_UNKNOWN)
	{
		WWASSERT_PRINT(tex->Get_Texture_Format() == texture_format, ("Texture %s has already been loaded with different format",filename));
	}

	/*
	** Didn't have it so we have to create a new texture
	*/
	if (!tex)
	{
		tex = NEW_REF(TextureClass, (lower_case_name, nullptr, mip_level_count, texture_format, allow_compression));
		TextureHash.Insert(tex->Get_Texture_Name(),tex);
//		if (TheGlobalData->m_preloadAssets)
//		{
//			extern std::vector<std::string>	preloadTextureNamesGlobalHack;
//			preloadTextureNamesGlobalHack.push_back(tex->Get_Texture_Name());
//		}
#if defined(RTS_DEBUG) || defined(_ALLOW_DEBUG_CHEATS_IN_RELEASE)
		if (TheGlobalData->m_preloadReport)
		{
			//loading a new asset and app is requesting a log of all loaded assets.
			FILE *logfile=fopen("PreloadedAssets.txt","a+");	//append to log
			if (logfile)
			{
				fprintf(logfile,"TX: %s\n",tex->Get_Texture_Name());
				fclose(logfile);
			}
		}
#endif
	}

	tex->Add_Ref();

#ifdef DUMP_PERF_STATS
	GetPrecisionTimer(&endTime64);
	Total_Get_Texture_Time += endTime64-startTime64;
#endif

	return tex;
}

#endif

//---------------------------------------------------------------------
RenderObjClass * W3DAssetManager::Create_Render_Obj(const char* name)
{
	return WW3DAssetManager::Create_Render_Obj(name);
}

//---------------------------------------------------------------------
/** 'Generals' specific munging to encode team color and scale in model name */
static inline void Munge_Render_Obj_Name(char *newname, size_t newname_size, const char *oldname, float scale,
	const int color, const char *textureName)
{
	char lower_case_name[255];
	strlcpy(lower_case_name, oldname, ARRAY_SIZE(lower_case_name));
	_strlwr(lower_case_name);

	if (!textureName)
		textureName = "";

	snprintf(newname, newname_size, "#%d!%g!%s#%s", color, scale, textureName, lower_case_name);
}

//---------------------------------------------------------------------
static inline void Munge_Texture_Name(char *newname, size_t newname_size, const char *oldname, const int color)
{
	char lower_case_name[255];
	strlcpy(lower_case_name, oldname, ARRAY_SIZE(lower_case_name));
	_strlwr(lower_case_name);
	snprintf(newname, newname_size, "#%d#%s", color, lower_case_name);
}

//---------------------------------------------------------------------
int W3DAssetManager::replaceAssetTexture(RenderObjClass *robj, TextureClass *oldTex, TextureClass *newTex)
{
	switch (robj->Class_ID())	{
	case RenderObjClass::CLASSID_MESH:
		return replaceMeshTexture(robj, oldTex, newTex);
		break;
	case RenderObjClass::CLASSID_HLOD:
		return replaceHLODTexture(robj, oldTex, newTex);
		break;
	}
	return 0;
}

//---------------------------------------------------------------------
Int W3DAssetManager::replaceHLODTexture(RenderObjClass *robj, TextureClass *oldTex, TextureClass *newTex)
{
	int didReplace=0;

	int num_sub = robj->Get_Num_Sub_Objects();
	for(int i = 0; i < num_sub; i++) {
		RenderObjClass *sub_obj = robj->Get_Sub_Object(i);
		didReplace |= replaceAssetTexture(sub_obj, oldTex, newTex);
		REF_PTR_RELEASE(sub_obj);
	}
	return didReplace;
}

//---------------------------------------------------------------------
Int W3DAssetManager::replaceMeshTexture(RenderObjClass *robj, TextureClass *oldTex, TextureClass *newTex)
{
	int i;
	int didReplace=0;

	MeshClass *mesh=(MeshClass*) robj;
	MeshModelClass * model = mesh->Get_Model();
	MaterialInfoClass	*material = mesh->Get_Material_Info();

	for (i=0; i<material->Texture_Count(); i++)
	{
		if (material->Peek_Texture(i) == oldTex)
		{
			model->Replace_Texture(oldTex,newTex);
			material->Replace_Texture(i,newTex);
			didReplace=1;
		}
	}

	REF_PTR_RELEASE(material);
	REF_PTR_RELEASE(model);
	return didReplace;
}

//---------------------------------------------------------------------
/** Replaces all references to old texture with new texture.  Operation is performed on
	asset prototype so it will affect most instances of this object.  Objects which have
	been customized with house color will not be affected unless they are created after
	this function is called.
*/
int W3DAssetManager::replacePrototypeTexture(RenderObjClass *robj, const char * oldname, const char * newname)
{
	//search model for old texture
	TextureClass *oldTex=Get_Texture(oldname);
	TextureClass *newTex=Get_Texture(newname);

	int retCode=replaceAssetTexture(robj,oldTex,newTex);

	REF_PTR_RELEASE(oldTex);
	REF_PTR_RELEASE(newTex);

	return retCode;
}

//---------------------------------------------------------------------
/** Generals specific version that looks for a texture which has been house-color tinted.
	Don't use this unless you really need it.  For normal textures, use Get_Texture().
*/
TextureClass * W3DAssetManager::Find_Texture(const char * name, const int color)
{
	char newname[512];
	Munge_Texture_Name(newname, ARRAY_SIZE(newname), name, color);

	// see if we have a cached copy
	TextureClass *newtex = TextureHash.Get(newname);
	if (newtex) {
		newtex->Add_Ref();
	}
	return newtex;
}

//---------------------------------------------------------------------
TextureClass * W3DAssetManager::Recolor_Texture(TextureClass *texture, const int color)
{
	const char *name=texture->Get_Texture_Name();

	TextureClass *newtex = Find_Texture(name, color);
	if (newtex) {
		return newtex;
	}

	return Recolor_Texture_One_Time(texture, color);
}

//---------------------------------------------------------------------
const Int TEAM_COLOR_PALETTE_SIZE = 16;
const UnsignedShort houseColorScale[TEAM_COLOR_PALETTE_SIZE] =
{
	255,239,223,211,195,174,167,151,135,123,107,91,79,63,47,35
};

//---------------------------------------------------------------------
static void remapPalette16Bit(SurfaceClass::SurfaceDescription *sd, UnsignedShort *palette, unsigned int color)
{
	UnsignedShort pal[TEAM_COLOR_PALETTE_SIZE];
	Vector3 rgb,v_color((float)((color>>16)&0xff)/255.0f/255.0f,(float)((color>>8)&0xff)/255.0f/255.0f,(float)(color&0xff)/255.0f/255.0f);

	//Generate a new color gradient palette based on reference color
	for (Int y=0; y<TEAM_COLOR_PALETTE_SIZE; y++)
	{
		rgb.X=(Real)houseColorScale[y]*v_color.X;
		rgb.Y=(Real)houseColorScale[y]*v_color.Y;
		rgb.Z=(Real)houseColorScale[y]*v_color.Z;
		pal[y]=0xffff;	//preset alpha to known value
		Convert_Pixel((unsigned char *)&pal[y],*sd,rgb);
	}

	//check if this pixel is part of team color palette
	for (Int p=0; p<TEAM_COLOR_PALETTE_SIZE; p++)
	{	palette[p]=pal[p];	//replace color with house color
	}
}

//---------------------------------------------------------------------
static void remapTexture16Bit(Int dx, Int dy, Int pitch, SurfaceClass::SurfaceDescription *sd, UnsignedShort *palette, UnsignedShort *data, unsigned int color)
{
	UnsignedShort pal[TEAM_COLOR_PALETTE_SIZE];
	Vector3 rgb,v_color((float)((color>>16)&0xff)/255.0f/255.0f,(float)((color>>8)&0xff)/255.0f/255.0f,(float)(color&0xff)/255.0f/255.0f);

	//Generate a new color gradient palette based on reference color
	Int y=0;
	for (; y<TEAM_COLOR_PALETTE_SIZE; y++)
	{
		rgb.X=(Real)houseColorScale[y]*v_color.X;
		rgb.Y=(Real)houseColorScale[y]*v_color.Y;
		rgb.Z=(Real)houseColorScale[y]*v_color.Z;
		pal[y]=0xffff;	//preset alpha to known value
		Convert_Pixel((unsigned char *)&pal[y],*sd,rgb);
	}

	for (y=0; y<dy; y++)
	{	for (Int x=0; x<dx; x++)
		{	//check if this pixel is part of team color palette
			for (Int p=0; p<TEAM_COLOR_PALETTE_SIZE; p++)
			{	if (palette[p]==data[x])
				{	data[x]=pal[p];	//replace color with house color
					break;
				}
			}
		}

		data += pitch;
	}
}

//Use hue shift instead of alpha blend to apply house color to textures
#define DO_HUE_SHIFT

//---------------------------------------------------------------------
//Input texture is assumed to be in ARGB 4444 format.
static void remapAlphaTexture16Bit(Int dx, Int dy, Int pitch, SurfaceClass::SurfaceDescription *sd, UnsignedShort *data, unsigned int color)
{
	UnsignedShort pixel;
	UnsignedShort pixelAlpha;
#ifndef DO_HUE_SHIFT
	float fpixelAlpha,fpixelAlphaInv;
#endif
	Vector3 rgb,v_color((float)((color>>16)&0xff)/255.0f,(float)((color>>8)&0xff)/255.0f,(float)(color&0xff)/255.0f);
	Int x,y;

#ifdef DO_HUE_SHIFT
	Vector3 hsv;
	Vector3 hsv_color;
	RGB_To_HSV(hsv_color,v_color);
#endif

	for (y=0; y<dy; y++)
	{
		for (x=0; x<dx; x++)
		{
			pixel=data[x];
			pixelAlpha=15-(pixel>>12);	//get alpha for house color
			if (pixelAlpha)
			{	//some house color needs to show through
				///@todo: optimize this alpha blend to use fixed point math.
#ifdef DO_HUE_SHIFT
				RGB_To_HSV(hsv,Vector3(((pixel>>8)&0xf)/15.0f,((pixel>>4)&0xf)/15.0f,(pixel &0xf)/15.0f));
				hsv.X=hsv_color.X;
				hsv.Y*=hsv_color.Y;
				HSV_To_RGB(rgb,hsv);
#else
				fpixelAlpha=pixelAlpha/15.0f;
				fpixelAlphaInv=1.0f-fpixelAlpha;
				rgb.X=fpixelAlpha * v_color.X + fpixelAlphaInv*(Real)((pixel>>8)&0xf)/15.0f;	//red
				rgb.Y=fpixelAlpha * v_color.Y + fpixelAlphaInv*(Real)((pixel>>4)&0xf)/15.0f; //green
				rgb.Z=fpixelAlpha * v_color.Z + fpixelAlphaInv*(Real)(pixel&0xf)/15.0f; //blue
#endif
				data[x] = REAL_TO_INT(rgb.X*15.0f)<<8 | REAL_TO_INT(rgb.Y*15.0f)<<4 | REAL_TO_INT(rgb.Z*15.0f);
			}
			data[x] |= 0xf000;	//force alpha to opaque.
		}
		data += pitch;
	}
}

//---------------------------------------------------------------------
static void remapPalette32Bit(SurfaceClass::SurfaceDescription *sd, UnsignedInt *palette, unsigned int color)
{
	UnsignedInt pal[TEAM_COLOR_PALETTE_SIZE];
	Vector3 rgb,v_color((float)((color>>16)&0xff)/255.0f/255.0f,(float)((color>>8)&0xff)/255.0f/255.0f,(float)(color&0xff)/255.0f/255.0f);

	//Generate a new color gradient palette based on reference color
	for (Int y=0; y<TEAM_COLOR_PALETTE_SIZE; y++)
	{
		rgb.X=(Real)houseColorScale[y]*v_color.X;
		rgb.Y=(Real)houseColorScale[y]*v_color.Y;
		rgb.Z=(Real)houseColorScale[y]*v_color.Z;
		pal[y]=0xffffffff;	//preset alpha to known value
		Convert_Pixel((unsigned char *)&pal[y],*sd,rgb);
	}

	//check if this pixel is part of team color palette
	for (Int p=0; p<TEAM_COLOR_PALETTE_SIZE; p++)
	{	palette[p]=pal[p];	//replace color with house color
	}
}

//---------------------------------------------------------------------
static void remapTexture32Bit(Int dx, Int dy, Int pitch, SurfaceClass::SurfaceDescription *sd, UnsignedInt *palette, UnsignedInt *data, unsigned int color)
{
	UnsignedInt pal[TEAM_COLOR_PALETTE_SIZE];
	Vector3 rgb,v_color((float)((color>>16)&0xff)/255.0f/255.0f,(float)((color>>8)&0xff)/255.0f/255.0f,(float)(color&0xff)/255.0f/255.0f);

	//Generate a new color gradient palette based on reference color
	Int y=0;
	for (; y<TEAM_COLOR_PALETTE_SIZE; y++)
	{
		rgb.X=(Real)houseColorScale[y]*v_color.X;
		rgb.Y=(Real)houseColorScale[y]*v_color.Y;
		rgb.Z=(Real)houseColorScale[y]*v_color.Z;
		pal[y]=0xffffffff;	//preset alpha to known value
		Convert_Pixel((unsigned char *)&pal[y],*sd,rgb);
	}

	for (y=0; y<dy; y++)
	{	for (Int x=0; x<dx; x++)
		{	//check if this pixel is part of team color palette
			for (Int p=0; p<TEAM_COLOR_PALETTE_SIZE; p++)
			{	if (palette[p]==data[x])
				{	data[x]=pal[p];	//replace color with house color
					break;
				}
			}
		}
		data += pitch;
	}
}

//---------------------------------------------------------------------
static void remapAlphaTexture32Bit(Int dx, Int dy, Int pitch, SurfaceClass::SurfaceDescription *sd, UnsignedInt *data, unsigned int color)
{
	UnsignedInt pixel;
	UnsignedInt pixelAlpha;
#ifndef DO_HUE_SHIFT
	float fpixelAlpha,fpixelAlphaInv;
#endif
	Vector3 rgb,v_color((float)((color>>16)&0xff)/255.0f,(float)((color>>8)&0xff)/255.0f,(float)(color&0xff)/255.0f);
	Int x,y;
#ifdef DO_HUE_SHIFT
	Vector3 hsv;
	Vector3 hsv_color;
	RGB_To_HSV(hsv_color,v_color);
#endif

	for (y=0; y<dy; y++)
	{	for (x=0; x<dx; x++)
		{
			pixel=data[x];
			pixelAlpha=255-(pixel>>24);	//get alpha for house color
			if (pixelAlpha)
			{	//some house color needs to show through
#ifdef DO_HUE_SHIFT
				RGB_To_HSV(hsv,Vector3(((pixel>>16)&0xff)/255.0f,((pixel>>8)&0xff)/255.0f,(pixel &0xff)/255.0f));
				hsv.X=hsv_color.X;
				hsv.Y*=hsv_color.Y;
				HSV_To_RGB(rgb,hsv);
#else
				///@todo: optimize this alpha blend to use fixed point math.
				fpixelAlpha=pixelAlpha/255.0f;
				fpixelAlphaInv=1.0f-fpixelAlpha;
				rgb.X=fpixelAlpha * v_color.X + fpixelAlphaInv*(Real)((pixel>>16)&0xff)/255.0f;	//red
				rgb.Y=fpixelAlpha * v_color.Y + fpixelAlphaInv*(Real)((pixel>>8)&0xff)/255.0f; //green
				rgb.Z=fpixelAlpha * v_color.Z + fpixelAlphaInv*(Real)(pixel&0xff)/255.0f; //blue
#endif
				data[x] = REAL_TO_INT(rgb.X*255.0f)<<16 |	REAL_TO_INT(rgb.Y*255.0f)<<8 | REAL_TO_INT(rgb.Z*255.0f);
			}
			data[x] |= 0xff000000;	//force alpha to opaque.
		}
		data += pitch;
	}
}

//---------------------------------------------------------------------
/** Surface is assumed to come in the following format:
First 16 pixels are a palette composed of 24-Bit RGB values.
Any pixels in remainder of image that use these 24-bit values
will be remapped using a pre-defined formula.
*/
void W3DAssetManager::Remap_Palette(SurfaceClass *surface, const int color, Bool doPaletteOnly, Bool useAlpha)
{
//	unsigned int x;
	SurfaceClass::SurfaceDescription sd;
	surface->Get_Description(sd);
	int pitch,size;
//	UnsignedInt newPalette[TEAM_COLOR_PALETTE_SIZE];

	size=Get_Bytes_Per_Pixel(sd.Format);
	unsigned char *bits=(unsigned char*) surface->Lock(&pitch);

	if (doPaletteOnly)
	{	//only recolor the palette which is stored in top row.  Model only references these pixels.
		if (size == 2)
			remapPalette16Bit(&sd, (UnsignedShort *)bits, color);
		else
		if (size == 4)
			remapPalette32Bit(&sd, (UnsignedInt *)bits,color);
	}
	else
	{
		if (useAlpha)
		{
			if (size == 2)
				remapAlphaTexture16Bit(sd.Width, sd.Height, pitch>>1, &sd, (UnsignedShort *)bits,color);
			else
			if (size == 4)
				remapAlphaTexture32Bit(sd.Width, sd.Height, pitch>>2, &sd, (UnsignedInt *)bits,color);
		}
		else
		{	//Recolor the image using the palette stored in top row
			if (size == 2)
				remapTexture16Bit(sd.Width, sd.Height-1, pitch>>1, &sd, (UnsignedShort *)bits, (UnsignedShort *)(bits+pitch), color);
			else
			if (size == 4)
				remapTexture32Bit(sd.Width, sd.Height-1, pitch>>2, &sd, (UnsignedInt *)bits, (UnsignedInt *)(bits+pitch),color);
		}
	}

	surface->Unlock();
}

//---------------------------------------------------------------------
TextureClass * W3DAssetManager::Recolor_Texture_One_Time(TextureClass *texture, const int color)
{
	const char *name=texture->Get_Texture_Name();

	// if texture is procedural return nullptr
	if (name && name[0]=='!') return nullptr;

	// make sure texture is loaded
	if (!texture->Is_Initialized())
		TextureLoader::Request_Foreground_Loading(texture);

	SurfaceClass::SurfaceDescription desc;
	SurfaceClass *newsurf, *oldsurf;
	texture->Get_Level_Description(desc);

	Int psize;
	psize=Get_Bytes_Per_Pixel(desc.Format);
	DEBUG_ASSERTCRASH( psize == 2 || psize == 4, ("Can't Recolor Texture %s", name) );

	oldsurf=texture->Get_Surface_Level();

	newsurf=NEW_REF(SurfaceClass,(desc.Width,desc.Height,desc.Format));
	newsurf->Copy(0,0,0,0,desc.Width,desc.Height,oldsurf);

	if (*(name+3) == 'D' || *(name+3) == 'd')
		Remap_Palette(newsurf,color, true, false );	//texture only contains a palette stored in top row.
	else
	if (*(name+3) == 'A' || *(name+3) == 'a')
		Remap_Palette(newsurf,color, false, true );	//texture only contains a palette stored in top row.

	TextureClass * newtex=NEW_REF(TextureClass,(newsurf,(MipCountType)texture->Get_Mip_Level_Count()));
	newtex->Get_Filter().Set_Mag_Filter(texture->Get_Filter().Get_Mag_Filter());
	newtex->Get_Filter().Set_Min_Filter(texture->Get_Filter().Get_Min_Filter());
	newtex->Get_Filter().Set_Mip_Mapping(texture->Get_Filter().Get_Mip_Mapping());
	newtex->Get_Filter().Set_U_Addr_Mode(texture->Get_Filter().Get_U_Addr_Mode());
	newtex->Get_Filter().Set_V_Addr_Mode(texture->Get_Filter().Get_V_Addr_Mode());

	char newname[512];
	Munge_Texture_Name(newname, ARRAY_SIZE(newname), name, color);
	newtex->Set_Texture_Name(newname);

	TextureHash.Insert(newtex->Get_Texture_Name(), newtex);
	newtex->Add_Ref();

	REF_PTR_RELEASE(oldsurf);
	REF_PTR_RELEASE(newsurf);

	return newtex;
}

#ifdef DUMP_PERF_STATS
__int64 Total_Create_Render_Obj_Time=0;
#endif
//---------------------------------------------------------------------
/** Generals specific code to generate customized render objects for each team color
	Scale==1.0, color==0x00000000, and oldTexture==nullptr are defaults that do nothing.
*/
RenderObjClass * W3DAssetManager::Create_Render_Obj(
	const char * name,
	float scale,
	const int color,
	const char *oldTexture,
	const char *newTexture
)
{
	#ifdef DUMP_PERF_STATS
	__int64 startTime64,endTime64;
	GetPrecisionTimer(&startTime64);
	#endif

	Bool reallyscale = (WWMath::Fabs(scale - ident_scale) > scale_epsilon);
	Bool reallycolor = (color & 0xFFFFFF) != 0;	//black is not a valid color and assumes no custom coloring.
	Bool reallytexture = (oldTexture != nullptr && newTexture != nullptr);

	// base case, no scale or color
	if (!reallyscale && !reallycolor && !reallytexture)
	{
		RenderObjClass *robj=WW3DAssetManager::Create_Render_Obj(name);
	#ifdef DUMP_PERF_STATS
		GetPrecisionTimer(&endTime64);
		Total_Create_Render_Obj_Time += endTime64-startTime64;
	#endif
		return robj;
	}

	char newname[512];
	Munge_Render_Obj_Name(newname, ARRAY_SIZE(newname), name, scale, color, newTexture);

	// see if we got a cached version
	RenderObjClass *rendobj = nullptr;

	Set_WW3D_Load_On_Demand(false); // munged name will never be found in a file.
	rendobj = WW3DAssetManager::Create_Render_Obj(newname);
	if (rendobj)
	{	//store the color that we used to create asset so we can read it back out
		//when we need to save this render object to a file.  Used during saving
		//of fog of war ghost objects.
		rendobj->Set_ObjectColor(color);
		Set_WW3D_Load_On_Demand(true); // Auto Load.

	#ifdef DUMP_PERF_STATS
		GetPrecisionTimer(&endTime64);
		Total_Create_Render_Obj_Time += endTime64-startTime64;
	#endif
		return rendobj;
	}

	// create a new one based on existing prototype

	WWPROFILE( "WW3DAssetManager::Create_Render_Obj" );
	WWMEMLOG(MEM_GEOMETRY);

	// Try to find a prototype
	PrototypeClass * proto = Find_Prototype(name);

	Set_WW3D_Load_On_Demand(true); // Auto Load.
	if (WW3D_Load_On_Demand && proto == nullptr)
	{
		// If we didn't find one, try to load on demand
		char filename [MAX_PATH];
		const char *mesh_name = strchr (name, '.');
		if (mesh_name != nullptr)
		{
			lstrcpyn(filename, name, ((int)mesh_name) - ((int)name) + 1);
			lstrcat(filename, ".w3d");
		} else {
			snprintf( filename, ARRAY_SIZE(filename), "%s.w3d", name);
		}

		// If we can't find it, try the parent directory
		if ( Load_3D_Assets( filename ) == false )
		{
			StringClass	new_filename = StringClass("..\\") + filename;
			Load_3D_Assets(new_filename);
		}

		proto = Find_Prototype(name);		// try again
	}

	if (proto == nullptr)
	{
		static int warning_count = 0;
		if (++warning_count <= 20)
		{
			WWDEBUG_SAY(("WARNING: Failed to create Render Object: %s",name));
		}
	#ifdef DUMP_PERF_STATS
		GetPrecisionTimer(&endTime64);
		Total_Create_Render_Obj_Time += endTime64-startTime64;
	#endif
		return nullptr;		// Failed to find a prototype
	}

	rendobj = proto->Create();

	if (!rendobj)
	{
	#ifdef DUMP_PERF_STATS
		GetPrecisionTimer(&endTime64);
		Total_Create_Render_Obj_Time += endTime64-startTime64;
	#endif
		return nullptr;
	}

	if (reallyscale)
		rendobj->Scale(scale);	//this also makes it unique

	Make_Unique(rendobj,reallyscale,reallycolor);

	if (reallytexture)
	{
		TextureClass *oldTex = Get_Texture(oldTexture);
		TextureClass *newTex = Get_Texture(newTexture);
		replaceAssetTexture(rendobj,oldTex,newTex);
		REF_PTR_RELEASE(newTex);
		REF_PTR_RELEASE(oldTex);
	}

	if (reallycolor)
		Recolor_Asset(rendobj,color);

	W3DPrototypeClass *w3dproto = newInstance(W3DPrototypeClass)(rendobj, newname);
	rendobj->Release_Ref();
	Add_Prototype(w3dproto);

	rendobj = w3dproto->Create();
	rendobj->Set_ObjectColor(color);

#ifdef DUMP_PERF_STATS
	GetPrecisionTimer(&endTime64);
	Total_Create_Render_Obj_Time += endTime64-startTime64;
#endif

	return rendobj;
}

//---------------------------------------------------------------------
/** Generals specific code to generate customized render objects for each team color
*/
int W3DAssetManager::Recolor_Asset(RenderObjClass *robj, const int color)
{
	if (TheGlobalData->m_headless)
		return 0;

	switch (robj->Class_ID())	{
	case RenderObjClass::CLASSID_MESH:
		return Recolor_Mesh(robj,color);
		break;
	case RenderObjClass::CLASSID_HLOD:
		return Recolor_HLOD(robj,color);
		break;
	}
	return 0;
}

//---------------------------------------------------------------------
/** HOUSEFX: house color applied per material pass, chosen by the pass blend mode, so artists
	mask where the color goes with a (compressed, shared) texture instead of splitting geometry
	or using per-color ZHC texture copies.

	Opt-in (any one):
	- Texture name of the pass starts with "HOUSEFX" (e.g. "HOUSEFX_tank_glow.tga").
	- Material name starts with "HOUSEFX" (the 3ds Max / RenX material name, e.g. "HOUSEFX_Glow").
	  Texture/material opt-in only affects those passes, so meshes keep any name
	  (BARREL01, TURRET, ...) for game logic.
	- Mesh name starts with "HOUSEFX" (e.g. "HOUSEFX01"): every material on the mesh qualifies.

	Qualifying materials are recolored depending on the blend mode of the pass that uses them:
	- Additive pass     (dest = ONE), authored Emissive > 0:   emissive = house color, ambient/diffuse = 0
	                                                           -> unlit glow, texture = grayscale glow mask.
	- Additive pass     (dest = ONE), authored Emissive = 0:   ambient/diffuse = house color, emissive = 0
	                                                           -> lit (non-glowing) color with the same mask;
	                                                              paint the base dark under the mask.
	- Multiply pass     (src = ZERO, dest = SRC_COLOR):        emissive = house color, ambient/diffuse = 0
	                                                           -> tint, use Primary Gradient ADD and a mask
	                                                              with white = no change, black = full color.
	- Alpha blend pass  (src = SRC_ALPHA, dest = 1-SRC_ALPHA): ambient/diffuse = house color
	                                                           -> lit paint, texture alpha = mask.
	- Opaque or any other blend mode: recolored like an Add pass ONLY when the pass's own texture is
	  HOUSEFX named (glow drawn underneath a base pass with alpha holes). Otherwise left untouched,
	  as it's the base pass.
	A vertex material that is also used by a pass of a different kind (e.g. shared with the base
	pass because the exporter merged identical settings) is skipped.
*/
enum HouseFxPassType
{
	HOUSEFX_PASS_NONE,
	HOUSEFX_PASS_ADD,		//glow or lit, decided by the authored emissive
	HOUSEFX_PASS_EMISSIVE,
	HOUSEFX_PASS_LIT
};

static Bool isHouseFxName(const char *name)
{
	return name != nullptr && _strnicmp(name, "HOUSEFX", 7) == 0;
}

static const char *getMeshBaseName(MeshClass *mesh)
{
	//Meshes which are part of another model have names in the form "name.name" while
	//isolated meshes are just "name".
	const char *meshName = mesh->Get_Name();
	if (meshName == nullptr)
		return nullptr;
	const char *dot = strchr(meshName, '.');
	if (dot != nullptr && *(dot + 1))
		return dot + 1;
	return meshName;
}

static Bool isHouseFxMesh(MeshClass *mesh)
{
	return isHouseFxName(getMeshBaseName(mesh));
}

/** True if the mesh is HOUSEFX named or uses at least one HOUSEFX named vertex material. */
static Bool meshUsesHouseFx(MeshClass *mesh)
{
	if (isHouseFxMesh(mesh))
		return true;

	Bool found = false;
	MaterialInfoClass *material = mesh->Get_Material_Info();
	if (material)
	{
		for (int i = 0; i < material->Vertex_Material_Count() && !found; i++)
		{
			VertexMaterialClass *vmat = material->Peek_Vertex_Material(i);
			found = (vmat != nullptr && isHouseFxName(vmat->Get_Name()));
		}
		for (int i = 0; i < material->Texture_Count() && !found; i++)
		{
			TextureClass *tex = material->Peek_Texture(i);
			found = (tex != nullptr && isHouseFxName(tex->Get_Texture_Name()));
		}
		REF_PTR_RELEASE(material);
	}
	return found;
}

static HouseFxPassType getHouseFxPassType(const ShaderClass &shader)
{
	const ShaderClass::SrcBlendFuncType src = shader.Get_Src_Blend_Func();
	const ShaderClass::DstBlendFuncType dst = shader.Get_Dst_Blend_Func();

	if (dst == ShaderClass::DSTBLEND_ONE)
		return HOUSEFX_PASS_ADD;		//additive: glow (emissive authored) or lit color (emissive 0)
	if (src == ShaderClass::SRCBLEND_ZERO && dst == ShaderClass::DSTBLEND_SRC_COLOR)
		return HOUSEFX_PASS_EMISSIVE;	//multiply tint
	if (src == ShaderClass::SRCBLEND_SRC_ALPHA && dst == ShaderClass::DSTBLEND_ONE_MINUS_SRC_ALPHA)
		return HOUSEFX_PASS_LIT;		//alpha blended paint
	return HOUSEFX_PASS_NONE;
}

/** Pass kind for HOUSEFX purposes. A pass whose own texture is HOUSEFX named is recolored whatever
	its blend mode (e.g. an Opaque glow pass drawn underneath an alpha-tested base pass): any blend
	mode that isn't Multiply / Alpha Blend then behaves like an Add pass (authored Emissive > 0 = glow,
	Emissive 0 = lit color). Material/mesh name opt-in can't tell the passes of one material apart,
	so for those, Opaque and other blend modes stay untouched (they are the base pass). */
static HouseFxPassType resolveHouseFxPassType(const ShaderClass &shader, Bool passTextureIsHouseFx)
{
	const HouseFxPassType type = getHouseFxPassType(shader);
	if (type == HOUSEFX_PASS_NONE && passTextureIsHouseFx)
		return HOUSEFX_PASS_ADD;
	return type;
}

struct HouseFxMaterialUse
{
	VertexMaterialClass *vmat;
	HouseFxPassType type;
	Bool conflict;	//used by passes of different kinds
	Bool namedByTexture;	//used with a HOUSEFX named texture
	Bool applied;
};

static Bool isHouseFxTexture(TextureClass *tex)
{
	return tex != nullptr && isHouseFxName(tex->Get_Texture_Name());
}

/** True if any texture stage of this pass (for polygon pidx, or the single texture) is HOUSEFX named. */
static Bool passUsesHouseFxTexture(MeshModelClass *model, int pass, int pidx)
{
	for (int stage = 0; stage < MeshMatDescClass::MAX_TEX_STAGES; ++stage)
	{
		TextureClass *tex = model->Has_Texture_Array(pass, stage)
			? model->Peek_Texture(pidx, pass, stage)
			: model->Peek_Single_Texture(pass, stage);
		if (isHouseFxTexture(tex))
			return true;
	}
	return false;
}

enum { MAX_HOUSEFX_MATERIAL_USES = 64 };

static Bool addHouseFxMaterialUse(HouseFxMaterialUse *uses, Int &count, VertexMaterialClass *vmat, HouseFxPassType type, Bool namedByTexture)
{
	if (vmat == nullptr)
		return true;

	for (Int i = 0; i < count; ++i)
	{
		if (uses[i].vmat == vmat)
		{
			if (uses[i].type != type)
				uses[i].conflict = true;
			if (namedByTexture)
				uses[i].namedByTexture = true;
			return true;
		}
	}

	if (count >= MAX_HOUSEFX_MATERIAL_USES)
		return false;

	uses[count].vmat = vmat;
	uses[count].type = type;
	uses[count].conflict = false;
	uses[count].namedByTexture = namedByTexture;
	uses[count].applied = false;
	++count;
	return true;
}

static int recolorHouseFxPasses(MeshModelClass *model, Bool wholeMeshIsHouseFx, const int color)
{
	const Vector3 houseColor(	(Real)((color >> 16) & 0xff) / 255.0f,
								(Real)((color >> 8) & 0xff) / 255.0f,
								(Real)(color & 0xff) / 255.0f );
	const Vector3 black(0.0f, 0.0f, 0.0f);

	HouseFxMaterialUse uses[MAX_HOUSEFX_MATERIAL_USES];
	Int useCount = 0;
	Bool ok = true;

	const int passCount = model->Get_Pass_Count();
	const int polyCount = model->Get_Polygon_Count();
	const TriIndex *polys = model->Get_Polygon_Array();

	//Gather every (vertex material, pass kind) combination used by this mesh.
	for (int pass = 0; pass < passCount && ok; ++pass)
	{
		const bool shaderArray = model->Has_Shader_Array(pass);
		const bool materialArray = model->Has_Material_Array(pass);

		if (!shaderArray && !materialArray)
		{
			const Bool texFx = passUsesHouseFxTexture(model, pass, 0);
			ok = addHouseFxMaterialUse(uses, useCount, model->Peek_Single_Material(pass),
				resolveHouseFxPassType(model->Get_Single_Shader(pass), texFx), texFx);
			continue;
		}

		//Multi-material mesh: shaders are per polygon, materials per vertex.
		for (int p = 0; p < polyCount && ok; ++p)
		{
			const Bool texFx = passUsesHouseFxTexture(model, pass, p);
			const HouseFxPassType type = resolveHouseFxPassType(shaderArray ? model->Get_Shader(p, pass) : model->Get_Single_Shader(pass), texFx);
			if (materialArray && polys != nullptr)
			{
				ok = addHouseFxMaterialUse(uses, useCount, model->Peek_Material(polys[p].I, pass), type, texFx)
					&& addHouseFxMaterialUse(uses, useCount, model->Peek_Material(polys[p].J, pass), type, texFx)
					&& addHouseFxMaterialUse(uses, useCount, model->Peek_Material(polys[p].K, pass), type, texFx);
			}
			else
			{
				ok = addHouseFxMaterialUse(uses, useCount, model->Peek_Single_Material(pass), type, texFx);
			}
		}
	}

	if (!ok)
	{
		DEBUG_LOG(("HOUSEFX mesh '%s' uses more than %d vertex material combinations; house color skipped.",
			model->Get_Name(), (int)MAX_HOUSEFX_MATERIAL_USES));
		return 0;
	}

	int didRecolor = 0;
	for (Int i = 0; i < useCount; ++i)
	{
		HouseFxMaterialUse &use = uses[i];
		if (use.type == HOUSEFX_PASS_NONE)
			continue;
		if (!wholeMeshIsHouseFx && !use.namedByTexture && !isHouseFxName(use.vmat->Get_Name()))
			continue;
		if (use.conflict)
		{
			DEBUG_LOG(("HOUSEFX mesh '%s': vertex material '%s' is shared with a pass of a different kind; give the house color pass its own vertex material settings. Skipped.",
				model->Get_Name(), use.vmat->Get_Name()));
			continue;
		}

		VertexMaterialClass *vmat = use.vmat;

		//Take all lighting colors from the material, not from exported vertex colors (DCG/DIG),
		//otherwise the house color set below is ignored.
		vmat->Set_Ambient_Color_Source(VertexMaterialClass::MATERIAL);
		vmat->Set_Diffuse_Color_Source(VertexMaterialClass::MATERIAL);
		vmat->Set_Emissive_Color_Source(VertexMaterialClass::MATERIAL);
		vmat->Set_Lighting(true);	//emissive/ambient/diffuse only apply with lighting enabled

		HouseFxPassType applyType = use.type;
		if (applyType == HOUSEFX_PASS_ADD)
		{
			//Artist choice via the authored emissive: any emissive = glow, black emissive = lit color.
			Vector3 authoredEmissive;
			vmat->Get_Emissive(&authoredEmissive);
			const Bool glow = (authoredEmissive.X > 0.0f || authoredEmissive.Y > 0.0f || authoredEmissive.Z > 0.0f);
			applyType = glow ? HOUSEFX_PASS_EMISSIVE : HOUSEFX_PASS_LIT;
		}

		if (applyType == HOUSEFX_PASS_EMISSIVE)
		{
			vmat->Set_Emissive(houseColor);
			vmat->Set_Ambient(black);
			vmat->Set_Diffuse(black);
		}
		else
		{
			vmat->Set_Emissive(black);
			vmat->Set_Ambient(houseColor);
			vmat->Set_Diffuse(houseColor);
		}
		use.applied = true;
		didRecolor = 1;
	}

	//Primary Gradient "Disable" ignores the lighting result entirely, so the house color would
	//never show. Fall back to Modulate (texture * house color) on single shader passes we recolored.
	for (int pass = 0; pass < passCount; ++pass)
	{
		if (model->Has_Shader_Array(pass) || model->Has_Material_Array(pass))
			continue;

		VertexMaterialClass *vmat = model->Peek_Single_Material(pass);
		Bool recolored = false;
		for (Int i = 0; i < useCount && !recolored; ++i)
			recolored = (uses[i].vmat == vmat && uses[i].applied);
		if (!recolored)
			continue;

		ShaderClass shader = model->Get_Single_Shader(pass);
		if (shader.Get_Primary_Gradient() == ShaderClass::GRADIENT_DISABLE)
		{
			shader.Set_Primary_Gradient(ShaderClass::GRADIENT_MODULATE);
			model->Set_Single_Shader(shader, pass);
		}
	}

	return didRecolor;
}

//---------------------------------------------------------------------
/** Generals specific code to generate customized render objects for each team color
*/
int W3DAssetManager::Recolor_Mesh(RenderObjClass *robj, const int color)
{
	if (TheGlobalData->m_headless)
		return 0;

	int i;
	int didRecolor=0;
	const char *meshName;

	MeshClass *mesh=(MeshClass*) robj;
	MeshModelClass * model = mesh->Get_Model();
	MaterialInfoClass	*material = mesh->Get_Material_Info();

	// recolor vertex material (assuming mesh is housecolor)
	if ( (( (meshName=strchr(mesh->Get_Name(),'.') ) != nullptr && *(meshName++)) || ( (meshName=mesh->Get_Name()) != nullptr)) &&
		_strnicmp(meshName,"HOUSECOLOR", 10) == 0)
	{	for (i=0; i<material->Vertex_Material_Count(); i++)
			Recolor_Vertex_Material(material->Peek_Vertex_Material(i),color);
		didRecolor=1;
	}

	// recolor HOUSEFX passes (house color applied per pass by blend mode)
	if (meshUsesHouseFx(mesh))
		didRecolor |= recolorHouseFxPasses(model, isHouseFxMesh(mesh), color);

	// recolor textures
	TextureClass *newtex,*oldtex;
	for (i=0; i<material->Texture_Count(); i++)
	{
		oldtex=material->Peek_Texture(i);
		if (_strnicmp(oldtex->Get_Texture_Name(),"ZHC", 3) == 0)
		{	//This texture needs to be adjusted for housecolor
			newtex=Recolor_Texture(oldtex,color);
			if (newtex)
			{
				model->Replace_Texture(oldtex,newtex);
				material->Replace_Texture(i,newtex);
				REF_PTR_RELEASE(newtex);
				didRecolor=1;
			}
		}
	}

	REF_PTR_RELEASE(material);
	REF_PTR_RELEASE(model);
	return didRecolor;
}

//---------------------------------------------------------------------
/** Generals specific code to generate customized render objects for each team color
*/

int W3DAssetManager::Recolor_HLOD(RenderObjClass *robj, const int color)
{
	if (TheGlobalData->m_headless)
		return 0;

	int didRecolor=0;

	int num_sub = robj->Get_Num_Sub_Objects();
	for(int i = 0; i < num_sub; i++) {
		RenderObjClass *sub_obj = robj->Get_Sub_Object(i);
		didRecolor |= Recolor_Asset(sub_obj,color);
		REF_PTR_RELEASE(sub_obj);
	}
	return didRecolor;
}

//---------------------------------------------------------------------
/** Generals specific code to generate customized render objects for each team color
*/
void W3DAssetManager::Recolor_Vertex_Material(VertexMaterialClass *vmat, const int color)
{
	Vector3 rgb,rgb2;

	rgb.X = (Real)((color >> 16) & 0xff )/255.0f;
	rgb.Y = (Real)((color >> 8) & 0xff )/255.0f;
	rgb.Z = (Real)(color & 0xff)/255.0f;

	//We ignore the existing ambinent/diffuse and assume they were 1.0.  We can change
	//to scaling them if required.

//	vmat->Get_Ambient(&rgb2);
//	Recolor(rgb,hsv_shift);
	rgb2.X = rgb.X;	//scale colors
	rgb2.Y = rgb.Y;	//scale colors
	rgb2.Z = rgb.Z;	//scale colors
	vmat->Set_Ambient(rgb2);

//	vmat->Get_Diffuse(&rgb2);
//	Recolor(rgb,hsv_shift);
	rgb2.X = rgb.X;	//scale colors
	rgb2.Y = rgb.Y;	//scale colors
	rgb2.Z = rgb.Z;	//scale colors
	vmat->Set_Diffuse(rgb2);
}

#ifdef DUMP_PERF_STATS
__int64 Total_Load_3D_Assets=0;
static Int Load_3D_Asset_Recursions=0;
#endif
//---------------------------------------------------------------------
bool W3DAssetManager::Load_3D_Assets( const char * filename )
{
#ifdef DUMP_PERF_STATS
		Load_3D_Asset_Recursions++;

		__int64 startTime64,endTime64;
		GetPrecisionTimer(&startTime64);
#endif

	// Try to find an existing prototype
	char basename[512];
	strlcpy(basename, filename, ARRAY_SIZE(basename));
	char *pext = strrchr(basename, '.');	//find file extension
	if (pext)
		*pext = '\0';	//drop the extension
	PrototypeClass * proto = Find_Prototype(basename);
	if (proto)
	{
#ifdef DUMP_PERF_STATS
		if (Load_3D_Asset_Recursions == 1)
		{	GetPrecisionTimer(&endTime64);
			Total_Load_3D_Assets += endTime64-startTime64;
		}
		Load_3D_Asset_Recursions--;
#endif
		return TRUE;	//this file has already been loaded.
	}

	bool result = WW3DAssetManager::Load_3D_Assets(filename);

#if defined(RTS_DEBUG)
	if (result && TheGlobalData->m_preloadReport)
	{
		//loading a new asset and app is requesting a log of all loaded assets.
		FILE *logfile=fopen("PreloadedAssets.txt","a+");	//append to log
		if (logfile)
		{
			StringClass lower_case_name(filename,true);
			_strlwr(lower_case_name.Peek_Buffer());
			fprintf(logfile,"3D: %s\n",lower_case_name.str());
			fclose(logfile);
		}
	}
#endif
#ifdef DUMP_PERF_STATS
	if (Load_3D_Asset_Recursions == 1)
	{	GetPrecisionTimer(&endTime64);
		Total_Load_3D_Assets += endTime64-startTime64;
	}
	Load_3D_Asset_Recursions--;
#endif
	return result;

}

#ifdef DUMP_PERF_STATS
__int64 Total_Get_HAnim_Time=0;
static Int HAnim_Recursions=0;
#endif
//---------------------------------------------------------------------
HAnimClass *	W3DAssetManager::Get_HAnim(const char * name)
{
#ifdef DUMP_PERF_STATS
	HAnim_Recursions++;

	__int64 startTime64,endTime64;
	GetPrecisionTimer(&startTime64);
#endif
	WWPROFILE( "WW3DAssetManager::Get_HAnim" );

	HAnimClass *anim=WW3DAssetManager::Get_HAnim(name);
#ifdef DUMP_PERF_STATS
	if (HAnim_Recursions == 1)
	{
		GetPrecisionTimer(&endTime64);
		Total_Get_HAnim_Time += endTime64-startTime64;
	}
	HAnim_Recursions--;
#endif
	return anim;

}

//---------------------------------------------------------------------
// Uniqing
//---------------------------------------------------------------------

//---------------------------------------------------------------------
/** Generals specific code to generate customized render objects for each team color
*/
void W3DAssetManager::Make_HLOD_Unique(RenderObjClass *robj, Bool geometry, Bool colors)
{
	int num_sub = robj->Get_Num_Sub_Objects();
	for(int i = 0; i < num_sub; i++) {
		RenderObjClass *sub_obj = robj->Get_Sub_Object(i);
		Make_Unique(sub_obj, geometry, colors);
		REF_PTR_RELEASE(sub_obj);
	}
}

//---------------------------------------------------------------------
/** Generals specific code to generate customized render objects for each team color
*/
void W3DAssetManager::Make_Unique(RenderObjClass *robj, Bool geometry, Bool colors)
{
	switch (robj->Class_ID())	{
	case RenderObjClass::CLASSID_MESH:
		Make_Mesh_Unique(robj,geometry,colors);
		break;
	case RenderObjClass::CLASSID_HLOD:
		Make_HLOD_Unique(robj,geometry,colors);
		break;
	}
}

//---------------------------------------------------------------------
/** Determine what method is used to apply house color to this mesh (if any) */
static Bool getMeshColorMethods(MeshClass *mesh, Bool &vertexColor, Bool &textureColor)
{
	vertexColor = false;
	textureColor = false;

	//Check if mesh is using custom texture containing house color
	MaterialInfoClass *material = mesh->Get_Material_Info();
	if (material)
	{	for (int j=0; j<material->Texture_Count(); j++)
			if (_strnicmp(material->Peek_Texture(j)->Get_Texture_Name(),"ZHC",3) == 0)
			{	textureColor = true;
				break;
			}
		REF_PTR_RELEASE(material);
	}

	//Check if mesh is using a custom mesh which contains house color in material.
	//Meshes which are part of another model have names in the form "name.name" while
	//isolated meshes are just "name".  We check for both starting with "HOUSECOLOR".
	const char *meshName;
	if ( ( (meshName=strchr(mesh->Get_Name(),'.') ) != nullptr && *(meshName++)) || ( (meshName=mesh->Get_Name()) != nullptr) )
	{	//Check if this object has housecolors on mesh
		if ( _strnicmp(meshName,"HOUSECOLOR", 10) == 0)
			vertexColor = true;
	}

	//HOUSEFX meshes/materials recolor individual pass vertex materials, so they need unique vertex materials too.
	if (meshUsesHouseFx(mesh))
		vertexColor = true;

	return (vertexColor || textureColor);
}

//---------------------------------------------------------------------
/** Generals specific code to generate customized render objects for each team color
*/
void W3DAssetManager::Make_Mesh_Unique(RenderObjClass *robj, Bool geometry, Bool colors)
{
	int i;
	MeshClass *mesh=(MeshClass*) robj;
	Bool isVertexColor, isTextureColor;

	//figure out what type of coloring this mesh requires (if any)
	if ((colors && getMeshColorMethods(mesh,isVertexColor,isTextureColor)) || geometry)
	{	//mesh has some house color applied so make those components unique to mesh.

		//Create unique data for this mesh
		if (!geometry)	//scaling geometry automatically makes it unique so not needed here.
			mesh->Make_Unique();

		MeshModelClass * model = mesh->Get_Model();

		if (colors && isVertexColor)
		{
			MaterialInfoClass	*material=mesh->Get_Material_Info();
			for (i=0; i<material->Vertex_Material_Count(); i++)
				material->Peek_Vertex_Material(i)->Make_Unique();
			REF_PTR_RELEASE(material);
		}

		REF_PTR_RELEASE(model);
	}
}

//---------------------------------------------------------------------
/**Report prototypes that have all assets with reference count
equal to 1*/
void W3DAssetManager::Report_Used_Prototypes()
{
	int count = Prototypes.Count();
	while (count-- > 0) {

		PrototypeClass * proto = Prototypes[count];
		if (proto->Get_Class_ID() == RenderObjClass::CLASSID_HLOD || proto->Get_Class_ID() == RenderObjClass::CLASSID_MESH)
		{
			DEBUG_LOG(("**Unfreed Prototype On Map Reset: %s",proto->Get_Name()));
		}
	}
}

//---------------------------------------------------------------------
/**Report any assets with reference counts > 1.  This means they are still
referenced by something besides the asset manager.*/
void W3DAssetManager::Report_Used_Assets()
{
	Report_Used_Prototypes();

	///@todo: Report unfreed skeletons and animations
	//	HAnimManager.Free_All_Anims();
	//	HTreeManager.Free_All_Trees();

	Report_Used_Textures();
	Report_Used_Font3DDatas();
	Report_Used_FontChars();
}

//---------------------------------------------------------------------
void W3DAssetManager::Report_Used_FontChars()
{
	Int count=FontCharsList.Count();

	while (count-- > 0)
	{
		if (FontCharsList[count]->Num_Refs() >= 1)
		{
			DEBUG_LOG(("**Unfreed FontChar On Map Reset: %s",FontCharsList[count]->Get_Name()));
			//FontCharsList[count]->Release_Ref();
			//FontCharsList.Delete(count);
		}
	}
}

//---------------------------------------------------------------------
/**Report all textures with refcounts >= 1*/
void W3DAssetManager::Report_Used_Textures()
{
	/*
	** for each texture in the list, get it, check it's refcount, and and release ref it if the
	** refcount is one.
	*/

//	unsigned count=0;
//	TextureClass* temp_textures[256];

	HashTemplateIterator<StringClass,TextureClass*> ite(TextureHash);
	for (ite.First();!ite.Is_Done();ite.Next()) {
		TextureClass* tex=ite.Peek_Value();
		if (tex->Num_Refs() <= 1) {
	/*		temp_textures[count++]=tex;
			if (count==256) {
				for (unsigned i=0;i<256;++i) {
					TextureHash.Remove(temp_textures[i]->Get_Texture_Name());
					temp_textures[i]->Release_Ref();
				}
				count=0;
				ite.First();	// iterator doesn't support modifying the hash table while iterating, so start from the
									// beginning.
			}*/
		}
		else
		{
			DEBUG_LOG(("**Texture \"%s\" referenced %d times on map reset",tex->Get_Texture_Name().str(),tex->Num_Refs()-1));
		}
	}
/*	for (unsigned i=0;i<count;++i) {
		TextureHash.Remove(temp_textures[i]->Get_Texture_Name());
		temp_textures[i]->Release_Ref();
	}*/
}

//---------------------------------------------------------------------
/**Report all used fonts*/
void W3DAssetManager::Report_Used_Font3DDatas()
{
	/*
	** for each font data in the list, get it, check it's refcount, and and release ref it if the
	** refcount is one.
	*/
	SLNode<Font3DDataClass> *node, * next;
	for (	node = Font3DDatas.Head(); node; node = next) {
		next = node->Next();
		Font3DDataClass *font = node->Data();
		if (font->Num_Refs() == 1) {
/*			Font3DDatas.Remove(font);
			font->Release_Ref();*/
		}
		else
		{
			DEBUG_LOG(("**Unfreed Font3DDatas On Map Reset: %s",font->Name));
		}
	}
}

//---------------------------------------------------------------------
// E&B Coloring Code
//---------------------------------------------------------------------
/*

//---------------------------------------------------------------------
// Uniqing
//---------------------------------------------------------------------

void W3DAssetManager::Make_Mesh_Unique(RenderObjClass *robj, Bool geometry, Bool colors)
{
	int i;
	MeshClass *mesh=(MeshClass*) robj;
	mesh->Make_Unique();
	MeshModelClass * model = mesh->Get_Model();

	if (colors)	{
		// make all vertex materials unique
		MaterialInfoClass	*material = mesh->Get_Material_Info();
		for (i=0; i<material->Vertex_Material_Count(); i++)
			material->Peek_Vertex_Material(i)->Make_Unique();
		REF_PTR_RELEASE(material);
		// make all color arrays unique
		model->Make_Color_Array_Unique(0);
		model->Make_Color_Array_Unique(1);
		// do not do textures yet
		// because we want to do the color conversion
		// for the top mip level and then
		// mip filter instead of converting all mip levels
	}

	if (geometry)	{
		// make geometry unique
		model->Make_Geometry_Unique();
	}

	REF_PTR_RELEASE(model);
}

void W3DAssetManager::Make_HLOD_Unique(RenderObjClass *robj, Bool geometry, Bool colors)
{
	int num_sub = robj->Get_Num_Sub_Objects();
	for(int i = 0; i < num_sub; i++) {
		RenderObjClass *sub_obj = robj->Get_Sub_Object(i);
		Make_Unique(sub_obj,geometry,colors);
		REF_PTR_RELEASE(sub_obj);
	}
}

void W3DAssetManager::Make_Unique(RenderObjClass *robj, Bool geometry, Bool colors)
{
	switch (robj->Class_ID())	{
	case RenderObjClass::CLASSID_MESH:
		Make_Mesh_Unique(robj,geometry,colors);
		break;
	case RenderObjClass::CLASSID_HLOD:
		Make_HLOD_Unique(robj,geometry,colors);
		break;
	}
}

static inline void Munge_Render_Obj_Name(char *newname, const char *oldname, float scale, const Vector3 &hsv_shift)
{
	char lower_case_name[255];
	strlcpy(lower_case_name, oldname, ARRAY_SIZE(lower_case_name));
	_strlwr(lower_case_name);
	sprintf(newname,"#%s!%gH%gS%gV%g", lower_case_name, scale, hsv_shift.X, hsv_shift.Y, hsv_shift.Z);
}

static inline void Munge_Texture_Name(char *newname, const char *oldname, const Vector3 &hsv_shift)
{
	char lower_case_name[255];
	strlcpy(lower_case_name, oldname, ARRAY_SIZE(lower_case_name));
	_strlwr(lower_case_name);
	sprintf(newname,"#%s!H%gS%gV%g", lower_case_name, hsv_shift.X, hsv_shift.Y, hsv_shift.Z);
}

RenderObjClass * W3DAssetManager::Create_Render_Obj(const char * name,float scale, const Vector3 &hsv_shift)
{
	Bool isGranny = false;
	Bool reallyscale = (WWMath::Fabs(scale - ident_scale) > scale_epsilon);
	Bool reallyhsv_shift = (WWMath::Fabs(hsv_shift.X - ident_HSV.X) > H_epsilon ||
		WWMath::Fabs(hsv_shift.Y - ident_HSV.Y) > S_epsilon || WWMath::Fabs(hsv_shift.Z - ident_HSV.Z) > V_epsilon);

	// base case, no scale or hue shifting
	if (!reallyscale && !reallyhsv_shift) return WW3DAssetManager::Create_Render_Obj(name);

	char newname[512];
	Munge_Render_Obj_Name(newname, name, scale, hsv_shift);

	// see if we got a cached version
	RenderObjClass *rendobj=nullptr;

	if (isGranny)
	{	//Granny objects share the same prototype since they allow instance scaling.
		strlcpy(newname, name, ARRAY_SIZE(newname));	//use same name for all granny objects at any scale.
	}
	Set_WW3D_Load_On_Demand(false); // munged name will never be found in a file.
	rendobj=WW3DAssetManager::Create_Render_Obj(newname);
	if (rendobj)
	{	if (isGranny)
			///@todo Granny objects are realtime scaled - fix to scale like W3D.
			rendobj->Set_ObjectScale(scale);
		Set_WW3D_Load_On_Demand(true); // Auto Load.
		return rendobj;
	}

	// create a new one based on
	// existing prototype

	WWPROFILE( "WW3DAssetManager::Create_Render_Obj" );
	WWMEMLOG(MEM_GEOMETRY);

	// Try to find a prototype
	PrototypeClass * proto = Find_Prototype(name);

	Set_WW3D_Load_On_Demand(true); // Auto Load.
	if (WW3D_Load_On_Demand && proto == nullptr) {	// If we didn't find one, try to load on demand
		char filename [MAX_PATH];
		char *mesh_name = ::strchr (name, '.');
		if (mesh_name != nullptr) {
			::lstrcpyn (filename, name, ((int)mesh_name) - ((int)name) + 1);
			if (isGranny)
				::lstrcat (filename, ".gr2");
			else
				::lstrcat (filename, ".w3d");
		} else {
			sprintf( filename, "%s.w3d", name);
		}

		// If we can't find it, try the parent directory
		if ( Load_3D_Assets( filename ) == false ) {
			StringClass	new_filename = StringClass("..\\") + filename;
			if (Load_3D_Assets( new_filename ) == false)
			{
				char *mesh_name = ::strchr (filename, '.');
				::lstrcpyn (mesh_name, ".gr2",5);
				Load_3D_Assets( filename );
				isGranny=true;
			}
		}

		proto = Find_Prototype(name);		// try again
	}

	if (proto == nullptr) {
		static int warning_count = 0;
		if (++warning_count <= 20) {
			WWDEBUG_SAY(("WARNING: Failed to create Render Object: %s",name));
		}
		return nullptr;		// Failed to find a prototype
	}

	rendobj=proto->Create();

	if (!rendobj) return nullptr;

	if (!isGranny)
	{	Make_Unique(rendobj,reallyscale,reallyhsv_shift);
		if (reallyscale) rendobj->Scale(scale);
		if (reallyhsv_shift) Recolor_Asset(rendobj,hsv_shift);
	}
	else
	{	///@todo Granny objects are realtime scaled - fix to scale like W3D.
		rendobj->Set_ObjectScale(scale);
		return rendobj;
	}

	W3DPrototypeClass *w3dproto=NEW W3DPrototypeClass(rendobj,newname);
	rendobj->Release_Ref();
	Add_Prototype(w3dproto);

	return w3dproto->Create();
}

TextureClass * W3DAssetManager::Get_Texture_With_HSV_Shift(const char * filename, const Vector3 &hsv_shift, TextureClass::MipCountType mip_level_count)
{
	WWPROFILE( "W3DAssetManager::Get_Texture with HSV shift" );

	Bool is_hsv_shift = (WWMath::Fabs(hsv_shift.X - ident_HSV.X) > H_epsilon ||
		WWMath::Fabs(hsv_shift.Y - ident_HSV.Y) > S_epsilon || WWMath::Fabs(hsv_shift.Z - ident_HSV.Z) > V_epsilon);

	if (!is_hsv_shift) {

		return Get_Texture(filename, mip_level_count);

	} else {

		//
		// Bail if the user isn't really asking for anything
		//
		if ((filename == nullptr) || (strlen(filename) == 0)) {
			return nullptr;
		}

		TextureClass *newtex = Find_Texture(filename, hsv_shift);

		if (!newtex) {

			// No cached texture - need to create
			char lower_case_name[255];
			strlcpy(lower_case_name, filename, ARRAY_SIZE(lower_case_name));
			_strlwr(lower_case_name);
			TextureClass *oldtex = TextureHash.Get(lower_case_name);
			if (!oldtex) {
				oldtex = NEW_REF(TextureClass,(lower_case_name, nullptr, mip_level_count));
				TextureHash.Insert(oldtex->Get_Texture_Name(), oldtex);
			}

			newtex = Recolor_Texture_One_Time(oldtex, hsv_shift);

			// If the recolorization failed, return the original texture
			if (!newtex) {
				newtex = oldtex;
				newtex->Add_Ref();
			}

		}

		return newtex;

	}
}

void W3DAssetManager::Recolor_Vertex_Material(VertexMaterialClass *vmat, const Vector3 &hsv_shift)
{
	Vector3 rgb;

	vmat->Get_Ambient(&rgb);
	Recolor(rgb,hsv_shift);
	vmat->Set_Ambient(rgb);

	vmat->Get_Diffuse(&rgb);
	Recolor(rgb,hsv_shift);
	vmat->Set_Diffuse(rgb);

	vmat->Get_Emissive(&rgb);
	Recolor(rgb,hsv_shift);
	vmat->Set_Emissive(rgb);

	vmat->Get_Specular(&rgb);
	Recolor(rgb,hsv_shift);
	vmat->Set_Specular(rgb);
}

void W3DAssetManager::Recolor_Vertices(unsigned int *color, int count, const Vector3 &hsv_shift)
{
	int i;
	Vector4 rgba;

	for (i=0; i<count; i++)
	{
		rgba=DX8Wrapper::Convert_Color(color[i]);
		Recolor(reinterpret_cast<Vector3&>(rgba),hsv_shift);
		color[i]=DX8Wrapper::Convert_Color_Clamp(rgba);
	}
}

TextureClass * W3DAssetManager::Recolor_Texture(TextureClass *texture, const Vector3 &hsv_shift)
{
	const char *name=texture->Get_Texture_Name();

	TextureClass *newtex = Find_Texture(name, hsv_shift);
	if (newtex) {
		return newtex;
	}

	return Recolor_Texture_One_Time(texture, hsv_shift);
}

TextureClass * W3DAssetManager::Recolor_Texture_One_Time(TextureClass *texture, const Vector3 &hsv_shift)
{
	const char *name=texture->Get_Texture_Name();

	// if texture is procedural return nullptr
	if (name && name[0]=='!') return nullptr;

	// make sure texture is loaded
	if (!texture->Is_Initialized())
		TextureLoader::Request_High_Priority_Loading(texture, (TextureClass::MipCountType)texture->Get_Mip_Level_Count());

	SurfaceClass::SurfaceDescription desc;
	SurfaceClass *newsurf, *oldsurf, *smallsurf;
	texture->Get_Level_Description(desc);

	// if texture is monochrome and no value shifting
	// return nullptr
	smallsurf=texture->Get_Surface_Level((TextureClass::MipCountType)texture->Get_Mip_Level_Count()-1);
	if (hsv_shift.Z==0.0f && smallsurf->Is_Monochrome())
	{
		REF_PTR_RELEASE(smallsurf);
		return nullptr;
	}
	REF_PTR_RELEASE(smallsurf);

	oldsurf=texture->Get_Surface_Level();

	newsurf=NEW_REF(SurfaceClass,(desc.Width,desc.Height,desc.Format));
	newsurf->Copy(0,0,0,0,desc.Width,desc.Height,oldsurf);
	newsurf->Hue_Shift(hsv_shift);
	TextureClass * newtex=NEW_REF(TextureClass,(newsurf,(TextureClass::MipCountType)texture->Get_Mip_Level_Count()));
	newtex->Set_Mag_Filter(texture->Get_Mag_Filter());
	newtex->Set_Min_Filter(texture->Get_Min_Filter());
	newtex->Set_Mip_Mapping(texture->Get_Mip_Mapping());
	newtex->Set_U_Addr_Mode(texture->Get_U_Addr_Mode());
	newtex->Set_V_Addr_Mode(texture->Get_V_Addr_Mode());

	char newname[512];
	Munge_Texture_Name(newname, name, hsv_shift);
	newtex->Set_Texture_Name(newname);

	TextureHash.Insert(newtex->Get_Texture_Name(), newtex);
	newtex->Add_Ref();

	REF_PTR_RELEASE(oldsurf);
	REF_PTR_RELEASE(newsurf);

	return newtex;
}

TextureClass * W3DAssetManager::Find_Texture(const char * name, const Vector3 &hsv_shift)
{
	char newname[512];
	Munge_Texture_Name(newname, name, hsv_shift);

	// see if we have a cached copy
	TextureClass *newtex = TextureHash.Get(newname);
	if (newtex) {
		newtex->Add_Ref();
	}
	return newtex;
}

void W3DAssetManager::Recolor_Mesh(RenderObjClass *robj, const Vector3 &hsv_shift)
{
	int i;

	MeshClass *mesh=(MeshClass*) robj;
	MeshModelClass * model = mesh->Get_Model();
	MaterialInfoClass	*material = mesh->Get_Material_Info();

	// recolor vertex material
	for (i=0; i<material->Vertex_Material_Count(); i++)
			Recolor_Vertex_Material(material->Peek_Vertex_Material(i),hsv_shift);

	// recolor color arrays
	unsigned int * color;
	color=model->Get_Color_Array(0,false);
	if (color) Recolor_Vertices(color,model->Get_Vertex_Count(),hsv_shift);
	color=model->Get_Color_Array(1,false);
	if (color) Recolor_Vertices(color,model->Get_Vertex_Count(),hsv_shift);

	// recolor textures

	TextureClass *newtex,*oldtex;
	for (i=0; i<material->Texture_Count(); i++)
	{
		oldtex=material->Peek_Texture(i);
		newtex=Recolor_Texture(oldtex,hsv_shift);
		if (newtex)
		{
			model->Replace_Texture(oldtex,newtex);
			material->Replace_Texture(i,newtex);
			REF_PTR_RELEASE(newtex);
		}
	}

	REF_PTR_RELEASE(material);
	REF_PTR_RELEASE(model);
}

void W3DAssetManager::Recolor_HLOD(RenderObjClass *robj, const Vector3 &hsv_shift)
{
	int num_sub = robj->Get_Num_Sub_Objects();
	for(int i = 0; i < num_sub; i++) {
		RenderObjClass *sub_obj = robj->Get_Sub_Object(i);
		Recolor_Asset(sub_obj,hsv_shift);
		REF_PTR_RELEASE(sub_obj);
	}
}

void W3DAssetManager::Recolor_ParticleEmitter(RenderObjClass *robj, const Vector3 &hsv_shift)
{
	unsigned int i;

	ParticleEmitterClass* emit=(ParticleEmitterClass*) robj;
	ParticlePropertyStruct<Vector3> colors;

	emit->Get_Color_Key_Frames(colors);
	Recolor(colors.Start,hsv_shift);
	Recolor(colors.Rand,hsv_shift);
	for (i=0; i<colors.NumKeyFrames; i++)
		Recolor(colors.Values[i],hsv_shift);
	emit->Reset_Colors(colors);

	delete colors.Values;
	delete colors.KeyTimes;

	TextureClass *tex=emit->Get_Texture();
	TextureClass *newtex=Recolor_Texture(tex,hsv_shift);
	if (newtex)
	{
		emit->Set_Texture(newtex);
		REF_PTR_RELEASE(newtex);
	}
	REF_PTR_RELEASE(tex);
}

void W3DAssetManager::Recolor_Asset(RenderObjClass *robj, const Vector3 &hsv_shift)
{
	switch (robj->Class_ID())	{
	case RenderObjClass::CLASSID_MESH:
		Recolor_Mesh(robj,hsv_shift);
		break;
	case RenderObjClass::CLASSID_HLOD:
		Recolor_HLOD(robj,hsv_shift);
		break;
	case RenderObjClass::CLASSID_PARTICLEEMITTER:
		Recolor_ParticleEmitter(robj,hsv_shift);
		break;
	}
}
*/
