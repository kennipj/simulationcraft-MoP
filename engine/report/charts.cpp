#include "simulationcraft.hpp"
#include "sc_report.hpp"
#include <util/json.hpp>
#include <sstream>
#include "charts.h"

using json = nlohmann::json;


namespace charts {
	namespace color
	{
		// http://www.wowwiki.com/Class_colors
		const std::string light_blue = "69CCF0";
		const std::string pink = "F58CBA";
		const std::string purple = "9482C9";
		const std::string red = "C41F3B";
		const std::string tan = "C79C6E";
		const std::string yellow = "FFF569";
		const std::string blue = "0070DE";
		const std::string hunter_green = "ABD473";
		const std::string jade_green = "00FF96";

		// http://www.brobstsystems.com/colors1.htm
		const std::string purple_dark = "7668A1";
		const std::string white = "FFFFFF";
		const std::string nearly_white = "FCFFFF";
		const std::string green = "336600";
		const std::string grey = "C0C0C0";
		const std::string olive = "909000";
		const std::string orange = "FF7D0A";
		const std::string teal = "009090";
		const std::string darker_blue = "59ADCC";
		const std::string darker_silver = "8A8A8A";
		const std::string darker_yellow = "C0B84F";
		const std::string holy_yellow = "030201";


		std::string format_hex(std::string color)
		{
			std::stringstream hex_color;
			hex_color << "#" << color;
			return hex_color.str();
		}

		/* Creates the average color of two given colors
		 */
		std::string mix(const std::string& color1, const std::string& color2)
		{
			assert((color1.length() == 6) && (color2.length() == 6));

			std::stringstream converter1(color1);
			unsigned int value;
			converter1 >> std::hex >> value;
			std::stringstream converter2(color2);
			unsigned int value2;
			converter2 >> std::hex >> value2;

			value += value2;
			value /= 2;
			std::stringstream out;
			out << std::uppercase << std::hex << value;
			return out.str();
		}

		/* Creates the average of all sequentially given color codes
		 */
		std::string mix_multiple(const std::string& color)
		{
			assert(color.size() % 6 == 0);

			unsigned i = 0, total_value = 0;
			for (; (i + 1) * 6 < color.length(); ++i)
			{
				std::stringstream converter1(color.substr(i * 6, 6));
				unsigned value;
				converter1 >> std::hex >> value;
				total_value += value;
			}
			if (i) total_value /= i;

			std::stringstream out;
			out << std::uppercase << std::noskipws << std::hex << total_value;
			return out.str();
		}
	}

	std::string chart_options()
	{
		json options;
		options["credits"] = false;
		options["lang"] = { {"decimalPoint", "."}, {"thousandsSep", ","} };
		options["legend"] = { {"enabled", false}, {"itemStyle", {{"fontsize", "14px"}, {"color", "#CACACA"}}} };
		options["chart"] = {
			{"borderRadius", 4},
			{"backgroundColor", "#242424"},
			{"style", {
				{"fontsize", "13px"}
			}},
			{"spacing", {2, 2, 2, 2}},
		};
		options["xAxis"] = {
			{"lineColor", "#CACACA"},
			{"tickColor", "#CACACA"},
			{"title", {{"style", {{"color", "#CACACA"}}}}},
			{"labels", {{"style", {{"color", "#CACACA"}, {"fontSize", "14px"}}}}}
		};
		options["yAxis"] = {
			{"lineColor", "#CACACA"},
			{"tickColor", "#CACACA"},
			{"title", {{"style", {{"color", "#CACACA"}}}}},
			{"labels", {{"style", {{"color", "#CACACA"}, {"fontSize", "14px"}}}}}
		};
		options["title"] = {
			{"style", {{"fontSize", "15px"}, {"color", "#CACACA"}}}
		};
		options["subtitle"] = {
			{"style", {{"fontSize", "15px"}}}
		};
		options["tooltip"] = {
			{"backgroundColor", "#3F3E38"},
			{"style", {{"color", "#CACACA"}}},
			{"valueDecimals", 1}
		};
		options["plotOptions"] = {
			{"series", {{"shadow", true}, {"dataLabels", {{"style", {{"color", "#CACACA"}}}}}}},
			{"pie", {{"fillOpacity", 0.2f}, {"dataLabels", {{"enabled", true}, {"style", {{"fontWeight", "none"}}}}}}},
			{"bar", {{"borderWidth", 0}, {"pointWidth", 18}}},
			{"column", {{"borderWidth", 0}, {"pointWidth", 8}}},
			{"area", {{"lineWidth", 1.25}, {"fillOpacity", 0.2}, {"states", {{"hover", {{"lineWidth", 1}}}}}}},
		};
		auto option_str = options.dump(4);

		std::stringstream hc_opt;
		hc_opt << "<script type=\"text/javascript\">\n";
		hc_opt << "Highcharts.setOptions(" << option_str << ");\n";
		hc_opt << "</script>\n";
		return hc_opt.str();
	}


