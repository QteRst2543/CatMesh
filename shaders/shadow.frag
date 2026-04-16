#version 460 core

void main() {
    // Write depth value explicitly for shadow mapping
    gl_FragDepth = gl_FragCoord.z;
}
