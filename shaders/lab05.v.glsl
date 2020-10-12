#version 410 core

// uniform inputs
uniform mat4 mvpMatrix;                 // the precomputed Model-View-Projection Matrix
// the normal matrix
uniform mat3 mNormal;
// the direction the incident ray of light is traveling
uniform vec3 lightDirection;
// the color of the light
uniform vec3 lightColor;

uniform vec3 materialColor;             // the material color for our vertex (& whole object)
// attribute inputs
layout(location = 0) in vec3 vPos;      // the position of this specific vertex in object space
// the normal of this specific vertex in object space
attribute vec3 vNormal;
// varying outputs
layout(location = 0) out vec3 color;    // color to apply to this vertex

void main() {
    // transform & output the vertex in clip space
    gl_Position = mvpMatrix * vec4(vPos, 1.0);

    vec3 normalLightDirection = normalize(-lightDirection);

    vec3 worldSpaceNormal = mNormal * vNormal;

    vec3 diffuseComponent = lightColor * materialColor * max(dot(normalLightDirection, worldSpaceNormal), 0);

    color = diffuseComponent;              // assign the color for this vertex
}