	std::string name_color_to_span(std::string name, std::string hex_color)
	{
		std::stringstream span;
		span << "<span style=\"color:" << hex_color << "\">" << name << "</span>";
		return span.str();
	}

	class chart_data_point
	{
	private:
		std::string _name;
		std::string _hex_color;
		float _value;

	public:
		chart_data_point(std::string name, std::string hex_color, float value) :
			_name(name), _hex_color(hex_color), _value(value) {}

		json to_json()
		{
			json data;
			std::stringstream name_stream;
			auto _color = color::format_hex(_hex_color);


			data["color"] = _color;
			data["name"] = name_color_to_span(_name, _color);
			data["y"] = _value;
			return data;
		}
	};

	class horizontal_bar_chart
	{
	private:
		std::string _title;
		std::string _actor_name;
		std::string _series_name;
		std::vector<charts::chart_data_point> _data;
	public:
		horizontal_bar_chart(std::string title, std::string actor_name, std::string series_name, std::vector<charts::chart_data_point> data) :
			_title(title), _actor_name(actor_name), _series_name(series_name), _data(data) {}

		json to_json() {
			std::stringstream title_stream;
			title_stream << _actor_name << " " << _title;

			json data = json::array();
			for (int i = 0; i < _data.size(); i++)
				data.push_back(_data.at(i).to_json());

			json chart = {
				{"chart", {{"type", "bar"}}},
				{"xAxis", {
					{"tickLength", 0},
					{"type", "category"},
					{"labels", {
						{"step", 1},
						{"y", 4}}
					},
					{"offset", 0}},
				},
				{"yAxis", {
					{"title",{
						{"text", _title}
					}}
				}},
				{"title", {{"text", title_stream.str()}}},
				{"series", json::array({{{"data", data}, {"name", _series_name}}})}
			};
			return chart;
		}
	};

	class pie_chart
	{
	private:
		std::string _title;
		std::string _actor_name;
		std::string _label_format;
		std::vector<charts::chart_data_point> _data;
	public:
		pie_chart(std::string title, std::string actor_name, std::string label_format, std::vector<charts::chart_data_point> data) :
			_title(title), _actor_name(actor_name), _label_format(label_format), _data(data) {}

		json to_json() {
			std::stringstream title_stream;
			title_stream << _actor_name << " " << _title;

			json data = json::array();
			for (int i = 0; i < _data.size(); i++)
				data.push_back(_data.at(i).to_json());

			json chart = {
				{"chart", {{"type", "pie"}}},
				{"plotOptions", {
					{"pie", {
						{"dataLabels", {{"format", _label_format}},
					}},
				}}},
				{"title", {{"text", title_stream.str()}}},
				{"series", json::array({{{"data", data}, {"name", _title}}})}
			};
			return chart;
		}
	};

