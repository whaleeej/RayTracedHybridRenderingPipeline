struct VertexPositionNormalTexture
{
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD;
};

struct VertexShaderOutput
{
    float4 PositionWS : POSITION;
    float3 NormalWS   : NORMAL;
    float2 TexCoord   : TEXCOORD;
    float4 Position   : SV_Position;
};

struct Mat
{
	matrix ModelMatrix;
	matrix InverseTransposeModelMatrix;
	matrix ModelViewProjectionMatrix;
};
ConstantBuffer<Mat> MatCB : register(b0, space0);

VertexShaderOutput main(VertexPositionNormalTexture IN)
{
    VertexShaderOutput OUT;

    OUT.Position = mul( MatCB.ModelViewProjectionMatrix, float4(IN.Position, 1.0f));
	OUT.PositionWS = mul(MatCB.ModelMatrix, float4(IN.Position, 1.0f));
	OUT.NormalWS = mul((float3x3) MatCB.InverseTransposeModelMatrix, IN.Normal);
    OUT.TexCoord = IN.TexCoord;

    return OUT;
}