// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Blizzard Community Platform API
// ==========================================================================

namespace { // UNNAMED NAMESPACE

	struct player_spec_t
	{
		std::string region, server, name, url, local_json, origin, talent_spec;
	};

	// download_id ==============================================================

	js::js_node_t download_id(sim_t* sim,
		const std::string& region,
		unsigned item_id,
		cache::behavior_e caching)
	{
		if (item_id == 0) return js::js_node_t();

		std::string url = "http://" + region + ".battle.net/api/wow/item/" + util::to_string(item_id) + "?locale=en_US";

		std::string result;
		if (!http::get(result, url, caching))
			return js::js_node_t();

		return js::create(sim, result);
	}

	// parse_profession =========================================================

	void parse_profession(std::string& professions_str,
		const js::js_node_t& profile,
		int index)
	{
		std::string key = "professions/primary/" + util::to_string(index);
		if (js::js_node_t profession = js::get_node(profile, key))
		{
			int id;
			std::string rank;
			if (js::get_value(id, profession, "id") && js::get_value(rank, profession, "rank"))
			{
				if (professions_str.length() > 0)
					professions_str += '/';
				professions_str += util::profession_type_string(util::translate_profession_id(id));
				professions_str += '=' + rank;
			}
		}
	}

	// pick_talents =============================================================

	js::js_node_t pick_talents(const js::js_node_t& talents,
		const std::string& specifier)
	{
		js::js_node_t spec1 = js::get_child(talents, "0");
		js::js_node_t spec2 = js::get_child(talents, "1");

		assert(spec1);

		bool spec1_is_active = js::get_child(spec1, "selected") != 0;

		if (util::str_compare_ci(specifier, "active"))
			return spec1_is_active ? spec1 : spec2;

		if (util::str_compare_ci(specifier, "inactive"))
			return spec1_is_active ? spec2 : spec1;

		if (util::str_compare_ci(specifier, "primary"))
			return spec1;

		if (util::str_compare_ci(specifier, "secondary"))
			return spec2;

		std::string spec_name;
		if (!js::get_value(spec_name, spec1, "name"))
			return js::js_node_t();
		if (util::str_prefix_ci(spec_name, specifier))
			return spec1;

		if (spec2 && js::get_value(spec_name, spec2, "name"))
		{
			if (util::str_prefix_ci(spec_name, specifier))
				return spec2;
		}

		return js::js_node_t();
	}

	// parse_talents ============================================================

	bool parse_talents(player_t* p,
		const js::js_node_t& talents)
	{
		std::string buffer;
		if (js::get_value(buffer, talents, "calcSpec"))
		{
			char stag = buffer[0];
			unsigned sid;
			switch (stag)
			{
			case 'a': sid = 0; break;
			case 'Z': sid = 1; break;
			case 'b': sid = 2; break;
			case 'Y': sid = 3; break;
			default:  sid = 99; break;
			}
			p->_spec = p->dbc.spec_by_idx(p->type, sid);
		}

		std::string talent_encoding;
		js::get_value(talent_encoding, talents, "calcTalent");

		for (size_t i = 0; i < talent_encoding.size(); ++i)
		{
			switch (talent_encoding[i])
			{
			case '.':
				talent_encoding[i] = '0';
				break;
			case '0':
			case '1':
			case '2':
				talent_encoding[i] += 1;
				break;
			default:
				p->sim->errorf("BCP API: Invalid character '%c' in talent encoding for player %s.\n", talent_encoding[i], p->name());
				return false;
			}
		}

		if (!p->parse_talents_numbers(talent_encoding))
		{
			p->sim->errorf("BCP API: Can't parse talent encoding '%s' for player %s.\n", talent_encoding.c_str(), p->name());
			return false;
		}

		p->create_talents_armory();

		return true;
	}

	// parse_glyphs =============================================================

