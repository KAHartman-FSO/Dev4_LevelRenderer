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
struct SHADER_MODEL_DATA
{
	float4 lightDirection;
	float4 lightColor;
	float4 ambientLight;
	float4 cameraPos;
	float4x4 vMatrix;
	float4x4 pMatrix;
	float4x4 wMatrix[1024];
	OBJ_ATTR material[1024];
};
[[vk::push_constant]]
cbuffer MESH_INDEX {
	uint mesh_ID;
};
StructuredBuffer<SHADER_MODEL_DATA> SCENE_DATA;
float4 main(float4 posH : SV_POSITION, float3 nrmW : NORMAL, float3 posW : WORLD) : SV_TARGET
{
	// Setup
	float4 OUTPUT;
	nrmW = normalize(nrmW);																								// Normalize normals beacause of Automatic LERP in Rasterizer
	float4 lightDirection = normalize(SCENE_DATA[0].lightDirection);									// Normalize Light Direction so DOT products in teh future are good
	float4 surface_color = float4(SCENE_DATA[0].material[mesh_ID].Kd, 1);						// Store Surface Color (pixel color) in an easier to access variable
	//			Directional Lighting
	float4 directionalLightRatio = saturate(dot(-lightDirection, nrmW));								// Calculate Ratio of How much Light is added to surface from Directional
	float4 dir_Light = surface_color * directionalLightRatio * SCENE_DATA[0].lightColor;	// surface color *  dir_light ratio * light color
	//			Ambient Lighting
	float4 ambient_Light = saturate(SCENE_DATA[0].ambientLight * surface_color);			// Get Ambient Light
	//			Specular Lighting
	float3 viewDirection = normalize(SCENE_DATA[0].cameraPos - posW);							// Get View Direction Vector by Subtracting surfPos from camPos
	float3 halfVector = normalize((-lightDirection) + viewDirection);									// Get the Half-Vector between viewDirection and -LightDirection
	float intensity;																													// Calculate Intensity
	intensity = saturate(dot(nrmW, halfVector));																	//		Get Ratio between Half Vector and nrmW
	intensity = pow(intensity, SCENE_DATA[0].material[mesh_ID].Ns);									//		Raise it to the power of Spec_Exponent
	intensity = max(intensity, 0);																							//		Make sure result is greater than 0

	// intensity * specular_intensity (provided) * light color
	float4 spec_Light = SCENE_DATA[0].lightColor * float4(SCENE_DATA[0].material[mesh_ID].Ks, 1) * intensity;

	OUTPUT = saturate(dir_Light + ambient_Light + spec_Light);										// Add all Lighting together
	return OUTPUT;
}