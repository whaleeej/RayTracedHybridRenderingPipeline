Texture2D<float4> Texture : register( t0 );

float3 LinearToSRGB(float3 x)
{
    // This is exactly the sRGB curve
	return x < 0.0031308 ? 12.92 * x : 1.055 * pow(abs(x), 1.0 / 2.4) - 0.055;

    // This is cheaper but nearly equivalent
	//return x < 0.0031308 ? 12.92 * x : 1.13005 * sqrt(abs(x - 0.00228)) - 0.13448 * x + 0.005719;
}

float3 simpleToneMapping(float3 c)
{
	return float3(c.x / (c.x + 1.0f), c.y / (c.y + 1.0f), c.z / (c.z + 1.0f));
}


float4 main( float4 Position : SV_Position ) : SV_Target0
{
    int2 texCoord = ( int2 )Position.xy;
	float3 color = Texture.Load(int3(texCoord.x, texCoord.y, 0)).rgb;
	
	//return float4(color.x / 10.0f, color.y / 10.0f, color.z / 10.0f, 1); // position testing
	//return float4(color.r, color.r, color.r, 1); // metallic and roughness testing
	//return float4(((color) + 1.0f) / 2.0f, 1); // normal testing from -1 1 -> 0 1
	//return float4(normalize(color), 1); // normal testing // ac 0 1 space
	return float4((color), 1); // albedo testing
	//return float4(LinearToSRGB(simpleToneMapping(color)), 1); // albedo testing
	
}