	bool parse_glyphs(player_t* p, const js::js_node_t& build)
	{
		static const char* const glyph_e_names[] =
		{
		  "glyphs/major", "glyphs/minor"
		};

		for (std::size_t i = 0; i < sizeof_array(glyph_e_names); ++i)
		{
			if (js::js_node_t glyphs = js::get_node(build, glyph_e_names[i]))
			{
				std::vector<js::js_node_t> children = js::get_children(glyphs);
				if (!children.empty())
				{
					for (std::size_t j = 0; j < children.size(); ++j)
					{
						std::string glyph_name;
						if (!js::get_value(glyph_name, children[j], "name"))
						{
							std::string glyph_id;
							if (js::get_value(glyph_id, children[j], "item"))
								item_t::download_glyph(p, glyph_name, glyph_id);
						}

						util::glyph_name(glyph_name);
						if (!glyph_name.empty())
						{
							if (!p->glyphs_str.empty())
								p->glyphs_str += '/';
							p->glyphs_str += glyph_name;
						}
					}
				}
			}
		}

		return true;
	}

	// parse_items ==============================================================

	bool parse_items(player_t* p,
		const js::js_node_t& items)
	{
		if (!items) return true;

		static const char* const slot_map[] =
		{
		  "head",
		  "neck",
		  "shoulder",
		  "shirt",
		  "chest",
		  "waist",
		  "legs",
		  "feet",
		  "wrist",
		  "hands",
		  "finger1",
		  "finger2",
		  "trinket1",
		  "trinket2",
		  "back",
		  "mainHand",
		  "offHand",
		  "ranged",
		  "tabard"
		};

		assert(sizeof_array(slot_map) == SLOT_MAX);

		for (unsigned i = 0; i < SLOT_MAX; ++i)
		{
			item_t& item = p->items[i];

			js::js_node_t js = js::get_child(items, slot_map[i]);
			if (!js) continue;

			if (!js::get_value(item.parsed.data.id, js, "id"))
				continue;

			js::get_value(item.parsed.gem_id[0], js, "tooltipParams/gem0");
			js::get_value(item.parsed.gem_id[1], js, "tooltipParams/gem1");
			js::get_value(item.parsed.gem_id[2], js, "tooltipParams/gem2");
			js::get_value(item.parsed.enchant_id, js, "tooltipParams/enchant");
			js::get_value(item.parsed.reforge_id, js, "tooltipParams/reforge");
			js::get_value(item.parsed.addon_id, js, "tooltipParams/tinker");
			js::get_value(item.parsed.suffix_id, js, "tooltipParams/suffix");
			js::get_value(item.parsed.upgrade_level, js, "tooltipParams/upgrade/current");

			if (!item_t::download_slot(item))
				return false;
		}

		return true;
	}

	// parse_player =============================================================

