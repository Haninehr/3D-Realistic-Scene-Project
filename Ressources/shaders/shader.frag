#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 UV;

out vec4 FragColor;

struct DirLight { vec3 dir; vec3 ambient; vec3 diffuse; vec3 spec; bool on; };
struct PointLight { vec3 pos; vec3 ambient; vec3 diffuse; vec3 spec; float constant; float linear; float quad; bool on; };

uniform DirLight uSun;
uniform PointLight uLamp;
uniform PointLight uCeiling;
uniform DirLight uDoorLight;
uniform vec3 viewPos;

uniform bool useTexture;
uniform sampler2D tex0;
uniform vec3 objectColor;
uniform vec3 emission;
uniform float alpha;

// simple Blinn-Phong
vec3 CalcDirLight(DirLight L, vec3 N, vec3 V, vec3 base){
    if(!L.on) return vec3(0);
    vec3 ld = normalize(-L.dir);
    float diff = max(dot(N, ld), 0.0);
    vec3 H = normalize(ld + V);
    float spec = pow(max(dot(N,H),0.0), 32.0);
    vec3 amb = L.ambient * base;
    vec3 dif = L.diffuse * diff * base;
    vec3 spc = L.spec * spec;
    return amb + dif + spc;
}
vec3 CalcPointLight(PointLight L, vec3 N, vec3 V, vec3 FragPos, vec3 base){
    if(!L.on) return vec3(0);
    vec3 ld = normalize(L.pos - FragPos);
    float diff = max(dot(N, ld), 0.0);
    vec3 H = normalize(ld + V);
    float spec = pow(max(dot(N,H),0.0), 32.0);
    float dist = length(L.pos - FragPos);
    float att = 1.0/(L.constant + L.linear*dist + L.quad*dist*dist);
    vec3 amb = L.ambient * base * att;
    vec3 dif = L.diffuse * diff * base * att;
    vec3 spc = L.spec * spec * att;
    return amb + dif + spc;
}

void main(){
    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);
    
    // Sample texture early
    vec3 texColor = texture(tex0, UV).rgb;
    
    // Base color: either objectColor or texture
    vec3 base = useTexture ? texColor : objectColor;
    
    // Compute regular lighting (ambient + diffuse + specular)
    vec3 lit = vec3(0.0);
    lit += CalcDirLight(uSun, N, V, base);
    lit += CalcPointLight(uLamp, N, V, FragPos, base);
    lit += CalcPointLight(uCeiling, N, V, FragPos, base);
    lit += CalcDirLight(uDoorLight, N, V, base);
    
    // Default result: lit surface + any extra emission (like glowing parts)
    vec3 result = lit + emission;
    
    // SPECIAL CASE: If this is the PC screen AND it's turned on,
    // we make it fully emissive → show the texture at full brightness regardless of lighting
    // We detect this by checking if the texture is used AND emission is non-zero
    // (we'll set emission to (1,1,1) only for the screen when on)
    if (useTexture && length(emission) > 0.5) {  // simple threshold
        // Screen is on → show full texture + some subtle lighting (optional)
        result = texColor + lit * 0.3;  // 0.3 = subtle ambient reflection, adjust as you like
        // Or for pure emission (no lighting at all on screen):
        // result = texColor;
    }
    
    FragColor = vec4(result, alpha);
}