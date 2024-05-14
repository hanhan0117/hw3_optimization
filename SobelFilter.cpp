#include <cmath>
#ifndef NATIVE_SYSTEMC
#include "stratus_hls.h"
#endif

#include "SobelFilter.h"

SobelFilter::SobelFilter(sc_module_name n) : sc_module(n)
{
#ifndef NATIVE_SYSTEMC
	HLS_FLATTEN_ARRAY(val);
#endif
	SC_THREAD(do_filter);
	sensitive << i_clk.pos();
	dont_initialize();
	reset_signal_is(i_rst, false);

#ifndef NATIVE_SYSTEMC
	i_rgb.clk_rst(i_clk, i_rst);
	o_result.clk_rst(i_clk, i_rst);
#endif
}

SobelFilter::~SobelFilter() {}

// sobel mask
const int mask[MASK_N][MASK_X][MASK_Y] = {{{1, 4, 7, 4, 1}, {4, 16, 26, 16, 4}, {7, 26, 41, 26, 7}, {4, 16, 26, 16, 4}, {1, 4, 7, 4, 1}}};

void SobelFilter::do_filter()
{
	{
#ifndef NATIVE_SYSTEMC
		HLS_DEFINE_PROTOCOL("main_reset");
		i_rgb.reset();
		o_result.reset();
#endif
		wait();
	}
	unsigned char grey[5][5] = {0};
	while (true)
	{
		for (unsigned int i = 0; i < MASK_N; ++i)
		{
			// HLS_CONSTRAIN_LATENCY(0, 1, "lat00");
			val[i] = 0;
		}

		for (unsigned int x = 0; x < MASK_X; ++x)
		{
			sc_dt::sc_uint<24> rgb;
#ifndef NATIVE_SYSTEMC
			{
				HLS_DEFINE_PROTOCOL("input");
				rgb = i_rgb.get();
				wait();
			}
#else
			rgb = i_rgb.read();
#endif
			grey[x][MASK_Y - 1] = (rgb.range(7, 0) + rgb.range(15, 8) + rgb.range(23, 16)) / 3;
		}
		// grey = (rgb.range(7, 0) + rgb.range(15, 8) + rgb.range(23, 16)) / 3;
		for (unsigned int v = 0; v < MASK_Y; ++v)
		{
			for (unsigned int u = 0; u < MASK_X; ++u)
			{
				for (unsigned int i = 0; i != MASK_N; ++i)
				{
					// HLS_CONSTRAIN_LATENCY(0, 1, "lat01");
					val[i] += grey[u][v] * mask[i][u][v];
					if (v > 0 && v < MASK_Y)
					{
						grey[u][v - 1] = grey[u][v];
					}
				}
			}
		}
		/*
		for (unsigned int i = 0; i != MASK_N; ++i)
		{
			HLS_CONSTRAIN_LATENCY(0, 1, "lat01");
			val[i] += grey * mask[i][u][v];
		}
		*/

		sc_uint<17> total = 0;
		for (unsigned int i = 0; i != MASK_N; ++i)
		{
			HLS_CONSTRAIN_LATENCY(0, 1, "lat01");
			total += val[i];
			// total = (int)(total / 273);
		}
		sc_uint<8> result = total / 273;

#ifndef NATIVE_SYSTEMC
		{
			HLS_DEFINE_PROTOCOL("output");
			o_result.put(result);
			wait();
		}
#else
		o_result.write(total);
#endif
	}
}