	player_t* parse_player(sim_t* sim,
		player_spec_t& player,
		cache::behavior_e  caching)
	{
		sim->current_slot = 0;

		std::string result;

		if (player.local_json.empty())
		{
			if (!http::get(result, player.url, caching))
				return 0;
		}
		else
		{
			io::ifstream ifs;
			ifs.open(player.local_json);
			result.assign((std::istreambuf_iterator<char>(ifs)),
				(std::istreambuf_iterator<char>()));
		}

		// if ( sim -> debug ) util::fprintf( sim -> output_file, "%s\n%s\n", url.c_str(), result.c_str() );
		js::js_node_t profile = js::create(sim, result);
		if (!profile)
		{
			sim->errorf("BCP API: Unable to download player from '%s'\n", player.url.c_str());
			return 0;
		}
		if (sim->debug)
			sim->out_debug.raw() << profile;

		std::string status, reason;
		if (js::get_value(status, profile, "status") && util::str_compare_ci(status, "nok"))
		{
			js::get_value(reason, profile, "reason");

			sim->errorf("BCP API: Unable to download player from '%s', reason: %s\n",
				player.url.c_str(),
				reason.c_str());
			return 0;
		}

		std::string name;
		if (js::get_value(name, profile, "name"))
			player.name = name;
		else
			name = player.name;
		if (player.talent_spec != "active" && !player.talent_spec.empty())
		{
			name += '_';
			name += player.talent_spec;
		}
		if (!name.empty())
			sim->current_name = name;

		int level;
		if (!js::get_value(level, profile, "level"))
		{
			sim->errorf("BCP API: Unable to extract player level from '%s'.\n", player.url.c_str());
			return 0;
		}

		int cid;
		if (!js::get_value(cid, profile, "class"))
		{
			sim->errorf("BCP API: Unable to extract player class from '%s'.\n", player.url.c_str());
			return 0;
		}
		std::string class_name = util::player_type_string(util::translate_class_id(cid));

		int rid;
		if (!js::get_value(rid, profile, "race"))
		{
			sim->errorf("BCP API: Unable to extract player race from '%s'.\n", player.url.c_str());
			return 0;
		}
		race_e race = util::translate_race_id(rid);

		const module_t* module = module_t::get(class_name);

		if (!module || !module->valid())
		{
			sim->errorf("\nModule for class %s is currently not available.\n", class_name.c_str());
			return 0;
		}
		player_t* p = sim->active_player = module->create_player(sim, name, race);

		if (!p)
		{
			sim->errorf("BCP API: Unable to build player with class '%s' and name '%s' from '%s'.\n",
				class_name.c_str(), name.c_str(), player.url.c_str());
			return 0;
		}

		p->level = level;
		p->region_str = player.region.empty() ? sim->default_region_str : player.region;

		if (!js::get_value(p->server_str, profile, "realm") && !player.server.empty())
			p->server_str = player.server;

		if (!player.origin.empty())
			p->origin_str = player.origin;

		if (js::get_value(p->report_information.thumbnail_url, profile, "thumbnail"))
		{
			p->report_information.thumbnail_url = "http://" + p->region_str + ".battle.net/static-render/" +
				p->region_str + '/' + p->report_information.thumbnail_url;
		}

		parse_profession(p->professions_str, profile, 0);
		parse_profession(p->professions_str, profile, 1);

		js::js_node_t build = js::get_node(profile, "talents");
		if (!build)
		{
			sim->errorf("BCP API: Player '%s' from URL '%s' has no talents.\n",
				name.c_str(), player.url.c_str());
			return 0;
		}

		build = pick_talents(build, player.talent_spec);
		if (!build)
		{
			sim->errorf("BCP API: Invalid talent spec '%s' for player '%s'.\n",
				player.talent_spec.c_str(), name.c_str());
			return 0;
		}

		if (!parse_talents(p, build))
			return 0;

		if (!parse_glyphs(p, build))
			return 0;

		if (!parse_items(p, js::get_child(profile, "items")))
			return 0;

		if (!p->server_str.empty())
			p->armory_extensions(p->region_str, p->server_str, player.name, caching);

		return p;
	}

	// download_item_data =======================================================

