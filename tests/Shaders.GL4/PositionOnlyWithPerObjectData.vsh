uniform vec4 ss[5];

layout(location = 0) in vec3 attr0;
uniform vec4 cb0[1];

out gl_PerVertex {
        vec4 gl_Position;
#ifdef PS
        float gl_ClipDistance[1];
#endif
};

void main()
{
    gl_Position = vec4( attr0.x + cb0[0].x, attr0.y + cb0[0].y, attr0.z, 1.0 );
#ifdef PS
gl_ClipDistance[0]=dot(ss[1],gl_Position);
#endif
gl_Position.xy += ss[0].xy*gl_Position.w;
gl_Position.y*=ss[0].z;
gl_Position.z=gl_Position.z*2.0-gl_Position.w;
}
