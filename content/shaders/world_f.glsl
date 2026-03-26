#version 410 core

#define TF_NONE                 0x00000000u
#define TF_WALKABLE             0x00000001u
#define TF_UNDISCOVERED         0x00000002u
#define TF_SUNLIGHT             0x00000004u
#define TF_WET                  0x00000008u
#define TF_GRASS                0x00000010u
#define TF_NOPASS               0x00000020u
#define TF_BLOCKED              0x00000040u
#define TF_DOOR                 0x00000080u
#define TF_STOCKPILE            0x00000100u
#define TF_GROVE                0x00000200u
#define TF_FARM                 0x00000400u
#define TF_TILLED               0x00000800u
#define TF_WORKSHOP             0x00001000u
#define TF_ROOM                 0x00002000u
#define TF_LAVA                 0x00004000u
#define TF_WATER                0x00008000u
#define TF_JOB_FLOOR            0x00010000u
#define TF_JOB_WALL             0x00020000u
#define TF_JOB_BUSY_FLOOR       0x00040000u
#define TF_JOB_BUSY_WALL        0x00080000u
#define TF_MOUSEOVER            0x00100000u
#define TF_WALKABLEANIMALS      0x00200000u
#define TF_WALKABLEMONSTERS     0x00400000u
#define TF_PASTURE              0x00800000u
#define TF_INDIRECT_SUNLIGHT    0x01000000u
#define TF_TRANSPARENT          0x40000000u
#define TF_OVERSIZE             0x80000000u

#define WATER_TOP               0x01u
#define WATER_EDGE              0x02u
#define WATER_WALL              0x10u
#define WATER_FLOOR             0x20u
#define WATER_ONFLOOR           0x40u

#define CAT(x, y) CAT_(x, y)
#define CAT_(x, y) x ## y
#define UNPACKSPRITE(alias, src) uint CAT(alias, ID) = src & 0xffffu; uint CAT(alias, Flags) = src >> 16u;
layout(location = 0) noperspective in vec2 vTexCoords;
layout(location = 1) flat in uvec4  block1;
layout(location = 2) flat in uvec4  block2;
layout(location = 3) flat in uvec4  block3;
layout(location = 4) flat in uvec2  vTileExtra; // x=tileZ, y=aoFlags
layout(location = 5) flat in uvec4  block4;     // x=lightGradient, y=lightColorHint, z=shadowFlags, w=reserved

layout(location = 0) out vec4 fColor;

uniform sampler2DArray uTexture[16];

uniform int uTickNumber;

uniform int uUndiscoveredTex;
uniform int uWaterTex;

uniform int uWorldRotation;
uniform bool uOverlay;
uniform bool uDebug;
uniform bool uWallsLowered;
uniform float uDaylight;
uniform float uLightMin;
uniform bool uPaintFrontToBack;

uniform bool uShowJobs;

uniform float uViewLevel;
uniform float uRenderDepth;
uniform int uSeason;
uniform int uWeather;          // 0=clear, 1=storm, 2=heatwave, 3=coldsnap
uniform float uWeatherIntensity; // 0-1 ramp

const float waterAlpha = 0.6;
const float flSize =  ( 1.0 / 32. );
const int rightWallOffset = 4;
const int leftWallOffset = 8;

const vec3 perceivedBrightness = vec3(0.299, 0.587, 0.114);

vec4 getTexel( uint spriteID, uint rot, uint animFrame )
{
	uint absoluteId = ( spriteID + animFrame ) * 4;
	uint tex = absoluteId / 2048;
	uint localBaseId = absoluteId % 2048;
	uint localID = localBaseId + rot;
	
	ivec3 samplePos = ivec3( vTexCoords.x * 32, vTexCoords.y * 64, localID);

	// Need to unroll each access to texelFetch with a different element from uTexture into a distinct instruction
	// Otherwise we are triggering a bug on AMD GPUs, where threads start sampling from the wrong texture
	#define B(X) case X: return texelFetch( uTexture[X], samplePos, 0);
	#define C(X) B(X) B(X+1) B(X+2) B(X+3)
	#define D(X) C(X) C(X+4) C(X+8) C(X+12)
	switch(tex)
	{
		D(0)
	}
	#undef D
	#undef C
	#undef B
}