	js::js_node_t download_item_data(item_t& item, cache::behavior_e caching)
	{
		js::js_node_t js = download_id(item.sim, item.sim->default_region_str, item.parsed.data.id, caching);
		if (!js)
		{
			if (caching != cache::ONLY)
			{
				item.sim->errorf("BCP API: Player '%s' unable to download item id '%u' at slot %s.\n",
					item.player->name(), item.parsed.data.id, item.slot_name());
			}
			return js;
		}
		if (item.sim->debug)
			item.sim->out_debug.raw() << js;

		try
		{
			{
				int id;
				if (!js::get_value(id, js, "id")) throw("id");
				item.parsed.data.id = id;
			}

			if (!js::get_value(item.name_str, js, "name")) throw("name");

			util::tokenize(item.name_str);

			js::get_value(item.icon_str, js, "icon");

			if (!js::get_value(item.parsed.data.level, js, "itemLevel")) throw("level");

			js::get_value(item.parsed.data.req_level, js, "requiredLevel");
			js::get_value(item.parsed.data.req_skill, js, "requiredSkill");
			js::get_value(item.parsed.data.req_skill_level, js, "requiredSkillRank");

			if (!js::get_value(item.parsed.data.quality, js, "quality")) throw("quality");

			if (!js::get_value(item.parsed.data.inventory_type, js, "inventoryType")) throw("inventory type");
			if (!js::get_value(item.parsed.data.item_class, js, "itemClass")) throw("item class");
			if (!js::get_value(item.parsed.data.item_subclass, js, "itemSubClass")) throw("item subclass");
			js::get_value(item.parsed.data.bind_type, js, "itemBind");

			if (js::js_node_t w = js::get_child(js, "weaponInfo"))
			{
				double minDmg, maxDmg;
				double speed, dps;
				if (!js::get_value(dps, w, "dps")) throw("dps");
				if (!js::get_value(speed, w, "weaponSpeed")) throw("weapon speed");
				if (!js::get_value(minDmg, w, "damage/exactMin")) throw("weapon minimum damage");
				if (!js::get_value(maxDmg, w, "damage/exactMax")) throw("weapon maximum damage");
				item.parsed.data.delay = speed * 1000.0;
				// Estimate damage range from the min damage blizzard gives us. Note that this is not exact, as the min dps is actually floored
				item.parsed.data.dmg_range = 2 - 2 * minDmg / (dps * speed);
			}

			if (js::js_node_t classes = js::get_child(js, "allowableClasses"))
			{
				std::vector<js::js_node_t> nodes = js::get_children(classes);
				for (size_t i = 0, n = nodes.size(); i < n; ++i)
				{
					int cid;
					if (js::get_value(cid, nodes[i]))
						item.parsed.data.class_mask |= 1 << (cid - 1);
				}
			}
			else
				item.parsed.data.class_mask = -1;

			if (js::js_node_t races = js::get_child(js, "allowableRaces"))
			{
				std::vector<js::js_node_t> nodes = js::get_children(races);
				for (size_t i = 0, n = nodes.size(); i < n; ++i)
				{
					int rid;
					if (js::get_value(rid, nodes[i]))
						item.parsed.data.race_mask |= 1 << (rid - 1);
				}
			}
			else
				item.parsed.data.race_mask = -1;

			if (js::js_node_t stats = js::get_child(js, "bonusStats"))
			{
				std::vector<js::js_node_t> nodes = js::get_children(stats);
				for (size_t i = 0, n = std::min(nodes.size(), sizeof_array(item.parsed.data.stat_type_e)); i < n; ++i)
				{
					if (!js::get_value(item.parsed.data.stat_type_e[i], nodes[i], "stat")) throw("bonus stat");
					if (!js::get_value(item.parsed.data.stat_val[i], nodes[i], "amount")) throw("bonus stat amount");

					// Soo, weapons need a flag to indicate caster weapon for correct DPS calculation.
					if (item.parsed.data.delay > 0 && (item.parsed.data.stat_type_e[i] == ITEM_MOD_INTELLECT || item.parsed.data.stat_type_e[i] == ITEM_MOD_SPIRIT || item.parsed.data.stat_type_e[i] == ITEM_MOD_SPELL_POWER))
						item.parsed.data.flags_2 |= ITEM_FLAG2_CASTER_WEAPON;
				}
			}

			if (js::js_node_t sockets = js::get_node(js, "socketInfo/sockets"))
			{
				std::vector<js::js_node_t> nodes = js::get_children(sockets);
				for (size_t i = 0, n = std::min(nodes.size(), sizeof_array(item.parsed.data.socket_color)); i < n; ++i)
				{
					std::string color;
					if (js::get_value(color, nodes[i], "type"))
					{
						if (color == "META")
							item.parsed.data.socket_color[i] = SOCKET_COLOR_META;
						else if (color == "RED")
							item.parsed.data.socket_color[i] = SOCKET_COLOR_RED;
						else if (color == "YELLOW")
							item.parsed.data.socket_color[i] = SOCKET_COLOR_YELLOW;
						else if (color == "BLUE")
							item.parsed.data.socket_color[i] = SOCKET_COLOR_BLUE;
						else if (color == "PRISMATIC")
							item.parsed.data.socket_color[i] = SOCKET_COLOR_PRISMATIC;
						else if (color == "COGWHEEL")
							item.parsed.data.socket_color[i] = SOCKET_COLOR_COGWHEEL;
						else if (color == "HYDRAULIC")
							item.parsed.data.socket_color[i] = SOCKET_COLOR_HYDRAULIC;
					}
				}
			}

			js::get_value(item.parsed.data.id_set, js, "itemSet");

			std::string nameDescription;
			if (js::get_value(nameDescription, js, "nameDescription"))
			{
				if (util::str_in_str_ci(nameDescription, "heroic"))
					item.parsed.data.type_flags |= RAID_TYPE_HEROIC;
				else if (util::str_in_str_ci(nameDescription, "raid finder"))
					item.parsed.data.type_flags |= RAID_TYPE_LFR;
				else if (util::str_in_str_ci(nameDescription, "flexible"))
					item.parsed.data.type_flags |= RAID_TYPE_FLEXIBLE;

				if (util::str_in_str_ci(nameDescription, "thunderforged"))
					item.parsed.data.type_flags |= RAID_TYPE_ELITE;

				if (util::str_in_str_ci(nameDescription, "warforged"))
					item.parsed.data.type_flags |= RAID_TYPE_ELITE;

				if (util::str_in_str_ci(nameDescription, "timeless"))
					item.parsed.data.type_flags |= RAID_TYPE_ELITE;
			}

			js::js_node_t item_spell = js::get_node(js, "itemSpells");
			std::vector<js::js_node_t> nodes = js::get_children(item_spell);
			size_t spell_idx = 0;
			for (size_t i = 0; i < nodes.size() && spell_idx < sizeof_array(item.parsed.data.id_spell); i++)
			{
				int spell_id = 0;
				int trigger_type = -1;
				std::string spell_trigger;
				js::get_value(spell_id, nodes[i], "spellId");
				js::get_value(spell_trigger, nodes[i], "trigger");

				if (util::str_compare_ci(spell_trigger, "ON_EQUIP"))
					trigger_type = ITEM_SPELLTRIGGER_ON_EQUIP;
				else if (util::str_compare_ci(spell_trigger, "ON_USE"))
					trigger_type = ITEM_SPELLTRIGGER_ON_USE;
				else if (util::str_compare_ci(spell_trigger, "ON_PROC"))
					trigger_type = ITEM_SPELLTRIGGER_CHANCE_ON_HIT;

				if (trigger_type != -1 && spell_id > 0)
				{
					item.parsed.data.id_spell[spell_idx] = spell_id;
					item.parsed.data.trigger_spell[spell_idx] = trigger_type;
					spell_idx++;
				}
			}
		}
		catch (const char* fieldname)
		{
			std::string error_str;
			js::get_value(error_str, js, "reason");

			if (caching != cache::ONLY)
				item.sim->errorf("BCP API: Player '%s' unable to parse item '%u' %s at slot '%s': %s\n",
					item.player->name(), item.parsed.data.id, fieldname, item.slot_name(), error_str.c_str());
			return js::js_node_t();
		}

		return js;
	}