	class area_chart
	{
	private:
		std::string _title;
		std::string _actor_name;
		std::string _series_name;
		std::string _line_color;
		std::string _tooltip;
		std::string _xaxis_title;
		std::string _mean_color;
		float _mean;
		std::vector<double> _data;
	public:
		area_chart(
			std::string title,
			std::string actor_name,
			std::string series_name,
			std::string line_color,
			std::vector<double> data,
			std::string mean_color,
			float mean,
			std::string tooltip,
			std::string xaxis_title
		) :
			_title(title), _actor_name(actor_name), _series_name(series_name), _line_color(line_color), _data(data), _mean_color(mean_color), _mean(mean), _tooltip(tooltip), _xaxis_title(xaxis_title) {}

		json to_json() {
			std::stringstream title_stream;
			title_stream << _actor_name << " " << _title;

			std::stringstream mean_span;
			mean_span << "<span style=\"color: " << _mean_color << "\";>mean=" << _mean << "</span>";

			json data = json::array();
			for (int i = 0; i < _data.size(); i++)
				data.push_back(_data.at(i));

			json chart = {
				{"chart", {{"type", "area"}}},
				{"tooltip", {{"headerFormat", _tooltip}}},
				{"yAxis", {
					{"plotLines", {
							{{"color", _mean_color},
							{"value", _mean},
							{"width", 1.25},
							{"zIndex", 5}}
					}},
					{"min", 0}, {"title", {{"text", _title}}},
				}},
				{"xAxis", {
					{"title", {"text", _xaxis_title}},
				}},
				{"colors", {color::format_hex(_line_color)}},
				{"title", {{"text", title_stream.str()}}},
				{"subtitle", {{"text", mean_span.str()}}},
				{"series", json::array({{{"data", data}, {"name", _series_name}, {"type", "area"}}})}
			};
			return chart;
		}
	};

	class column_chart
	{
	private:
		std::string _title;
		std::string _actor_name;
		std::string _tooltip;
		std::string _column_color;
		std::string _yaxis_title;
		std::vector<charts::chart_data_point> _data;
		int _mean_bucket;
	public:
		column_chart(std::string title, std::string actor_name, std::string tooltip, std::string column_color, std::string yaxis_title, std::vector<charts::chart_data_point> data, int mean_bucket) :
			_title(title), _actor_name(actor_name), _tooltip(tooltip), _column_color(column_color), _yaxis_title(yaxis_title), _data(data), _mean_bucket(mean_bucket){}

		json to_json() {
			std::stringstream title_stream;
			title_stream << _actor_name << " " << _title;

			json data = json::array();
			for (int i = 0; i < _data.size(); i++)
			{
				data.push_back(_data.at(i).to_json());
			}

			json chart = {
				{"chart", {{"type", "column"}}},
				{"xAxis", {
					{"tickLength", 0},
					{"type", "category"},
					{"tickInterval", 25},
					{"tickAtEnd", true},
					{"tickPositions", {0, _mean_bucket, 49}},
				}},
				{"yAxis", {
					{"title",{
						{"text", _yaxis_title}
					}}
				}},
				{"plotOptions", {
					{"column", {
						{"color", _column_color},
					}},
				}},
				{"title", {{"text", title_stream.str()}}},
				{"tooltip", {{"headerFormat", _tooltip}}},
				{"series", json::array({{{"data", data}, {"name", _title}}})}
			};
			return chart;
		}
	};

	namespace util
	{
		struct compare_downtime
		{
			bool operator()(player_t* l, player_t* r) const
			{
				return l->collected_data.waiting_time.mean() > r->collected_data.waiting_time.mean();
			}
		};

		struct filter_non_performing_players
		{
			bool dps;
			filter_non_performing_players(bool dps_) : dps(dps_) {}
			bool operator()(player_t* p) const
			{
				if (dps) { if (p->collected_data.dps.mean() <= 0) return true; }
				else if (p->collected_data.hps.mean() <= 0) return true; return false;
			}
		};

		struct compare_dpet
		{
			bool operator()(const stats_t* l, const stats_t* r) const
			{
				return l->apet > r->apet;
			}
		};

