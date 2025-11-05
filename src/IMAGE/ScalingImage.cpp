#include "ScalingImage.h"

Image IntegerScaleImage(const Image& input, const int& scale){
    Image output;
    output.width = input.width*scale;
    output.height = input.height*scale;
    output.channels = 4;
    output.pixels.resize(output.width*output.height);

    for(int x = 0; x < input.width; x++){
        for(int y = 0; y < input.height; y++){
            int index = y * input.width + x;
            const Pixel& p = input.pixels[index];

            for(int x_scaled = 0; x_scaled < scale; x_scaled++){
                for(int y_scaled = 0; y_scaled < scale; y_scaled++){

                    int index_scaled = (y*scale + y_scaled) * input.width*scale + (x*scale + x_scaled);

                    output.pixels[index_scaled] = p;
                }
            }
        }
    }

    return output;
}

Image BilinearScaleImage(const Image& input, const int& scale) {
    Image output;
    output.width = input.width * scale;
    output.height = input.height * scale;
    output.channels = 4;
    output.pixels.resize(output.width * output.height);

    float inv_scale_x = 1.0f / scale;
    float inv_scale_y = 1.0f / scale;

    for (int y_out = 0; y_out < output.height; ++y_out) {
        for (int x_out = 0; x_out < output.width; ++x_out) {
            float x_in = (x_out + 0.5f) * inv_scale_x - 0.5f;
            float y_in = (y_out + 0.5f) * inv_scale_y - 0.5f;

            int x0 = static_cast<int>(std::floor(x_in));
            int y0 = static_cast<int>(std::floor(y_in));
            int x1 = x0 + 1;
            int y1 = y0 + 1;

            x0 = std::max(0, std::min(x0, input.width - 1));
            x1 = std::max(0, std::min(x1, input.width - 1));
            y0 = std::max(0, std::min(y0, input.height - 1));
            y1 = std::max(0, std::min(y1, input.height - 1));

            float dx = x_in - x0;
            float dy = y_in - y0;

            const Pixel& p00 = input.pixels[y0 * input.width + x0];
            const Pixel& p01 = input.pixels[y0 * input.width + x1];
            const Pixel& p10 = input.pixels[y1 * input.width + x0];
            const Pixel& p11 = input.pixels[y1 * input.width + x1];

            Pixel& out_pixel = output.pixels[y_out * output.width + x_out];
            out_pixel.r = (1 - dy) * ((1 - dx) * p00.r + dx * p01.r) + dy * ((1 - dx) * p10.r + dx * p11.r);
            out_pixel.g = (1 - dy) * ((1 - dx) * p00.g + dx * p01.g) + dy * ((1 - dx) * p10.g + dx * p11.g);
            out_pixel.b = (1 - dy) * ((1 - dx) * p00.b + dx * p01.b) + dy * ((1 - dx) * p10.b + dx * p11.b);
            out_pixel.a = (1 - dy) * ((1 - dx) * p00.a + dx * p01.a) + dy * ((1 - dx) * p10.a + dx * p11.a);
        }
    }

    return output;
}


Image BicubicScaleImage(const Image& input, const int& scale) {

    auto CubicHermite = [](long double A, long double B, long double C, long double D, long double t) -> long double{
        long double a = -A / 2.0L + (3.0L * B) / 2.0L - (3.0L * C) / 2.0L + D / 2.0L;
        long double b = A - (5.0L * B) / 2.0L + 2.0L * C - D / 2.0L;
        long double c = -A / 2.0L + C / 2.0L;
        long double d = B;
        return a * t * t * t + b * t * t + c * t + d;
    };


    Image output;
    output.width = input.width * scale;
    output.height = input.height * scale;
    output.channels = 4;
    output.pixels.resize(output.width * output.height);

    long double inv_scale_x = 1.0L / (long double)scale;
    long double inv_scale_y = 1.0L / (long double)scale;

    for (int y_out = 0; y_out < output.height; ++y_out) {
        for (int x_out = 0; x_out < output.width; ++x_out) {
            long double x_in = (x_out + 0.5L) * inv_scale_x - 0.5L;
            long double y_in = (y_out + 0.5L) * inv_scale_y - 0.5L;

            int x0 = static_cast<int>(std::floor(x_in));
            int y0 = static_cast<int>(std::floor(y_in));

            long double dx = x_in - (long double)x0;
            long double dy = y_in - (long double)y0;

            Pixel samples[4][4];
            for (int j = -1; j <= 2; ++j) {
                for (int i = -1; i <= 2; ++i) {
                    int sx = std::max(0, std::min(x0 + i, input.width - 1));
                    int sy = std::max(0, std::min(y0 + j, input.height - 1));
                    samples[j + 1][i + 1] = input.pixels[sy * input.width + sx];
                }
            }

            Pixel& out_pixel = output.pixels[y_out * output.width + x_out];

            long double row[4];
            for (int j = 0; j < 4; ++j) {
                row[j] = CubicHermite(samples[j][0].r, samples[j][1].r, samples[j][2].r, samples[j][3].r, dx);
            }
            out_pixel.r = CubicHermite(row[0], row[1], row[2], row[3], dy);

            for (int j = 0; j < 4; ++j) {
                row[j] = CubicHermite(samples[j][0].g, samples[j][1].g, samples[j][2].g, samples[j][3].g, dx);
            }
            out_pixel.g = CubicHermite(row[0], row[1], row[2], row[3], dy);

            for (int j = 0; j < 4; ++j) {
                row[j] = CubicHermite(samples[j][0].b, samples[j][1].b, samples[j][2].b, samples[j][3].b, dx);
            }
            out_pixel.b = CubicHermite(row[0], row[1], row[2], row[3], dy);

            for (int j = 0; j < 4; ++j) {
                row[j] = CubicHermite(samples[j][0].a, samples[j][1].a, samples[j][2].a, samples[j][3].a, dx);
            }
            out_pixel.a = CubicHermite(row[0], row[1], row[2], row[3], dy);
        }
    }

    return output;
}
