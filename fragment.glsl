// ==========================================================================
// Vertex program for barebones GLFW boilerplate
//
// Author:  Sonny Chan, University of Calgary
// Date:    December 2015
// ==========================================================================
#version 410
#extension GL_NV_shader_buffer_load : enable
#define MAX_OBJECT_NUM 50

const float PI = 3.14159265359;	

layout(location = 0) out vec3 color;

// interpolated colour received from vertex stage
in vec3 Colour;
in vec2 textureCoords;

// first output is mapped to the framebuffer's colour index by default
out vec4 FragmentColour;

uniform int objectTypes[MAX_OBJECT_NUM] = NULL;
uniform vec3 xs[MAX_OBJECT_NUM] = NULL;
uniform vec3 ys[MAX_OBJECT_NUM] = NULL;
uniform vec3 zs[MAX_OBJECT_NUM] = NULL;
uniform vec4 colors[MAX_OBJECT_NUM] = NULL;
uniform vec4 specularities[MAX_OBJECT_NUM] = NULL;
uniform int shininesses[MAX_OBJECT_NUM] = NULL;
uniform float reflectances[MAX_OBJECT_NUM] = NULL;
uniform float refractions[MAX_OBJECT_NUM]=NULL;
uniform int numOfObjects = 0;

uniform vec3 lights[MAX_OBJECT_NUM] = NULL;
uniform float lightIntensities[MAX_OBJECT_NUM] = NULL;
uniform int lightNum = 1;

uniform float fieldOfView = PI/(3.f); 
uniform vec3 cameraPos = vec3(0,0,0.14);

uniform float ambientLight = 1;

uniform float theta=0;
uniform float phi=0;
mat3 ry = mat3	(cos(theta), 0, sin(theta),
				 0,1,0,
				 -sin(theta), 0, cos(theta));

mat3 rx = mat3 (1, 0, 0,
				0, cos(phi), -sin(phi),
				0, sin(phi), cos(phi));

struct object
{
	int type;
	vec3 x, y, z;
	vec4 color;
	vec4 specularity;
	int shininess;
};

float getMagnitude(vec3 v)
{
	return sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
}
	

vec3 calculateRay(vec2 coords)
{
	float z = -1/tan(fieldOfView/2);
		
	vec3 ray = vec3(coords, z);//- cameraPos;
	
	ray=ry*ray;
	ray=rx*ray;
	
	//ray[2] = -ray[2];
	
	return ray/sqrt(dot(ray,ray));
}

float sphereIntersection(vec3 ray, vec3 origin, vec3 center, float radius)
{		 

	float a = dot(ray,ray);
	float b = -2*dot(center, ray)+2*dot(ray,origin);
	float c = -2*dot(origin,center)+dot(center,center)
			  -radius*radius+dot(origin,origin);

	float discriminant =b*b - 4*a*c;
		
	float t, t1, t2;
	if(discriminant < 0)
	{
		return -1;
	}
	
	else
	{
		t1=(-b-sqrt(discriminant))/(2*a);
		t2=(-b+sqrt(discriminant))/(2*a);
	}
	
	if(t1<0 && t2>=0)
		t1=t2;
	else if (t1>=0 && t2<0)
		t2=t1;

	return min(t1,t2);
}

float planeIntersection(vec3 ray, vec3 origin, vec3 n, vec3 q)
{
	n=n/getMagnitude(n);
	if(dot(ray,n)!=0)
		return (dot(q,n)-dot(n,origin))/dot(ray,n);

	return -1;
}

float triangleIntersection(vec3 ray, vec3 origin, vec3 p0, vec3 p1, vec3 p2)
{
	vec3 s = origin - p0;
	vec3 e1 = p1-p0;
	vec3 e2 = p2-p0;
	
	mat3 mt = mat3(s, e1, e2);
	mat3 mu = mat3(-ray, s, e2);
	mat3 mv = mat3(-ray, e1, s);
	mat3 md = mat3(-ray,e1,e2);

	float t = determinant(mt)/determinant(md);
	float u = determinant(mu)/determinant(md);
	float v = determinant(mv)/determinant(md);

	if(t > 0 && (u+v)<1 && (u+v)>0 && u<1 && u>0 && v<1 && v>0)
	{
		return t;
	}

	return -1;

}

struct lightRay
{
	vec4 color;
	float distance;
	int object;
};

lightRay getColour(vec3 ray, vec3 position, int ob)
{
	lightRay info;
	info.color = vec4(0);
	info.distance = -1;
	info.object=-1;

	float mt = -1;
	for(int i = 0; i<numOfObjects; i++)
	{
		float t = 0;
		if(objectTypes[i]==0)
		{
			t = sphereIntersection(ray, position, xs[i], ys[i][0]);
		}
		
		else if(objectTypes[i]==1)
		{
			t = planeIntersection(ray, position, xs[i], ys[i]);
		}
		
		else if(objectTypes[i]==2)
		{
			t = triangleIntersection(ray, position, xs[i], ys[i], zs[i]);
		}
		
		if (t>0 && (mt > t || mt == -1) && !(ob==i))
		{
			mt = t;
			info.color = colors[i];
			info.object=i;
		}
	}
	info.distance = mt;
	return info;
}

