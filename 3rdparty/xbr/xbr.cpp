//// *** Super-xBR code begins here - MIT LICENSE *** ///
 
/*
 
*******  Super XBR Scaler  *******
 
Copyright (c) 2016 Hyllian - sergiogdb@gmail.com
 
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
 
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
 
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
 
*/

#include <stdlib.h>
#include <math.h>
#include <algorithm>
#include <stdint.h>

#define R(_col) ((_col>> 0)&0xFF)
#define G(_col) ((_col>> 8)&0xFF)
#define B(_col) ((_col>>16)&0xFF)
#define A(_col) ((_col>>24)&0xFF)
 
 
#define wgt1 0.129633f
#define wgt2 0.175068f
#define w1  (-wgt1)
#define w2  (wgt1+0.5f)
#define w3  (-wgt2)
#define w4  (wgt2+0.5f)
 
 
 
float df(float A, float B)
{
    return abs(A - B);
}
 
float min4(float a, float b, float c, float d)
{
    return std::min(std::min(a,b),std::min(c, d));
}
 
float max4(float a, float b, float c, float d)
{
    return std::max(std::max(a, b), std::max(c, d));
}
 
template<class T>
T clamp(T x, T floor, T ceil)
{
    return std::max(std::min(x, ceil), floor);
}
 
/*
                         P1
|P0|B |C |P1|         C     F4          |a0|b1|c2|d3|
|D |E |F |F4|      B     F     I4       |b0|c1|d2|e3|   |e1|i1|i2|e2|
|G |H |I |I4|   P0    E  A  I     P3    |c0|d1|e2|f3|   |e3|i3|i4|e4|
|P2|H5|I5|P3|      D     H     I5       |d0|e1|f2|g3|
                      G     H5
                         P2
 
sx, sy  
-1  -1 | -2  0   (x+y) (x-y)    -3  1  (x+y-1)  (x-y+1)
-1   0 | -1 -1                  -2  0
-1   1 |  0 -2                  -1 -1
-1   2 |  1 -3                   0 -2
 
 0  -1 | -1  1   (x+y) (x-y)      ...     ...     ...
 0   0 |  0  0
 0   1 |  1 -1
 0   2 |  2 -2
 
 1  -1 |  0  2   ...
 1   0 |  1  1
 1   1 |  2  0
 1   2 |  3 -1
 
 2  -1 |  1  3   ...
 2   0 |  2  2
 2   1 |  3  1
 2   2 |  4  0
 
                         
*/
 
float diagonal_edge(float mat[][4], float *wp) {
    float dw1 = wp[0]*(df(mat[0][2], mat[1][1]) + df(mat[1][1], mat[2][0]) + df(mat[1][3], mat[2][2]) + df(mat[2][2], mat[3][1])) +\
                wp[1]*(df(mat[0][3], mat[1][2]) + df(mat[2][1], mat[3][0])) + \
                wp[2]*(df(mat[0][3], mat[2][1]) + df(mat[1][2], mat[3][0])) +\
                wp[3]*df(mat[1][2], mat[2][1]) +\
                wp[4]*(df(mat[0][2], mat[2][0]) + df(mat[1][3], mat[3][1])) +\
                wp[5]*(df(mat[0][1], mat[1][0]) + df(mat[2][3], mat[3][2]));
 
    float dw2 = wp[0]*(df(mat[0][1], mat[1][2]) + df(mat[1][2], mat[2][3]) + df(mat[1][0], mat[2][1]) + df(mat[2][1], mat[3][2])) +\
                wp[1]*(df(mat[0][0], mat[1][1]) + df(mat[2][2], mat[3][3])) +\
                wp[2]*(df(mat[0][0], mat[2][2]) + df(mat[1][1], mat[3][3])) +\
                wp[3]*df(mat[1][1], mat[2][2]) +\
                wp[4]*(df(mat[1][0], mat[3][2]) + df(mat[0][1], mat[2][3])) +\
                wp[5]*(df(mat[0][2], mat[1][3]) + df(mat[2][0], mat[3][1]));
 
    return (dw1 - dw2);
}
 
