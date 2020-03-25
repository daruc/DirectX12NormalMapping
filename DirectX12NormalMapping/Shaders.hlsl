struct VS_INPUT
{
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float2 texCoord : TEXCOORD;
};

struct VS_OUTPUT
{
	float4 pos : WORLD_POS;
	float3 worldPos : WORLD_POS1;
	float4 lightWvpPos : TEXCOORD1;
	float4 wvpPos : SV_POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
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
Texture2D occlusionTex : register(t2);
Texture2D roughnessTex : register(t3);
Texture2D depthTex : register(t4);
SamplerState samplerState : register(s0);
SamplerComparisonState cmpSampler : register(s1);

VS_OUTPUT vsMain(VS_INPUT input)
{
	VS_OUTPUT output;

	output.pos = float4(input.pos, 1.0f);
	output.wvpPos = mul(output.pos, wvp);

	output.worldPos = mul(output.pos, world);

	float3 worldNormal = normalize(mul(input.normal, world));
	output.normal = worldNormal;

	float3 worldTangent = mul(input.tangent, world);
	output.tangent = worldTangent;

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

	float occlusion = tex.Sample(samplerState, input.texCoord);
	occlusion = clamp(occlusion - 0.5f, -0.5f, 0.5f);

	if (inShadow)
	{
		return clamp(baseColor * (ambient + 0.4 * occlusion), 0.0f, 1.0f);
	}
	else
	{
		input.tangent = normalize(input.tangent - dot(input.tangent, input.normal) * input.normal);
		float3 bitangent = cross(input.normal, input.tangent);

		float3 normalMapVec = normalTex.Sample(samplerState, input.texCoord);
		normalMapVec = 2.0f * normalMapVec - 1.0f;
		normalMapVec.z = clamp(normalMapVec.z, 0.0f, 1.0f);
		float3x3 TBN2World = float3x3(input.tangent, bitangent, input.normal);
		float3 absoluteNormal = normalize(mul(normalMapVec, TBN2World));

		float diffuse = clamp(dot(-lightVec, absoluteNormal), 0.0f, 1.0f);

		float3 cameraDir = normalize(cameraPos - input.pos.xyz);
		float3 specularDir = lightVec - 2 * dot(lightVec, absoluteNormal) * absoluteNormal;
		float specular = clamp(dot(specularDir, cameraDir), 0.0f, 1.0f);

		float specularFactor = tex.Sample(samplerState, input.texCoord);
		specular = pow(specular, 1 / specularFactor);

		return clamp(baseColor * (ambient + 0.4 * occlusion +
			lightFactor * (1.2 * diffuse + specularFactor * specular)), 0.0f, 1.0f);
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