#pragma once

//------------------------------------------------------------------------------
/**
    @class Models::Block
    
	A block for clipmap grid
    
    (C) 2013-2015 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------

#include "math\float2.h"

namespace Terrain
{
	struct Block
	{
		size_t offset;
		size_t count;
		Math::float2 range;
	};

} // namespace Models
//------------------------------------------------------------------------------