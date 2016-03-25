#pragma once
#include "graphics/spotlightentity.h"
#include "graphics/modelentity.h"
#include "graphics/globallightentity.h"
#include "graphics/pointlightentity.h"
#include "graphicsfeatureunit.h"
#include "physicsfeatureunit.h"
#include "basegamefeatureunit.h"
#include "appgame/gameapplication.h"
#include "scriptingfeature/scriptingfeature.h"
#include "ui/uifeatureunit.h"
#include "effects/effectsfeatureunit.h"
#include "forest/forestrendermodule.h"
#include "posteffect/posteffectfeatureunit.h"

//------------------------------------------------------------------------------
/**
    @class Tools::TerrainRendererApplication
    
    terrain demo project with a terrain entity attached

    (C) 2011-2013 Individual contributors, see AUTHORS file
*/
namespace  Tools
{
class TerrainRendererApplication : public App::GameApplication
{
public:
	/// constructor
	TerrainRendererApplication(void);
	/// destructor
	virtual ~TerrainRendererApplication(void);
	/// open application
	virtual bool Open();
	/// close application
	virtual void Close();

private:

	/// setup application state handlers
	virtual void SetupStateHandlers();
	/// setup game features
	virtual void SetupGameFeatures();
	/// cleanup game features
	virtual void CleanupGameFeatures();

	Ptr<UI::UiFeatureUnit> uiFeature;
	Ptr<UI::UiLayout> mainLayout;
	Ptr<PhysicsFeature::PhysicsFeatureUnit> physicsFeature;
	Ptr<EffectsFeature::EffectsFeatureUnit> fxFeature;
	Ptr<GraphicsFeature::GraphicsFeatureUnit> graphicsFeature;
	Ptr<BaseGameFeature::BaseGameFeatureUnit> baseGameFeature;
	Ptr<ScriptingFeature::ScriptingFeatureUnit> scriptingFeature;	
	Ptr<PostEffect::PostEffectFeatureUnit> postEffectFeature;

};

}
