cbuffer vars : register(b0) {
	float2 uResolution;
};

Texture2D srctex : register(t0);
SamplerState smp : register(s0);

struct Functions
{

#define QUALITY_PERCENT 85
#define PI 3.1415927

#define N(v) (v.yx*float2(1,-1))

// // Constant buffer
// cbuffer Constants : register(b0)
// {
    float BrushDetail;
    float StrokeBend;
    float BrushSize;
    
    float layerScale;

    float SrcContrast;
    float SrcBright;

    float2 RES;
// }

float hash(float2 p) {
    p = frac(p * float2(123.34, 456.21));
    p += dot(p, p + 78.233);
    return frac(sin(p.x + p.y) * 43758.5453);
}

float rand(float2 uv) {
    return hash(floor(uv));
}

float4 rand4(float2 uv) {
    return float4(
        rand(uv + float2(0.0, 0.0)),
        rand(uv + float2(1.0, 0.0)),
        rand(uv + float2(0.0, 1.0)),
        rand(uv + float2(1.0, 1.0))
    );
}

float4 noise(in float2 uv) {
    uv *= 1000;
    float2 i = floor(uv);
    float2 f = frac(uv);

    // Four corners in 2D of a tile
    float4 a = rand4(i);
    float4 b = rand4(i + float2(1.0, 0.0));
    float4 c = rand4(i + float2(0.0, 1.0));
    float4 d = rand4(i + float2(1.0, 1.0));

    float2 u = f * f * (3.0 - 2.0 * f);

    float4 ab = lerp(a, b, u.x);
    float4 cd = lerp(c, d, u.x);
    return lerp(ab, cd, u.y);
}

float4 noise(int u) {
    return float4(
        rand(u + float2(0.0, 0.0)),
        rand(u + float2(1.0, 0.0)),
        rand(u + float2(0.0, 1.0)),
        1.0
    );
}

float3 getColor(Texture2D tex, float2 uv, float lod)
{
	float4 col = clamp(((tex.SampleLevel(smp, uv, lod) - 0.5) * SrcContrast + 0.5 * SrcBright), 0.0, 1.0);
	return col.xyz;
}

float compsignedmax(float3 c)
{
    float3 s=sign(c);
    float3 a=abs(c);
    if (a.x>a.y && a.x>a.z) return c.x;
    if (a.y>a.x && a.y>a.z) return c.y;
    return c.z;
}

float2 getGrad(Texture2D tex, float2 uv, float eps)
{
	float2 d = float2(eps, 0.);
	float lod = log2(2.*eps);
	//f'(x) ~= [f(x+h) - f(x-h)] / (2h)
	return float2(
           compsignedmax(getColor(tex, uv + d.xy, lod) - getColor(tex, uv - d.xy, lod)),
           compsignedmax(getColor(tex, uv + d.yx, lod) - getColor(tex, uv - d.yx, lod))
           ) / (2. * eps);
}

float4 debugCircle(float2 uv, float2 center, float radius, float aspect, float bend)
{
    float2 d = uv - center;
    d.x *= aspect;
    float dist = length(d);
    float alpha = 1;
    return float4(bend < 0.5 ? 
           lerp(float3(1.0, 0.0, 0.0), float3(0.0, 1.0, 0.0), bend/2) : 
           lerp(float3(0.0, 1.0, 0.0), float3(0.0, 0.0, 1.0), bend*2-1.), alpha )* step(dist, radius);
}

float4 Paint(Texture2D tex, float2 uv)
{
    float aspect = RES.x / RES.y;

    // Canvas Texture
    float canv = 0.0;

    canv = max(canv, (noise(uv * float2(0.7, 0.03).xy)).x);
    canv = max(canv, (noise(uv * float2(0.7, 0.03).yx * aspect)).x);
    canv = pow(canv, 3); //return canv;
    canv = float4(float(0.93 + 0.07 * canv).xxx, 1.0);

    // Brush Layers
    float3 brushPos;

    int NumGrid = int(float(0x10000 / 1) * min(pow(RES.x * RES.y / 1920. / 1440., .5), 1.0) * (1.0 - layerScale * layerScale));
    //NumGrid = 10000;
    
    int NumX = int(sqrt(float(NumGrid) * aspect) + 0.5);
    int NumY = int(sqrt(float(NumGrid) / aspect) + 0.5);
    float gridW0 = /*RES.x*/ 1. / float(NumX);
//    return float4(float(NumGrid)/10000.,1,1,1);
    int gridsum = 0;
    // min-scale layer has at least 10 strokes in y
    // NumY*layerScaleFact^maxLayer==10
    int maxLayer = int(log2(10.0 / float(NumY)) / log2(layerScale));
    //If scale layerScale each time, times it will take to scale down from NumY to 10.

    float4 col = canv;//float4(getColor(tex, uv,0.), 1.);/

    for (int layer = 11; layer >= 0; layer--)
    {
        if (layer > maxLayer) continue; //for (int layer = min(maxLayer, 11) ... no variable in for loop

        int NumX2 = int(float(NumX) * pow(layerScale, float(layer)) + 0.5);
        int NumY2 = int(float(NumY) * pow(layerScale, float(layer)) + 0.5);
        float gridW = /*RES.x*/ 1. / float(NumX2);

        
        int n0 = int(dot(floor(float2(uv/* / RES.xy*/ * float2(NumX2, NumY2))), float2(1, NumX2)));
//		return float(n0) / NumGrid;
		
        for (int ni = 0; ni < 25; ni++) //nine near grids
        {
            int nx = ni % 5 - 1;
            int ny = ni / 5 - 1;

            int gridn = n0 + nx + ny * NumX2;
            int gridsumn = gridsum + gridn;
            brushPos.xy = (float2(gridn % NumX2, gridn / NumX2) + 0.5) / float2(NumX2, NumY2)/* * RES*/;
            brushPos.x += gridW * 0.5 * (float((gridn / NumX2) % 2) - 0.5);

            //randomize brushPos
            brushPos.xy += gridW * (noise(gridsumn + layer * 123).xy - 0.5);//*0.8;

            //Brush Direction -----------
            float sampleDist = .1;
            float2 grad = getGrad(tex, brushPos.xy, gridW * sampleDist)+getGrad(tex, brushPos.xy,gridW * sampleDist);
            
            grad /= 2.;
            grad += sin(uv*100.) * .05
            ; // add random noise (for plain area
            
            float l = length(grad);
            float2 n = normalize(grad);
            float2 t = N(n);
            
            
            // width and length of brush stroke --------------------------------------------------------------------------------------------------------------
            float bw = (gridW-.6*gridW0)*1.2; //gridW * .75;
            
            float bl = bw;
    		float stretch=sqrt(1.43*pow(3.,1./float(layer+1)));
		    bw*=BrushSize*(.8+.4*noise(gridsumn).y)/stretch;
		    bl*=BrushSize*(.8+.4*noise(gridsumn).z)*stretch;
		    
 		    bw /= 1.-.25*abs(StrokeBend);
		    bw = (l*.001 < bw && bw < gridW0*0.8 && layer != maxLayer) ? 0 : bw; 
		    
            // convert to local space --------------------------------------------------------------------------------------------------------------
        	float2 uv_screen = (uv - brushPos.xy) * float2(aspect, 1.);
            float2 pos = float2(dot(uv_screen, n), dot(uv_screen, t)) / float2(bw, bl);
            pos *= 0.5;
            
            // Bend
            pos.x -= .225*StrokeBend * (noise(gridsumn).x - .5)*2;
			pos.x += pos.y*pos.y*StrokeBend;
			pos.x /= 1.-.25*abs(StrokeBend);
			
            pos += float2(0.5, 0.5);


			// Brush shape ----------------------------------------------------------------------------------------------------------------------
			float s = 0; float mask;
			
			    s = pos.x * (1. - pos.x) * 4. * pos.y * (1. - pos.y) * 4. * 2.53;
				s = clamp((s - 0.5) * 2.,0.,1.);
				// ----------------------------------------------
				float s_ = s;
				float fur = noise(pos * float2(0.01,0.001) * 4.4).x + noise(pos * float2(0.01,0.001) * 1.21).x;
				s *= fur;
				// ----------------------------------------------
				mask = max(
					pow( (cos(pos.x * 3.14159 * 2. + 1.5 * (noise(-gridsumn).x-.5))+1.)/2., 1.2) - 0.135, 
					0.35 * exp(-pos.y * pos.y * 5.) * (1. - pos.y)
				) * (1. - pos.y);
				mask = clamp(mask+0.3, 0., 1.);
				s = s * 0.43 + s_ * mask * .55 - .5 * pos.y;
				s *= 1.;
				
				s = clamp(s,0.,1.) * step(-0.5,-abs(pos.x-.5)) * step(-0.5,-abs(pos.y-.5));
				//s = pow(s, 1.2) * pow(1.285 - pos.y, 0.8) - .25 * pos.y;
				s *= .75;
			
            float4 dcoll;
            dcoll.xyz = getColor(tex, brushPos.xy, 1.0);// * lerp(s*.13+.87,1.,mask);
            col = lerp(col, dcoll, s);
//            dcoll = debugCircle(uv, brushPos.xy, gridW / 4, aspect, float(layer) / float(maxLayer));
//            col = lerp(col, dcoll, dcoll.w);
        }
    	gridsum+=NumX2*NumY2;
    }
    return col;
}
};

float4 main (float4 fragCoord : SV_POSITION): SV_TARGET
{
    float2 uv = fragCoord.xy/uResolution;
    Functions F;
    F.BrushDetail = 0.1;
    F.StrokeBend = -1.;
    F.BrushSize = 1;
    F.layerScale = float(QUALITY_PERCENT) / 100.0;
    F.SrcContrast = 1.4;
    F.SrcBright = 0.9;
    F.RES = float2(3800, 3800)*0.5;

    return F.Paint(srctex, uv);
}