		struct filter_stats_dpet
		{
			bool player_is_healer;
			filter_stats_dpet(player_t& p) : player_is_healer(p.primary_role() == ROLE_HEAL) {}
			bool operator()(const stats_t* st) const
			{
				if (st->quiet) return true;
				if (st->apet <= 0) return true;
				if (st->num_refreshes.mean() > 4 * st->num_executes.mean()) return true;
				if (player_is_healer != (st->type != STATS_DMG)) return true;

				return false;
			}
		};

		struct compare_amount
		{
			bool operator()(const stats_t* l, const stats_t* r) const
			{
				return l->actual_amount.mean() > r->actual_amount.mean();
			}
		};

		struct compare_stats_time
		{
			bool operator()(const stats_t* l, const stats_t* r) const
			{
				return l->total_time > r->total_time;
			}
		};

		struct filter_waiting_stats
		{
			bool operator()(const stats_t* st) const
			{
				if (st->quiet) return true;
				if (st->total_time <= timespan_t::zero()) return true;
				if (st->background) return true;

				return false;
			}
		};

		struct compare_gain
		{
			bool operator()(const gain_t* l, const gain_t* r) const
			{
				return l->actual > r->actual;
			}
		};

		player_e get_player_or_owner_type(player_t* p)
		{
			if (p->is_pet())
				p = p->cast_pet()->owner;

			return p->type;
		}
	}

	namespace colors
	{
		std::string school_color(school_e type)
		{
			switch (type)
			{
				// -- Single Schools
				// Doesn't use the same colors as the blizzard ingame UI, as they are ugly
			case SCHOOL_NONE:         return color::white;
			case SCHOOL_PHYSICAL:     return color::tan;
			case SCHOOL_HOLY:         return color::holy_yellow;
			case SCHOOL_FIRE:         return color::red;
			case SCHOOL_NATURE:       return color::green;
			case SCHOOL_FROST:        return color::blue;
			case SCHOOL_SHADOW:       return color::purple;
			case SCHOOL_ARCANE:       return color::light_blue;
				// -- Physical and a Magical
			case SCHOOL_FLAMESTRIKE:  return color::mix(school_color(SCHOOL_PHYSICAL), school_color(SCHOOL_FIRE));
			case SCHOOL_FROSTSTRIKE:  return color::mix(school_color(SCHOOL_PHYSICAL), school_color(SCHOOL_FROST));
			case SCHOOL_SPELLSTRIKE:  return color::mix(school_color(SCHOOL_PHYSICAL), school_color(SCHOOL_ARCANE));
			case SCHOOL_STORMSTRIKE:  return color::mix(school_color(SCHOOL_PHYSICAL), school_color(SCHOOL_NATURE));
			case SCHOOL_SHADOWSTRIKE: return color::mix(school_color(SCHOOL_PHYSICAL), school_color(SCHOOL_SHADOW));
			case SCHOOL_HOLYSTRIKE:   return color::mix(school_color(SCHOOL_PHYSICAL), school_color(SCHOOL_HOLY));
				// -- Two Magical Schools
			case SCHOOL_FROSTFIRE:    return color::mix(school_color(SCHOOL_FROST), school_color(SCHOOL_FIRE));
			case SCHOOL_SPELLFIRE:    return color::mix(school_color(SCHOOL_ARCANE), school_color(SCHOOL_FIRE));
			case SCHOOL_FIRESTORM:    return color::mix(school_color(SCHOOL_FIRE), school_color(SCHOOL_NATURE));
			case SCHOOL_SHADOWFLAME:  return color::mix(school_color(SCHOOL_SHADOW), school_color(SCHOOL_FIRE));
			case SCHOOL_HOLYFIRE:     return color::mix(school_color(SCHOOL_HOLY), school_color(SCHOOL_FIRE));
			case SCHOOL_SPELLFROST:   return color::mix(school_color(SCHOOL_ARCANE), school_color(SCHOOL_FROST));
			case SCHOOL_FROSTSTORM:   return color::mix(school_color(SCHOOL_FROST), school_color(SCHOOL_NATURE));
			case SCHOOL_SHADOWFROST:  return color::mix(school_color(SCHOOL_SHADOW), school_color(SCHOOL_FROST));
			case SCHOOL_HOLYFROST:    return color::mix(school_color(SCHOOL_HOLY), school_color(SCHOOL_FROST));
			case SCHOOL_SPELLSTORM:   return color::mix(school_color(SCHOOL_ARCANE), school_color(SCHOOL_NATURE));
			case SCHOOL_SPELLSHADOW:  return color::mix(school_color(SCHOOL_ARCANE), school_color(SCHOOL_SHADOW));
			case SCHOOL_DIVINE:       return color::mix(school_color(SCHOOL_ARCANE), school_color(SCHOOL_HOLY));
			case SCHOOL_SHADOWSTORM:  return color::mix(school_color(SCHOOL_SHADOW), school_color(SCHOOL_NATURE));
			case SCHOOL_HOLYSTORM:    return color::mix(school_color(SCHOOL_HOLY), school_color(SCHOOL_NATURE));
			case SCHOOL_SHADOWLIGHT:  return color::mix(school_color(SCHOOL_SHADOW), school_color(SCHOOL_HOLY));
				//-- Three or more schools
			case SCHOOL_ELEMENTAL:    return color::mix_multiple(school_color(SCHOOL_FIRE) +
				school_color(SCHOOL_FROST) +
				school_color(SCHOOL_NATURE));
			case SCHOOL_CHROMATIC:    return color::mix_multiple(school_color(SCHOOL_FIRE) +
				school_color(SCHOOL_FROST) +
				school_color(SCHOOL_ARCANE) +
				school_color(SCHOOL_NATURE) +
				school_color(SCHOOL_SHADOW));
			case SCHOOL_MAGIC:    return color::mix_multiple(school_color(SCHOOL_FIRE) +
				school_color(SCHOOL_FROST) +
				school_color(SCHOOL_ARCANE) +
				school_color(SCHOOL_NATURE) +
				school_color(SCHOOL_SHADOW) +
				school_color(SCHOOL_HOLY));
			case SCHOOL_CHAOS:    return color::mix_multiple(school_color(SCHOOL_PHYSICAL) +
				school_color(SCHOOL_FIRE) +
				school_color(SCHOOL_FROST) +
				school_color(SCHOOL_ARCANE) +
				school_color(SCHOOL_NATURE) +
				school_color(SCHOOL_SHADOW) +
				school_color(SCHOOL_HOLY));

			default: return std::string();
			}
		}


