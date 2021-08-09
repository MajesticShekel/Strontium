#version 440
/*
* Lighting fragment shader for a deferred PBR pipeline. Computes the point light
* component.
*/

#define PI 3.141592654
#define THRESHHOLD 0.00005
#define MIN_RADIANCE 0.00390625
#define INVERSE_MIN_RADIANCE 256.0

// Camera specific uniforms.
layout(std140, binding = 0) uniform CameraBlock
{
  mat4 u_viewMatrix;
  mat4 u_projMatrix;
  vec3 u_camPosition;
};

// Directional light uniforms.
layout(std140, binding = 5) uniform PointBlock
{
  vec4 u_lColourIntensity;
  vec4 u_lPosition;
  vec4 u_screenSizeRadiusFalloff;
};

// Uniforms for the geometry buffer.
layout(binding = 3) uniform sampler2D gPosition;
layout(binding = 4) uniform sampler2D gNormal;
layout(binding = 5) uniform sampler2D gAlbedo;
layout(binding = 6) uniform sampler2D gMatProp;

// Output colour variable.
layout(location = 0) out vec4 fragColour;

//------------------------------------------------------------------------------
// PBR helper functions.
//------------------------------------------------------------------------------
// Trowbridge-Reitz distribution function.
float TRDistribution(vec3 N, vec3 H, float alpha);
// Smith-Schlick-Beckmann geometry function.
float SSBGeometry(vec3 N, vec3 L, vec3 V, float roughness);
// Schlick approximation to the Fresnel factor.
vec3 SFresnel(float cosTheta, vec3 F0);
// Schlick approximation to the Fresnel factor, with roughness!
vec3 SFresnelR(float cosTheta, vec3 F0, float roughness);

void main()
{
  vec2 fTexCoords = gl_FragCoord.xy / u_screenSizeRadiusFalloff.xy;

  vec3 position = texture(gPosition, fTexCoords).xyz;
  vec3 normal = normalize(texture(gNormal, fTexCoords).xyz);
  vec3 albedo = texture(gAlbedo, fTexCoords).rgb;
  float metallic = texture(gMatProp, fTexCoords).r;
  float roughness = texture(gMatProp, fTexCoords).g;

  vec3 F0 = mix(vec3(texture(gAlbedo, fTexCoords).a), albedo, metallic);

  vec3 view = normalize(u_camPosition - position);
  vec3 light = normalize(u_lPosition.xyz - position);
  float dist = length(u_lPosition.xyz - position);
  vec3 halfWay = normalize(view + light);

  float maxRadiance = u_lColourIntensity.a * max(u_lColourIntensity.r, max(u_lColourIntensity.b, u_lColourIntensity.g));
  float radius = u_screenSizeRadiusFalloff.z;
  float linearFalloff = u_screenSizeRadiusFalloff.w;
  float quadFalloff = (256.0 * maxRadiance - linearFalloff * radius - 1.0) / (radius * radius);
  float attenuation = 1.0 / (1.0 + linearFalloff * dist + quadFalloff * dist * dist);

  float NDF = TRDistribution(normal, halfWay, roughness);
  float G = SSBGeometry(normal, view, light, roughness);
  vec3 F = SFresnel(max(dot(halfWay, view), THRESHHOLD), F0);

  vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

  vec3 num = NDF * G * F;
  float den = 4.0 * max(dot(normal, view), THRESHHOLD) * max(dot(normal, light), THRESHHOLD);
  vec3 spec = num / max(den, THRESHHOLD);

  fragColour = vec4((kD * albedo / PI + spec) * u_lColourIntensity.rgb * attenuation
                    * u_lColourIntensity.a * max(dot(normal, light), THRESHHOLD), 1.0);
}

// Trowbridge-Reitz distribution function.
float TRDistribution(vec3 N, vec3 H, float roughness)
{
  float alpha = roughness * roughness;
  float a2 = alpha * alpha;
  float NdotH = max(dot(N, H), THRESHHOLD);
  float NdotH2 = NdotH * NdotH;

  float nom = a2;
  float denom = (NdotH2 * (a2 - 1.0) + 1.0);
  denom = PI * denom * denom;

  return nom / denom;
}

// Schlick-Beckmann geometry function.
float Geometry(float NdotV, float roughness)
{
  float r = (roughness + 1.0);
  float k = (roughness * roughness) / 8.0;

  float nom   = NdotV;
  float denom = NdotV * (1.0 - k) + k;

  return nom / denom;
}

// Smith's modified geometry function.
float SSBGeometry(vec3 N, vec3 L, vec3 V, float roughness)
{
  float NdotV = max(dot(N, V), THRESHHOLD);
  float NdotL = max(dot(N, L), THRESHHOLD);
  float g2 = Geometry(NdotV, roughness);
  float g1 = Geometry(NdotL, roughness);

  return g1 * g2;
}

// Schlick approximation to the Fresnel factor.
vec3 SFresnel(float cosTheta, vec3 F0)
{
  return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, THRESHHOLD), 5.0);
}

vec3 SFresnelR(float cosTheta, vec3 F0, float roughness)
{
  return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(max(1.0 - cosTheta, THRESHHOLD), 5.0);
}