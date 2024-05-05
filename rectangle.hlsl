struct VS_Input
{
    float2 pos : POS;
};

struct VS_Output
{
    float4 position : SV_POSITION;
};

cbuffer square_cbuffer : register(b0)
{
    float4 data;
};

VS_Output vs_main(VS_Input input)
{
    VS_Output output;
    output.position = float4(input.pos, 0.0f, 1.0f);

    return output;
}

float4 ps_main(VS_Output input) : SV_TARGET
{
    return data;
}