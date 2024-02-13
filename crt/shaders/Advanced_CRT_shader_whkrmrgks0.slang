#version 450

/*
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

copyright 2022 whkrmrgks0
*/

layout(push_constant) uniform Push{
    float cus;
    float vstr;
    float marginv;
    float dts;
    float AAz;
    float vex;
    float capa;
    float capaiter;
    float capashape;
    float scl;
    float gma;
    float sling;
};

layout(set = 0, binding = 0, std140) uniform UBO
{
   mat4 MVP;
   vec4 OutputSize;
   vec4 SourceSize;
};

#pragma parameter cus "CRT curvature" 0.15 0.0 1.0 0.01
#pragma parameter vstr "Vignette strength" 0.05 0.0 1.0 0.01
#pragma parameter marginv "Display margin" 0.02 0.0 0.1 0.005
#pragma parameter dts "Phosper size" 1.0 1.0 3.0 1.0
#pragma parameter AAz "De-moire conv iteration" 64.0 2.0 256.0 1.0
#pragma parameter vex "De-moire conv size" 2.0 0.0 4.0 0.1
#pragma parameter capa "Horizontal convolution size" 1.0 0.0 4.0 0.1
#pragma parameter capaiter "Horizontal convolution iterations" 5.0 1.0 20.0 1.0
#pragma parameter capashape "Horizontal convololution kernel shape" 3.0 1.0 40.0 0.1
#pragma parameter scl "Scnaline count, set 0 to match with input" 240.0 0.0 1080.0 1.0
#pragma parameter gma "Gamma correction" 1.0 0.1 4.0 0.1
#pragma parameter sling "line bleed" 2.0 1.0 2.0 0.1

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 2) out vec2 vTexCoord;

void main(){
   gl_Position = MVP * Position;
   vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 2) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(binding = 1) uniform sampler2D Source;
#define tau 6.28318530718
#define cr vec2(4.,0.)
#define cb vec2(2.,0.)
#define cg vec2(0.,0.)
#define cw vec2(3.,1.)

float sawtooth(float inp){
    return inp-floor(inp);
}

float square(float zed,float marchpoint,float floaz){
    return step(sawtooth(zed/floaz),marchpoint/floaz);
}

float triangle(float zed){
    return abs(sawtooth(zed+0.5)-.5)*2.;
}

vec2 hash22(vec2 seed){
    return fract(75.345*sin(623.3754*seed));
}

float grd(vec2 uv, vec2 disp){
    uv +=disp*dts;
    uv /= dts;
    return square(uv.x,2.,6.)*square(uv.y,1.,2.);
}

vec3 tpscany (vec3 bef,vec3 ucj,vec3 dcj,float temp){
    vec3 scan = vec3(.0);
    scan += max((triangle(temp)-1.+(bef*sling)),.0);
    scan += max((clamp(.0,1.,temp*2.-1.)-2.)+(ucj*sling),.0);
    scan += max((clamp(.0,1.,-(temp*2.-1.))-2.)+(dcj*sling),.0);
    return scan/(sling*0.5);
}
        
void pinc(vec2 uv, inout vec2 uv2, inout float mxbf, inout float vign, float ar){
    uv2 = (uv*vec2(2.)-vec2(1.))*vec2((1.+marginv),(1.+marginv*ar));
    uv2 = vec2(uv2.x/(cos(abs(uv2.y*cus)*tau/4.)),uv2.y/(cos(abs(uv2.x*cus*ar)*tau/4.)));
    vec2 uvbef = abs(uv2)-vec2(1.);//boarder
    mxbf = max(uvbef.x,uvbef.y);
    vign = max(uvbef.x*uvbef.y,.0);
    uv2 = (uv2+vec2(1.))*vec2(.5);//recoordination
}

float scimpresp(float range){//scanline IR
    return sin(pow(range,capashape)*tau)+1.;
}

void main(){
    vec2 uv2;
    float mxbf, vign;
    float scanline = (1.-min(scl,1.))*SourceSize.y+scl;

    vec2 ratd = vTexCoord*OutputSize.xy;
    vec2 uv = vTexCoord;
    pinc(uv,uv2,mxbf,vign,OutputSize.x/OutputSize.y);
    vec2 nuv = uv2;
    vec2 nuvyud = vec2(floor(nuv.y*scanline-1.)/scanline,floor(nuv.y*scanline+1.)/scanline);
    nuv.y = floor(nuv.y*scanline)/scanline;

    vec3 bef = vec3(.0);
    vec3 ucj = vec3(.0);
    vec3 dcj = vec3(.0);

    float capatemp, capainteg = .0;
    for(float i=-capaiter/2.;i<=capaiter/2.;i++){
        capatemp = scimpresp((i+capaiter/2.)/capaiter);
        capainteg += capatemp;
        bef += texture(Source,vec2(sawtooth(nuv.x-capa/scanline*i/(capaiter/2.)),nuv.y)).xyz*capatemp;
        ucj += texture(Source,vec2(sawtooth(nuv.x-capa/scanline*i/(capaiter/2.)),nuvyud.y)).xyz*capatemp;
        dcj += texture(Source,vec2(sawtooth(nuv.x-capa/scanline*i/(capaiter/2.)),nuvyud.x)).xyz*capatemp;
    }

    dcj /= capainteg;
    bef /= capainteg;
    ucj /= capainteg;

    vec3 scan =vec3(.0);
    float temp;
    float snippet;
    float integral = .0;
    for(float i = -AAz/2.; i<=AAz/2. ;i++){
        snippet = (AAz/2.-abs(i))/AAz/2.;
        integral += snippet;
        temp = sawtooth(uv2.y*scanline);
        scan += tpscany(bef,ucj,dcj,temp+(i/AAz*2.)*vex/OutputSize.y*scanline)*snippet;//antimoire convololution
    }
    scan /= integral;

    float brd = step(mxbf,.0);
    vign = pow(vign,vstr);

    vec3 grid = vec3(grd(ratd,cr),grd(ratd,cg),grd(ratd,cb));
    grid += vec3(grd(ratd,cr+cw),grd(ratd,cg+cw),grd(ratd,cb+cw));

    float mask = brd*vign;

    scan /= sling;
    scan = pow(scan,vec3(0.5));
    scan = pow(scan,vec3(1.+1./3.));
    scan = pow(scan,vec3(gma));

    vec3 grided = scan*grid*3.;
    vec3 final = min(vec3(mix(grided,scan,scan)),vec3(1.))*mask;

    FragColor = vec4(final,1.);
}