		std::string class_color(player_e type)
		{
			switch (type)
			{
			case PLAYER_NONE:  return color::grey;
			case DEATH_KNIGHT: return color::red;
			case DRUID:        return color::orange;
			case HUNTER:       return color::hunter_green;
			case MAGE:         return color::light_blue;
			case MONK:         return color::jade_green;
			case PALADIN:      return color::pink;
			case PRIEST:       return color::white;
			case ROGUE:        return color::yellow;
			case SHAMAN:       return color::blue;
			case WARLOCK:      return color::purple;
			case WARRIOR:      return color::tan;
			case ENEMY:        return color::grey;
			case ENEMY_ADD:    return color::grey;
			case HEALING_ENEMY:    return color::grey;
			default: assert(0); return std::string();
			}
		}


		std::string stat_color(stat_e type)
		{
			switch (type)
			{
			case STAT_STRENGTH:                 return class_color(WARRIOR);
			case STAT_AGILITY:                  return class_color(HUNTER);
			case STAT_INTELLECT:                return class_color(MAGE);
			case STAT_SPIRIT:                   return color::darker_silver;
			case STAT_ATTACK_POWER:             return class_color(ROGUE);
			case STAT_SPELL_POWER:              return class_color(WARLOCK);
			case STAT_HIT_RATING:               return class_color(DEATH_KNIGHT);
			case STAT_CRIT_RATING:              return class_color(PALADIN);
			case STAT_HASTE_RATING:             return class_color(SHAMAN);
			case STAT_MASTERY_RATING:           return class_color(ROGUE);
			case STAT_EXPERTISE_RATING:         return color::mix(color::red, color::tan);
			case STAT_DODGE_RATING:             return class_color(MONK);
			case STAT_PARRY_RATING:             return color::teal;
			case STAT_ARMOR:                    return class_color(PRIEST);
			default:                            return std::string();
			}
		}
	}
}


