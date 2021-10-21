// an ultra simple hlsl pixel shader
struct OBJ_ATTR
{
	float3		Kd;				// diffuse reflectivity
	float			d;					// dissolve (transparency) 
	float3		Ks;				// specular reflectivity
	float			Ns;				// specular exponent
	float3		Ka;				// ambient reflectivity
	float			sharpness;	// local reflection map sharpness
	float3		Tf;					// transmission filter
	float			Ni;				// optical density (index of refraction)
	float3		Ke;				// emissive reflectivity
	float			illum;			// illumination model
};
struct LEVEL_MODEL_DATA
{
	float4 lightDirection;
	float4 lightColor;
	float4 ambientLight;
	float4 cameraPos;
	float4x4 vMatrix;
	float4x4 pMatrix;
	float4 pointLightPositions[16];
	float4 pointLightColor;
	float4x4 wMatrix[1024];
	OBJ_ATTR material[1024];
	float numLights;
};
[[vk::push_constant]]
cbuffer MATRIX_DATA {
	float meshID;
	float materialID;
};

StructuredBuffer<LEVEL_MODEL_DATA> SCENE_DATA;
float4 main(float4 posH : SV_POSITION, float3 nrmW : NORMAL, float3 posW : WORLD) : SV_TARGET
{
	// SetUp
	float4 OUTPUT;
	nrmW = normalize(nrmW);

	// The Variables of  Readability
	float4 SURFACE_COLOR = float4(SCENE_DATA[0].material[materialID].Kd, 1);
	
	// Directional Lighting
	float4 dir_light_ratio = saturate(dot(-SCENE_DATA[0].lightDirection, nrmW));
	float4 dir_light = SURFACE_COLOR * dir_light_ratio * SCENE_DATA[0].lightColor;

	// Ambient Lighting
	float4 ambient_light = saturate(SCENE_DATA[0].ambientLight * SURFACE_COLOR * 0.5);

	// Specular Lighting
	float3 viewDir = normalize(SCENE_DATA[0].cameraPos - posW);
	float3 halfVector = normalize((-SCENE_DATA[0].lightDirection) + viewDir);
	float intensity;
	intensity = saturate(dot(nrmW, halfVector));
	intensity = pow(intensity, SCENE_DATA[0].material[meshID].Ns);
	intensity = max(intensity, 0);

	float4 spec_light = SCENE_DATA[0].lightColor * float4(SCENE_DATA[0].material[materialID].Ks, 1) * intensity;

	float4 point_lights = float4(0,0,0,0);
	// Point Lights
	for (float i = 0; i < SCENE_DATA[0].numLights; i++)
	{
		// Point Light Calculations
		float4 p_LightDir = normalize(SCENE_DATA[0].pointLightPositions[i] - float4(posW, 1));
		float4 p_LightRatio = saturate(dot(p_LightDir, float4(nrmW, 1)));

		// Range Attenuation
		float Attenuation = 1 - saturate(length(SCENE_DATA[0].pointLightPositions[i] - float4(posW, 1)) / 5);
		point_lights = point_lights + (p_LightRatio * SCENE_DATA[0].pointLightColor * Attenuation);
	}
	
	OUTPUT = saturate(dir_light + ambient_light + spec_light + point_lights);
	return OUTPUT;
}