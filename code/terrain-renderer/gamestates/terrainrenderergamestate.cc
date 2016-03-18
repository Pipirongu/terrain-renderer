//------------------------------------------------------------------------------
//  DemoGameState.cc
//  (C) 2013-2015 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "terrainrenderergamestate.h"
#include "math/vector.h"
#include "math/matrix44.h"
#include "graphicsfeatureunit.h"
#include "managers/factorymanager.h"
#include "managers/entitymanager.h"
#include "managers/enventitymanager.h"
#include "scriptfeature/managers/scripttemplatemanager.h"
#include "attr/attribute.h"
#include "graphicsfeature/graphicsattr/graphicsattributes.h"
#include "managers/focusmanager.h"
#include "input/keyboard.h"
#include "scriptingfeature/properties/scriptingproperty.h"
#include "scriptingfeature/scriptingprotocol.h"
#include "effects/effectsfeatureunit.h"
#include "terrain-renderer/terrainrendererapplication.h"

namespace Tools
{
	__ImplementClass(Tools::TerrainRendererGameState, 'TRGS', BaseGameFeature::GameStateHandler);

using namespace BaseGameFeature;
using namespace GraphicsFeature;
using namespace Util;
using namespace Math;
using namespace Input;


//------------------------------------------------------------------------------
/**
*/
TerrainRendererGameState::TerrainRendererGameState()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
TerrainRendererGameState::~TerrainRendererGameState()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainRendererGameState::OnStateEnter(const Util::String& prevState)
{
	GameStateHandler::OnStateEnter(prevState);	

	//// setup the camera util object
	//this->mayaCameraUtil.Setup(point(0.0f, 0.0f, 0.0f), point(200.0f, 10.f, 200.0f), vector(0.0f, 1.0f, 0.0f));
	//this->mayaCameraUtil.Update();
	//this->camera->SetTransform(this->mayaCameraUtil.GetCameraTransform());
	//const Ptr<Graphics::CameraEntity>& camera = Graphics::GraphicsServer::Instance()->GetCurrentView()->GetCameraEntity();
	//Math::matrix44 test;
	//test.set_position(float4(0, 10, 0, 1));
	//camera->SetTransform(test);
	//const Math::frustum viewFrustum = camera->GetViewFrustum();
	//viewFrustum.

	this->stage = GraphicsFeature::GraphicsFeatureUnit::Instance()->GetDefaultStage();

	const Ptr<UI::UiLayout>& layout = UI::UiFeatureUnit::Instance()->GetLayout("demo");
	Ptr<UI::UiElement> element = layout->GetElement("updatetext");
	element->SetText("Entered state");

	//////////////////////////////////////////////////////////////////////////
	this->terrain_entity = Graphics::TerrainEntity::Create();
	this->stage->AttachEntity(this->terrain_entity.cast<Graphics::GraphicsEntity>());
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainRendererGameState::OnStateLeave(const Util::String& nextState)
{
	GameStateHandler::OnStateLeave(nextState);
}

//------------------------------------------------------------------------------
/**
*/
Util::String 
TerrainRendererGameState::OnFrame()
{
	//handle all user input
	if (Input::InputServer::HasInstance())
	{
		this->HandleInput();
	}
		
	return GameStateHandler::OnFrame();
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainRendererGameState::OnLoadBefore()
{
	
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainRendererGameState::OnLoadAfter()
{

}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainRendererGameState::HandleInput()
{
	const Ptr<Input::Keyboard>& kbd = Input::InputServer::Instance()->GetDefaultKeyboard();


	// reload layout if key gets pressed
	if (kbd->KeyDown(Input::Key::F1))
	{
		this->terrain_entity->SetSurface("sur:geoclipmap_surfaces/geoclipmap");
	}
	if (kbd->KeyDown(Input::Key::F2))
	{
		this->terrain_entity->SetSurface("sur:geoclipmap_surfaces/wireframe");
	}
	if (kbd->KeyDown(Input::Key::Escape))
	{
		TerrainRendererApplication::Instance()->RequestState("Exit");
	}
}

} // namespace Tools