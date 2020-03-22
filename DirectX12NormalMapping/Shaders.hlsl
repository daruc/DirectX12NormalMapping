struct VS_INPUT
{
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float2 texCoord : TEXCOORD;
};

struct VS_OUTPUT
{
	float4 pos : WORLD_POS;
	float3 worldPos : WORLD_POS1;
	float4 lightWvpPos : TEXCOORD1;
	float4 wvpPos : SV_POSITION;
	float3 normal : NORMAL;
	float2 texCoord : TEXCOORD;
};

cbuffer WvpConstantBuffer : register(b0)
{
	float4x4 world;
	float4x4 wvp;
	float3 cameraPos;
	float4x4 lightWvp;
	float3 lightWorldPos;
	float3 lightDirection;	// light's normalized camera forward vector
	float lightFov;
};

Texture2D tex : register(t0);
Texture2D normalTex : register(t1);
Texture2D depthTex : register(t2);
SamplerState samplerState : register(s0);
SamplerComparisonState cmpSampler : register(s1);

VS_OUTPUT vsMain(VS_INPUT input)
{
	VS_OUTPUT output;

	output.pos = float4(input.pos, 1.0f);
	output.wvpPos = mul(output.pos, wvp);

	output.worldPos = mul(output.pos, world);

	float3 worldNormal = mul(input.normal, world);
	output.normal = worldNormal;

	output.texCoord = input.texCoord;

	output.lightWvpPos = float4(input.pos, 1.0f);
	output.lightWvpPos = mul(output.lightWvpPos, lightWvp);

	return output;
}

float4 psMain(VS_OUTPUT input) : SV_TARGET
{
	float4 baseColor = tex.Sample(samplerState, input.texCoord);
	bool inShadow = false;
	const float ambient = 0.2f;

	input.lightWvpPos /= input.lightWvpPos.w;

	float2 shadowmapTexCoord = input.lightWvpPos.xy;
	shadowmapTexCoord.x = shadowmapTexCoord.x / 2.0 + 0.5f;
	shadowmapTexCoord.y = -shadowmapTexCoord.y / 2.0 + 0.5f;

	const float bias = 0.00001f;	// to avoid self shadowing
	input.lightWvpPos.z -= bias;


	float lightFactor = 0.0f;
	const float shadowMapRes = 1024.0f;

	for (float y = -0.5f; y <= 0.5f; ++y)
	{
		for (float x = -0.5f; x <= 0.5f; ++x)
		{
			float2 offset;
			offset.x = x / shadowMapRes;
			offset.y = y / shadowMapRes;
			lightFactor += depthTex.SampleCmpLevelZero(cmpSampler, shadowmapTexCoord + offset, input.lightWvpPos.z);
		}
	}

	lightFactor /= 4.0f;

	float depthFromVertices = input.lightWvpPos.z;
	float3 lightVec = normalize(input.worldPos - lightWorldPos);

	inShadow = lightFactor <= 0.0f ||
		(dot(lightDirection, lightVec) < cos(lightFov / 2.0f));


	if (inShadow)
	{
		return clamp(baseColor * ambient, 0.0f, 1.0f);
	}
	else
	{
		float diffuse = clamp(dot(-lightVec, input.normal), 0.0f, 1.0f);
		float3 cameraDir = normalize(cameraPos - input.pos.xyz);
		float3 specularDir = lightVec - 2 * dot(lightVec, input.normal) * input.normal;
		float specular = clamp(dot(specularDir, cameraDir), 0.0f, 1.0f);
		specular = pow(specular, 2);
		return clamp(baseColor * (ambient + lightFactor * (0.8 * diffuse + 0.5f * specular)), 0.0f, 1.0f);
	}
}

VS_OUTPUT vsDepth(VS_INPUT input)
{
	VS_OUTPUT output;

	output.pos = float4(input.pos, 1.0f);
	output.wvpPos = mul(output.pos, lightWvp);

	return output;
}

void psDepth(VS_OUTPUT input)
{
	// depth only
}