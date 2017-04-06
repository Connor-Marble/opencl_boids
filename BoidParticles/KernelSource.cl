

float len(float4 vec){
	vec.w=0.0;
	return length(vec);
}

float4 norm(float4 vec){
	if(len(vec)==0.0)
		return (float4)(0.0,0.0,0.0,1.0);

	return vec/len(vec);
}

float4 nlerp(float4 a, float4 b, float t){
	float4 result = (float4)((1.0-t)*a.x + b.x*t,(1.0-t)*a.y + b.y*t,(1.0-t)*a.z + b.z*t,1.0);
	
	result.w=0.0;

	return norm(result);
}

__kernel void KernelSource(__global float4* pos, __global float4* vel, float dt, int goal)
{
    unsigned int i = get_global_id(0);
    float4 p = pos[i];
    float4 v = vel[i];

	

	float neighbors=0.1;
	float4 sum = (float4)(0.0,0.0,0.0,1.0);

	float4 avoid = (float4)(0.0,0.0,0.0,1.0);
	
	float4 goalvec = (float4)(0.0,0.0,0.0,1.0);

	switch(goal){
		case 0:
			goalvec = (float4)(0.0,4.0,0.0,1.0);
			break;
		case 1:
			goalvec = (float4)(-4.0,0.0,0.0,1.0);
			break;
		case 2:
			goalvec = (float4)(0.0,-4.0,0.0,1.0);
			break;
		case 3:
			goalvec = (float4)(4.0,0.0,0.0,1.0);
			break;
		case 4:
			goalvec = (float4)(0.0,0.0,0.0,1.0);
			break;
	}

	for(unsigned int j=0;j<get_global_size(0);j++){
		if(j != i){
			float dist = distance(pos[i],pos[j]);
			if(dist < 0.5 && dist>0.2){
				neighbors+=1.0;
				sum+=norm(pos[j]);
				sum += norm(vel[j]);
			}

			else if(dist<0.2){
				avoid+=norm(p-pos[j])/len(p-pos[j]);
			}
		}
	}

	float4 center = sum/neighbors;

	center=norm(center);
	avoid=norm(avoid);

	float4 goalseek = norm(goalvec-p);

	float4 targetVel = (center-p) + (p+avoid*1.0) + (goalseek*2.0);

	

	v=nlerp(v,targetVel,0.005);
	
	v.w=1.0;
	
	p.w=1.0;
	p+=v*0.008;
	pos[i] = p;
	vel[i] = v;

}

