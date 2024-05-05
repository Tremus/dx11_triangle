struct VS_Output
{
    float4 position : SV_POSITION;
    float2 rpos : NORMAL;
};

cbuffer CONSTANTS : register(b0)
{
    float2 data;
};

VS_Output vs_main(float2 pos : POS)
{
    VS_Output output;
    output.position = float4(pos, 0.0f, 1.0f);
    output.rpos = pos;

    return output;
}

float4 ps_main(VS_Output input) : SV_TARGET
{
    float4 col = float4(0, 0, 0, 1);
    float distance = 1.0 - length(input.rpos);
    distance = smoothstep(0.0, 0.002, distance);
    col.rgb = distance;

    return col;
}