void main()
{
	vec4 texel = vec4( 0,  0,  0, 0 );
	
	uint rot = 0;
	uint spriteID = 0;
	uint animFrame = 0;
	
	UNPACKSPRITE(floorSprite, block1.x);
	UNPACKSPRITE(jobFloorSprite, block1.y);
	UNPACKSPRITE(wallSprite, block1.z);
	UNPACKSPRITE(jobWallSprite, block1.w);
	UNPACKSPRITE(itemSprite, block2.x);
	UNPACKSPRITE(creatureSprite, block2.y);

	uint vFluidLevelPacked1 = block2.z;
	bool uIsWall = ( block2.w != 0 );

	uint vFlags = block3.x;
	uint vFlags2 = block3.y;

	uint vLightLevel = block3.z;
	uint vVegetationLevel = block3.w;

	uint vLightGradient = block4.x;
	uint vShadowFlags = block4.z;

	uint vFluidLevel = (vFluidLevelPacked1 >> 0) & 0xffu;
	uint vFluidLevelLeft = (vFluidLevelPacked1 >> 8) & 0xffu;
	uint vFluidLevelRight = (vFluidLevelPacked1 >> 16) & 0xffu;
	uint vFluidFlags = (vFluidLevelPacked1 >> 24) & 0xffu;

	
	if( !uIsWall )
	{
		if( ( vFlags & TF_UNDISCOVERED ) != 0 && !uDebug )
		{
			if( !uWallsLowered )
			{
				vec4 tmpTexel = getTexel( uUndiscoveredTex / 4 + 2, 0, 0 );

				// Dark grey — visible shape without revealing content
				texel.rgb = vec3( 0.08, 0.08, 0.10 ) * tmpTexel.a;
				texel.a = tmpTexel.a;
			}
			// Skip all further floor rendering for undiscovered tiles
		}
		else
		{
			spriteID = floorSpriteID;
			if( spriteID != 0 )
			{
				rot = floorSpriteFlags & 3u;
				rot = ( rot + uWorldRotation ) % 4;
				
				if( ( floorSpriteFlags & 4u ) == 4 )
				{
					animFrame = ( uTickNumber / 10 ) % 4;
				}
				
				vec4 tmpTexel = getTexel( spriteID, rot, animFrame );
				
				if( ( vFlags & TF_GRASS ) != 0 )
				{
					vec4 roughFloor = getTexel( uUndiscoveredTex / 4 + 3, 0, 0 );
					float interpol = 1.0 - ( float( vVegetationLevel ) / 100. );
					tmpTexel.rgb = mix( tmpTexel.rgb, roughFloor.rgb, interpol * roughFloor.a );
					texel.a = max(tmpTexel.a , roughFloor.a);
				}

				texel.rgb = mix( texel.rgb, tmpTexel.rgb, tmpTexel.a );
				texel.a = max(texel.a , tmpTexel.a);
			}
			
			spriteID = jobFloorSpriteID;
			animFrame = 0;
			if( uShowJobs && ( spriteID != 0 )  )
			{
				rot = jobFloorSpriteFlags & 3u;
				rot = ( rot + uWorldRotation ) % 4;
				
				vec4 tmpTexel = getTexel( spriteID, rot, animFrame );
				
				if( ( vFlags & TF_JOB_BUSY_FLOOR ) != 0 )
				{
					tmpTexel.r *= 0.3;
					tmpTexel.g *= 0.7;
					tmpTexel.b *= 0.3;
				}
				else
				{
					tmpTexel.r *= 0.7;
					tmpTexel.g *= 0.7;
					tmpTexel.b *= 0.3;
				}

				texel.rgba = tmpTexel.rgba;
			}

			if( uOverlay && 0 != ( vFlags & ( TF_STOCKPILE | TF_FARM | TF_GROVE | TF_PASTURE | TF_WORKSHOP | TF_ROOM | TF_NOPASS ) ) )
			{
				vec3 roomColor = vec3( 0.0 );
			
				if( ( vFlags & TF_STOCKPILE ) != 0 ) //stockpile
				{
					roomColor = vec3(1, 1, 0);
				}
				
				else if( ( vFlags & TF_FARM ) != 0 ) //farm
				{
					roomColor = vec3(0.5, 0, 1);
				}
				else if( ( vFlags & TF_GROVE ) != 0 ) //grove
				{
					roomColor = vec3(0, 1, 0.5);
				}
				else if( ( vFlags & TF_PASTURE ) != 0 ) 
				{
					roomColor = vec3(0, 0.9, 0.9);
				}
				else if( ( vFlags & TF_WORKSHOP ) != 0 ) //workshop
				{
					if( ( vFlags & TF_BLOCKED ) != 0 )
					{
						roomColor = vec3(1, 0, 0);
					}
					else
					{
						roomColor = vec3(1, 1, 0);
					}
				}
				else if( ( vFlags & TF_ROOM ) != 0 ) //room
				{
					roomColor = vec3(0, 0, 1);
				}
				else if( ( vFlags & TF_NOPASS ) != 0 ) //room
				{
					roomColor = vec3(1, 0, 0);
				}
				if( texel.a != 0 )
				{
					float brightness = dot(texel.rgb, perceivedBrightness.xyz);
					// Preserve perceived brightness of original pixel during tinting, but drop saturation partially
					texel.rgb = mix( roomColor, mix( texel.rgb, vec3(1,1,1) * brightness, 0.7), 0.7);
				}
				
			}
		}
		////////////////////////////////////////////////////////////////////////////////////////////////////
		//
		// water related calculations
		//
		////////////////////////////////////////////////////////////////////////////////////////////////////
		if( ( vFluidFlags & ( WATER_FLOOR | WATER_EDGE ) ) != 0 )
		{
			bool renderAboveFloor = ( vFluidFlags & WATER_ONFLOOR ) != 0;
			int startLevel =  renderAboveFloor ? 2 : int(min(vFluidLevel, 2));
			int referenceLevel = int(vTexCoords.x < 0.5 ? vFluidLevelLeft : vFluidLevelRight);
			int offset = vTexCoords.x < 0.5 ? leftWallOffset : rightWallOffset;

			float fl = float( startLevel - 2 ) * flSize;

			vec4 tmpTexel = vec4( 0, 0, 0, 0 );

			if( ( vFluidFlags & WATER_FLOOR ) != 0 )
			{
				float y = vTexCoords.y + fl;
				tmpTexel = texture( uTexture[0], vec3( vec2( vTexCoords.x, y ), uWaterTex ) );
			}

			if( ( vFluidFlags & WATER_EDGE ) != 0 )
			{
				float y = vTexCoords.y + fl;
				for(int i = startLevel; i > referenceLevel; --i)
				{
					y -= flSize;
					tmpTexel += texture( uTexture[0], vec3( vec2( vTexCoords.x, y ), uWaterTex + offset ) );
				}
			}

			// Turn into slight tint instead
			if( renderAboveFloor && vFluidLevel == 1 )
			{
				tmpTexel.a *= 0.5;
			}


			texel.rgb = mix( texel.rgb, tmpTexel.rgb, waterAlpha * tmpTexel.a );
			texel.a = max(texel.a , tmpTexel.a);

			// Water caustics — animated dappled light on shallow water
			if( vFluidLevel <= 4u && vFluidLevel >= 1u )
			{
				float time = float(uTickNumber) * 0.05;
				vec2 p = vTexCoords * 4.0 + vec2( float(vTileExtra.x) * 0.3, float(vTileExtra.x) * 0.7 );
				float c1 = sin( p.x * 3.0 + time ) * cos( p.y * 2.5 - time * 0.7 );
				float c2 = sin( p.x * 2.0 - time * 0.5 + 1.5 ) * cos( p.y * 3.5 + time * 0.3 );
				float caustic = ( c1 + c2 ) * 0.5 + 0.5;
				caustic = caustic * caustic;
				float shallowFactor = 1.0 - float( vFluidLevel ) / 5.0;
				texel.rgb += vec3( 0.15, 0.2, 0.25 ) * caustic * shallowFactor * 0.4;
			}
		}
		////////////////////////////////////////////////////////////////////////////////////////////////////
		//
		// end water related calculations
		//
		////////////////////////////////////////////////////////////////////////////////////////////////////


	}
	else
	{
		if( ( vFlags & TF_UNDISCOVERED ) != 0 && !uDebug )
		{
			if( !uWallsLowered )
			{
				vec4 tmpTexel = getTexel( uUndiscoveredTex / 4, 0, 0 );

				// Dark grey — visible shape without revealing content
				texel.rgb = vec3( 0.08, 0.08, 0.10 ) * tmpTexel.a;
				texel.a = tmpTexel.a;
			}
			// Skip all further wall rendering for undiscovered tiles
		}
		else
		{
			spriteID = wallSpriteID;
			if( spriteID != 0 )
			{
				rot = wallSpriteFlags & 3u;
				rot = ( rot + uWorldRotation ) % 4;

				if( ( wallSpriteFlags & 4u ) == 4 )
				{
					animFrame = ( uTickNumber / 3 ) % 4;
				}
				if( uWallsLowered )
				{
					animFrame = 0;
					if( ( wallSpriteFlags & 8u ) == 8 )
					{
						spriteID = spriteID + 1;
					}
				}
				vec4 tmpTexel = getTexel( spriteID, rot, animFrame );
				
				texel.rgb = mix( texel.rgb, tmpTexel.rgb, tmpTexel.a );
				texel.a = max(texel.a , tmpTexel.a);
			}
			
			spriteID = itemSpriteID;
			animFrame = 0;
			if( spriteID != 0 )
			{
				rot = itemSpriteID & 3u;
				rot = ( rot + uWorldRotation ) % 4;
				
				vec4 tmpTexel = getTexel( spriteID, rot, animFrame );
				
				texel.rgb = mix( texel.rgb, tmpTexel.rgb, tmpTexel.a );
				texel.a = max(texel.a , tmpTexel.a);
			}
		}
	
		spriteID = jobWallSpriteID;
		animFrame = 0;
		if( uShowJobs && spriteID != 0 )
		{
			rot = jobWallSpriteFlags & 3u;
			rot = ( rot + uWorldRotation ) % 4;
			
			vec4 tmpTexel = getTexel( spriteID, rot, animFrame );
			
			if( ( vFlags & TF_JOB_BUSY_WALL ) != 0 )
			{
				tmpTexel.r *= 0.3;
				tmpTexel.g *= 0.7;
				tmpTexel.b *= 0.3;
			}
			else
			{
				tmpTexel.r *= 0.7;
				tmpTexel.g *= 0.7;
				tmpTexel.b *= 0.3;
			}

			texel.rgba = tmpTexel.rgba;
		}

	
		// Only render creatures on discovered tiles
		if( ( vFlags & TF_UNDISCOVERED ) == 0 || uDebug )
		{
			spriteID = creatureSpriteID;
			animFrame = 0;
			if( spriteID != 0 )
			{
				rot = creatureSpriteFlags & 3u;
				rot = ( rot + uWorldRotation ) % 4;

				vec4 tmpTexel = getTexel( spriteID, rot, animFrame );

				texel.rgb = mix( texel.rgb, tmpTexel.rgb, tmpTexel.a );
				texel.a = max(texel.a , tmpTexel.a);
			}
		}
		
		////////////////////////////////////////////////////////////////////////////////////////////////////
		//
		// water related calculations
		//
		////////////////////////////////////////////////////////////////////////////////////////////////////
		if( ( vFluidFlags & ( WATER_TOP | WATER_WALL ) ) != 0 && vFluidLevel > 2 )
		{
			int startLevel = int(vFluidLevel - 2);
			int referenceLevel = int( vTexCoords.x < 0.5 ? max(2, vFluidLevelLeft) : max(2, vFluidLevelRight) ) - 2;
			int offset = vTexCoords.x < 0.5 ? leftWallOffset : rightWallOffset;

			float fl = float( startLevel ) * flSize;

			vec4 tmpTexel = vec4( 0, 0, 0, 0 );
			
			if( ( vFluidFlags & WATER_TOP ) != 0 )
			{
				float y = vTexCoords.y + fl;
				tmpTexel = texture( uTexture[0], vec3( vec2( vTexCoords.x, y ), uWaterTex ) );
			}

			if( ( vFluidFlags & WATER_WALL ) != 0)
			{
				float y = vTexCoords.y + fl;
				for(int i = startLevel; i > referenceLevel; --i)
				{
					y -= flSize;
					tmpTexel += texture( uTexture[0], vec3( vec2( vTexCoords.x, y ), uWaterTex + offset ) );
				}
			}
			
			texel.rgb = mix( texel.rgb, tmpTexel.rgb, waterAlpha * tmpTexel.a );
			texel.a = max(texel.a , tmpTexel.a);
		}
		////////////////////////////////////////////////////////////////////////////////////////////////////
		//
		// end water related calculations
		//
		////////////////////////////////////////////////////////////////////////////////////////////////////
	}

	if( texel.a <= 0 )
	{
		discard;
	}
	else if(uPaintFrontToBack)
	{
		// Flush to 1 in case of front-to-back rendering
		texel.a = 1;
	}
	
	
	if( !uDebug && ( vFlags & TF_UNDISCOVERED ) == 0 )
	{
		// === LIGHTING MODEL: Light vs Darkness ===
		float torchLight = float( vLightLevel ) / 20.0;
		float light = torchLight;
		bool hasSunlight = ( vFlags & ( TF_SUNLIGHT | TF_INDIRECT_SUNLIGHT ) ) != 0;
		bool isWallTile = uIsWall;

		if( hasSunlight )
		{
			light = max( light, uDaylight );
		}

		// Discovered walls get a base ambient visibility — you remember what you've mined
		float ambientWall = 0.0;
		if( isWallTile && ( vFlags & TF_UNDISCOVERED ) == 0 )
		{
			ambientWall = 0.12; // subtle wall visibility even in darkness
		}
		light = max( light, ambientWall );

		// Smooth S-curve falloff — gradual transition from lit to dark with grey boundary zone
		float lightCurved = smoothstep( 0.0, 1.0, light );

		// True darkness: near-black in unlit areas, but with a grey buffer zone
		float lightMult = mix( uLightMin, 1.0, lightCurved );

		// Desaturation in darkness: color drains smoothly, grey zone before full darkness
		float saturation = smoothstep( 0.0, 0.5, light );
		float brightness = dot( texel.rgb, perceivedBrightness.xyz );
		// In ambient-only areas (walls in darkness), desaturate more strongly
		if( light <= ambientWall + 0.01 && ambientWall > 0.0 )
		{
			saturation *= 0.3; // heavily desaturated — grey stone memory
		}
		texel.rgb = mix( brightness * vec3( 1.0 ), texel.rgb, saturation ) * lightMult;

		// Underground cave atmosphere — gradual shift to deep darkness
		if( !hasSunlight && light < 0.4 )
		{
			vec3 caveColor = vec3( 0.05, 0.04, 0.07 );
			float caveBlend = smoothstep( 0.4, 0.0, light ) * 0.6;
			texel.rgb = mix( texel.rgb, caveColor, caveBlend );
		}

		// Night atmosphere — oppressive cold blue outdoors
		float nightAmount = clamp( 1.0 - uDaylight, 0.0, 1.0 );
		if( nightAmount > 0.0 )
		{
			vec3 nightTint = vec3( 0.55, 0.62, 1.0 );
			float nightStrength = nightAmount * nightAmount;
			if( hasSunlight )
			{
				texel.rgb = mix( texel.rgb, texel.rgb * nightTint * 0.6, nightStrength * 0.7 );
			}
			else if( torchLight < 0.1 )
			{
				texel.rgb = mix( texel.rgb, texel.rgb * nightTint * 0.7, nightStrength * 0.4 );
			}
		}

		// Torch warmth — only in non-sunlit areas (sunlight dominates)
		if( torchLight > 0.01 && !hasSunlight )
		{
			vec3 torchColor = vec3( 1.0, 0.85, 0.55 );
			float warmth = torchLight * torchLight;
			// Multiplicative tint for color
			texel.rgb = mix( texel.rgb, texel.rgb * torchColor, min( warmth * 0.6, 1.0 ) );
			// Additive HDR boost — pushes bright torch-lit pixels above 1.0 for bloom extraction
			texel.rgb += torchColor * warmth * 0.15;
		}

		// Light boundary glow — warm halo at the edge of light radius
		float boundaryGlow = smoothstep( 0.05, 0.3, light ) * smoothstep( 0.6, 0.3, light );
		texel.rgb += vec3( 0.6, 0.35, 0.1 ) * boundaryGlow * 0.08;

		// Seasonal color grading
		vec3 seasonGrade;
		float seasonStrength;
		if( uSeason == 0 )      { seasonGrade = vec3( 0.90, 1.10, 0.85 ); seasonStrength = 0.3; }  // spring: lush green
		else if( uSeason == 1 ) { seasonGrade = vec3( 1.10, 1.02, 0.85 ); seasonStrength = 0.25; } // summer: warm golden
		else if( uSeason == 2 ) { seasonGrade = vec3( 1.15, 0.85, 0.60 ); seasonStrength = 0.4; }  // autumn: strong amber
		else                    { seasonGrade = vec3( 0.80, 0.88, 1.15 );  seasonStrength = 0.35; } // winter: cold blue-grey
		texel.rgb = mix( texel.rgb, texel.rgb * seasonGrade, seasonStrength );

		// Weather effects on sunlit tiles
		if( hasSunlight && uWeatherIntensity > 0.0 )
		{
			float wi = uWeatherIntensity;
			if( uWeather == 1 ) // Storm — darken, increase saturation, blue tint
			{
				texel.rgb *= mix( 1.0, 0.7, wi );
				float grey = dot( texel.rgb, perceivedBrightness );
				texel.rgb = mix( vec3(grey), texel.rgb, 1.0 + wi * 0.3 );
				texel.rgb = mix( texel.rgb, texel.rgb * vec3( 0.8, 0.85, 1.0 ), wi * 0.3 );
			}
			else if( uWeather == 2 ) // HeatWave — warm, slight haze
			{
				texel.rgb = mix( texel.rgb, texel.rgb * vec3( 1.1, 1.0, 0.85 ), wi * 0.3 );
			}
			else if( uWeather == 3 ) // ColdSnap — brighten, desaturate, blue shift
			{
				float grey = dot( texel.rgb, perceivedBrightness );
				texel.rgb = mix( texel.rgb, vec3(grey), wi * 0.3 );
				texel.rgb = mix( texel.rgb, texel.rgb * vec3( 0.9, 0.95, 1.15 ), wi * 0.4 );
				texel.rgb *= mix( 1.0, 1.1, wi );
			}
		}

		// Depth fog — fade tiles below view level toward atmospheric haze
		if( uRenderDepth > 1.0 )
		{
			float zBelow = uViewLevel - float(vTileExtra.x);
			float depthFade = clamp( ( zBelow - 1.0 ) / ( uRenderDepth - 1.0 ), 0.0, 1.0 );
			depthFade *= depthFade;
			float fogStrength = 0.6;
			if( uWeather == 1 || uWeather == 3 )
				fogStrength += uWeatherIntensity * 0.2;
			// Fog color adapts to darkness — darker fog at night
			vec3 nightFog = vec3( 0.05, 0.05, 0.08 );
			vec3 dayFog   = vec3( 0.45, 0.50, 0.60 );
			vec3 fogColor = mix( nightFog, dayFog, uDaylight );
			texel.rgb = mix( texel.rgb, fogColor, depthFade * fogStrength );
		}

		// Ambient occlusion — darken edges near solid walls
		uint aoFlags = vTileExtra.y;
		if( aoFlags != 0u )
		{
			float aoStrength = 0.25;
			float ao = 0.0;

			if( ( aoFlags & 0x01u ) != 0u )
				ao += 0.15;

			uint rotatedAO = aoFlags >> 1;
			rotatedAO = ( ( rotatedAO >> uint(uWorldRotation) ) | ( rotatedAO << ( 4u - uint(uWorldRotation) ) ) ) & 0x0fu;

			float northAO = float( ( rotatedAO & 0x01u ) != 0u );
			float eastAO  = float( ( rotatedAO & 0x02u ) != 0u );
			float southAO = float( ( rotatedAO & 0x04u ) != 0u );
			float westAO  = float( ( rotatedAO & 0x08u ) != 0u );

			float topDist    = vTexCoords.y;
			float bottomDist = 1.0 - vTexCoords.y;
			float leftDist   = vTexCoords.x;
			float rightDist  = 1.0 - vTexCoords.x;

			float edgeFalloff = 0.25;
			ao += northAO * smoothstep( edgeFalloff, 0.0, rightDist ) * smoothstep( edgeFalloff, 0.0, topDist ) * aoStrength;
			ao += eastAO  * smoothstep( edgeFalloff, 0.0, rightDist ) * smoothstep( edgeFalloff, 0.0, bottomDist ) * aoStrength;
			ao += southAO * smoothstep( edgeFalloff, 0.0, leftDist )  * smoothstep( edgeFalloff, 0.0, bottomDist ) * aoStrength;
			ao += westAO  * smoothstep( edgeFalloff, 0.0, leftDist )  * smoothstep( edgeFalloff, 0.0, topDist ) * aoStrength;

			texel.rgb *= ( 1.0 - min( ao, 0.5 ) );
		}

		// Wall shadow casting — directional shadows from light-blocking walls
		if( vShadowFlags != 0u && light > 0.05 && light < 0.8 )
		{
			// Rotate shadow directions by world rotation (same pattern as AO)
			uint rotShadow = ( ( vShadowFlags >> uint(uWorldRotation) ) | ( vShadowFlags << ( 4u - uint(uWorldRotation) ) ) ) & 0x0fu;

			float shadowDarken = 0.0;
			float shadowFalloff = 0.35;

			float topDist    = vTexCoords.y;
			float bottomDist = 1.0 - vTexCoords.y;
			float leftDist   = vTexCoords.x;
			float rightDist  = 1.0 - vTexCoords.x;

			// Directional shadow darkening from each wall direction
			if( ( rotShadow & 0x01u ) != 0u ) // shadow from screen-north (top-right)
				shadowDarken += smoothstep( shadowFalloff, 0.0, rightDist ) * smoothstep( shadowFalloff, 0.0, topDist ) * 0.18;
			if( ( rotShadow & 0x02u ) != 0u ) // shadow from screen-east (bottom-right)
				shadowDarken += smoothstep( shadowFalloff, 0.0, rightDist ) * smoothstep( shadowFalloff, 0.0, bottomDist ) * 0.18;
			if( ( rotShadow & 0x04u ) != 0u ) // shadow from screen-south (bottom-left)
				shadowDarken += smoothstep( shadowFalloff, 0.0, leftDist ) * smoothstep( shadowFalloff, 0.0, bottomDist ) * 0.18;
			if( ( rotShadow & 0x08u ) != 0u ) // shadow from screen-west (top-left)
				shadowDarken += smoothstep( shadowFalloff, 0.0, leftDist ) * smoothstep( shadowFalloff, 0.0, topDist ) * 0.18;

			// Shadows are most visible in partially lit areas (not full bright, not full dark)
			float shadowVisibility = smoothstep( 0.05, 0.2, light ) * smoothstep( 0.8, 0.4, light );
			texel.rgb *= ( 1.0 - min( shadowDarken * shadowVisibility, 0.4 ) );
		}
	}

	// Mouseover rim highlight
	if( ( vFlags & TF_MOUSEOVER ) != 0 )
	{
		float edgeDist = min( min( vTexCoords.x, 1.0 - vTexCoords.x ), min( vTexCoords.y, 1.0 - vTexCoords.y ) );
		float rim = smoothstep( 0.0, 0.08, edgeDist );
		texel.rgb = mix( vec3( 1.0, 0.95, 0.7 ), texel.rgb, rim * 0.7 + 0.3 );
	}

	fColor = texel;
}
