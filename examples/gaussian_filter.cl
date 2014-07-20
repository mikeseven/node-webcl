/*
 Adapted from OpenCL Programming Guide, Aaftab Munshi, Benedict Gaster, Timothy Mattson, James Fung, Dan Ginsburg, Addison-Wesley Professional, http://www.openclprogrammingguide.com
 Chapter 8 - ImageFilter2D.cpp
 */

// Gaussian filter of image

__kernel void gaussian_filter(__read_only image2d_t srcImg,
                              __write_only image2d_t dstImg,
                              sampler_t sampler,
                              int width, int height)
{
    // Gaussian Kernel is:
    // 1  2  1
    // 2  4  2
    // 1  2  1
    float kernelWeights[9] = { 1.0f, 2.0f, 1.0f,
                               2.0f, 4.0f, 2.0f,
                               1.0f, 2.0f, 1.0f };

    int x0 = get_global_id(0), y0 = get_global_id(1);
    int2 startImageCoord = (int2) (x0 - 1, y0 - 1);
    int2 endImageCoord   = (int2) (x0 + 1, y0 + 1);
    int2 outImageCoord = (int2) (x0, y0);

    int weight = 0;
    float4 outColor = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
    for( int y = startImageCoord.y; y <= endImageCoord.y; y++)
    {
        for( int x = startImageCoord.x; x <= endImageCoord.x; x++)
        {
            outColor += (read_imagef(srcImg, sampler, (int2)(x, y)) * kernelWeights[weight]);
            weight += 1;
        }
    }

    // Write the output value to image
    write_imagef(dstImg, outImageCoord, outColor/16.0f);
}
