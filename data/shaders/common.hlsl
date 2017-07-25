static const float pi = 3.14159265358979;


float3x3 tangent_space_matrix(float3 oz)
{
	// see Unreal engine GetTangentBasis():
	// https://github.com/EpicGames/UnrealEngine/blob/f794321ffcad597c6232bc706304c0c9b4e154b2/Engine/Shaders/MonteCarlo.usf

	const float3 up = (abs(oz.z) < 0.999f) ? float3(0, 0, 1) : float3(1, 0, 0);
	const float3 ox = normalize(cross(up, oz));
	const float3 oy = cross(oz, ox);
	return float3x3(ox, oy, oz);
}

float3 tangent_to_world(float3 v, float oz)
{
	return mul(tangent_space_matrix(oz), v);
}