	// download_roster ==========================================================

	js::js_node_t download_roster(sim_t* sim,
		const std::string& region,
		const std::string& server,
		const std::string& name,
		cache::behavior_e  caching)
	{
		std::string url = "http://" + region + ".battle.net/api/wow/guild/" + server + '/' +
			name + "?fields=members";

		std::string result;
		if (!http::get(result, url, caching, "\"members\""))
		{
			sim->errorf("BCP API: Unable to download guild %s|%s|%s.\n", region.c_str(), server.c_str(), name.c_str());
			return js::js_node_t();
		}
		js::js_node_t js = js::create(sim, result);
		if (!js || !(js = js::get_child(js, "members")))
		{
			sim->errorf("BCP API: Unable to get members of guild %s|%s|%s.\n", region.c_str(), server.c_str(), name.c_str());
			return js::js_node_t();
		}

		return js;
	}

	// parse_gem_stats ==========================================================

	std::vector<stat_pair_t> parse_gem_stats(const std::string& bonus)
	{
		std::vector<stat_pair_t> stats;

		std::istringstream in(bonus);

		int amount;
		std::string stat;

		in >> amount;
		in >> stat;

		stat_e st = util::parse_gem_stat(stat);
		if (st != STAT_NONE)
			stats.push_back(stat_pair_t(st, amount));

		in >> stat;
		if (in)
		{
			if (util::str_compare_ci(stat, "Rating"))
				in >> stat;

			if (in)
			{
				if (util::str_compare_ci(stat, "and"))
				{
					in >> amount;
					in >> stat;

					st = util::parse_stat_type(stat);
					if (st != STAT_NONE)
						stats.push_back(stat_pair_t(st, amount));
				}
			}
		}

		return stats;
	}

