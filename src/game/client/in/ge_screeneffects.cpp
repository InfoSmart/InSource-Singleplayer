///////////// Copyright � 2009, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_screeneffects.cpp
// Description:
//      Post process effects for GoldenEye: Source
//
// Created On: 25 Nov 09
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////
 
#include "cbase.h"
#include "screenspaceeffects.h"
#include "rendertexture.h"
#include "model_types.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imaterialvar.h"
#include "cdll_client_int.h"
#include "materialsystem/itexture.h"
#include "keyvalues.h"
#include "ClientEffectPrecacheSystem.h"
//#include "view_scene.h" //Uncomment me if you plan to use multiple screeneffects at once.
 
#include "ge_screeneffects.h"
#include "tier0/vprof.h"
 
ADD_SCREENSPACE_EFFECT( CEntGlowEffect, ge_entglow );
 
static float rBlack[4] = {0,0,0,1};
static float rWhite[4] = {1,1,1,1};
 
void CEntGlowEffect::Init( void ) 
{
	// Initialize the white overlay material to render our model with
	KeyValues *pVMTKeyValues = new KeyValues( "VertexLitGeneric" );
	pVMTKeyValues->SetString("$basetexture", "vgui/white");
	pVMTKeyValues->SetInt("$selfillum", 1);
	pVMTKeyValues->SetString("$selfillummask", "vgui/white");
	pVMTKeyValues->SetInt("$vertexalpha", 1);
	pVMTKeyValues->SetInt("$model", 1);
	m_WhiteMaterial.Init("__geglowwhite", TEXTURE_GROUP_CLIENT_EFFECTS, pVMTKeyValues);
	m_WhiteMaterial->Refresh();
 
	// Initialize the Effect material that will be blitted to the Frame Buffer
	KeyValues *pVMTKeyValues2 = new KeyValues( "UnlitGeneric" );
	pVMTKeyValues2->SetString("$basetexture", "_rt_FullFrameFB");
	pVMTKeyValues2->SetInt("$additive", 1);
	m_EffectMaterial.Init("__geglowcomposite", TEXTURE_GROUP_CLIENT_EFFECTS, pVMTKeyValues2);
	m_EffectMaterial->Refresh();
 
	// Initialize render targets for our blurring
	m_GlowBuff1.InitRenderTarget(ScreenWidth()/2, ScreenHeight()/2, RT_SIZE_DEFAULT, IMAGE_FORMAT_RGBA8888, MATERIAL_RT_DEPTH_SEPARATE, false, "_rt_geglowbuff1" );
	m_GlowBuff2.InitRenderTarget(ScreenWidth()/2, ScreenHeight()/2, RT_SIZE_DEFAULT, IMAGE_FORMAT_RGBA8888, MATERIAL_RT_DEPTH_SEPARATE, false, "_rt_geglowbuff2" );
 
	// Load the blur textures
	m_BlurX.Init(materials->FindMaterial("effects/glowx", TEXTURE_GROUP_OTHER, true));
	m_BlurY.Init(materials->FindMaterial("effects/glowy", TEXTURE_GROUP_OTHER, true));
}
 
void CEntGlowEffect::Shutdown()
{
	// Tell the materials we are done referencing them
	m_WhiteMaterial.Shutdown();
	m_EffectMaterial.Shutdown();
 
	m_GlowBuff1.Shutdown();
	m_GlowBuff2.Shutdown();
 
	m_BlurX.Shutdown();
	m_BlurY.Shutdown();
}
 
//-----------------------------------------------------------------------------
// Purpose: Render the effect
//-----------------------------------------------------------------------------
ConVar cl_ge_glowscale("cl_ge_glowscale",		"0.05",	FCVAR_CLIENTDLL);
ConVar cl_ge_glowstencil("cl_ge_glowstencil",	"1",	FCVAR_CLIENTDLL);
 