namespace p_charts
{

	std::string chart_to_js(std::string chart, int player_idx, std::string chart_name)
	{
		std::stringstream chart_js;
		chart_js << "$('#actor" << player_idx << chart_name << "').highcharts(" << chart << ");";
		return chart_js.str();
	}

	charts::chart _action_dpet_chart(player_t* player, int idx)
	{
		std::vector<stats_t*> stats_list;

		// Copy all stats * from p->stats_list to stats_list, which satisfy the filter
		range::remove_copy_if(player->stats_list, back_inserter(stats_list), charts::util::filter_stats_dpet(*player));
		
		int num_stats = (int)stats_list.size();
		if (num_stats == 0)
			return charts::chart();

		range::sort(stats_list, charts::util::compare_dpet());

		auto data = std::vector<charts::chart_data_point>();

		for (int i = 0; i < num_stats; i++)
		{
			stats_t* st = stats_list[i];
			data.push_back(charts::chart_data_point(st->name(), charts::colors::school_color(st->school), st->apet));
		}
		auto chart = charts::horizontal_bar_chart("Damage per Execute Time", player->name_str, "Damage per Execute Time", data);
		return charts::chart(525, 30 + (30 * num_stats), chart_to_js(chart.to_json().dump(), idx, "dpet"), "dpet", idx);
	}

	charts::chart _sources_chart(player_t* player, int idx)
	{
		std::vector<stats_t*> stats_list;

		for (size_t i = 0; i < player->stats_list.size(); ++i)
		{
			stats_t* st = player->stats_list[i];
			if (st->quiet) continue;
			if (st->actual_amount.mean() <= 0) continue;
			if ((player->primary_role() == ROLE_HEAL) != (st->type != STATS_DMG)) continue;
			stats_list.push_back(st);
		}

		for (size_t i = 0; i < player->pet_list.size(); ++i)
		{
			pet_t* pet = player->pet_list[i];
			for (size_t j = 0; j < pet->stats_list.size(); ++j)
			{
				stats_t* st = pet->stats_list[j];
				if (st->quiet) continue;
				if (st->actual_amount.mean() <= 0) continue;
				if ((player->primary_role() == ROLE_HEAL) != (st->type != STATS_DMG)) continue;
				stats_list.push_back(st);
			}
		}

		int num_stats = (int)stats_list.size();
		if (num_stats == 0)
			return charts::chart();

		std::vector<charts::chart_data_point> data;

		for (int i = 0; i < num_stats; i++)
		{
			stats_t* st = stats_list.at(i);
			float val = 100.0 * st->actual_amount.mean() / ((player->primary_role() == ROLE_HEAL) ? player->collected_data.heal.mean() : player->collected_data.dmg.mean());
			data.push_back(charts::chart_data_point(st->name_str, charts::colors::school_color(st->school), val));
		}

		std::stringstream title;
		title << (player->primary_role() == ROLE_HEAL ? " Healing" : " Damage") << " Sources";

		auto chart = charts::pie_chart(title.str(), player->name_str, "<b>{point.name}</b> : {point.y:.1f}%", data);
		return charts::chart(525, 250, chart_to_js(chart.to_json().dump(), idx, "dps_sources"), "dps_sources", idx);
	}


