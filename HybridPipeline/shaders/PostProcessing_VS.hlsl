// Trick
// Use a single triangle to cover the viewport(-1 1)
// Avoid rasterizing two triangle for performance improvements

float4 main( uint VertexID : SV_VertexID ) : SV_Position
{
    // Texture coordinates range [0, 2], but only [0, 1] appears on screen.
    float2 texCoord = float2( uint2( VertexID, VertexID << 1 ) & 2 );
    float4 position = float4( lerp( float2( -1, 1 ), float2( 1, -1 ), texCoord ), 0, 1 );

    return position;
}