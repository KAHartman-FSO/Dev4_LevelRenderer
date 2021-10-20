// an ultra simple hlsl pixel shader
float4 main(float4 posH : SV_POSITION, float3 nrmW : NORMAL, float3 posW : WORLD) : SV_TARGET
{
	return float4(nrmW, 1);
}