	bool parse_gems(item_t& item, const js::js_node_t& js)
	{
		bool match = true;

		item.parsed.gem_stats.clear();

		for (size_t i = 0; i < sizeof_array(item.parsed.gem_id); i++)
		{
			if (item.parsed.gem_id[i] == 0)
			{
				// Check if there's a gem slot, if so, this is ungemmed item.
				if (item.parsed.data.socket_color[i])
					match = false;
				continue;
			}

			if (item.parsed.data.socket_color[i])
			{
				if (!(item_t::parse_gem(item, item.parsed.gem_id[i]) & item.parsed.data.socket_color[i]))
					match = false;
			}
			else
			{
				// Naively accept gems to wrist/hands/waist past the "official" sockets, but only a
				// single extra one. Wrist/hands should be checked against player professions at
				// least ..
				// Also accept it on main/offhands for the new 5.1 legendary questline stuff
				if (item.slot == SLOT_WRISTS || item.slot == SLOT_HANDS ||
					item.slot == SLOT_WAIST || item.slot == SLOT_MAIN_HAND ||
					item.slot == SLOT_OFF_HAND)
				{
					item_t::parse_gem(item, item.parsed.gem_id[i]);
					break;
				}
			}
		}

		// Socket bonus
		if (match)
		{
			std::string socketBonus;
			if (js::get_value(socketBonus, js, "socketInfo/socketBonus"))
			{
				std::string stat;
				util::fuzzy_stats(stat, socketBonus);
				std::vector<stat_pair_t> bonus = item_t::str_to_stat_pair(stat);
				item.parsed.gem_stats.insert(item.parsed.gem_stats.end(), bonus.begin(), bonus.end());
			}
		}

		return true;
	}

} // close anonymous namespace ==============================================

// bcp_api::download_player =================================================

player_t* bcp_api::download_player(sim_t* sim,
	const std::string& region,
	const std::string& server,
	const std::string& name,
	const std::string& talents,
	cache::behavior_e  caching)
{
	sim->current_name = name;

	player_spec_t player;

	std::string battlenet = "http://" + region + ".battle.net/";

	player.url = battlenet + "api/wow/character/" +
		server + '/' + name + "?fields=talents,items,professions&locale=en_US";
	player.origin = battlenet + "wow/en/character/" + server + '/' + name + "/advanced";

	player.region = region;
	player.server = server;
	player.name = name;

	player.talent_spec = talents;

	return parse_player(sim, player, caching);
}

// bcp_api::from_local_json =================================================

player_t* bcp_api::from_local_json(sim_t* sim,
	const std::string& name,
	const std::string& file_path,
	const std::string& talents
)
{
	sim->current_name = name;

	player_spec_t player;

	player.local_json = file_path;
	player.origin = file_path;

	player.name = name;
	player.talent_spec = talents;

	return parse_player(sim, player, cache::ANY);
}

// bcp_api::download_item() =================================================

bool bcp_api::download_item(item_t& item, cache::behavior_e caching)
{
	bool ret = download_item_data(item, caching) != NULL;
	if (ret)
		item.source_str = "Blizzard";
	return ret;
}

// bcp_api::download_slot() =================================================

bool bcp_api::download_slot(item_t& item, cache::behavior_e caching)
{
	js::js_node_t js = download_item_data(item, caching);
	if (!js)
		return false;

	parse_gems(item, js);

	item.source_str = "Blizzard";

	return true;
}

// bcp_api::download_guild ==================================================

