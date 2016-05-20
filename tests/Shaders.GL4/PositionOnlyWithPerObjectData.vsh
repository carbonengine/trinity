uniform vec3 ssyf;
#ifdef PS
uniform vec4 ssf[4];
out float ssv;
#endif

in vec3 attr0;
uniform vec4 cb0[1];

void main()
{
    gl_Position = vec4( attr0.x + cb0[0].x, attr0.y + cb0[0].y, attr0.z, 1.0 );
#ifdef PS
ssv=dot(ssf[0],gl_Position);
#endif
gl_Position.xy += ssyf.xy*gl_Position.w;
gl_Position.y*=ssyf.z;
gl_Position.z=gl_Position.z*2.0-gl_Position.w;
}
