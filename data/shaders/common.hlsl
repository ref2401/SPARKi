static const float c_pi = 3.14159265358979;
static const float c_pi_2 = c_pi / 2.0f;
static const float c_2pi = 2.0f * c_pi;
static const float c_1_pi = 1.0f / c_pi;


float3x3 tangent_to_world_matrix(float3 oz)
{
	// see Unreal engine GetTangentBasis():
	// https://github.com/EpicGames/UnrealEngine/blob/f794321ffcad597c6232bc706304c0c9b4e154b2/Engine/Shaders/MonteCarlo.usf
	
	// all the verctors are in world space
	const float3 up = (abs(oz.z) < 0.999f) ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
	const float3 ox = normalize(cross(up, oz));
	const float3 oy = cross(oz, ox);
	return transpose(float3x3(ox, oy, oz));
}

float3 tangent_to_world(float3 v, float3 oz)
{
	return mul(tangent_to_world_matrix(oz), v);
}

// Calculates tangent of an angle which cosine is given.
// Returns infinity if cosine equals to zero.
float tangent(float cos_angle)
{
	const float sin_angle = sqrt(1.0f - cos_angle * cos_angle);
	return sin_angle / cos_angle;
}