bool bcp_api::download_guild(sim_t* sim,
	const std::string& region,
	const std::string& server,
	const std::string& name,
	const std::vector<int>& ranks,
	int player_filter,
	int max_rank,
	cache::behavior_e caching)
{
	std::cout << "foo\n";
	js::js_node_t js = download_roster(sim, region, server, name, caching);
	if (!js) return false;

	std::vector<std::string> names;
	std::vector<js::js_node_t> characters = js::get_children(js);
	std::cout << "foo\n";

	for (std::size_t i = 0, n = characters.size(); i < n; ++i)
	{
		int level;
		if (!js::get_value(level, characters[i], "character/level") || level < 85)
			continue;

		int cid;
		if (!js::get_value(cid, characters[i], "character/class") ||
			(player_filter != PLAYER_NONE && player_filter != util::translate_class_id(cid)))
			continue;

		int rank;
		if (!js::get_value(rank, characters[i], "rank") ||
			((max_rank > 0) && (rank > max_rank)) ||
			(!ranks.empty() && range::find(ranks, rank) == ranks.end()))
			continue;

		std::string cname;
		if (!js::get_value(cname, characters[i], "character/name")) continue;
		names.push_back(cname);
	}

	if (names.empty()) return true;
	std::cout << "foo\n";

	range::sort(names);

	for (std::size_t i = 0, n = names.size(); i < n; ++i)
	{
		const std::string& cname = names[i];
		util::printf("Downloading character: %s\n", cname.c_str());
		player_t* player = download_player(sim, region, server, cname, "active", caching);
		if (!player)
		{
			sim->errorf("BCP API: Failed to download player '%s' trying Wowhead instead\n", cname.c_str());
			//player = wowhead::download_player( sim, region, server, cname, "active", wowhead::LIVE, caching );
			//if ( !player )
			//  sim -> errorf( "Wowhead: Failed to download player '%s'\n", cname.c_str() );
			// Just ignore invalid players
		}
	}
	std::cout << "foo\n";

	return true;
}

// bcp_api::download_glyph ==================================================

bool bcp_api::download_glyph(player_t* player,
	std::string& glyph_name,
	const std::string& glyph_id,
	cache::behavior_e  caching)
{
	const std::string& region =
		(player->region_str.empty() ? player->sim->default_region_str : player->region_str);

	unsigned glyphid = strtoul(glyph_id.c_str(), 0, 10);
	js::js_node_t item = download_id(player->sim, region, glyphid, caching);
	if (!item || !js::get_value(glyph_name, item, "name"))
	{
		if (caching != cache::ONLY)
			player->sim->errorf("BCP API: Unable to download glyph id '%s'\n", glyph_id.c_str());
		return false;
	}

	return true;
}

// bcp_api::parse_gem =======================================================

gem_e bcp_api::parse_gem(item_t& item, unsigned gem_id, cache::behavior_e caching)
{
	const std::string& region =
		item.player->region_str.empty()
		? item.sim->default_region_str
		: item.player->region_str;

	js::js_node_t js = download_id(item.sim, region, gem_id, caching);
	if (!js)
		return GEM_NONE;

	if (item.sim->debug)
		item.sim->out_debug.raw() << js;

	std::string type_str;
	if (!js::get_value(type_str, js, "gemInfo/type/type"))
		return GEM_NONE;
	util::tokenize(type_str);

	gem_e type = util::parse_gem_type(type_str);

	std::string result;
	if (type == GEM_META)
	{
		if (!js::get_value(result, js, "name"))
			return GEM_NONE;

		std::string::size_type pos = result.rfind(" Diamond");
		if (pos != std::string::npos) result.erase(pos);
		// Set meta gem here.
		util::tokenize(result);
		meta_gem_e meta_type = util::parse_meta_gem_type(result);
		if (meta_type != META_GEM_NONE)
			item.player->meta_gem = meta_type;
		result.clear();
	}
	else
	{
		std::string bonus;
		if (!js::get_value(bonus, js, "gemInfo/bonus/name"))
			return GEM_NONE;
		std::vector<stat_pair_t> stats = parse_gem_stats(bonus);
		item.parsed.gem_stats.insert(item.parsed.gem_stats.end(), stats.begin(), stats.end());
	}

	return type;
}

#if USE_WOWREFORGE
player_t* wowreforge::download_player(sim_t* sim,
	const std::string& profile_id,
	const std::string& talents,
	cache::behavior_e  caching)
{
	sim->current_name = profile_id;

	player_spec_t player;

	player.origin = "http://wowreforge.com/Profiles/" + profile_id;
	player.url = player.origin + "?json";
	player.name = "wowreforge_" + profile_id;

	player.talent_spec = talents;

	return parse_player(sim, player, caching);
}

#endif // USE_WOWREFORGE
