#version 430

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in vec4 FragPosLightSpace;

out vec4 color;

uniform sampler2D ourTexture;
uniform vec3 viewPos;
uniform sampler2D shadowMap;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
    float reflectivity;
}; 

struct DirectionLight {
    int enable;
    vec3 direction;
    vec3 lightColor;
};

struct PointLight {
    int enable;
    vec3 position;  
    vec3 lightColor;

    float constant;
    float linear;
    float quadratic;
};

struct Spotlight {
    int enable;
    vec3 position;
    vec3 direction;
    vec3 lightColor;
    float cutOff;

    float constant;
    float linear;
    float quadratic;      
}; 

uniform Material material;
uniform DirectionLight dl;
uniform PointLight pl;
uniform Spotlight sl;
uniform samplerCube skybox;

float CalculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir){
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = texture(shadowMap, projCoords.xy).r;

    float currentDepth = projCoords.z;
    float bias = max(0.0001 * (1.0 - dot(normal, lightDir)), 0);
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += (currentDepth - bias > pcfDepth) ? 1.0 : 0.0;        
        }    
    }
    
    shadow /= 9.0;
    
    return shadow;
}

vec3 calculateDirectionLight(DirectionLight dl, vec3 normal, vec3 viewDir, float shadow) {
    vec3 lightDir = normalize(-dl.direction);
    
    vec3 ambient = dl.lightColor * material.ambient;

    float diff = max(dot(normal, lightDir), 0.0); 
    vec3 diffuse = dl.lightColor * material.diffuse * diff;

    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    vec3 specular = dl.lightColor *  material.specular * spec; 

    return ambient + (1.0 - shadow) * (diffuse + specular); 
} 

vec3 calculatePointLight(PointLight pl, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(pl.position - FragPos);
    float distance = length(pl.position - FragPos);
    float attenuation = 1.0 / (pl.constant + pl.linear * distance + pl.quadratic * (distance * distance));
    
    vec3 ambient = pl.lightColor * material.ambient;

    float diff = max(dot(normal, lightDir), 0.0); 
    vec3 diffuse = pl.lightColor * material.diffuse * diff;

    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    vec3 specular = pl.lightColor * material.specular * spec;

    return attenuation * (ambient + diffuse + specular);
}

vec3 calculateSpotLight(Spotlight sl, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(sl.position - FragPos);
    float distance = length(sl.position - FragPos);
    float attenuation = 1.0 / (sl.constant + sl.linear * distance + sl.quadratic * (distance * distance));
    float theta = dot(lightDir, normalize(-sl.direction));

    vec3 ambient = sl.lightColor * material.ambient;
   
    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);

    if(theta > sl.cutOff){
        float diff = max(dot(normal, lightDir), 0.0); 
        diffuse = sl.lightColor * material.diffuse * diff;
        
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
        specular = sl.lightColor * material.specular * spec; 
     }

    return attenuation * (ambient +  diffuse + specular);      
}

vec3 calculateSpotLightSoft(Spotlight sl, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(sl.position - FragPos);
    float distance = length(sl.position - FragPos);
    float attenuation = 1.0 / (sl.constant + sl.linear * distance + sl.quadratic * (distance * distance));
    
    float theta = dot(lightDir, normalize(-sl.direction));
    vec3 ambient = sl.lightColor * material.ambient;
    
    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);

    if(theta > sl.cutOff) {
        float epsilon = 0.1;        // edge¬X¤Æµ{«×
        float brightness = 2.0;    

        float intensity = clamp((theta - sl.cutOff) / epsilon, 0.0, 1.0);
        float diff = max(dot(normal, lightDir), 0.0); 
        diffuse = sl.lightColor * material.diffuse * diff;

        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
        specular =  sl.lightColor * material.specular * spec; 
        diffuse *= intensity * brightness;
        specular *= intensity * brightness;
    }
    
    return attenuation * (ambient + diffuse + specular);
}

vec4 computeReflection(){
    vec3 I = normalize(FragPos - viewPos);
    vec3 R = reflect(I, normalize(Normal));
    return texture(skybox, R);
}

void main() {
    vec4 texColor = texture(ourTexture, TexCoord);
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 result = vec3(0.0); 

    if(dl.enable == 1){
        vec3 lightDir = normalize(-dl.direction);
        float shadow = 0.0;
        shadow = CalculateShadow(FragPosLightSpace, norm, lightDir);
        result += calculateDirectionLight(dl, norm, viewDir, shadow); 
    }

    if(pl.enable == 1){
        result += calculatePointLight(pl, norm, viewDir); 
    }

    if(sl.enable == 1){
        if( pl.enable == 1  || dl.enable == 1){
            result += calculateSpotLightSoft(sl, norm, viewDir);
        }

        else{
            result += calculateSpotLight(sl, norm, viewDir);
        }
    }

    vec4 reflectionColor = computeReflection(); 
    color = mix(vec4(result, 1.0) * texColor , reflectionColor, material.reflectivity); 
}