//------------------------------------------------------------------------------
//  DemoGameState.cc
//  (C) 2013-2015 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "terrainrendergamestate.h"
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
#include "terrainrender/terrainrenderapplication.h"

namespace Tools
{
	__ImplementClass(Tools::TerrainRenderGameState, 'TRGS', BaseGameFeature::GameStateHandler);

using namespace BaseGameFeature;
using namespace GraphicsFeature;
using namespace Util;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
TerrainRenderGameState::TerrainRenderGameState()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
TerrainRenderGameState::~TerrainRenderGameState()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainRenderGameState::OnStateEnter(const Util::String& prevState)
{
	GameStateHandler::OnStateEnter(prevState);	

	const Ptr<UI::UiLayout>& layout = UI::UiFeatureUnit::Instance()->GetLayout("demo");
	Ptr<UI::UiElement> element = layout->GetElement("updatetext");
	element->SetText("Entered state");
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainRenderGameState::OnStateLeave(const Util::String& nextState)
{
	GameStateHandler::OnStateLeave(nextState);
}

//------------------------------------------------------------------------------
/**
*/
Util::String 
TerrainRenderGameState::OnFrame()
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
TerrainRenderGameState::OnLoadBefore()
{
	
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainRenderGameState::OnLoadAfter()
{

}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainRenderGameState::HandleInput()
{
	const Ptr<Input::Keyboard>& kbd = Input::InputServer::Instance()->GetDefaultKeyboard();


	// reload layout if key gets pressed
	if (kbd->KeyDown(Input::Key::F1))
	{
        const Ptr<UI::UiLayout>& layout = UI::UiFeatureUnit::Instance()->GetLayout("demo");
        layout->Reload();
	}
	if(kbd->KeyDown(Input::Key::X))
	{
		TerrainRenderApplication::Instance()->RequestState("Exit");
	}
	if (kbd->KeyDown(Input::Key::F2))
	{
		EffectsFeature::EffectsFeatureUnit::Instance()->EmitGraphicsEffect(Math::matrix44::translation(n_rand(-5, 5), 10, n_rand(-5, 5)), "mdl:particles/newparticle.n3", 10.0f);
	}
}
} // namespace Tools