// Effects File
// Written By: Utkarsh Khokhar
// Dated     : 14 - August - 2012

struct Light
{
	float3 dir;
	float3 pos;
	float4 ambient;
	float4 diffuse;
	float4 specular;
};

struct Material
{
	float4 ambient;
	float4 diffuse;
	float4 emissive;
	float4 specular;
	float shininess;
};

//-----------------------------------------------------------------------------
// Globals.
//-----------------------------------------------------------------------------

float4x4 worldMatrix;
float4x4 worldInverseTransposeMatrix;

float3 cameraPos;
float4 globalAmbient;

Light light;
Material material;

//-----------------------------------------------------------------------------
// Textures.
//-----------------------------------------------------------------------------

texture colorMapTexture;

sampler2D colorMapL = sampler_state
{
    Texture = <colorMapTexture>;
    MagFilter = Linear;
    MinFilter = Linear;
    MipFilter = Point;
};

sampler2D colorMapA = sampler_state
{
    Texture = <colorMapTexture>;
    MagFilter = Linear;
    MinFilter = Anisotropic;
    MipFilter = Linear;
    MaxAnisotropy = 4;
};

//-----------------------------------------------------------------------------
// Vertex Shaders.
//-----------------------------------------------------------------------------

struct VS_INPUT
{
	float3 position : POSITION;
	float2 texCoord : TEXCOORD0;
	float3 normal : NORMAL;
};

struct VS_OUTPUT_DIR
{
	float4 position : POSITION;
	float2 texCoord : TEXCOORD0;
	float3 halfVector : TEXCOORD1;
	float3 lightDir : TEXCOORD2;
	float3 normal : TEXCOORD3;
	float4 diffuse : COLOR0;
	float4 specular : COLOR1;
};

VS_OUTPUT_DIR VS_Lighting(VS_INPUT IN)
{
	VS_OUTPUT_DIR OUT;

	float3 worldPos = mul(float4(IN.position, 1.0f), worldMatrix).xyz;
	float3 viewDir = cameraPos - worldPos;
		
	OUT.position = float4(IN.position, 1.0f);
	OUT.texCoord = IN.texCoord;
	OUT.lightDir = -light.dir;
	OUT.halfVector = normalize(normalize(OUT.lightDir) + normalize(viewDir));
	OUT.normal = mul(IN.normal, (float3x3)worldInverseTransposeMatrix);
	OUT.diffuse = material.diffuse * light.diffuse;
	OUT.specular = material.specular * light.specular;
	        
	return OUT;
}

//-----------------------------------------------------------------------------
// Pixel Shaders.
//-----------------------------------------------------------------------------

float4 PS_LightingL(VS_OUTPUT_DIR IN) : COLOR
{
    float3 n = normalize(IN.normal);
    float3 h = normalize(IN.halfVector);
    float3 l = normalize(IN.lightDir);
    
    float nDotL = saturate(dot(n, l));
    float nDotH = saturate(dot(n, h));
    float power = (nDotL == 0.0f) ? 0.0f : pow(nDotH, material.shininess);   

    float4 color = (material.ambient * (globalAmbient + light.ambient)) +
                   (IN.diffuse * nDotL) + (IN.specular * power);

    return color * tex2D(colorMapL, IN.texCoord);
}

float4 PS_LightingA(VS_OUTPUT_DIR IN) : COLOR
{
    float3 n = normalize(IN.normal);
    float3 h = normalize(IN.halfVector);
    float3 l = normalize(IN.lightDir);

    float nDotL = saturate(dot(n, l));
    float nDotH = saturate(dot(n, h));
    float power = (nDotL == 0.0f) ? 0.0f : pow(nDotH, material.shininess);

    float4 color = (material.ambient * (globalAmbient + light.ambient)) +
                   (IN.diffuse * nDotL) + (IN.specular * power);

    return color * tex2D(colorMapA, IN.texCoord);
}

//-----------------------------------------------------------------------------
// Techniques.
//-----------------------------------------------------------------------------

technique UTKShadingL
{
	pass
	{
		VertexShader = compile vs_2_0 VS_Lighting();
		PixelShader = compile ps_2_0 PS_LightingL();
	}
}

technique UTKShadingA
{
	pass
	{
		VertexShader = compile vs_2_0 VS_Lighting();
		PixelShader = compile ps_2_0 PS_LightingA();
	}
}