vec2 calculateShadow(vec3 position, int j, int objectSeen)
{
	float shadow = 1;
	vec3 darkRay = lights[j]-position;

	float maxT = getMagnitude(darkRay);

	darkRay = darkRay/sqrt(dot(darkRay,darkRay));
	
	position += darkRay*0.001;
	
	float mt = -1;
	int objectHit = -1;
	for(int i = 0; i<numOfObjects; i++)
	{
		float t = 0;
		if(objectTypes[i]==0)
		{
			t = sphereIntersection(darkRay, position, xs[i], ys[i][0]);
		}
		
		else if(objectTypes[i]==1)
		{
			t = planeIntersection(darkRay, position, xs[i], ys[i]);
		}
		
		else if(objectTypes[i]==2)
		{
			t = triangleIntersection(darkRay, position, xs[i], ys[i], zs[i]);
		}
		
		if (t>0 && (mt > t || mt == -1))
		{
			mt = t;
			objectHit = i;
		}
	}

	if (mt>0 && (mt < maxT))
	{
		if(objectTypes[objectSeen]==0)
		{
			float diameter = 2*ys[objectSeen][0];

			vec3 v = darkRay*mt;
			float length = getMagnitude(v); 
			
			shadow *= sin(pow((diameter-length)/diameter*PI/2.f,1.2));

			//shadow = 1.f/pow(1+mt,(maxT-mt)/maxT);
		}
			
		else
		{
			shadow*=(atan(mt*2+colors[objectHit][3])/(PI/2)*0.3+0.7);

		}
	}

	return vec2(shadow, maxT);
}

vec2 calculateShadows(vec3 ray,vec3 pos, float t, int objectSeen)
{
	vec3 position = t*ray + pos;
	vec2 info;

	bool sawLight = false;
	float nearestLight = -1;
	float darkFactor = 1;
	if(objectTypes[objectSeen]==0)
		darkFactor = 0;

	for (int i = 0; i < lightNum; ++i)
	{
		info = calculateShadow(position, i, objectSeen);
		if(objectTypes[objectSeen]==0)
			 darkFactor = max(darkFactor, info[0]);
		else
			darkFactor=darkFactor*info[0];

		if(nearestLight>info[1] || nearestLight<0)
		{
			nearestLight = info[1];
		}
		
	}
	float luminosity = atan(lightNum-1)/(PI/2);
	return vec2(darkFactor*(1-luminosity) + luminosity, nearestLight);
}

struct reflection
{
	vec3 ray;
	vec3 n;
};

reflection findReflectedRay(vec3 ray, vec3 position, float t, int objectSeen)
{
	ray = ray/getMagnitude(ray);

	vec3 contactPoint = ray*t + position;
	vec3 n =vec3(3);
	if(objectTypes[objectSeen]==0)
	{
		n = xs[objectSeen];
		n= contactPoint-n;
		n = n/getMagnitude(n);
	}
	else if(objectTypes[objectSeen]==1)
	{
		n = xs[objectSeen];
		n=n/getMagnitude(n);
	}
	else if(objectTypes[objectSeen]==2)
	{
		vec3 p0 = xs[objectSeen];
		vec3 p1 = ys[objectSeen];
		vec3 p2 = zs[objectSeen];

		vec3 v1 = p1-p0;
		vec3 v2 = p2-p0;

		n = cross(v1,v2);

		n=n/getMagnitude(n);
		/*if(dot(ray,n)<0)
			n=-n;*/
	}
	reflection ref;

	ref.ray = ray-2*(dot(ray,n)*n);
	ref.ray = normalize(ref.ray);
	ref.n = n;
	return ref;
}

vec4 getBrightness(vec3 ray, vec3 position, float t, int objectSeen)
{
	vec3 pos = position+ray*t;
	vec3 brightRay = lights[0]-pos;
	vec3 sight = cameraPos-pos;
	sight = sight/getMagnitude(sight); 

	vec4 c = vec4(0);
	vec4 temp = vec4(0);
	for(int i=0; i<lightNum; i++)
	{
		brightRay = lights[i]-pos;
		sight = cameraPos-pos;
		sight = sight/getMagnitude(sight); 

		float maxT = getMagnitude(brightRay);

		brightRay = brightRay/getMagnitude(brightRay);

		vec3 h = sight + brightRay;
		h = h/getMagnitude(h);

		reflection ref = findReflectedRay(brightRay, pos, 0, objectSeen);
		
		vec3 r = -brightRay + 2*dot(brightRay,ref.n)*ref.n;

		c = colors[objectSeen]*(ambientLight + lightIntensities[i]*max(0,dot(brightRay,ref.n)))+ 
			lightIntensities[i]*specularities[objectSeen]*pow(max(0,dot(ref.n,h)),shininesses[objectSeen]);//max(0,(pow(dot(ref.n,h),shininesses[objectSeen])));

		temp += c/lightNum;
	}

	return temp;
}

