
float3 Phong(float3 lightCol, float3 pixelToLight, float3 pixelToView, float3 normal, float ambientIntensity, float lightIntensity, float matShininess)
{
    float3 ambient = ambientIntensity * lightCol;
    
    float diffuseFactor = max(dot(normal, pixelToLight), 0.0f);
    float3 diffuse = lightIntensity * lightCol * diffuseFactor;
    
    float3 specular = float3(0.0f, 0.0f, 0.0f);
    if (diffuseFactor >= 0.0001f)
    {
        float3 halfwayDir = normalize(pixelToView + pixelToLight);
        float specularFactor = pow(max(dot(normal, halfwayDir), 0.0f), matShininess);
        specular = lightIntensity * lightCol * specularFactor;
    }

    
    return ambient + diffuse + specular;
}

float Attenuation(float c, float l, float q, float d)
{
    return 1.0f / (c + l * d + q * d * d);
}