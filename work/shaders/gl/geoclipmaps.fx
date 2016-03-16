//------------------------------------------------------------------------------
//  geoclipmaps.fx
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/shared.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"

sampler2D height_map;
sampler2D albedo_map;

float height_map_size;
float height_map_multiplier;
vec4 instance_debug_color;
int base_instance;

// struct PerInstanceData
// {
	// vec2 offset; // World-space offset in XZ plane.
	// float scale; // Scaling factor for vertex offsets (per-instance)
	// float level; // lod-level to use when sampling heightmap
	// //vec3 debug_color;
// };

#define HEIGHTMAP_MIN -20.0
#define HEIGHTMAP_MAX 20.0
#define INSTANCE_SIZE 500

shared varblock InstanceData[bool System = true;] // Use std140 packing rules for uniform block. set binding point to 0
{
	//PerInstanceData instance[256];
	vec2 offset[INSTANCE_SIZE]; // World-space offset in XZ plane.
	float scale[INSTANCE_SIZE]; // Scaling factor for vertex offsets (per-instance)
	float level[INSTANCE_SIZE]; // lod-level to use when sampling heightmap
};



samplerstate HeightmapSampler
{
	Samplers = {height_map};
	Filter = Linear;
	AddressU = Wrap;
	AddressV = Wrap;
};

samplerstate HeightmapSampler1
{
	Samplers = {albedo_map};
	//Filter = Linear;
	AddressU = Mirror;
	AddressV = Mirror;
};

state GeoclipState;
// {
	// CullMode = None;	
	// BlendEnabled[0] = true;
	// SrcBlend[0] = SrcAlpha;	
	// DstBlend[0] = OneMinusSrcAlpha;
	// DepthEnabled = true;
	// DepthWrite = true;
	// MultisampleEnabled = true;
// };

state WireframeState
{
	CullMode = None;
	FillMode = Line;
};

//------------------------------------------------------------------------------
/**
*/
shader
void
vsGeoclipmap(in vec2 position, out float height_value, out vec4 debug_color, out vec2 terrain_uv, out float terrain_level) //out uvcoord
{
	vec2 local_offset = position * scale[gl_InstanceID];
	vec2 pos = offset[gl_InstanceID] + local_offset;
	
	float level = level[gl_InstanceID];
	vec2 uvcoord = pos/height_map_size; //send in the size of the heightmap
	float height = textureLod(height_map, uvcoord, level).r;
	//height = clamp(height, HEIGHTMAP_MIN, HEIGHTMAP_MAX);
	
	vec4 vert = vec4(pos.x, height_map_multiplier*height, pos.y, 1.0);
	
	gl_Position = ViewProjection * vert;
	
	height_value = height;
	debug_color = instance_debug_color;
	terrain_uv = uvcoord;
	terrain_level = level;
	
	
	// vec4 pos = vec4(position.x + EyePos.x, 0, position.y + EyePos.z, 1);
	// gl_Position = ViewProjection * pos;
	// WorldPos = pos;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psGeoclipmap(in float height_value, in vec4 debug_color, in vec2 terrain_uv, in float terrain_level, [color0] out vec4 color) //in uvcoord to sample
{
	vec3 color1 = textureLod(albedo_map, terrain_uv, terrain_level).rgb;
	//color = vec4(color1, 1);
	
	//color = vec4(height_value, height_value, height_value, 1);
	color = debug_color;
}

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(Geoclipmap, "GeoclipmapStatic", vsGeoclipmap(), psGeoclipmap(), GeoclipState);
SimpleTechnique(Geoclipmap_Wireframe, "GeoclipmapWireframe", vsGeoclipmap(), psGeoclipmap(), WireframeState);