Texture2D<float4> HDRTexture : register( t0 );

float3 LinearToSRGB(float3 x)
{
    // This is exactly the sRGB curve
    //return x < 0.0031308 ? 12.92 * x : 1.055 * pow(abs(x), 1.0 / 2.4) - 0.055;

    // This is cheaper but nearly equivalent
	return x < 0.0031308 ? 12.92 * x : 1.13005 * sqrt(abs(x - 0.00228)) - 0.13448 * x + 0.005719;
}


float4 main( float4 Position : SV_Position ) : SV_Target0
{
    int2 texCoord = ( int2 )Position.xy;
	float3 color = HDRTexture.Load(int3(texCoord.x, texCoord.y, 0)).rgb;
	
	//return float4(color.x / 20.0f, color.y / 20.0f, color.z / 20.0f, 1); // position testing
	return float4(color.r, color.r, color.r, 1); // metallic and roughness testing
	//return float4((normalize(color) + 1.0f) / 2.0f, 1); // normal testing from -1 1 -> 0 1
	//return float4(normalize(color), 1); // normal testing // ac 0 1 space
	//return float4(LinearToSRGB(color), 1); // albedo testing
}