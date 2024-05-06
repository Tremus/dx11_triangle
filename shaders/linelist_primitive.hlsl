struct VS_Input
{
    float2 pos : POS;
};

struct VS_Output
{
    float4 position : SV_POSITION;
};

VS_Output vs_main(VS_Input input)
{
    VS_Output output;
    output.position = float4(input.pos, 0.0f, 1.0f);

    return output;
}

half4 ps_main(VS_Output input) : SV_TARGET
{
    return half4(0, 0, 0, 1);
}