void CEntGlowEffect::Render(int x, int y, int w, int h)
{
	VPROF("CEntGlowEffect::Render");
 
	// Don't bother rendering if we have nothing to render!
	if ( !m_vGlowEnts.Count() || !IsEnabled() )
		return;
 
	// Grab the render context
	CMatRenderContextPtr pRenderContext(materials);
 
	// Apply our glow buffers as the base textures for our blurring operators
	IMaterialVar *var;
	// Set our effect material to have a base texture of our primary glow buffer
	var = m_BlurX->FindVar("$basetexture", NULL);
	var->SetTextureValue(m_GlowBuff1);
	var = m_BlurY->FindVar("$basetexture", NULL);
	var->SetTextureValue(m_GlowBuff2);
	var = m_EffectMaterial->FindVar("$basetexture", NULL);
	var->SetTextureValue(m_GlowBuff1);
 
	var = m_BlurX->FindVar("$bloomscale", NULL);
	var->SetFloatValue(10*cl_ge_glowscale.GetFloat());
	var = m_BlurY->FindVar("$bloomamount", NULL);
	var->SetFloatValue(10*cl_ge_glowscale.GetFloat());
 
	// Clear the glow buffer from the previous iteration
	pRenderContext->ClearColor4ub(0, 0, 0, 255);
	pRenderContext->PushRenderTargetAndViewport(m_GlowBuff1);
	pRenderContext->ClearBuffers(true, true);
	pRenderContext->PopRenderTargetAndViewport();
 
	pRenderContext->PushRenderTargetAndViewport(m_GlowBuff2);
	pRenderContext->ClearBuffers(true, true);
	pRenderContext->PopRenderTargetAndViewport();
 
	// Clear the stencil buffer in case someone dirtied it this frame
	pRenderContext->ClearStencilBufferRectangle(0, 0, ScreenWidth(), ScreenHeight(), 0);
 
	// Iterate over our registered entities and add them to the cut-out stencil and the glow buffer
	for ( int i=0; i < m_vGlowEnts.Count(); i++ )
	{
		if ( cl_ge_glowstencil.GetInt() )
			RenderToStencil(i, pRenderContext);

		RenderToGlowTexture(i, pRenderContext);
	}
 
	// Now we take the built up glow buffer (m_GlowBuff1) and blur it two ways
	// the intermediate buffer (m_GlowBuff2) allows us to do this properly
	pRenderContext->PushRenderTargetAndViewport(m_GlowBuff2);
		pRenderContext->DrawScreenSpaceQuad(m_BlurX);
	pRenderContext->PopRenderTargetAndViewport();
 
	pRenderContext->PushRenderTargetAndViewport( m_GlowBuff1);
		pRenderContext->DrawScreenSpaceQuad( m_BlurY );
	pRenderContext->PopRenderTargetAndViewport();
 
	if ( cl_ge_glowstencil.GetInt() )
	{
		// Setup the renderer to only draw where the stencil is not 1
		pRenderContext->SetStencilEnable( true );
		pRenderContext->SetStencilReferenceValue( 0 );
		pRenderContext->SetStencilTestMask( 1 );
		pRenderContext->SetStencilCompareFunction( STENCILCOMPARISONFUNCTION_EQUAL );
		pRenderContext->SetStencilPassOperation( STENCILOPERATION_ZERO );
	}
 
	// Finally draw our blurred result onto the screen
	pRenderContext->DrawScreenSpaceQuad( m_EffectMaterial );
	//DrawScreenEffectMaterial( m_EffectMaterial, x, y, w, h ); //Uncomment me and comment the above line if you plan to use multiple screeneffects at once.
 
	pRenderContext->SetStencilEnable( false );
}
 
void CEntGlowEffect::RenderToStencil(int idx, IMatRenderContext *pRenderContext)
{
	if ( idx < 0 || idx >= m_vGlowEnts.Count() )
		return;
 
	C_BaseEntity *pEnt = m_vGlowEnts[idx]->m_hEnt.Get();

	if ( !pEnt )
	{
		// We don't exist anymore, remove us!
		delete m_vGlowEnts[idx];
		m_vGlowEnts.Remove(idx);
		return;
	}
 
	pRenderContext->SetStencilEnable(true);
	pRenderContext->SetStencilFailOperation( STENCILOPERATION_KEEP );
	pRenderContext->SetStencilZFailOperation( STENCILOPERATION_KEEP );
	pRenderContext->SetStencilPassOperation( STENCILOPERATION_REPLACE );
	pRenderContext->SetStencilCompareFunction( STENCILCOMPARISONFUNCTION_ALWAYS );
	pRenderContext->SetStencilWriteMask( 1 );
	pRenderContext->SetStencilReferenceValue( 1 );
 
	pRenderContext->DepthRange( 0.0f, 0.01f );
	render->SetBlend( 0 );
 
	modelrender->ForcedMaterialOverride( m_WhiteMaterial );
		pEnt->DrawModel( STUDIO_RENDER );
	modelrender->ForcedMaterialOverride( NULL );
 
	render->SetBlend( 1 );
	pRenderContext->DepthRange( 0.0f, 1.0f );
 
	pRenderContext->SetStencilEnable( false );
}
 
