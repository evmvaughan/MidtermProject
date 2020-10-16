#version 410 core

// uniform inputs
uniform mat4 mvpMatrix;                 // the precomputed Model-View-Projection Matrix
// the model matrix for world-space coords
uniform mat4 modelMtx;
// the normal matrix
uniform mat3 mNormal;
// the direction the incident ray of light is traveling
uniform vec3 lightDirection;
// the color of the light
uniform vec3 lightColor;
// point light
uniform vec3 lightPosition;
// spot light
uniform vec3 lightPositionSpot;
uniform vec3 lightPositionDirection;

// specular alpha component
uniform float specularAlpha;
// the direction of the view, for specular
uniform vec3 viewDirection;

uniform vec3 materialColor;             // the material color for our vertex (& whole object)

// attribute inputs
layout(location = 0) in vec3 vPos;      // the position of this specific vertex in object space
// the normal of this specific vertex in object space
layout(location = 1) in vec3 vNormal;
// varying outputs
layout(location = 0) out vec3 color;    // color to apply to this vertex

void main() {
    // transform & output the vertex in clip space
    gl_Position = mvpMatrix * vec4(vPos, 1.0);


    vec4 vWPos4 = modelMtx * vec4(vPos, 1.0);
    vec3 vWorldPos = vec3(vWPos4.x, vWPos4.y, vWPos4.z);

    vec3 normalLightDirection = normalize(-lightDirection);
    vec3 normalViewDirection = normalize(viewDirection-vWorldPos);

    const float intensity = 0.1;
    vec3 worldSpaceNormal = mNormal * vNormal * intensity;
    // diffuse component
    vec3 diffuseComponent = lightColor * materialColor * max(dot(normalLightDirection, worldSpaceNormal),0.0);
    diffuseComponent = diffuseComponent + lightColor * materialColor * max(dot(vPos-lightPosition, worldSpaceNormal),0.0);


    vec3 ld = normalize(lightPositionSpot - vPos);
    vec3 lp = normalize(lightPositionDirection);
    if (dot(ld, lp) < 0.5) {
        diffuseComponent = diffuseComponent + lightColor * materialColor * max(dot(vPos-lightPositionSpot, worldSpaceNormal),0.0);
    }

    // specular component
    vec3 specH = normalize(normalLightDirection+normalViewDirection);
    float specAngle = max(dot(specH, worldSpaceNormal), 0.0);
    vec3 specularComponent = lightColor * materialColor * pow( specAngle, specularAlpha );
    // material-ambient component
    vec3 ambientComponent = lightColor * materialColor * 0.14;

//    color = specularComponent;              // assign the color for this vertex
//    color = diffuseComponent;
//    color = ambientComponent;
    color = diffuseComponent + specularComponent + ambientComponent;
}