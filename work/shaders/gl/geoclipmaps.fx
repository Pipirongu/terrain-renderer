//------------------------------------------------------------------------------
//  geoclipmaps.fx
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/shared.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"

sampler2D height_map;

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
	Samplers = { height_map };
	Filter = Linear;
	AddressU = Wrap;
	AddressV = Wrap;
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

//------------------------------------------------------------------------------
/**
*/
shader
void
vsGeoclipmap(in vec2 position, out float height_value) //in heightmap size, out uvcoord
{
	vec2 local_offset = position * scale[gl_InstanceID];
	vec2 pos = offset[gl_InstanceID] + local_offset;
	
	float level = level[gl_InstanceID];
	vec2 uvcoord = pos/4096; //send in the size of the heightmap
	float height = textureLod(height_map, uvcoord, level).r;
	//height = clamp(height, HEIGHTMAP_MIN, HEIGHTMAP_MAX);
	
	vec4 vert = vec4(pos.x, 100*height, pos.y, 1.0);
	
	gl_Position = ViewProjection * vert;
	
	height_value = height;
	
	
	// vec4 pos = vec4(position.x + EyePos.x, 0, position.y + EyePos.z, 1);
	// gl_Position = ViewProjection * pos;
	// WorldPos = pos;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psGeoclipmap(in float height_value, [color0] out vec4 color) //in uvcoord to sample
{
	color = vec4(height_value, height_value, height_value, 1);
	
	// float len = 1.0 - smoothstep(1.0f, 250.0f, distance(EyePos.xz, WorldPos.xz));
	// vec2 uv = (WorldPos.xz / vec2(GridSize)) - vec2(0.5);
	// vec4 c = texture(GridTex, uv).rgba;
	// //vec2 line = (vec2(cos(WorldPos.x / GridSize), cos(WorldPos.z / GridSize)) - vec2(0.90, 0.90));
	// //float c = saturate(max(line.x, line.y)) * 5;
	
	// //c = smoothstep(0.5f, 1, c);
	// float alpha = len * c.a;
	// color = vec4(c.rgb, saturate(alpha));
}

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(Geoclipmap, "GeoclipmapStatic", vsGeoclipmap(), psGeoclipmap(), GeoclipState);