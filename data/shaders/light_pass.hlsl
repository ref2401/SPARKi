// ----- vertex shader ------

struct vertex {
	uint id : SV_VertexID;
};

struct vs_output {
	float4 position : SV_Position;
};

vs_output vs_main(vertex vertex)
{
	vs_output o;
	o.position = float4(0, 0, 0, 1);
	return o;
}

// ----- pixel shader -----

struct ps_output {
	float4 rt_color0 : SV_Target0;
};

ps_output ps_main(vs_output pixel)
{
	ps_output o;
	o.rt_color0 = float4(1, 0, 0, 1);
	return o;
}