// Not used yet...
float cross_edge(float mat[][4], float *wp) {
    float hvw1 = wp[3] * (df(mat[1][1], mat[2][1]) + df(mat[1][2], mat[2][2])) + \
                 wp[0] * (df(mat[0][1], mat[1][1]) + df(mat[2][1], mat[3][1]) + df(mat[0][2], mat[1][2]) + df(mat[2][2], mat[3][2])) + \
                 wp[2] * (df(mat[0][1], mat[2][1]) + df(mat[1][1], mat[3][1]) + df(mat[0][2], mat[2][2]) + df(mat[1][2], mat[3][2]));
 
    float hvw2 = wp[3] * (df(mat[1][1], mat[1][2]) + df(mat[2][1], mat[2][2])) + \
                 wp[0] * (df(mat[1][0], mat[1][1]) + df(mat[2][0], mat[2][1]) + df(mat[1][2], mat[1][3]) + df(mat[2][2], mat[2][3])) + \
                 wp[2] * (df(mat[1][0], mat[1][2]) + df(mat[1][1], mat[1][3]) + df(mat[2][0], mat[2][2]) + df(mat[2][1], mat[2][3]));
 
    return (hvw1 - hvw2);
}
 
 
 
 
 
///////////////////////// Super-xBR scaling
// perform super-xbr (fast shader version) scaling by factor f=2 only.
template<int f>
void scaleSuperXBRT(uint32_t* data, uint32_t* out, int w, int h) {
    int outw = w*f, outh = h*f;
 
    float wp[6] = { 2.0f, 1.0f, -1.0f, 4.0f, -1.0f, 1.0f };
 
    // First Pass
    for (int y = 0; y < outh; ++y) {
        for (int x = 0; x < outw; ++x) {
            float r[4][4], g[4][4], b[4][4], a[4][4], Y[4][4];
            int cx = x / f, cy = y / f; // central pixels on original images
            // sample supporting pixels in original image
            for (int sx = -1; sx <= 2; ++sx) {
                for (int sy = -1; sy <= 2; ++sy) {
                    // clamp pixel locations
                    int csy = clamp(sy + cy, 0, h - 1);
                    int csx = clamp(sx + cx, 0, w - 1);
                    // sample & add weighted components
                    uint32_t sample = data[csy*w + csx];
                    r[sx + 1][sy + 1] = (float)R(sample);
                    g[sx + 1][sy + 1] = (float)G(sample);
                    b[sx + 1][sy + 1] = (float)B(sample);
                    a[sx + 1][sy + 1] = (float)A(sample);
                    Y[sx + 1][sy + 1] = (float)(0.2126*r[sx + 1][sy + 1] + 0.7152*g[sx + 1][sy + 1] + 0.0722*b[sx + 1][sy + 1]);
                }
            }
            float min_r_sample = min4(r[1][1], r[2][1], r[1][2], r[2][2]);
            float min_g_sample = min4(g[1][1], g[2][1], g[1][2], g[2][2]);
            float min_b_sample = min4(b[1][1], b[2][1], b[1][2], b[2][2]);
            float min_a_sample = min4(a[1][1], a[2][1], a[1][2], a[2][2]);
            float max_r_sample = max4(r[1][1], r[2][1], r[1][2], r[2][2]);
            float max_g_sample = max4(g[1][1], g[2][1], g[1][2], g[2][2]);
            float max_b_sample = max4(b[1][1], b[2][1], b[1][2], b[2][2]);
            float max_a_sample = max4(a[1][1], a[2][1], a[1][2], a[2][2]);
            float d_edge = diagonal_edge(Y, &wp[0]);
            float r1, g1, b1, a1, r2, g2, b2, a2, rf, gf, bf, af;
            r1 = (float)w1*(r[0][3] + r[3][0]) + (float)w2*(r[1][2] + r[2][1]);
            g1 = (float)w1*(g[0][3] + g[3][0]) + (float)w2*(g[1][2] + g[2][1]);
            b1 = (float)w1*(b[0][3] + b[3][0]) + (float)w2*(b[1][2] + b[2][1]);
            a1 = (float)w1*(a[0][3] + a[3][0]) + (float)w2*(a[1][2] + a[2][1]);
            r2 = (float)w1*(r[0][0] + r[3][3]) + (float)w2*(r[1][1] + r[2][2]);
            g2 = (float)w1*(g[0][0] + g[3][3]) + (float)w2*(g[1][1] + g[2][2]);
            b2 = (float)w1*(b[0][0] + b[3][3]) + (float)w2*(b[1][1] + b[2][2]);
            a2 = (float)w1*(a[0][0] + a[3][3]) + (float)w2*(a[1][1] + a[2][2]);
            // generate and write result
            if (d_edge <= 0.0f) { rf = r1; gf = g1; bf = b1; af = a1; }
            else { rf = r2; gf = g2; bf = b2; af = a2; }
            // anti-ringing, clamp.
            rf = clamp(rf, min_r_sample, max_r_sample);
            gf = clamp(gf, min_g_sample, max_g_sample);
            bf = clamp(bf, min_b_sample, max_b_sample);
            af = clamp(af, min_a_sample, max_a_sample);
            int ri = clamp(static_cast<int>(ceilf(rf)), 0, 255);
            int gi = clamp(static_cast<int>(ceilf(gf)), 0, 255);
            int bi = clamp(static_cast<int>(ceilf(bf)), 0, 255);
            int ai = clamp(static_cast<int>(ceilf(af)), 0, 255);
            out[y*outw + x] = out[y*outw + x + 1] = out[(y + 1)*outw + x] = data[cy*w + cx];
            out[(y+1)*outw + x+1] = (ai << 24) | (bi << 16) | (gi << 8) | ri;
            ++x;
        }
        ++y;
    }
   
 
 
    // Second Pass
    wp[0] = 2.0f;
    wp[1] = 0.0f;
    wp[2] = 0.0f;
    wp[3] = 0.0f;
    wp[4] = 0.0f;
    wp[5] = 0.0f;
 
    for (int y = 0; y < outh; ++y) {
        for (int x = 0; x < outw; ++x) {
            float r[4][4], g[4][4], b[4][4], a[4][4], Y[4][4];
            // sample supporting pixels in original image
            for (int sx = -1; sx <= 2; ++sx) {
                for (int sy = -1; sy <= 2; ++sy) {
                    // clamp pixel locations
                    int csy = clamp(sx - sy + y, 0, f*h - 1);
                    int csx = clamp(sx + sy + x, 0, f*w - 1);
                    // sample & add weighted components
                    uint32_t sample = out[csy*outw + csx];
                    r[sx + 1][sy + 1] = (float)R(sample);
                    g[sx + 1][sy + 1] = (float)G(sample);
                    b[sx + 1][sy + 1] = (float)B(sample);
                    a[sx + 1][sy + 1] = (float)A(sample);
                    Y[sx + 1][sy + 1] = (float)(0.2126*r[sx + 1][sy + 1] + 0.7152*g[sx + 1][sy + 1] + 0.0722*b[sx + 1][sy + 1]);
                }
            }
            float min_r_sample = min4(r[1][1], r[2][1], r[1][2], r[2][2]);
            float min_g_sample = min4(g[1][1], g[2][1], g[1][2], g[2][2]);
            float min_b_sample = min4(b[1][1], b[2][1], b[1][2], b[2][2]);
            float min_a_sample = min4(a[1][1], a[2][1], a[1][2], a[2][2]);
            float max_r_sample = max4(r[1][1], r[2][1], r[1][2], r[2][2]);
            float max_g_sample = max4(g[1][1], g[2][1], g[1][2], g[2][2]);
            float max_b_sample = max4(b[1][1], b[2][1], b[1][2], b[2][2]);
            float max_a_sample = max4(a[1][1], a[2][1], a[1][2], a[2][2]);
            float d_edge = diagonal_edge(Y, &wp[0]);
            float r1, g1, b1, a1, r2, g2, b2, a2, rf, gf, bf, af;
            r1 = (float)w3*(r[0][3] + r[3][0]) + (float)w4*(r[1][2] + r[2][1]);
            g1 = (float)w3*(g[0][3] + g[3][0]) + (float)w4*(g[1][2] + g[2][1]);
            b1 = (float)w3*(b[0][3] + b[3][0]) + (float)w4*(b[1][2] + b[2][1]);
            a1 = (float)w3*(a[0][3] + a[3][0]) + (float)w4*(a[1][2] + a[2][1]);
            r2 = (float)w3*(r[0][0] + r[3][3]) + (float)w4*(r[1][1] + r[2][2]);
            g2 = (float)w3*(g[0][0] + g[3][3]) + (float)w4*(g[1][1] + g[2][2]);
            b2 = (float)w3*(b[0][0] + b[3][3]) + (float)w4*(b[1][1] + b[2][2]);
            a2 = (float)w3*(a[0][0] + a[3][3]) + (float)w4*(a[1][1] + a[2][2]);
            // generate and write result
            if (d_edge <= 0.0f) { rf = r1; gf = g1; bf = b1; af = a1; }
            else { rf = r2; gf = g2; bf = b2; af = a2; }
            // anti-ringing, clamp.
            rf = clamp(rf, min_r_sample, max_r_sample);
            gf = clamp(gf, min_g_sample, max_g_sample);
            bf = clamp(bf, min_b_sample, max_b_sample);
            af = clamp(af, min_a_sample, max_a_sample);
            int ri = clamp(static_cast<int>(ceilf(rf)), 0, 255);
            int gi = clamp(static_cast<int>(ceilf(gf)), 0, 255);
            int bi = clamp(static_cast<int>(ceilf(bf)), 0, 255);
            int ai = clamp(static_cast<int>(ceilf(af)), 0, 255);
            out[y*outw + x + 1] = (ai << 24) | (bi << 16) | (gi << 8) | ri;
 
            for (int sx = -1; sx <= 2; ++sx) {
                for (int sy = -1; sy <= 2; ++sy) {
                    // clamp pixel locations
                    int csy = clamp(sx - sy + 1 + y, 0, f*h - 1);
                    int csx = clamp(sx + sy - 1 + x, 0, f*w - 1);
                    // sample & add weighted components
                    uint32_t sample = out[csy*outw + csx];
                    r[sx + 1][sy + 1] = (float)R(sample);
                    g[sx + 1][sy + 1] = (float)G(sample);
                    b[sx + 1][sy + 1] = (float)B(sample);
                    a[sx + 1][sy + 1] = (float)A(sample);
                    Y[sx + 1][sy + 1] = (float)(0.2126*r[sx + 1][sy + 1] + 0.7152*g[sx + 1][sy + 1] + 0.0722*b[sx + 1][sy + 1]);
                }
            }
            d_edge = diagonal_edge(Y, &wp[0]);
            r1 = (float)w3*(r[0][3] + r[3][0]) + (float)w4*(r[1][2] + r[2][1]);
            g1 = (float)w3*(g[0][3] + g[3][0]) + (float)w4*(g[1][2] + g[2][1]);
            b1 = (float)w3*(b[0][3] + b[3][0]) + (float)w4*(b[1][2] + b[2][1]);
            a1 = (float)w3*(a[0][3] + a[3][0]) + (float)w4*(a[1][2] + a[2][1]);
            r2 = (float)w3*(r[0][0] + r[3][3]) + (float)w4*(r[1][1] + r[2][2]);
            g2 = (float)w3*(g[0][0] + g[3][3]) + (float)w4*(g[1][1] + g[2][2]);
            b2 = (float)w3*(b[0][0] + b[3][3]) + (float)w4*(b[1][1] + b[2][2]);
            a2 = (float)w3*(a[0][0] + a[3][3]) + (float)w4*(a[1][1] + a[2][2]);
            // generate and write result
            if (d_edge <= 0.0f) { rf = r1; gf = g1; bf = b1; af = a1; }
            else { rf = r2; gf = g2; bf = b2; af = a2; }
            // anti-ringing, clamp.
            rf = clamp(rf, min_r_sample, max_r_sample);
            gf = clamp(gf, min_g_sample, max_g_sample);
            bf = clamp(bf, min_b_sample, max_b_sample);
            af = clamp(af, min_a_sample, max_a_sample);
            ri = clamp(static_cast<int>(ceilf(rf)), 0, 255);
            gi = clamp(static_cast<int>(ceilf(gf)), 0, 255);
            bi = clamp(static_cast<int>(ceilf(bf)), 0, 255);
            ai = clamp(static_cast<int>(ceilf(af)), 0, 255);
            out[(y+1)*outw + x] = (ai << 24) | (bi << 16) | (gi << 8) | ri;
            ++x;
        }
        ++y;
    }
 
    // Third Pass
    wp[0] =  2.0f;
    wp[1] =  1.0f;
    wp[2] = -1.0f;
    wp[3] =  4.0f;
    wp[4] = -1.0f;
    wp[5] =  1.0f;
 
    for (int y = outh - 1; y >= 0; --y) {
        for (int x = outw - 1; x >= 0; --x) {
            float r[4][4], g[4][4], b[4][4], a[4][4], Y[4][4];
            for (int sx = -2; sx <= 1; ++sx) {
                for (int sy = -2; sy <= 1; ++sy) {
                    // clamp pixel locations
                    int csy = clamp(sy + y, 0, f*h - 1);
                    int csx = clamp(sx + x, 0, f*w - 1);
                    // sample & add weighted components
                    uint32_t sample = out[csy*outw + csx];
                    r[sx + 2][sy + 2] = (float)R(sample);
                    g[sx + 2][sy + 2] = (float)G(sample);
                    b[sx + 2][sy + 2] = (float)B(sample);
                    a[sx + 2][sy + 2] = (float)A(sample);
                    Y[sx + 2][sy + 2] = (float)(0.2126*r[sx + 2][sy + 2] + 0.7152*g[sx + 2][sy + 2] + 0.0722*b[sx + 2][sy + 2]);
                }
            }
            float min_r_sample = min4(r[1][1], r[2][1], r[1][2], r[2][2]);
            float min_g_sample = min4(g[1][1], g[2][1], g[1][2], g[2][2]);
            float min_b_sample = min4(b[1][1], b[2][1], b[1][2], b[2][2]);
            float min_a_sample = min4(a[1][1], a[2][1], a[1][2], a[2][2]);
            float max_r_sample = max4(r[1][1], r[2][1], r[1][2], r[2][2]);
            float max_g_sample = max4(g[1][1], g[2][1], g[1][2], g[2][2]);
            float max_b_sample = max4(b[1][1], b[2][1], b[1][2], b[2][2]);
            float max_a_sample = max4(a[1][1], a[2][1], a[1][2], a[2][2]);
            float d_edge = diagonal_edge(Y, &wp[0]);
            float r1, g1, b1, a1, r2, g2, b2, a2, rf, gf, bf, af;
            r1 = (float)w1*(r[0][3] + r[3][0]) + (float)w2*(r[1][2] + r[2][1]);
            g1 = (float)w1*(g[0][3] + g[3][0]) + (float)w2*(g[1][2] + g[2][1]);
            b1 = (float)w1*(b[0][3] + b[3][0]) + (float)w2*(b[1][2] + b[2][1]);
            a1 = (float)w1*(a[0][3] + a[3][0]) + (float)w2*(a[1][2] + a[2][1]);
            r2 = (float)w1*(r[0][0] + r[3][3]) + (float)w2*(r[1][1] + r[2][2]);
            g2 = (float)w1*(g[0][0] + g[3][3]) + (float)w2*(g[1][1] + g[2][2]);
            b2 = (float)w1*(b[0][0] + b[3][3]) + (float)w2*(b[1][1] + b[2][2]);
            a2 = (float)w1*(a[0][0] + a[3][3]) + (float)w2*(a[1][1] + a[2][2]);
            // generate and write result
            if (d_edge <= 0.0f) { rf = r1; gf = g1; bf = b1; af = a1; }
            else { rf = r2; gf = g2; bf = b2; af = a2; }
            // anti-ringing, clamp.
            rf = clamp(rf, min_r_sample, max_r_sample);
            gf = clamp(gf, min_g_sample, max_g_sample);
            bf = clamp(bf, min_b_sample, max_b_sample);
            af = clamp(af, min_a_sample, max_a_sample);
            int ri = clamp(static_cast<int>(ceilf(rf)), 0, 255);
            int gi = clamp(static_cast<int>(ceilf(gf)), 0, 255);
            int bi = clamp(static_cast<int>(ceilf(bf)), 0, 255);
            int ai = clamp(static_cast<int>(ceilf(af)), 0, 255);
            out[y*outw + x] = (ai << 24) | (bi << 16) | (gi << 8) | ri;
        }
    }
 
}
 
 
//// *** Super-xBR code ends here - MIT LICENSE *** ///
 
 
void scaleSuperXBR(int factor, uint32_t* data, uint32_t* out, int w, int h) {
    switch (factor) {
    case 2: scaleSuperXBRT<2>(data, out, w, h); break;
    }
}
