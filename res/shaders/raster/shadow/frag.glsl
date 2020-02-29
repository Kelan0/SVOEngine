in float fs_worldDistance;

void main() {
    gl_FragDepth = 1.0 - 1.0 / (1.0 + fs_worldDistance);
}