void CEntGlowEffect::RenderToGlowTexture( int idx, IMatRenderContext *pRenderContext )
{
	if ( idx < 0 || idx >= m_vGlowEnts.Count() )
		return;
 
	C_BaseEntity *pEnt = m_vGlowEnts[idx]->m_hEnt.Get();
	if ( !pEnt )
	{
		// We don't exist anymore, remove us!
		delete m_vGlowEnts[idx];
		m_vGlowEnts.Remove(idx);
		return;
	}
 
	pRenderContext->PushRenderTargetAndViewport( m_GlowBuff1 );
 
	modelrender->SuppressEngineLighting( true );
	render->SetColorModulation( m_vGlowEnts[idx]->m_fColor );
 
	modelrender->ForcedMaterialOverride( m_WhiteMaterial );
		pEnt->DrawModel( STUDIO_RENDER );
	modelrender->ForcedMaterialOverride( NULL );
 
	render->SetColorModulation( rWhite );
	modelrender->SuppressEngineLighting( false );
 
	pRenderContext->PopRenderTargetAndViewport();
}
 
void CEntGlowEffect::RegisterEnt( EHANDLE hEnt, Color glowColor /*=Color(255,255,255,64)*/, float fGlowScale /*=1.0f*/ )
{
	// Don't add duplicates
	if ( FindGlowEnt(hEnt) != -1 || !hEnt.Get() )
		return;
 
	sGlowEnt *newEnt = new sGlowEnt;
	newEnt->m_hEnt = hEnt;
	newEnt->m_fColor[0] = glowColor.r() / 255.0f;
	newEnt->m_fColor[1] = glowColor.g() / 255.0f;
	newEnt->m_fColor[2] = glowColor.b() / 255.0f;
	newEnt->m_fColor[3] = glowColor.a() / 255.0f;
	newEnt->m_fGlowScale = fGlowScale;
	m_vGlowEnts.AddToTail( newEnt );
}
 
void CEntGlowEffect::DeregisterEnt( EHANDLE hEnt )
{
	int idx = FindGlowEnt(hEnt);
	if ( idx == -1 )
		return;
 
	delete m_vGlowEnts[idx];
	m_vGlowEnts.Remove( idx );
}
 
void CEntGlowEffect::SetEntColor( EHANDLE hEnt, Color glowColor )
{
	int idx = FindGlowEnt(hEnt);
	if ( idx == -1 )
		return;
 
	m_vGlowEnts[idx]->m_fColor[0] = glowColor.r() / 255.0f;
	m_vGlowEnts[idx]->m_fColor[1] = glowColor.g() / 255.0f;
	m_vGlowEnts[idx]->m_fColor[2] = glowColor.b() / 255.0f;
	m_vGlowEnts[idx]->m_fColor[3] = glowColor.a() / 255.0f;
}
 
void CEntGlowEffect::SetEntGlowScale( EHANDLE hEnt, float fGlowScale )
{
	int idx = FindGlowEnt(hEnt);
	if ( idx == -1 )
		return;
 
	m_vGlowEnts[idx]->m_fGlowScale = fGlowScale;
}
 
int CEntGlowEffect::FindGlowEnt( EHANDLE hEnt )
{
	for ( int i=0; i < m_vGlowEnts.Count(); i++ )
	{
		if ( m_vGlowEnts[i]->m_hEnt == hEnt )
			return i;
	}
 
	return -1;
}