	charts::chart _dps_chart(player_t* player, int idx)
	{
		sc_timeline_t timeline_dps;
		player->collected_data.timeline_dmg.build_derivative_timeline(timeline_dps);
		auto chart = charts::area_chart(
			"Damage per second",
			player->name_str,
			"DPS",
			charts::colors::class_color(charts::util::get_player_or_owner_type(player)),
			timeline_dps.data(),
			"#CC8888",
			player->collected_data.dps.mean(),
			"Second: {point.key}<br/>", "Time (seconds)"
		);

		return charts::chart(525, 250, chart_to_js(chart.to_json().dump(), idx, "dps"), "dps", idx);
	}

	charts::chart _dps_distribution_chart(player_t* player, int idx)
	{
		auto dist = player->collected_data.dps.distribution;

		int num_buckets = (int)dist.size();

		std::vector<charts::chart_data_point> data;
		std::stringstream min_stream;
		std::stringstream mean_stream;
		std::stringstream max_stream;

		auto min = player->collected_data.dps.min();
		auto mean = player->collected_data.dps.mean();
		auto max = player->collected_data.dps.max();

		min_stream << "min=" << min;
		mean_stream << "mean=" << mean;
		max_stream << "max=" << max;

		const int interval = (max - min) / num_buckets;


		int mean_bucket{};

		for (int i = 0; i < num_buckets; i++)
		{
			auto min_bucket = min + (interval * i);
			auto max_bucket = min + (interval * (i + 1));
			if (i == 0)
			{
				data.push_back(charts::chart_data_point(min_stream.str(), "BFB74E", (float)dist[i]));
				continue;
			}

			if (min_bucket <= mean && max_bucket >= mean)
			{
				data.push_back(charts::chart_data_point(mean_stream.str(), "BFB74E", (float)dist[i]));
				mean_bucket = i;
				continue;
			}

			if (i == num_buckets - 1)
			{
				data.push_back(charts::chart_data_point(max_stream.str(), "BFB74E", (float)dist[i]));
				continue;
			}

			std::stringstream name;
			name << min + (interval * i) << " to " << min + (interval * (i + 1));

			data.push_back(charts::chart_data_point(name.str(), "FFF569", (float)dist[i]));
		}

		auto chart = charts::column_chart("DPS Distribution", player->name_str, "<b>{point.key}</b> DPS<br/>", "FFF569", "# Iterations", data, mean_bucket);

		return charts::chart(525, 250, chart_to_js(chart.to_json().dump(), idx, "dps_dist"), "dps_dist", idx);
	}

	charts::chart _time_spent_chart(player_t* player, int idx)
	{
		std::vector<stats_t*> filtered_waiting_stats;

		// Filter stats we do not want in the chart ( quiet, background, zero total_time ) and copy them to filtered_waiting_stats
		range::remove_copy_if(player->stats_list, back_inserter(filtered_waiting_stats), charts::util::filter_waiting_stats());

		size_t num_stats = filtered_waiting_stats.size();
		if (num_stats == 0 && player->collected_data.waiting_time.mean() == 0)
			return charts::chart();

		range::sort(filtered_waiting_stats, charts::util::compare_stats_time());

		auto data = std::vector<charts::chart_data_point>();

		for (int i = 0; i < num_stats; i++)
		{
			stats_t* st = filtered_waiting_stats[i];
			data.push_back(charts::chart_data_point(st->name(), charts::colors::school_color(st->school), st->total_time.total_seconds()));
		}

		auto chart = charts::pie_chart("Spent time", player->name_str, "<b>{point.name}</b>: {point.y:.1f}s", data);

	
		return charts::chart(525, 250, chart_to_js(chart.to_json().dump(), idx, "time_spent"), "time_spent", idx);
	}

	void create_player_charts(player_t* player, int idx)
	{
		player->report_information.charts.action_dpet = _action_dpet_chart(player, idx);
		player->report_information.charts.sources = _sources_chart(player, idx);
		player->report_information.charts.timeline_dps = _dps_chart(player, idx);
		player->report_information.charts.dps_distribution = _dps_distribution_chart(player, idx);
		player->report_information.charts.time_spent = _time_spent_chart(player, idx);
	}
}