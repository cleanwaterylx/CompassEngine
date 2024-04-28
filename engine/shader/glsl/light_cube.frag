#version 310 es

layout(location = 0) out highp vec4 FragColor;

highp vec3 lightColor = vec3(1.0, 1.0, 1.0);

void main()
{           
    FragColor = vec4(lightColor, 1.0);
}