#pragma once
namespace charts
{
	std::string chart_options();
	struct chart
	{
		int width;
		int height;
		std::string chart_str;
		std::string chart_div;

		chart() : width(0), height(0), chart_str(""), chart_div(""){}
		chart(int width, int height, std::string chart_str, std::string name, int idx) : width(width), height(height), chart_str(chart_str)
		{
			std::stringstream chart_stream;
			chart_stream << "<div id = \"actor" << idx << name << "\" style=\"min-width: " << width << "px; min-height: " << height << "px; margin: 5px; \"></div>";
			chart_div = chart_stream.str();
		}
	};
}
namespace p_charts
{
#ifndef CHARTS_H
#define CHARTS_H
	void create_player_charts(player_t* player, int idx);
#endif
}