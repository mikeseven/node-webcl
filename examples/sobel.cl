__constant const int order = 1;

__kernel void sobel(__global const uchar* data, __global uchar* out, const uint width, const uint height,  
                    __global const float* coef_ver,  __global const float* coef_hor, 
                    __local float *coef_ver_shared, __local float *coef_hor_shared)
{
	int i, j, gid, lid, coef_width;
	float out_ver = 0, out_hor = 0;
	gid = get_global_id(0);
	lid = get_local_id(0);
	
	coef_width = 2 * order + 1;
	
	if(lid < 9)
	{
		coef_ver_shared[lid] = coef_ver[lid];
		coef_hor_shared[lid] = coef_hor[lid];
	}
	
	barrier(CLK_LOCAL_MEM_FENCE);
	
	if(gid < width) return;
	if(gid > (height-1)*width) return;
	if((gid << (int) log2((float) gid)) == 1 || ((gid + 1) << (int) log2((float) gid)) == 1) return;
	
	for(i = -1 * order; i < (order + 1); i++)
	{
		for(j = -1 * order; j < order + 1; j++)
		{
			out_ver += data[gid + width * i + j] * coef_ver_shared[(i + order) * coef_width + (j + order)];
			out_hor += data[gid + width * i + j] * coef_hor_shared[(i + order) * coef_width + (j + order)];
		}
	}
	out[gid] = (uchar) sqrt((float) (pow((float) out_ver, 2) + pow((float) out_hor, 2)));
	
	barrier(CLK_LOCAL_MEM_FENCE);
}