reflection calculateRefractedRay(vec3 ray, vec3 position, float n, int objectSeen)
{
	float nt = refractions[objectSeen];
	ray = normalize(ray);
	reflection ref = findReflectedRay(ray, position, 0, objectSeen);
	if(dot(ref.n,-ray)<0)
		ref.n=-ref.n;

	float theta = acos(dot(-ray,ref.n));
	float phi = asin((n/nt)*sin(theta));
	reflection r;
	r.ray = ((n*(ray+ref.n*cos(theta))/nt) -ref.n*cos(phi));
	r.n=ref.n;
	return r;
}

vec4 getRefractedColour(vec3 ray, vec3 position, float t, int objectSeen, vec4 colour)
{	
	int i=10, j=0;
	vec4 finalc = vec4(1);
	int obj = objectSeen;
	vec3 oray = ray;
	vec3 n;
	float refIndex = 1;
	bool once = true;
	vec4 r;
	
	vec4 newc[10];
	
	while(i>0)
	{
		i--;
		vec4 c;
	
		//position+=vec3(0.001);
		reflection refRay = calculateRefractedRay(ray, position+ray*t, refIndex, obj);
		//refRay.ray=ray;
		lightRay lumos = getColour(refRay.ray, position+ray*t, obj);
		c = lumos.color;
		finalc = c;
		if(once)
		{
			once =false;
			n=refRay.n;
		}

		if(lumos.distance>=0)
		{
			vec2 darkness = calculateShadows(refRay.ray, position+ray*t, lumos.distance, lumos.object);
			//if(darkness[0]==1)
			c = (getBrightness(refRay.ray, position, lumos.distance, lumos.object));
			c=c*(darkness[0]*1.f/pow(darkness[1],0.7));

		    c[3]=colors[lumos.object][3];
			newc[j]=c;//mix(r,c,dot(-ray, refRay.n));
			j++;
			if(colors[lumos.object][3]>0)
			{				
				position=position+ray*t;
				ray = refRay.ray;
				t=lumos.distance;
				obj = lumos.object;	
				if(refIndex==1)
					refIndex=colors[lumos.object][3];
				else
					refIndex=1;
			}
			
			else break;
		}

		else break;
	}
	//finalc=newc[0];
	while(j>0)
	{
		j--;
		finalc = mix(newc[j], finalc, newc[j][3]);
	}
	
	//return finalc;
	finalc = mix(colour, finalc, min( max(0,dot(n,-ray))+0.2, 1));
	//return finalc;
	return mix(colors[objectSeen], finalc, colors[objectSeen][3]);
}

vec4 getRelectedColour(vec3 ray, vec3 position, float t, int objectSeen)
{
	int i=10, j=0;
	vec4 finalc = vec4(0);
	int obj = objectSeen;
	
	vec4 newc[10];
	
	while(i>0)
	{
		i--;
		vec4 c;

		//position+=vec3(0.001);
		reflection ref = findReflectedRay(ray, position, t, obj);
		lightRay lumos = getColour(ref.ray, position+ray*t, obj);
		c = lumos.color;

		if(lumos.distance>=0)
		{
			vec2 darkness = calculateShadows(ref.ray, position+ray*t, lumos.distance, lumos.object);
			//if(darkness[0]==1)
			c = (getBrightness(ref.ray, position, lumos.distance, lumos.object));
		 	c=c*(darkness[0]*1.f/pow(darkness[1],0.7));
			
			if(colors[lumos.object][3]>0)
			{
				c = getRefractedColour(ref.ray, position+ray*t, lumos.distance, lumos.object, c);
			
			}
					
			c[3]=reflectances[lumos.object];
			newc[j]=c;
			j++;
			
			
			if(reflectances[lumos.object]>0)
			{				
				position=position+ray*t;
				ray = ref.ray;
				t=lumos.distance;
				obj = lumos.object;	
			}
			
			else break;
		}

		else break;
	}
	finalc=vec4(0);
	while(j>0)
	{
		j--;
		finalc = mix(newc[j], finalc, newc[j][3]);
	}
	
	//return finalc;
	return finalc;
} 

void main(void)
{  
	vec4 colour = vec4(0);
	vec3 ray = calculateRay(textureCoords);

	ray = ray/getMagnitude(ray);
	vec3 rcamPos = cameraPos;
	
	float t;
	lightRay photon;
	photon = getColour(ray, rcamPos, -1);
	t=photon.distance;
	colour=photon.color;

	if(t>=0)
	{
		vec2 darkness = calculateShadows(ray, rcamPos, t, photon.object);
		//if(darkness[0]==1)
		colour = (getBrightness(ray, rcamPos, t, photon.object));
		colour=colour*(darkness[0]*1.f/pow(darkness[1],0.7));
		vec4 r = (getRelectedColour(ray,rcamPos, t, photon.object));
		colour = mix(colour, r, reflectances[photon.object]);
		
		if(colors[photon.object][3]>0)
		{
			colour = getRefractedColour(ray, rcamPos, t, photon.object, r);
			
		}
		
		
			//1.f/pow(darkness[1],0.7)
	}
	FragmentColour = colour;
}
