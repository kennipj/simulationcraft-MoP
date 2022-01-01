// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#include "simulationcraft.hpp"


/*TODO for 5.4:

 Implement demonic breath talent
 */
namespace { // unnamed namespace

	static const int META_FURY_MINIMUM = 40;

	struct warlock_t;

	namespace pets {
		struct wild_imp_pet_t;
	}

	struct warlock_td_t : public actor_pair_t
	{
		dot_t* dots_corruption;
		dot_t* dots_unstable_affliction;
		dot_t* dots_agony;
		dot_t* dots_doom;
		dot_t* dots_immolate;
		dot_t* dots_drain_soul;
		dot_t* dots_shadowflame;
		dot_t* dots_malefic_grasp;
		dot_t* dots_seed_of_corruption;
		dot_t* dots_soulburn_seed_of_corruption;
		dot_t* dots_haunt;

		buff_t* debuffs_haunt;

		bool ds_started_below_20;
		int shadowflame_stack;
		int agony_stack;
		double soc_trigger, soulburn_soc_trigger;

		warlock_td_t(player_t* target, warlock_t* source);

		void reset()
		{
			ds_started_below_20 = false;
			shadowflame_stack = 1;
			agony_stack = 1;
			soc_trigger = 0;
			soulburn_soc_trigger = 0;
		}
	};

	struct warlock_t : public player_t
	{
	public:

		player_t* havoc_target;
		player_t* latest_corruption_target; //pointer to the latest corruption target

		// Active Pet
		struct pets_t
		{
			pet_t* active;
			pet_t* last;
			static const int WILD_IMP_LIMIT = 25;
			std::array<pets::wild_imp_pet_t*, WILD_IMP_LIMIT> wild_imps;
		} pets;

		// Buffs
		struct buffs_t
		{
			buff_t* backdraft;
			buff_t* dark_soul;
			buff_t* metamorphosis;
			buff_t* molten_core;
			buff_t* soulburn;
			buff_t* havoc;
			buff_t* grimoire_of_sacrifice;
			buff_t* demonic_calling;
			buff_t* fire_and_brimstone;
			buff_t* soul_swap;
			buff_t* demonic_rebirth;
			buff_t* mannoroths_fury;

			buff_t* tier16_4pc_ember_fillup;
			buff_t* tier16_2pc_destructive_influence;
			buff_t* tier16_2pc_empowered_grasp;
			buff_t* tier16_2pc_fiery_wrath;

		} buffs;

		// Cooldowns
		struct cooldowns_t
		{
			cooldown_t* demonic_calling;
			cooldown_t* infernal;
			cooldown_t* doomguard;
			cooldown_t* imp_swarm;
			cooldown_t* hand_of_guldan;
		} cooldowns;

		// Talents

		struct talents_t
		{
			const spell_data_t* soul_leech;
			const spell_data_t* harvest_life;
			const spell_data_t* mortal_coil;
			const spell_data_t* shadowfury;
			const spell_data_t* soul_link;
			const spell_data_t* grimoire_of_supremacy;
			const spell_data_t* grimoire_of_service;
			const spell_data_t* grimoire_of_sacrifice;
			const spell_data_t* archimondes_darkness;
			const spell_data_t* kiljaedens_cunning;
			const spell_data_t* mannoroths_fury;
		} talents;

		// Specialization Spells
		struct specs_t
		{
			// General
			const spell_data_t* dark_soul;
			const spell_data_t* nethermancy;
			const spell_data_t* fel_armor;
			const spell_data_t* pandemic;
			const spell_data_t* imp_swarm;

			// Affliction
			const spell_data_t* nightfall;

			// Demonology
			const spell_data_t* decimation;
			const spell_data_t* demonic_fury;
			const spell_data_t* metamorphosis;
			const spell_data_t* molten_core;
			const spell_data_t* doom;
			const spell_data_t* touch_of_chaos;
			const spell_data_t* chaos_wave;
			const spell_data_t* demonic_rebirth;
			const spell_data_t* wild_imps;

			// Destruction
			const spell_data_t* aftermath;
			const spell_data_t* backdraft;
			const spell_data_t* burning_embers;
			const spell_data_t* chaotic_energy;
			const spell_data_t* fire_and_brimstone;
			const spell_data_t* pyroclasm;

		} spec;

		struct mastery_spells_t
		{
			const spell_data_t* potent_afflictions;
			const spell_data_t* master_demonologist;
			const spell_data_t* emberstorm;
		} mastery_spells;

		// Gains
		struct gains_t
		{
			gain_t* life_tap;
			gain_t* soul_leech;
			gain_t* tier13_4pc;
			gain_t* nightfall;
			gain_t* drain_soul;
			gain_t* incinerate;
			gain_t* incinerate_fnb;
			gain_t* incinerate_t15_4pc;
			gain_t* conflagrate;
			gain_t* conflagrate_fnb;
			gain_t* rain_of_fire;
			gain_t* immolate;
			gain_t* immolate_fnb;
			gain_t* fel_flame;
			gain_t* shadowburn;
			gain_t* miss_refund;
			gain_t* siphon_life;
			gain_t* seed_of_corruption;
			gain_t* haunt_tier16_4pc;
		} gains;

		// Procs
		struct procs_t
		{
			proc_t* wild_imp;
		} procs;

		struct glyphs_t
		{
			const spell_data_t* conflagrate;
			const spell_data_t* curse_of_elements;
			const spell_data_t* demon_training;
			const spell_data_t* havoc;
			const spell_data_t* life_tap;
			const spell_data_t* imp_swarm;
			const spell_data_t* soul_shards;
			const spell_data_t* burning_embers;
			const spell_data_t* shadow_bolt;
			const spell_data_t* siphon_life;
			const spell_data_t* unstable_affliction;
		} glyphs;

		struct spells_t
		{
			spell_t* seed_of_corruption_aoe;
			spell_t* soulburn_seed_of_corruption_aoe;
			spell_t* metamorphosis;
			spell_t* melee;

			const spell_data_t* tier15_2pc;
		} spells;

		struct soul_swap_buffer_t
		{
			player_t* source;//where inhaled from

			//<foo>_was_inhaled stores whether <foo> was ticking when inhaling SS. We can't store it in the dot as the dot does not tick while in the buffer

			int agony_stack; //remove with 5.4

			bool agony_was_inhaled;
			dot_t* agony;
			bool corruption_was_inhaled;
			dot_t* corruption;
			bool unstable_affliction_was_inhaled;
			dot_t* unstable_affliction;

			bool seed_of_corruption_was_inhaled;
			dot_t* seed_of_corruption;

			static void inhale_dot(dot_t* to_inhale, dot_t* soul_swap_buffer_target)//copies over the state
			{
				soul_swap_buffer_target->cancel(); //clear any previous versions in the buffer;

				if (soul_swap_buffer_target->state == NULL)
					soul_swap_buffer_target->state = to_inhale->current_action->get_state(); //make sure we have memory allocated for the state

				soul_swap_buffer_target->state->copy_state(to_inhale->state); //copy state and the other stuff
				soul_swap_buffer_target->current_action = to_inhale->current_action;
				soul_swap_buffer_target->current_tick = to_inhale->current_tick;
				soul_swap_buffer_target->num_ticks = to_inhale->num_ticks;
				soul_swap_buffer_target->tick_amount = to_inhale->tick_amount;

			}
		} soul_swap_buffer;

		struct demonic_calling_event_t : event_t
		{
			bool initiator;

			demonic_calling_event_t(player_t* p, timespan_t delay, bool init = false) :
				event_t(*p, "demonic_calling"), initiator(init)
			{
				add_event(delay);
			}

			virtual void execute()
			{
				warlock_t* p = static_cast<warlock_t*>(actor);
				p->demonic_calling_event = new (sim()) demonic_calling_event_t(p,
					timespan_t::from_seconds((p->spec.wild_imps->effectN(1).period().total_seconds() + p->spec.imp_swarm->effectN(3).base_value()) * p->cache.spell_speed()));
				if (!initiator) p->buffs.demonic_calling->trigger();
			}
		};

		core_event_t* demonic_calling_event;

		int initial_burning_embers, initial_demonic_fury;
		std::string default_pet;

		timespan_t ember_react, shard_react;

		warlock_t(sim_t* sim, const std::string& name, race_e r = RACE_UNDEAD);
		virtual ~warlock_t();

		// Character Definition
		virtual void      init_spells();
		virtual void      init_base_stats();
		virtual void      init_scaling();
		virtual void      create_buffs();
		virtual void      init_gains();
		virtual void      init_benefits();
		virtual void      init_procs();
		virtual void      init_action_list();
		virtual void      init_resources(bool force);
		virtual void      reset();
		virtual void      create_options();
		virtual action_t* create_action(const std::string& name, const std::string& options);
		virtual pet_t* create_pet(const std::string& name, const std::string& type = std::string());
		virtual void      create_pets();
		virtual bool      create_profile(std::string& profile_str, save_e = SAVE_ALL, bool save_html = false);
		virtual void      copy_from(player_t* source);
		virtual set_e       decode_set(const item_t&) const;
		virtual resource_e primary_resource() const { return RESOURCE_MANA; }
		virtual role_e primary_role() const { return ROLE_SPELL; }
		virtual double    matching_gear_multiplier(attribute_e attr) const;
		virtual double composite_player_multiplier(school_e school) const;
		virtual void invalidate_cache(cache_e);
		virtual double composite_spell_crit() const;
		virtual double composite_spell_haste() const;
		virtual double composite_mastery() const;
		virtual double resource_gain(resource_e, double, gain_t* = 0, action_t* = 0);
		virtual double mana_regen_per_second() const;
		virtual double composite_armor() const;

		virtual void halt();
		virtual void combat_begin();
		virtual expr_t* create_expression(action_t* a, const std::string& name_str);

		double emberstorm_e3_from_e1() const
		{
			return mastery_spells.emberstorm->effectN(3).coeff() / mastery_spells.emberstorm->effectN(1).coeff();
		}

		target_specific_t<warlock_td_t*> target_data;

		virtual warlock_td_t* get_target_data(player_t* target) const
		{
			warlock_td_t*& td = target_data[target];
			if (!td)
			{
				td = new warlock_td_t(target, const_cast<warlock_t*>(this));
			}
			return td;
		}
	private:
		void apl_precombat();
		void apl_default();
		void apl_affliction();
		void apl_demonology();
		void apl_destruction();
		void apl_global_filler();
	};

	void parse_spell_coefficient(action_t& a)
	{
		for (size_t i = 1; i <= a.data()._effects->size(); i++)
		{
			if (a.data().effectN(i).type() == E_SCHOOL_DAMAGE)
				a.direct_power_mod = a.data().effectN(i).m_average();
			else if (a.data().effectN(i).type() == E_APPLY_AURA && a.data().effectN(i).subtype() == A_PERIODIC_DAMAGE)
				a.tick_power_mod = a.data().effectN(i).m_average();
		}
	}

	namespace pets {
		// PETS

		struct warlock_pet_t : public pet_t
		{
			gain_t* owner_fury_gain;
			action_t* special_action;
			melee_attack_t* melee_attack;
			stats_t* summon_stats;
			const spell_data_t* supremacy;

			warlock_pet_t(sim_t* sim, warlock_t* owner, const std::string& pet_name, pet_e pt, bool guardian = false);
			virtual void init_base_stats();
			virtual void init_action_list();
			virtual timespan_t available() const;
			virtual void schedule_ready(timespan_t delta_time = timespan_t::zero(),
				bool   waiting = false);
			virtual double composite_player_multiplier(school_e school) const;
			virtual resource_e primary_resource() const { return RESOURCE_ENERGY; }
			warlock_t* o()
			{
				return static_cast<warlock_t*>(owner);
			}
			const warlock_t* o() const
			{
				return static_cast<warlock_t*>(owner);
			}
		};


		namespace actions {

			// Template for common warlock pet action code. See priest_action_t.
			template <class ACTION_BASE>
			struct warlock_pet_action_t : public ACTION_BASE
			{
			public:
			private:
				typedef ACTION_BASE ab; // action base, eg. spell_t
			public:
				typedef warlock_pet_action_t base_t;

				double generate_fury;

				warlock_pet_action_t(const std::string& n, warlock_pet_t* p,
					const spell_data_t* s = spell_data_t::nil()) :
					ab(n, p, s),
					generate_fury(get_fury_gain(ab::data()))
				{
					ab::may_crit = true;
				}
				virtual ~warlock_pet_action_t() {}

				warlock_pet_t* p()
				{
					return static_cast<warlock_pet_t*>(ab::player);
				}
				const warlock_pet_t* p() const
				{
					return static_cast<warlock_pet_t*>(ab::player);
				}

				virtual bool ready()
				{
					if (ab::background == false && ab::current_resource() == RESOURCE_ENERGY && ab::player->resources.current[RESOURCE_ENERGY] < 130)
						return false;

					return ab::ready();
				}

				virtual void execute()
				{
					ab::execute();

					if (ab::result_is_hit(ab::execute_state->result) && p()->o()->specialization() == WARLOCK_DEMONOLOGY && generate_fury > 0)
						p()->o()->resource_gain(RESOURCE_DEMONIC_FURY, generate_fury, p()->owner_fury_gain);
				}

				double get_fury_gain(const spell_data_t& data)
				{
					if (data._effects->size() >= 3 && data.effectN(3).trigger_spell_id() == 104330)
						return data.effectN(3).base_value();
					else
						return 0.0;
				}
			};

			struct warlock_pet_melee_t : public melee_attack_t
			{
				struct off_hand_swing : public melee_attack_t
				{
					off_hand_swing(warlock_pet_t* p, const char* name = "melee_oh") :
						melee_attack_t(name, p, spell_data_t::nil())
					{
						school = SCHOOL_PHYSICAL;
						weapon = &(p->off_hand_weapon);
						base_execute_time = weapon->swing_time;
						may_crit = true;
						background = true;
						base_multiplier = 0.5;
					}
				};

				off_hand_swing* oh;

				warlock_pet_melee_t(warlock_pet_t* p, const char* name = "melee") :
					melee_attack_t(name, p, spell_data_t::nil()), oh(0)
				{
					school = SCHOOL_PHYSICAL;
					weapon = &(p->main_hand_weapon);
					base_execute_time = weapon->swing_time;
					may_crit = true;
					background = true;
					repeating = true;

					if (p->dual_wield())
						oh = new off_hand_swing(p);
				}

				virtual void execute()
				{
					if (!player->executing && !player->channeling)
					{
						melee_attack_t::execute();
						if (oh)
						{
							oh->time_to_execute = time_to_execute;
							oh->execute();
						}
					}
					else
					{
						schedule_execute();
					}
				}
			};


			struct warlock_pet_melee_attack_t : public warlock_pet_action_t<melee_attack_t>
			{
			private:
				void _init_warlock_pet_melee_attack_t()
				{
					weapon = &(player->main_hand_weapon);
					special = true;
				}

			public:
				warlock_pet_melee_attack_t(warlock_pet_t* p, const std::string& n) :
					base_t(n, p, p -> find_pet_spell(n))
				{
					_init_warlock_pet_melee_attack_t();
				}

				warlock_pet_melee_attack_t(const std::string& token, warlock_pet_t* p, const spell_data_t* s = spell_data_t::nil()) :
					base_t(token, p, s)
				{
					_init_warlock_pet_melee_attack_t();
				}
			};


			struct warlock_pet_spell_t : public warlock_pet_action_t<spell_t>
			{
			private:
				void _init_warlock_pet_spell_t()
				{
					parse_spell_coefficient(*this);
				}

			public:
				warlock_pet_spell_t(warlock_pet_t* p, const std::string& n) :
					base_t(n, p, p -> find_pet_spell(n))
				{
					_init_warlock_pet_spell_t();
				}

				warlock_pet_spell_t(const std::string& token, warlock_pet_t* p, const spell_data_t* s = spell_data_t::nil()) :
					base_t(token, p, s)
				{
					_init_warlock_pet_spell_t();
				}
			};


			struct firebolt_t : public warlock_pet_spell_t
			{
				firebolt_t(warlock_pet_t* p) :
					warlock_pet_spell_t(p, "Firebolt")
				{
					if (p->owner->bugs)
						min_gcd = timespan_t::from_seconds(1.5);
				}

				virtual timespan_t execute_time() const
				{
					timespan_t t = warlock_pet_spell_t::execute_time();

					if (p()->o()->glyphs.demon_training->ok())
					{
						t *= 0.5;
					}

					return t;
				}
			};


			struct legion_strike_t : public warlock_pet_melee_attack_t
			{
				legion_strike_t(warlock_pet_t* p) :
					warlock_pet_melee_attack_t(p, "Legion Strike")
				{
					aoe = -1;
					split_aoe_damage = true;
					weapon = &(p->main_hand_weapon);
				}

				virtual bool ready()
				{
					if (p()->special_action->get_dot()->ticking) return false;

					return warlock_pet_melee_attack_t::ready();
				}
			};


			struct felstorm_tick_t : public warlock_pet_melee_attack_t
			{
				felstorm_tick_t(warlock_pet_t* p, const spell_data_t& s) :
					warlock_pet_melee_attack_t("felstorm_tick", p, s.effectN(1).trigger())
				{
					aoe = -1;
					background = true;
					weapon = &(p->main_hand_weapon);
				}
			};


			struct felstorm_t : public warlock_pet_melee_attack_t
			{
				felstorm_t(warlock_pet_t* p) :
					warlock_pet_melee_attack_t("felstorm", p, p -> find_spell(89751))
				{
					tick_zero = true;
					hasted_ticks = false;
					may_miss = false;
					may_crit = false;
					weapon_multiplier = 0;

					dynamic_tick_action = true;
					tick_action = new felstorm_tick_t(p, data());
				}

				virtual void cancel()
				{
					warlock_pet_melee_attack_t::cancel();

					get_dot()->cancel();
				}

				virtual void execute()
				{
					warlock_pet_melee_attack_t::execute();

					p()->melee_attack->cancel();
				}

				virtual void last_tick(dot_t* d)
				{
					warlock_pet_melee_attack_t::last_tick(d);

					if (!p()->is_sleeping() && !p()->melee_attack->target->is_sleeping())
						p()->melee_attack->execute();
				}
			};


			struct shadow_bite_t : public warlock_pet_spell_t
			{
				shadow_bite_t(warlock_pet_t* p) :
					warlock_pet_spell_t(p, "Shadow Bite")
				{ }
			};


			struct lash_of_pain_t : public warlock_pet_spell_t
			{
				lash_of_pain_t(warlock_pet_t* p) :
					warlock_pet_spell_t(p, "Lash of Pain")
				{
					if (p->owner->bugs) min_gcd = timespan_t::from_seconds(1.5);
				}
			};


			struct whiplash_t : public warlock_pet_spell_t
			{
				whiplash_t(warlock_pet_t* p) :
					warlock_pet_spell_t(p, "Whiplash")
				{
					aoe = -1;
				}
			};


			struct torment_t : public warlock_pet_spell_t
			{
				torment_t(warlock_pet_t* p) :
					warlock_pet_spell_t(p, "Torment")
				{ }
			};


			struct felbolt_t : public warlock_pet_spell_t
			{
				felbolt_t(warlock_pet_t* p) :
					warlock_pet_spell_t(p, "Felbolt")
				{
					if (p->owner->bugs)
						min_gcd = timespan_t::from_seconds(1.5);
				}

				virtual timespan_t execute_time() const
				{
					timespan_t t = warlock_pet_spell_t::execute_time();

					if (p()->o()->glyphs.demon_training->ok()) t *= 0.5;

					return t;
				}
			};


			struct mortal_cleave_t : public warlock_pet_melee_attack_t
			{
				mortal_cleave_t(warlock_pet_t* p) :
					warlock_pet_melee_attack_t(p, "Mortal Cleave")
				{
					aoe = -1;
					weapon = &(p->main_hand_weapon);
				}

				virtual bool ready()
				{
					if (p()->special_action->get_dot()->ticking) return false;

					return warlock_pet_melee_attack_t::ready();
				}
			};


			struct wrathstorm_tick_t : public warlock_pet_melee_attack_t
			{
				wrathstorm_tick_t(warlock_pet_t* p, const spell_data_t& s) :
					warlock_pet_melee_attack_t("wrathstorm_tick", p, s.effectN(1).trigger())
				{
					aoe = -1;
					background = true;
					weapon = &(p->main_hand_weapon);
				}
			};


			struct wrathstorm_t : public warlock_pet_melee_attack_t
			{
				wrathstorm_t(warlock_pet_t* p) :
					warlock_pet_melee_attack_t("wrathstorm", p, p -> find_spell(115831))
				{
					tick_zero = true;
					hasted_ticks = false;
					may_miss = false;
					may_crit = false;
					weapon_multiplier = 0;

					dynamic_tick_action = true;
					tick_action = new wrathstorm_tick_t(p, data());
				}

				virtual void cancel()
				{
					warlock_pet_melee_attack_t::cancel();

					get_dot()->cancel();
				}

				virtual void execute()
				{
					warlock_pet_melee_attack_t::execute();

					p()->melee_attack->cancel();
				}

				virtual void last_tick(dot_t* d)
				{
					warlock_pet_melee_attack_t::last_tick(d);

					if (!p()->is_sleeping()) p()->melee_attack->execute();
				}
			};


			struct tongue_lash_t : public warlock_pet_spell_t
			{
				tongue_lash_t(warlock_pet_t* p) :
					warlock_pet_spell_t(p, "Tongue Lash")
				{ }
			};


			struct bladedance_t : public warlock_pet_spell_t
			{
				bladedance_t(warlock_pet_t* p) :
					warlock_pet_spell_t(p, "Bladedance")
				{
					if (p->owner->bugs) min_gcd = timespan_t::from_seconds(1.5);
				}
			};


			struct fellash_t : public warlock_pet_spell_t
			{
				fellash_t(warlock_pet_t* p) :
					warlock_pet_spell_t(p, "Fellash")
				{
					aoe = -1;
				}
			};


			struct immolation_tick_t : public warlock_pet_spell_t
			{
				immolation_tick_t(warlock_pet_t* p, const spell_data_t& s) :
					warlock_pet_spell_t("immolation_tick", p, s.effectN(1).trigger())
				{
					aoe = -1;
					background = true;
					may_crit = false;
				}
			};

			struct immolation_t : public warlock_pet_spell_t
			{
				immolation_t(warlock_pet_t* p, const std::string& options_str) :
					warlock_pet_spell_t("immolation", p, p -> find_spell(19483))
				{
					parse_options(0, options_str);

					num_ticks = 1;
					hasted_ticks = false;

					dynamic_tick_action = true;
					tick_action = new immolation_tick_t(p, data());
				}

				virtual void tick(dot_t* d)
				{
					d->current_tick = 0; // ticks indefinitely

					warlock_pet_spell_t::tick(d);
				}

				virtual void cancel()
				{
					dot_t* dot = find_dot(target);
					if (dot) dot->reset();
					action_t::cancel();
				}
			};


			struct doom_bolt_t : public warlock_pet_spell_t
			{
				doom_bolt_t(warlock_pet_t* p) :
					warlock_pet_spell_t(p, "Doom Bolt")
				{
				}

				virtual timespan_t execute_time() const
				{
					// FIXME: Not actually how it works, but this achieves a consistent 17 casts per summon, which seems to match reality
					return timespan_t::from_seconds(3.4);
				}

				virtual double composite_target_multiplier(player_t* target) const
				{
					double m = warlock_pet_spell_t::composite_target_multiplier(target);

					if (target->health_percentage() < 20)
					{
						m *= 1.0 + data().effectN(2).percent();
					}

					return m;
				}
			};


			struct wild_firebolt_t : public warlock_pet_spell_t
			{
				wild_firebolt_t(warlock_pet_t* p) :
					warlock_pet_spell_t("firebolt", p, p -> find_spell(104318))
				{
					// FIXME: Exact casting mechanics need testing - this is copied from the old doomguard lag
					if (p->owner->bugs)
					{
						ability_lag = timespan_t::from_seconds(0.22);
						ability_lag_stddev = timespan_t::from_seconds(0.01);
					}
				}

				virtual double action_multiplier() const
				{
					double m = warlock_pet_spell_t::action_multiplier();

					m *= 1.0 + p()->o()->talents.grimoire_of_sacrifice->effectN(4).percent() * p()->o()->buffs.grimoire_of_sacrifice->stack();

					return m;
				}

				virtual void impact(action_state_t* s)
				{
					warlock_pet_spell_t::impact(s);

					if (result_is_hit(s->result)
						&& p()->o()->spec.molten_core->ok()
						&& rng().roll(0.08))
						p()->o()->buffs.molten_core->trigger();
				}

				virtual void execute()
				{
					warlock_pet_spell_t::execute();

					if (player->resources.current[RESOURCE_ENERGY] <= 0)
					{
						p()->dismiss();
						return;
					}
				}

				virtual bool ready()
				{
					return spell_t::ready();
				}
			};

		} // pets::actions

		warlock_pet_t::warlock_pet_t(sim_t* sim, warlock_t* owner, const std::string& pet_name, pet_e pt, bool guardian) :
			pet_t(sim, owner, pet_name, pt, guardian), special_action(0), melee_attack(0), summon_stats(0)
		{
			owner_fury_gain = owner->get_gain(pet_name);
			owner_coeff.ap_from_sp = 3.5;
			owner_coeff.sp_from_sp = 1.0;
			supremacy = find_spell(115578);
		}

		void warlock_pet_t::init_base_stats()
		{
			pet_t::init_base_stats();

			resources.base[RESOURCE_ENERGY] = 200;
			base_energy_regen_per_second = 10;

			// We only care about intellect - no other primary attribute affects anything interesting
			base.stats.attribute[ATTR_INTELLECT] = util::ability_rank(owner->level, 74, 90, 72, 89, 71, 88, 70, 87, 69, 85, 0, 1);
			// We don't have the data below level 85, so let's make a rough estimate
			if (base.stats.attribute[ATTR_INTELLECT] == 0)
				base.stats.attribute[ATTR_INTELLECT] = floor(0.81 * owner->level);

			if (owner->race == RACE_ORC)
				initial.spell_power_multiplier *= 1.0 + owner->find_spell(20575)->effectN(1).percent();

			base.spell_power_per_intellect = 1;

			intellect_per_owner = 0;
			stamina_per_owner = 0;

			main_hand_weapon.type = WEAPON_BEAST;

			double dmg = dbc.spell_scaling(owner->type, owner->level);
			if (owner->race == RACE_ORC) dmg *= 1.0 + owner->find_spell(20575)->effectN(1).percent();
			main_hand_weapon.min_dmg = main_hand_weapon.max_dmg = main_hand_weapon.damage = dmg;

			main_hand_weapon.swing_time = timespan_t::from_seconds(2.0);

			if (owner->race == RACE_ORC)
				base.attack_power_multiplier *= 1.0 + owner->find_spell(20575)->effectN(1).percent();
		}

		void warlock_pet_t::init_action_list()
		{
			if (special_action)
			{
				if (type == PLAYER_PET)
					special_action->background = true;
				else
					special_action->action_list = "default";
			}

			pet_t::init_action_list();

			if (summon_stats)
				for (size_t i = 0; i < action_list.size(); ++i)
					summon_stats->add_child(action_list[i]->stats);
		}

		timespan_t warlock_pet_t::available() const
		{
			assert(primary_resource() == RESOURCE_ENERGY);
			double energy = resources.current[RESOURCE_ENERGY];

			if (energy >= 130)
				return timespan_t::from_seconds(0.1);

			return std::max(
				timespan_t::from_seconds((130 - energy) / energy_regen_per_second()),
				timespan_t::from_seconds(0.1)
			);
		}

		void warlock_pet_t::schedule_ready(timespan_t delta_time, bool waiting)
		{
			dot_t* d;
			if (melee_attack && !melee_attack->execute_event && !(special_action && (d = special_action->get_dot()) && d->ticking))
			{
				melee_attack->schedule_execute();
			}

			pet_t::schedule_ready(delta_time, waiting);
		}

		double warlock_pet_t::composite_player_multiplier(school_e school) const
		{
			double m = pet_t::composite_player_multiplier(school);

			if (o()->mastery_spells.master_demonologist->ok())
				m *= 1.0 + owner->cache.mastery_value();

			if (o()->talents.grimoire_of_supremacy->ok() && pet_type != PET_WILD_IMP)
				m *= 1.0 + supremacy->effectN(1).percent(); // The relevant effect is not attatched to the talent spell, weirdly enough

			if (o()->buffs.tier16_2pc_fiery_wrath->up())
				m *= 1.0 + o()->buffs.tier16_2pc_fiery_wrath->value();
			return m;
		}


		struct imp_pet_t : public warlock_pet_t
		{
			imp_pet_t(sim_t* sim, warlock_t* owner, const std::string& name = "imp") :
				warlock_pet_t(sim, owner, name, PET_IMP, name != "imp")
			{
				action_list_str = "firebolt";
			}

			virtual action_t* create_action(const std::string& name, const std::string& options_str)
			{
				if (name == "firebolt") return new actions::firebolt_t(this);

				return warlock_pet_t::create_action(name, options_str);
			}
		};


		struct felguard_pet_t : public warlock_pet_t
		{
			felguard_pet_t(sim_t* sim, warlock_t* owner, const std::string& name = "felguard") :
				warlock_pet_t(sim, owner, name, PET_FELGUARD, name != "felguard")
			{
				action_list_str = "legion_strike";
			}

			virtual void init_base_stats()
			{
				warlock_pet_t::init_base_stats();

				melee_attack = new actions::warlock_pet_melee_t(this);
				special_action = new actions::felstorm_t(this);
			}

			virtual action_t* create_action(const std::string& name, const std::string& options_str)
			{
				if (name == "legion_strike") return new actions::legion_strike_t(this);

				return warlock_pet_t::create_action(name, options_str);
			}
		};


		struct felhunter_pet_t : public warlock_pet_t
		{
			felhunter_pet_t(sim_t* sim, warlock_t* owner, const std::string& name = "felhunter") :
				warlock_pet_t(sim, owner, name, PET_FELHUNTER, name != "felhunter")
			{
				action_list_str = "shadow_bite";
			}

			virtual void init_base_stats()
			{
				warlock_pet_t::init_base_stats();

				melee_attack = new actions::warlock_pet_melee_t(this);
			}

			virtual action_t* create_action(const std::string& name, const std::string& options_str)
			{
				if (name == "shadow_bite") return new actions::shadow_bite_t(this);

				return warlock_pet_t::create_action(name, options_str);
			}
		};


		struct succubus_pet_t : public warlock_pet_t
		{
			succubus_pet_t(sim_t* sim, warlock_t* owner, const std::string& name = "succubus") :
				warlock_pet_t(sim, owner, name, PET_SUCCUBUS, name != "succubus")
			{
				action_list_str = "lash_of_pain";
				owner_coeff.ap_from_sp = 1.667;
			}

			virtual void init_base_stats()
			{
				warlock_pet_t::init_base_stats();

				main_hand_weapon.swing_time = timespan_t::from_seconds(3.0);
				melee_attack = new actions::warlock_pet_melee_t(this);
				if (!util::str_compare_ci(name_str, "service_succubus"))
					special_action = new actions::whiplash_t(this);
			}

			virtual action_t* create_action(const std::string& name, const std::string& options_str)
			{
				if (name == "lash_of_pain") return new actions::lash_of_pain_t(this);

				return warlock_pet_t::create_action(name, options_str);
			}
		};


		struct voidwalker_pet_t : public warlock_pet_t
		{
			voidwalker_pet_t(sim_t* sim, warlock_t* owner, const std::string& name = "voidwalker") :
				warlock_pet_t(sim, owner, name, PET_VOIDWALKER, name != "voidwalker")
			{
				action_list_str = "torment";
			}

			virtual void init_base_stats()
			{
				warlock_pet_t::init_base_stats();

				melee_attack = new actions::warlock_pet_melee_t(this);
			}

			virtual action_t* create_action(const std::string& name, const std::string& options_str)
			{
				if (name == "torment") return new actions::torment_t(this);

				return warlock_pet_t::create_action(name, options_str);
			}
		};


		struct infernal_pet_t : public warlock_pet_t
		{
			infernal_pet_t(sim_t* sim, warlock_t* owner) :
				warlock_pet_t(sim, owner, "infernal", PET_INFERNAL, true)
			{
				action_list_str = "immolation,if=!ticking";
				owner_coeff.ap_from_sp = 0.5;
			}

			virtual void init_base_stats()
			{
				warlock_pet_t::init_base_stats();

				melee_attack = new actions::warlock_pet_melee_t(this);
			}

			virtual action_t* create_action(const std::string& name, const std::string& options_str)
			{
				if (name == "immolation") return new actions::immolation_t(this, options_str);

				return warlock_pet_t::create_action(name, options_str);
			}
		};


		struct doomguard_pet_t : public warlock_pet_t
		{
			doomguard_pet_t(sim_t* sim, warlock_t* owner) :
				warlock_pet_t(sim, owner, "doomguard", PET_DOOMGUARD, true)
			{ }

			virtual void init_base_stats()
			{
				warlock_pet_t::init_base_stats();

				action_list_str = "doom_bolt";
			}

			virtual action_t* create_action(const std::string& name, const std::string& options_str)
			{
				if (name == "doom_bolt") return new actions::doom_bolt_t(this);

				return warlock_pet_t::create_action(name, options_str);
			}
		};


		struct wild_imp_pet_t : public warlock_pet_t
		{
			stats_t** firebolt_stats;
			stats_t* regular_stats;
			stats_t* swarm_stats;

			wild_imp_pet_t(sim_t* sim, warlock_t* owner) :
				warlock_pet_t(sim, owner, "wild_imp", PET_WILD_IMP, true), firebolt_stats(0)
			{
				if (owner->pets.wild_imps[0])
					regular_stats = owner->pets.wild_imps[0]->get_stats("firebolt");
				else
					regular_stats = get_stats("firebolt");

				swarm_stats = owner->get_stats("firebolt");
			}

			virtual void init_base_stats()
			{
				warlock_pet_t::init_base_stats();

				action_list_str = "firebolt";

				resources.base[RESOURCE_ENERGY] = 10;
				base_energy_regen_per_second = 0;
			}

			virtual timespan_t available() const
			{
				return timespan_t::from_seconds(0.1);
			}

			virtual action_t* create_action(const std::string& name,
				const std::string& options_str)
			{
				if (name == "firebolt")
				{
					action_t* a = new actions::wild_firebolt_t(this);
					firebolt_stats = &(a->stats);
					return a;
				}

				return warlock_pet_t::create_action(name, options_str);
			}

			void trigger(bool swarm = false)
			{
				if (swarm)
					*firebolt_stats = swarm_stats;
				else
					*firebolt_stats = regular_stats;

				summon();
			}

			virtual void pre_analyze_hook()
			{
				warlock_pet_t::pre_analyze_hook();

				*firebolt_stats = regular_stats;

				if (this == o()->pets.wild_imps[0])
				{
					(*firebolt_stats)->merge(*swarm_stats);
					swarm_stats->quiet = 1;
				}
			}
		};


		struct fel_imp_pet_t : public warlock_pet_t
		{
			fel_imp_pet_t(sim_t* sim, warlock_t* owner) :
				warlock_pet_t(sim, owner, "fel_imp", PET_IMP)
			{
				action_list_str = "felbolt";
			}

			virtual action_t* create_action(const std::string& name, const std::string& options_str)
			{
				if (name == "felbolt") return new actions::felbolt_t(this);

				return warlock_pet_t::create_action(name, options_str);
			}
		};


		struct wrathguard_pet_t : public warlock_pet_t
		{
			wrathguard_pet_t(sim_t* sim, warlock_t* owner) :
				warlock_pet_t(sim, owner, "wrathguard", PET_FELGUARD)
			{
				action_list_str = "mortal_cleave";
				owner_coeff.ap_from_sp = 2.333; // FIXME: Retest this later
			}

			virtual void init_base_stats()
			{
				warlock_pet_t::init_base_stats();

				main_hand_weapon.min_dmg = main_hand_weapon.max_dmg = main_hand_weapon.damage = main_hand_weapon.damage * 0.667; // FIXME: Retest this later
				off_hand_weapon = main_hand_weapon;

				melee_attack = new actions::warlock_pet_melee_t(this);
				special_action = new actions::wrathstorm_t(this);
			}

			virtual action_t* create_action(const std::string& name, const std::string& options_str)
			{
				if (name == "mortal_cleave") return new actions::mortal_cleave_t(this);

				return warlock_pet_t::create_action(name, options_str);
			}
		};


		struct observer_pet_t : public warlock_pet_t
		{
			observer_pet_t(sim_t* sim, warlock_t* owner) :
				warlock_pet_t(sim, owner, "observer", PET_FELHUNTER)
			{
				action_list_str = "tongue_lash";
			}

			virtual void init_base_stats()
			{
				warlock_pet_t::init_base_stats();

				melee_attack = new actions::warlock_pet_melee_t(this);
			}

			virtual action_t* create_action(const std::string& name, const std::string& options_str)
			{
				if (name == "tongue_lash") return new actions::tongue_lash_t(this);

				return warlock_pet_t::create_action(name, options_str);
			}
		};


		struct shivarra_pet_t : public warlock_pet_t
		{
			shivarra_pet_t(sim_t* sim, warlock_t* owner) :
				warlock_pet_t(sim, owner, "shivarra", PET_SUCCUBUS)
			{
				action_list_str = "bladedance";
				owner_coeff.ap_from_sp = 1.111;
			}

			virtual void init_base_stats()
			{
				warlock_pet_t::init_base_stats();

				main_hand_weapon.swing_time = timespan_t::from_seconds(3.0);
				main_hand_weapon.min_dmg = main_hand_weapon.max_dmg = main_hand_weapon.damage = main_hand_weapon.damage * 0.667;
				off_hand_weapon = main_hand_weapon;

				melee_attack = new actions::warlock_pet_melee_t(this);
				special_action = new actions::fellash_t(this);
			}

			virtual action_t* create_action(const std::string& name, const std::string& options_str)
			{
				if (name == "bladedance") return new actions::bladedance_t(this);

				return warlock_pet_t::create_action(name, options_str);
			}
		};


		struct voidlord_pet_t : public warlock_pet_t
		{
			voidlord_pet_t(sim_t* sim, warlock_t* owner) :
				warlock_pet_t(sim, owner, "voidlord", PET_VOIDWALKER)
			{
				action_list_str = "torment";
			}

			virtual void init_base_stats()
			{
				warlock_pet_t::init_base_stats();

				melee_attack = new actions::warlock_pet_melee_t(this);
			}

			virtual action_t* create_action(const std::string& name, const std::string& options_str)
			{
				if (name == "torment") return new actions::torment_t(this);

				return warlock_pet_t::create_action(name, options_str);
			}
		};


		struct abyssal_pet_t : public warlock_pet_t
		{
			abyssal_pet_t(sim_t* sim, warlock_t* owner) :
				warlock_pet_t(sim, owner, "abyssal", PET_INFERNAL, true)
			{
				action_list_str = "immolation,if=!ticking";
				owner_coeff.ap_from_sp = 0.5;
			}

			virtual void init_base_stats()
			{
				warlock_pet_t::init_base_stats();

				melee_attack = new actions::warlock_pet_melee_t(this);
			}

			virtual action_t* create_action(const std::string& name, const std::string& options_str)
			{
				if (name == "immolation") return new actions::immolation_t(this, options_str);

				return warlock_pet_t::create_action(name, options_str);
			}
		};


		struct terrorguard_pet_t : public warlock_pet_t
		{
			terrorguard_pet_t(sim_t* sim, warlock_t* owner) :
				warlock_pet_t(sim, owner, "terrorguard", PET_DOOMGUARD, true)
			{
				action_list_str = "doom_bolt";
			}

			virtual action_t* create_action(const std::string& name, const std::string& options_str)
			{
				if (name == "doom_bolt") return new actions::doom_bolt_t(this);

				return warlock_pet_t::create_action(name, options_str);
			}
		};

	} // end namespace pets

	// SPELLS

	namespace actions {

		struct warlock_heal_t : public heal_t
		{
			warlock_heal_t(const std::string& n, warlock_t* p, const uint32_t id) :
				heal_t(n, p, p -> find_spell(id))
			{
				target = p;
			}

			warlock_t* p()
			{
				return static_cast<warlock_t*>(player);
			}
		};


		struct warlock_spell_t : public spell_t
		{
		private:
			void _init_warlock_spell_t()
			{
				may_crit = true;
				tick_may_crit = true;
				dot_behavior = DOT_REFRESH;
				weapon_multiplier = 0.0;
				gain = player->get_gain(name_str);
				generate_fury = 0;
				cost_event = 0;
				havoc_consume = backdraft_consume = 0;
				fury_in_meta = data().powerN(POWER_DEMONIC_FURY).aura_id() == 54879;
				ds_tick_stats = player->get_stats(name_str + "_ds", this);
				ds_tick_stats->school = school;
				mg_tick_stats = player->get_stats(name_str + "_mg", this);
				mg_tick_stats->school = school;

				parse_spell_coefficient(*this);
			}

		public:
			double generate_fury;
			gain_t* gain;
			bool fury_in_meta;
			stats_t* ds_tick_stats;
			stats_t* mg_tick_stats;
			mutable std::vector< player_t* > havoc_targets;

			int havoc_consume, backdraft_consume;

			struct cost_event_t : event_t
			{
				warlock_spell_t* spell;
				resource_e resource;

				cost_event_t(player_t* p, warlock_spell_t* s, resource_e r = RESOURCE_NONE) :
					event_t(*p, "cost_event"), spell(s), resource(r)
				{
					if (resource == RESOURCE_NONE) resource = spell->current_resource();
					add_event(timespan_t::from_seconds(1));
				}

				virtual void execute()
				{
					spell->cost_event = new (sim()) cost_event_t(p(), spell, resource);
					p()->resource_loss(resource, spell->costs_per_second[resource], spell->gain);
				}
			};

			core_event_t* cost_event;

			warlock_spell_t(warlock_t* p, const std::string& n) :
				spell_t(n, p, p -> find_class_spell(n))
			{
				_init_warlock_spell_t();
			}

			warlock_spell_t(const std::string& token, warlock_t* p, const spell_data_t* s = spell_data_t::nil()) :
				spell_t(token, p, s)
			{
				_init_warlock_spell_t();
			}

			warlock_t* p()
			{
				return static_cast<warlock_t*>(player);
			}
			const warlock_t* p() const
			{
				return static_cast<warlock_t*>(player);
			}

			warlock_td_t* td(player_t* t) const
			{
				return p()->get_target_data(t);
			}

			bool use_havoc() const
			{
				if (!p()->havoc_target || target == p()->havoc_target || !havoc_consume)
					return false;

				if (p()->buffs.havoc->check() < havoc_consume)
					return false;

				return true;
			}

			bool use_backdraft() const
			{
				if (!backdraft_consume)
					return false;

				if (p()->buffs.backdraft->check() < backdraft_consume)
					return false;

				return true;
			}

			virtual void init()
			{
				spell_t::init();

				if (harmful && !tick_action) trigger_gcd += p()->spec.chaotic_energy->effectN(3).time_value();
			}

			virtual void reset()
			{
				spell_t::reset();

				core_event_t::cancel(cost_event);
			}

			virtual int n_targets() const
			{
				if (aoe == 0 && use_havoc())
					return 2;

				return spell_t::n_targets();
			}

			virtual std::vector< player_t* >& target_list() const
			{
				if (use_havoc())
				{
					if (!target_cache.is_valid)
						available_targets(target_cache.list);

					havoc_targets.clear();
					if (std::find(target_cache.list.begin(), target_cache.list.end(), target) != target_cache.list.end())
						havoc_targets.push_back(target);

					if (!p()->havoc_target->is_sleeping() &&
						std::find(target_cache.list.begin(), target_cache.list.end(), p()->havoc_target) != target_cache.list.end())
						havoc_targets.push_back(p()->havoc_target);
					return havoc_targets;
				}
				else
					return spell_t::target_list();
			}

			virtual double cost() const
			{
				double c = spell_t::cost();

				if (current_resource() == RESOURCE_DEMONIC_FURY && p()->buffs.dark_soul->check())
					c *= 1.0 + p()->sets.set(SET_T15_2PC_CASTER)->effectN(3).percent();

				if (use_backdraft() && current_resource() == RESOURCE_MANA)
					c *= 1.0 + p()->buffs.backdraft->data().effectN(1).percent();

				return c;
			}

			virtual void execute()
			{
				spell_t::execute();

				if (result_is_hit(execute_state->result) && p()->specialization() == WARLOCK_DEMONOLOGY
					&& generate_fury > 0 && !p()->buffs.metamorphosis->check())
					p()->resource_gain(RESOURCE_DEMONIC_FURY, generate_fury, gain);
			}

			virtual timespan_t execute_time() const
			{
				timespan_t h = spell_t::execute_time();

				if (use_backdraft())
					h *= 1.0 + p()->buffs.backdraft->data().effectN(1).percent();

				return h;
			}

			void consume_resource()
			{
				spell_t::consume_resource();

				if (use_havoc())
				{
					p()->buffs.havoc->decrement(havoc_consume);
					if (p()->buffs.havoc->check() == 0)
						p()->havoc_target = 0;
				}

				if (p()->resources.current[RESOURCE_BURNING_EMBER] < 1.0)
					p()->buffs.fire_and_brimstone->expire();

				if (use_backdraft())
					p()->buffs.backdraft->decrement(backdraft_consume);
			}

			virtual bool ready()
			{
				if (p()->buffs.metamorphosis->check() && p()->resources.current[RESOURCE_DEMONIC_FURY] < META_FURY_MINIMUM)
					p()->spells.metamorphosis->cancel();

				return spell_t::ready();
			}

			virtual void tick(dot_t* d)
			{
				spell_t::tick(d);

				if (p()->specialization() == WARLOCK_DEMONOLOGY && generate_fury > 0)
					p()->resource_gain(RESOURCE_DEMONIC_FURY, generate_fury, gain);

				trigger_seed_of_corruption(td(d->state->target), p(), d->state->result_amount);
			}

			virtual void impact(action_state_t* s)
			{
				spell_t::impact(s);

				trigger_seed_of_corruption(td(s->target), p(), s->result_amount);
			}

			virtual double composite_target_multiplier(player_t* t) const
			{
				double m = 1.0;

				warlock_td_t* td = this->td(t);
				if (td->debuffs_haunt->check() && (channeled || tick_power_mod)) // Only applies to channeled or dots
				{
					m *= 1.0 + td->debuffs_haunt->data().effectN(3).percent();
				}

				return spell_t::composite_target_multiplier(t) * m;
			}

			virtual resource_e current_resource() const
			{
				if (fury_in_meta && p()->buffs.metamorphosis->data().ok())
				{
					if (p()->buffs.metamorphosis->check())
						return RESOURCE_DEMONIC_FURY;
					else
						return RESOURCE_MANA;
				}
				else
					return spell_t::current_resource();
			}

			void trigger_seed_of_corruption(warlock_td_t* td, warlock_t* p, double amount)
			{
				if (((td->dots_seed_of_corruption->current_action && id == td->dots_seed_of_corruption->current_action->id)
					|| td->dots_seed_of_corruption->ticking) && td->soc_trigger > 0)
				{
					td->soc_trigger -= amount;
					if (td->soc_trigger <= 0)
					{
						p->spells.seed_of_corruption_aoe->execute();
						td->dots_seed_of_corruption->cancel();
					}
				}
				if (((td->dots_soulburn_seed_of_corruption->current_action && id == td->dots_soulburn_seed_of_corruption->current_action->id)
					|| td->dots_soulburn_seed_of_corruption->ticking) && td->soulburn_soc_trigger > 0)
				{
					td->soulburn_soc_trigger -= amount;
					if (td->soulburn_soc_trigger <= 0)
					{
						p->spells.soulburn_seed_of_corruption_aoe->execute();
						td->dots_soulburn_seed_of_corruption->cancel();
					}
				}
			}

			void consume_tick_resource(dot_t* d)
			{
				resource_e r = current_resource();
				resource_consumed = costs_per_second[r] * base_tick_time.total_seconds();

				player->resource_loss(r, resource_consumed, 0, this);

				if (sim->log)
					sim->out_log.printf("%s consumes %.1f %s for %s tick (%.0f)", player->name(),
						resource_consumed, util::resource_type_string(r),
						name(), player->resources.current[r]);

				stats->consume_resource(r, resource_consumed);

				if (player->resources.current[r] < resource_consumed)
				{
					if (r == RESOURCE_DEMONIC_FURY && p()->buffs.metamorphosis->check())
						p()->spells.metamorphosis->cancel();
					else
						d->current_action->cancel();
				}
			}

			void trigger_extra_tick(dot_t* dot, double multiplier, bool tick_from_mg) //if tick_from_mg == true, then MG created the tick, otherwise DS created the tick
			{
				if (!dot->ticking) return;

				assert(multiplier != 0.0);

				action_state_t* tmp_state = dot->state;
				dot->state = get_state(tmp_state);
				dot->state->ta_multiplier *= multiplier;
				snapshot_internal(dot->state, update_flags | STATE_CRIT, tmp_state->result_type);

				dot->current_action->periodic_hit = true;
				stats_t* tmp = dot->current_action->stats;
				warlock_spell_t* spell = debug_cast<warlock_spell_t*>(dot->current_action);
				if (tick_from_mg)
					dot->current_action->stats = spell->mg_tick_stats;
				else
					dot->current_action->stats = spell->ds_tick_stats;
				dot->current_action->tick(dot);
				dot->current_action->stats->add_execute(timespan_t::zero(), dot->state->target);
				dot->current_action->stats = tmp;
				dot->current_action->periodic_hit = false;

				action_state_t::release(dot->state);
				dot->state = tmp_state;
			}

			void extend_dot(dot_t* dot, int ticks)
			{
				if (dot->ticking)
				{
					//FIXME: This is roughly how it works, but we need more testing
					int max_ticks = (int)util::ceil(dot->current_action->hasted_num_ticks(p()->cache.spell_speed()) * 1.5);
					int extend_ticks = std::min(ticks, max_ticks - dot->ticks());
					if (extend_ticks > 0) dot->extend_duration(extend_ticks);
				}
			}

			static void trigger_ember_gain(warlock_t* p, double amount, gain_t* gain, double chance = 1.0)
			{
				if (!p->rng().roll(chance)) return;


				if (p->sets.has_set_bonus(SET_T16_4PC_CASTER) && //check whether we fill one up.
					((p->resources.current[RESOURCE_BURNING_EMBER] < 1.0 && p->resources.current[RESOURCE_BURNING_EMBER] + amount >= 1.0) ||
						(p->resources.current[RESOURCE_BURNING_EMBER] < 2.0 && p->resources.current[RESOURCE_BURNING_EMBER] + amount >= 2.0) ||
						(p->resources.current[RESOURCE_BURNING_EMBER] < 3.0 && p->resources.current[RESOURCE_BURNING_EMBER] + amount >= 3.0) ||
						(p->resources.current[RESOURCE_BURNING_EMBER] < 4.0 && p->resources.current[RESOURCE_BURNING_EMBER] + amount >= 4.0)))
				{
					p->buffs.tier16_4pc_ember_fillup->trigger();
				}

				p->resource_gain(RESOURCE_BURNING_EMBER, amount, gain);

				// If getting to 1 full ember was a surprise, the player would have to react to it
				if (p->resources.current[RESOURCE_BURNING_EMBER] == 1.0 && (amount > 0.1 || chance < 1.0))
					p->ember_react = p->sim->current_time + p->total_reaction_time();
				else if (p->resources.current[RESOURCE_BURNING_EMBER] >= 1.0)
					p->ember_react = p->sim->current_time;
				else
					p->ember_react = timespan_t::max();
			}

			static void refund_embers(warlock_t* p)
			{
				double refund = ceil((p->resources.current[RESOURCE_BURNING_EMBER] + 1.0) / 4.0);

				if (refund > 0) p->resource_gain(RESOURCE_BURNING_EMBER, refund, p->gains.miss_refund);
			}

			static void trigger_soul_leech(warlock_t* p, double amount)
			{
				if (p->talents.soul_leech->ok())
				{
					p->resource_gain(RESOURCE_HEALTH, amount, p->gains.soul_leech);
				}
			}

			static void trigger_wild_imp(warlock_t* p)
			{
				for (size_t i = 0; i < p->pets.wild_imps.size(); i++)
				{
					if (p->pets.wild_imps[i]->is_sleeping())
					{
						p->pets.wild_imps[i]->trigger();
						p->procs.wild_imp->occur();
						return;
					}
				}
				p->sim->errorf("Player %s ran out of wild imps.\n", p->name());
				assert(false); // Will only get here if there are no available imps
			}
		};


		struct curse_of_the_elements_t : public warlock_spell_t
		{
			curse_of_the_elements_t* fnb;

			curse_of_the_elements_t(warlock_t* p) :
				warlock_spell_t(p, "Curse of the Elements"),
				fnb(new curse_of_the_elements_t("curse_of_elements", p, p -> find_spell(104225)))
			{
				havoc_consume = 1;
				background = (sim->overrides.magic_vulnerability != 0);
				num_ticks = 0;
				aoe = p->glyphs.curse_of_elements->ok() ? 3 : 0;
				may_crit = false;
			}

			curse_of_the_elements_t(const std::string& n, warlock_t* p, const spell_data_t* spell) :
				warlock_spell_t(n, p, spell),
				fnb(0)
			{
				aoe = -1;
				stats = p->get_stats("curse_of_elements_fnb");
			}

			double cost() const
			{
				if (fnb && p()->buffs.fire_and_brimstone->check())
					return fnb->cost();
				return warlock_spell_t::cost();
			}

			void schedule_execute(action_state_t* state)
			{
				if (fnb && p()->buffs.fire_and_brimstone->check())
					fnb->schedule_execute(state);
				else
					warlock_spell_t::schedule_execute();
			}

			virtual void impact(action_state_t* state)
			{
				warlock_spell_t::impact(state);

				if (result_is_hit(state->result))
				{
					if (!sim->overrides.magic_vulnerability)
						state->target->debuffs.magic_vulnerability->trigger(1, buff_t::DEFAULT_VALUE(), -1, data().duration());
				}
			}
		};


		struct agony_t : public warlock_spell_t
		{
			agony_t(warlock_t* p) :
				warlock_spell_t(p, "Agony")
			{
				may_crit = false;
				if (p->spec.pandemic->ok()) dot_behavior = DOT_EXTEND;
				base_multiplier *= 1.0 + p->sets.set(SET_T13_4PC_CASTER)->effectN(1).percent();
			}

			virtual void last_tick(dot_t* d)
			{
				td(d->state->target)->agony_stack = 1;
				warlock_spell_t::last_tick(d);
			}

			virtual void tick(dot_t* d)
			{
				if (td(d->state->target)->agony_stack < 10) td(d->state->target)->agony_stack++;
				warlock_spell_t::tick(d);
			}

			double composite_target_multiplier(player_t* target) const
			{
				double m = warlock_spell_t::composite_target_multiplier(target);

				m *= td(target)->agony_stack;

				return m;
			}

			virtual double action_multiplier() const
			{
				double m = warlock_spell_t::action_multiplier();

				if (p()->mastery_spells.potent_afflictions->ok())
					m *= 1.0 + p()->cache.mastery_value();

				return m;
			}
		};


		struct doom_t : public warlock_spell_t
		{
			doom_t(warlock_t* p) :
				warlock_spell_t("doom", p, p -> spec.doom)
			{
				may_crit = false;
				if (p->spec.pandemic->ok()) dot_behavior = DOT_EXTEND;
			}

			virtual void tick(dot_t* d)
			{
				warlock_spell_t::tick(d);

				if (d->state->result == RESULT_CRIT) trigger_wild_imp(p());

				if (p()->glyphs.siphon_life->ok())
					p()->resource_gain(RESOURCE_HEALTH, d->state->result_amount * p()->glyphs.siphon_life->effectN(1).percent(), p()->gains.siphon_life);
			}

			virtual bool ready()
			{
				bool r = warlock_spell_t::ready();

				if (!p()->buffs.metamorphosis->check()) r = false;

				return r;
			}
		};


		struct havoc_t : public warlock_spell_t
		{
			havoc_t(warlock_t* p) : warlock_spell_t(p, "Havoc")
			{
				may_crit = false;
				cooldown->duration = data().cooldown() + p->glyphs.havoc->effectN(2).time_value();
			}

			virtual void execute()
			{
				warlock_spell_t::execute();

				p()->buffs.havoc->trigger(p()->buffs.havoc->max_stack());
				p()->havoc_target = target;
			}
		};

		struct shadowflame_t : public warlock_spell_t
		{
			shadowflame_t(warlock_t* p) :
				warlock_spell_t("shadowflame", p, p -> find_spell(47960))
			{
				background = true;
				may_miss = false;
				generate_fury = 2;
			}

			virtual void tick(dot_t* d)
			{
				warlock_spell_t::tick(d);

				if (p()->spec.molten_core->ok() && rng().roll(0.08))
					p()->buffs.molten_core->trigger();
			}

			double composite_target_multiplier(player_t* target) const
			{
				double m = warlock_spell_t::composite_target_multiplier(target);

				m *= td(target)->shadowflame_stack;

				return m;
			}

			virtual void impact(action_state_t* s)
			{
				if (result_is_hit(s->result))
				{
					if (td(s->target)->dots_shadowflame->ticking)
						td(s->target)->shadowflame_stack++;
					else
						td(s->target)->shadowflame_stack = 1;
				}

				warlock_spell_t::impact(s);
			}
		};


		struct hand_of_guldan_t : public warlock_spell_t
		{
			hand_of_guldan_t(warlock_t* p) :
				warlock_spell_t(p, "Hand of Gul'dan")
			{
				aoe = -1;

				cooldown->duration = timespan_t::from_seconds(15);
				cooldown->charges = 2;

				impact_action = new shadowflame_t(p);

				parse_effect_data(p->find_spell(86040)->effectN(1));

				add_child(impact_action);
			}

			virtual double action_da_multiplier() const
			{
				double m = warlock_spell_t::action_da_multiplier();

				m *= 1.0 + p()->talents.grimoire_of_sacrifice->effectN(4).percent() * p()->buffs.grimoire_of_sacrifice->stack();

				return m;
			}

			virtual timespan_t travel_time() const
			{
				return timespan_t::from_seconds(1.5);
			}

			virtual bool ready()
			{
				bool r = warlock_spell_t::ready();

				if (p()->buffs.metamorphosis->check()) r = false;

				return r;
			}
		};



		struct shadow_bolt_copy_t : public warlock_spell_t
		{
			shadow_bolt_copy_t(warlock_t* p, spell_data_t* sd, warlock_spell_t& sb) :
				warlock_spell_t("shadow_bolt", p, sd)
			{
				background = true;
				callbacks = false;
				direct_power_mod = sb.direct_power_mod;
				base_dd_min = sb.base_dd_min;
				base_dd_max = sb.base_dd_max;
				base_multiplier = sb.base_multiplier;
				if (data()._effects->size() > 1) generate_fury = data().effectN(2).base_value();
			}

			virtual double action_multiplier() const
			{
				double m = warlock_spell_t::action_multiplier();

				m *= 1.0 + p()->talents.grimoire_of_sacrifice->effectN(4).percent() * p()->buffs.grimoire_of_sacrifice->stack();

				return m;
			}
		};

		struct shadow_bolt_t : public warlock_spell_t
		{
			shadow_bolt_copy_t* glyph_copy_1;
			shadow_bolt_copy_t* glyph_copy_2;

			hand_of_guldan_t* hand_of_guldan;

			shadow_bolt_t(warlock_t* p) :
				warlock_spell_t(p, "Shadow Bolt"), glyph_copy_1(0), glyph_copy_2(0), hand_of_guldan(new hand_of_guldan_t(p))
			{
				base_multiplier *= 1.0 + p->sets.set(SET_T14_2PC_CASTER)->effectN(3).percent();
				base_multiplier *= 1.0 + p->sets.set(SET_T13_4PC_CASTER)->effectN(1).percent();

				hand_of_guldan->background = true;
				hand_of_guldan->base_costs[RESOURCE_MANA] = 0;


				if (p->glyphs.shadow_bolt->ok())
				{
					base_multiplier *= 0.333;

					const spell_data_t* sd = p->find_spell(p->glyphs.shadow_bolt->effectN(1).base_value());
					glyph_copy_1 = new shadow_bolt_copy_t(p, sd->effectN(2).trigger(), *this);
					glyph_copy_2 = new shadow_bolt_copy_t(p, sd->effectN(3).trigger(), *this);
				}
				else
				{
					generate_fury = data().effectN(2).base_value();
				}
			}

			virtual double action_multiplier() const
			{
				double m = warlock_spell_t::action_multiplier();

				m *= 1.0 + p()->talents.grimoire_of_sacrifice->effectN(4).percent() * p()->buffs.grimoire_of_sacrifice->stack();

				return m;
			}

			virtual void impact(action_state_t* s)
			{
				warlock_spell_t::impact(s);

				if (result_is_hit(s->result))
				{
					trigger_soul_leech(p(), s->result_amount * p()->talents.soul_leech->effectN(1).percent());
				}
			}

			virtual void execute()
			{
				// FIXME!!! Ugly hack to ensure we don't proc any on-spellcast trinkets
				if (p()->bugs && p()->glyphs.shadow_bolt->ok()) background = true;
				warlock_spell_t::execute();
				background = false;

				//cast HoG
				if (p()->sets.has_set_bonus(SET_T16_4PC_CASTER) && rng().roll(0.08))//FIX after dbc update
				{
					hand_of_guldan->target = target;
					int current_charge = hand_of_guldan->cooldown->current_charge;
					bool pre_execute_add = true;
					if (current_charge == hand_of_guldan->cooldown->charges - 1)
					{
						pre_execute_add = false;
					}
					if (pre_execute_add) hand_of_guldan->cooldown->current_charge++;

					hand_of_guldan->execute();
					if (!pre_execute_add) hand_of_guldan->cooldown->current_charge++;
				}

				if (p()->buffs.demonic_calling->up())
				{
					trigger_wild_imp(p());
					p()->buffs.demonic_calling->expire();
				}

				if (result_is_hit(execute_state->result))
				{
					// Only applies molten core if molten core is not already up
					if (target->health_percentage() < p()->spec.decimation->effectN(1).base_value() && !p()->buffs.molten_core->check())
						p()->buffs.molten_core->trigger();

					if (p()->glyphs.shadow_bolt->ok())
					{
						assert(glyph_copy_1 && glyph_copy_2);
						glyph_copy_1->execute();
						glyph_copy_2->execute();
					}
				}
			}

			virtual bool ready()
			{
				bool r = warlock_spell_t::ready();

				if (p()->buffs.metamorphosis->check()) r = false;

				return r;
			}

			virtual bool usable_moving() const
			{
				bool um = warlock_spell_t::usable_moving();

				if (p()->talents.kiljaedens_cunning->ok())
					um = true;

				return um;
			}
		};


		struct shadowburn_t : public warlock_spell_t
		{
			struct mana_event_t : public event_t
			{
				shadowburn_t* spell;
				gain_t* gain;

				mana_event_t(warlock_t* p, shadowburn_t* s) :
					event_t(*p, "shadowburn_mana_return"), spell(s), gain(p -> gains.shadowburn)
				{
					add_event(spell->mana_delay);
				}

				virtual void execute()
				{
					p()->resource_gain(RESOURCE_MANA, p()->resources.max[RESOURCE_MANA] * spell->mana_amount, gain);
				}
			};

			mana_event_t* mana_event;
			double mana_amount;
			timespan_t mana_delay;


			shadowburn_t(warlock_t* p) :
				warlock_spell_t(p, "Shadowburn"), mana_event(0)
			{
				min_gcd = timespan_t::from_millis(500);
				havoc_consume = 1;

				mana_delay = data().effectN(1).trigger()->duration();
				mana_amount = p->find_spell(data().effectN(1).trigger()->effectN(1).base_value())->effectN(1).percent();
			}

			virtual void impact(action_state_t* s)
			{
				warlock_spell_t::impact(s);

				mana_event = new (*sim) mana_event_t(p(), this);
			}

			virtual double cost() const
			{
				double c = warlock_spell_t::cost();

				if (p()->buffs.dark_soul->check())
					c *= 1.0 + p()->sets.set(SET_T15_2PC_CASTER)->effectN(2).percent();

				return c;
			}

			virtual double action_multiplier() const
			{
				double m = warlock_spell_t::action_multiplier();

				if (p()->mastery_spells.emberstorm->ok())
					m *= 1.0 + p()->cache.mastery_value();

				m *= 1.0 + p()->talents.grimoire_of_sacrifice->effectN(5).percent() * p()->buffs.grimoire_of_sacrifice->stack();

				return m;
			}

			virtual void execute()
			{
				warlock_spell_t::execute();

				if (!result_is_hit(execute_state->result)) refund_embers(p());
			}

			virtual bool ready()
			{
				bool r = warlock_spell_t::ready();

				if (target->health_percentage() >= 20) r = false;

				return r;
			}
		};


		struct corruption_t : public warlock_spell_t
		{
			bool soc_triggered;

			corruption_t(warlock_t* p, bool soc = false) :
				warlock_spell_t("Corruption", p, p -> find_spell(146739)), soc_triggered(soc) //TODO Remove FIX after dbc update
			{
				may_crit = false;
				generate_fury = 4;
				if (p->spec.pandemic->ok()) dot_behavior = DOT_EXTEND;
				base_multiplier *= 1.0 + p->sets.set(SET_T14_2PC_CASTER)->effectN(1).percent();
				base_multiplier *= 1.0 + p->sets.set(SET_T13_4PC_CASTER)->effectN(1).percent();
			};

			virtual timespan_t travel_time() const
			{
				if (soc_triggered) return timespan_t::from_seconds(std::max(rng().gauss(sim->aura_delay, 0.25 * sim->aura_delay).total_seconds(), 0.01));

				return warlock_spell_t::travel_time();
			}

			virtual void execute()
			{
				p()->latest_corruption_target = target;

				warlock_spell_t::execute();
			}

			virtual void tick(dot_t* d)
			{
				warlock_spell_t::tick(d);

				if (p()->spec.nightfall->ok() && d->state->target == p()->latest_corruption_target && !periodic_hit) //5.4 only the latest corruption procs it
				{

					if (rng().roll(p()->spec.nightfall->effectN(1).percent()))
					{
						p()->resource_gain(RESOURCE_SOUL_SHARD, 1, p()->gains.nightfall);
						// If going from 0 to 1 shard was a surprise, the player would have to react to it
						if (p()->resources.current[RESOURCE_SOUL_SHARD] == 1)
							p()->shard_react = p()->sim->current_time + p()->total_reaction_time();
						else if (p()->resources.current[RESOURCE_SOUL_SHARD] >= 1)
							p()->shard_react = p()->sim->current_time;
						else
							p()->shard_react = timespan_t::max();
					}
				}

				if (p()->glyphs.siphon_life->ok())
				{
					if (d->state->result_amount > 0)
						p()->resource_gain(RESOURCE_HEALTH,
							player->resources.max[RESOURCE_HEALTH] * p()->glyphs.siphon_life->effectN(1).percent() / 100.0,
							p()->gains.siphon_life);
				}
			}

			virtual double action_multiplier() const
			{
				double m = warlock_spell_t::action_multiplier();

				if (p()->mastery_spells.potent_afflictions->ok())
					m *= 1.0 + p()->cache.mastery_value();

				if (p()->buffs.metamorphosis->up()) // FIXME: Is corruption an exception, or did they change it so it only applies to a few spells specifically?
				{
					double mastery_value = p()->mastery_spells.master_demonologist->ok() ? p()->cache.mastery_value() : 0.0;

					m /= 1.0 + p()->spec.demonic_fury->effectN(1).percent() * 3 + mastery_value * 3;
				}
				return m;
			}

			virtual bool ready()
			{
				bool r = warlock_spell_t::ready();

				if (p()->buffs.metamorphosis->check()) r = false;

				return r;
			}
		};


		struct drain_life_heal_t : public warlock_heal_t
		{
			const spell_data_t* real_data;
			const spell_data_t* soulburned_data;

			drain_life_heal_t(warlock_t* p) :
				warlock_heal_t("drain_life_heal", p, 89653)
			{
				background = true;
				may_miss = false;
				base_dd_min = base_dd_max = 0; // Is parsed as 2
				real_data = p->find_spell(689);
				soulburned_data = p->find_spell(89420);
			}

			virtual void execute()
			{
				double heal_pct = real_data->effectN(2).percent();

				if (p()->buffs.soulburn->up())
					heal_pct = soulburned_data->effectN(2).percent();

				base_dd_min = base_dd_max = player->resources.max[RESOURCE_HEALTH] * heal_pct;

				warlock_heal_t::execute();
			}
		};

		struct drain_life_t : public warlock_spell_t
		{
			drain_life_heal_t* heal;

			drain_life_t(warlock_t* p) :
				warlock_spell_t(p, "Drain Life"), heal(0)
			{
				channeled = true;
				hasted_ticks = false;
				may_crit = false;
				generate_fury = 10;

				heal = new drain_life_heal_t(p);
			}

			virtual void last_tick(dot_t* d)
			{
				warlock_spell_t::last_tick(d);

				if (p()->buffs.soulburn->check())
					p()->buffs.soulburn->expire();
			}

			virtual void tick(dot_t* d)
			{
				warlock_spell_t::tick(d);

				heal->execute();

				consume_tick_resource(d);
			}
			virtual double action_multiplier() const
			{
				double am = warlock_spell_t::action_multiplier();

				if (p()->talents.harvest_life->ok())
					am *= 1.0 + p()->talents.harvest_life->effectN(1).percent();

				return am;
			}

		};


		struct drain_soul_t : public warlock_spell_t
		{
			bool generate_shard;

			drain_soul_t(warlock_t* p) :
				warlock_spell_t(p, "Drain Soul"), generate_shard(false)
			{
				channeled = true;
				hasted_ticks = false;
				may_crit = false;
				stormlash_da_multiplier = 0.0;
				stormlash_ta_multiplier = 2.0;
				base_dd_min = base_dd_max = 0; // prevent it from picking up direct damage from that strange absorb effect

				stats->add_child(p->get_stats("agony_ds"));
				stats->add_child(p->get_stats("corruption_ds"));
				stats->add_child(p->get_stats("unstable_affliction_ds"));
			}

			virtual double composite_target_multiplier(player_t* t) const
			{
				double m = warlock_spell_t::composite_target_multiplier(t);

				if (t->health_percentage() <= data().effectN(3).base_value())
					m *= 1.0 + data().effectN(6).percent();

				return m;
			}

			virtual double action_multiplier() const
			{
				double m = warlock_spell_t::action_multiplier();

				m *= 1.0 + p()->talents.grimoire_of_sacrifice->effectN(3).percent() * p()->buffs.grimoire_of_sacrifice->stack();

				m *= 1.0 + p()->sets.set(SET_T15_4PC_CASTER)->effectN(1).percent();


				if (p()->buffs.tier16_2pc_empowered_grasp->up())
				{
					m *= 1.0 + p()->buffs.tier16_2pc_empowered_grasp->value();
				}
				return m;

			}

			virtual void tick(dot_t* d)
			{
				warlock_spell_t::tick(d);

				if (generate_shard)
				{
					p()->resource_gain(RESOURCE_SOUL_SHARD, 1, p()->gains.drain_soul);
					p()->shard_react = p()->sim->current_time;
				}
				generate_shard = !generate_shard;

				trigger_soul_leech(p(), d->state->result_amount * p()->talents.soul_leech->effectN(1).percent() * 2);

				if (d->state->target->health_percentage() <= data().effectN(3).base_value())
				{
					double multiplier = data().effectN(5).percent();

					// DO NOT CHANGE THE ORDER IF YOU ARE NOT 100% SURE
					if (p()->buffs.tier16_2pc_empowered_grasp->up())
					{
						multiplier += p()->buffs.tier16_2pc_empowered_grasp->value();
					}

					multiplier *= (1.0 + p()->talents.grimoire_of_sacrifice->effectN(3).percent() * p()->buffs.grimoire_of_sacrifice->stack());


					multiplier *= 1.0 + p()->sets.set(SET_T15_4PC_CASTER)->effectN(1).percent();

					trigger_extra_tick(td(d->state->target)->dots_agony, multiplier, false);
					trigger_extra_tick(td(d->state->target)->dots_corruption, multiplier, false);
					trigger_extra_tick(td(d->state->target)->dots_unstable_affliction, multiplier, false);
				}

				consume_tick_resource(d);
			}

			virtual void impact(action_state_t* s)
			{
				warlock_spell_t::impact(s);

				if (result_is_hit(s->result))
				{
					generate_shard = false;

					if (s->target->health_percentage() <= data().effectN(3).base_value())
						td(s->target)->ds_started_below_20 = true;
					else
						td(s->target)->ds_started_below_20 = false;
				}
			}
		};


		struct unstable_affliction_t : public warlock_spell_t
		{
			unstable_affliction_t(warlock_t* p) :
				warlock_spell_t(p, "Unstable Affliction")
			{
				may_crit = false;
				if (p->spec.pandemic->ok()) dot_behavior = DOT_EXTEND;

				if (p->glyphs.unstable_affliction->ok())
					base_execute_time *= 1.0 + p->glyphs.unstable_affliction->effectN(1).percent();
			}

			virtual double action_multiplier() const
			{
				double m = warlock_spell_t::action_multiplier();

				if (p()->mastery_spells.potent_afflictions->ok())
					m *= 1.0 + p()->cache.mastery_value();

				return m;
			}

			virtual void tick(dot_t* d)
			{
				warlock_spell_t::tick(d);

				if (p()->sets.has_set_bonus(SET_T16_2PC_CASTER) && d->state->result == RESULT_CRIT)
				{
					p()->buffs.tier16_2pc_empowered_grasp->trigger();
				}
			}
		};


		struct haunt_t : public warlock_spell_t
		{
			haunt_t(warlock_t* p) :
				warlock_spell_t(p, "Haunt")
			{
				hasted_ticks = false;
				tick_may_crit = false;
			}



			void try_to_trigger_soul_shard_refund()//t16_4pc_bonus
			{
				if (rng().roll(p()->sets.set(SET_T16_4PC_CASTER)->effectN(1).percent()))//refund shard when haunt expires
				{
					p()->resource_gain(RESOURCE_SOUL_SHARD, p()->find_spell(145159)->effectN(1).resource(RESOURCE_SOUL_SHARD), p()->gains.haunt_tier16_4pc);
				}
			}

			virtual double action_multiplier() const
			{
				double m = warlock_spell_t::action_multiplier();

				m *= 1.0 + p()->talents.grimoire_of_sacrifice->effectN(3).percent() * p()->buffs.grimoire_of_sacrifice->stack();

				return m;
			}

			virtual void impact(action_state_t* s)
			{
				timespan_t duration = data().duration();

				if (p()->sets.has_set_bonus(SET_T15_2PC_CASTER) && p()->buffs.dark_soul->check())
					duration += p()->spells.tier15_2pc->effectN(1).time_value();

				num_ticks = (int)(duration / base_tick_time);
				warlock_spell_t::impact(s);

				if (result_is_hit(s->result))
				{
					if (td(s->target)->debuffs_haunt->check())//also can refund on refresh
					{
						try_to_trigger_soul_shard_refund();
					}

					td(s->target)->debuffs_haunt->trigger(1, buff_t::DEFAULT_VALUE(), -1.0, td(s->target)->dots_haunt->remains());

					trigger_soul_leech(p(), s->result_amount * p()->talents.soul_leech->effectN(1).percent() * 2);
				}
			}



			virtual void last_tick(dot_t* d)
			{
				warlock_spell_t::last_tick(d);

				try_to_trigger_soul_shard_refund();
			}

		};


		struct immolate_t : public warlock_spell_t
		{
			immolate_t* fnb;

			immolate_t(warlock_t* p) :
				warlock_spell_t(p, "Immolate"),
				fnb(new immolate_t("immolate", p, p -> find_spell(108686)))
			{
				havoc_consume = 1;
				base_costs[RESOURCE_MANA] *= 1.0 + p->spec.chaotic_energy->effectN(2).percent();
			}

			immolate_t(const std::string& n, warlock_t* p, const spell_data_t* spell) :
				warlock_spell_t(n, p, spell),
				fnb(0)
			{
				aoe = -1;

				stats = p->get_stats("immolate_fnb", this);
			}

			void init()
			{
				warlock_spell_t::init();

				if (p()->spec.pandemic->ok())
					dot_behavior = DOT_EXTEND;
			}

			void schedule_execute(action_state_t* state)
			{
				if (fnb && p()->buffs.fire_and_brimstone->up())
					fnb->schedule_execute(state);
				else
					warlock_spell_t::schedule_execute(state);
			}

			double cost() const
			{
				if (fnb && p()->buffs.fire_and_brimstone->check())
					return fnb->cost();

				return warlock_spell_t::cost();
			}

			virtual double crit_chance(double crit, int delta_level) const
			{
				double cc = warlock_spell_t::crit_chance(crit, delta_level);

				if (p()->sets.has_set_bonus(SET_T16_2PC_CASTER) && p()->buffs.tier16_2pc_destructive_influence->check())
					cc += p()->buffs.tier16_2pc_destructive_influence->value();

				return cc;
			}

			virtual double action_multiplier() const
			{
				double m = warlock_spell_t::action_multiplier();

				if (p()->buffs.fire_and_brimstone->check())
				{
					if (p()->mastery_spells.emberstorm->ok())
						m *= 1.0 + p()->cache.mastery_value();

					m *= p()->buffs.fire_and_brimstone->data().effectN(6).percent();
				}

				if (p()->mastery_spells.emberstorm->ok())
					m *= 1.0 + p()->mastery_spells.emberstorm->effectN(3).percent() + p()->cache.mastery_value() * p()->emberstorm_e3_from_e1();

				return m;
			}

			virtual void impact(action_state_t* s)
			{
				warlock_spell_t::impact(s);

				gain_t* gain;
				if (!fnb && p()->spec.fire_and_brimstone->ok())
					gain = p()->gains.immolate_fnb;
				else
					gain = p()->gains.immolate;

				if (s->result == RESULT_CRIT) trigger_ember_gain(p(), 0.1, gain);
			}

			virtual void tick(dot_t* d)
			{
				warlock_spell_t::tick(d);

				gain_t* gain;
				if (!fnb && p()->spec.fire_and_brimstone->ok())
					gain = p()->gains.immolate_fnb;
				else
					gain = p()->gains.immolate;

				if (d->state->result == RESULT_CRIT) trigger_ember_gain(p(), 0.1, gain);

				if (p()->glyphs.siphon_life->ok())
				{
					if (d->state->result_amount > 0)
						p()->resource_gain(RESOURCE_HEALTH,
							player->resources.max[RESOURCE_HEALTH] * p()->glyphs.siphon_life->effectN(1).percent() / 100.0,
							p()->gains.siphon_life);
				}
			}
		};


		struct conflagrate_t : public warlock_spell_t
		{
			conflagrate_t* fnb;

			conflagrate_t(warlock_t* p) :
				warlock_spell_t(p, "Conflagrate"),
				fnb(new conflagrate_t("conflagrate", p, p -> find_spell(108685)))
			{
				havoc_consume = 1;
				base_costs[RESOURCE_MANA] *= 1.0 + p->spec.chaotic_energy->effectN(2).percent();
			}

			conflagrate_t(const std::string& n, warlock_t* p, const spell_data_t* spell) :
				warlock_spell_t(n, p, spell),
				fnb(0)
			{
				aoe = -1;
				stats = p->get_stats("conflagrate_fnb", this);
			}

			void schedule_execute(action_state_t* state)
			{
				if (fnb && p()->buffs.fire_and_brimstone->up())
					fnb->schedule_execute(state);
				else
					warlock_spell_t::schedule_execute(state);
			}

			double cost() const
			{
				if (fnb && p()->buffs.fire_and_brimstone->check())
					return fnb->cost();

				return warlock_spell_t::cost();
			}

			void init()
			{
				warlock_spell_t::init();

				// FIXME: No longer in the spell data for some reason
				cooldown->duration = timespan_t::from_seconds(12.0);
				cooldown->charges = 2;
			}

			void execute()
			{
				warlock_spell_t::execute();

				if (result_is_hit(execute_state->result) && p()->spec.backdraft->ok())
					p()->buffs.backdraft->trigger(3);
			}

			virtual double action_multiplier() const
			{
				double m = warlock_spell_t::action_multiplier();

				if (p()->buffs.fire_and_brimstone->check())
				{
					if (p()->mastery_spells.emberstorm->ok())
						m *= 1.0 + p()->cache.mastery_value();
					m *= p()->buffs.fire_and_brimstone->data().effectN(6).percent();
				}
				else
					m *= 1.0 + p()->talents.grimoire_of_sacrifice->effectN(5).percent() * p()->buffs.grimoire_of_sacrifice->stack();

				if (p()->mastery_spells.emberstorm->ok())
					m *= 1.0 + p()->mastery_spells.emberstorm->effectN(3).percent() + p()->cache.mastery_value() * p()->emberstorm_e3_from_e1();

				return m;
			}

			virtual void impact(action_state_t* s)
			{
				warlock_spell_t::impact(s);
				gain_t* gain;
				if (!fnb && p()->spec.fire_and_brimstone->ok())
					gain = p()->gains.conflagrate_fnb;
				else
					gain = p()->gains.conflagrate;

				trigger_ember_gain(p(), s->result == RESULT_CRIT ? 0.2 : 0.1, gain);

				//hotfixed extra gain with 15% chance from 13.09.13
				trigger_ember_gain(p(), 0.1, gain, 0.15);
				if (s->result == RESULT_CRIT && p()->sets.has_set_bonus(SET_T16_2PC_CASTER))
					p()->buffs.tier16_2pc_destructive_influence->trigger();
			}

			virtual bool ready()
			{
				if (fnb && p()->buffs.fire_and_brimstone->check())
					return fnb->ready();
				return warlock_spell_t::ready();
			}
		};

		struct incinerate_t : public warlock_spell_t
		{
			incinerate_t* fnb;

			// Normal incinerate
			incinerate_t(warlock_t* p) :
				warlock_spell_t(p, "Incinerate"),
				fnb(new incinerate_t("incinerate", p, p -> find_spell(114654)))
			{
				havoc_consume = 1;
				base_costs[RESOURCE_MANA] *= 1.0 + p->spec.chaotic_energy->effectN(2).percent();
			}

			// Fire and Brimstone incinerate
			incinerate_t(const std::string& n, warlock_t* p, const spell_data_t* spell) :
				warlock_spell_t(n, p, spell),
				fnb(0)
			{
				aoe = -1;
				stats = p->get_stats("incinerate_fnb", this);
			}

			void init()
			{
				warlock_spell_t::init();

				backdraft_consume = 1;

				base_multiplier *= 1.0 + p()->sets.set(SET_T14_2PC_CASTER)->effectN(2).percent();
				base_multiplier *= 1.0 + p()->sets.set(SET_T13_4PC_CASTER)->effectN(1).percent();
			}

			void schedule_execute(action_state_t* state)
			{
				if (fnb && p()->buffs.fire_and_brimstone->up())
					fnb->schedule_execute(state);
				else
					warlock_spell_t::schedule_execute(state);
			}

			double cost() const
			{
				if (fnb && p()->buffs.fire_and_brimstone->check())
					return fnb->cost();

				return warlock_spell_t::cost();
			}

			virtual double crit_chance(double crit, int delta_level) const
			{
				double cc = warlock_spell_t::crit_chance(crit, delta_level);

				if (p()->sets.has_set_bonus(SET_T16_2PC_CASTER) && p()->buffs.tier16_2pc_destructive_influence->check())
					cc += p()->buffs.tier16_2pc_destructive_influence->value();

				return cc;
			}

			virtual double action_multiplier() const
			{
				double m = warlock_spell_t::action_multiplier();

				if (p()->buffs.fire_and_brimstone->check())
				{
					if (p()->mastery_spells.emberstorm->ok())
						m *= 1.0 + p()->cache.mastery_value();
					m *= p()->buffs.fire_and_brimstone->data().effectN(6).percent();
				}
				else
					m *= 1.0 + p()->talents.grimoire_of_sacrifice->effectN(5).percent() * p()->buffs.grimoire_of_sacrifice->stack();

				m *= 1.0 + p()->mastery_spells.emberstorm->effectN(3).percent() + p()->composite_mastery() * p()->mastery_spells.emberstorm->effectN(3).mastery_value();

				return m;
			}

			virtual void impact(action_state_t* s)
			{
				warlock_spell_t::impact(s);
				gain_t* gain;
				if (!fnb && p()->spec.fire_and_brimstone->ok())
					gain = p()->gains.incinerate_fnb;
				else
					gain = p()->gains.incinerate;

				trigger_ember_gain(p(), s->result == RESULT_CRIT ? 0.2 : 0.1, gain);
				//hotfixed extra gain with 15% 13.09.13
				trigger_ember_gain(p(), 0.1, gain, 0.15);
				if (rng().roll(p()->sets.set(SET_T15_4PC_CASTER)->effectN(2).percent()))
					trigger_ember_gain(p(), s->result == RESULT_CRIT ? 0.2 : 0.1, p()->gains.incinerate_t15_4pc);

				if (result_is_hit(s->result))
					trigger_soul_leech(p(), s->result_amount * p()->talents.soul_leech->effectN(1).percent());
			}

			virtual bool usable_moving() const
			{
				bool um = warlock_spell_t::usable_moving();

				if (p()->talents.kiljaedens_cunning->ok())
					um = true;

				return um;
			}

			virtual bool ready()
			{
				if (fnb && p()->buffs.fire_and_brimstone->check())
					return fnb->ready();
				return warlock_spell_t::ready();
			}
		};

		struct soul_fire_t : public warlock_spell_t
		{
			warlock_spell_t* meta_spell;

			soul_fire_t(warlock_t* p, bool meta = false) :
				warlock_spell_t(meta ? "soul_fire_meta" : "soul_fire", p, meta ? p -> find_spell(104027) : p -> find_spell(6353)), meta_spell(0)
			{
				if (!meta)
				{
					generate_fury = data().effectN(2).base_value();
					meta_spell = new soul_fire_t(p, true);
				}
				else
				{
					background = true;
				}
			}

			virtual void parse_options(option_t* o, const std::string& options_str)
			{
				warlock_spell_t::parse_options(o, options_str);
				if (meta_spell) meta_spell->parse_options(o, options_str);
			}

			virtual void execute()
			{
				if (meta_spell && p()->buffs.metamorphosis->check())
				{
					meta_spell->time_to_execute = time_to_execute;
					meta_spell->execute();
				}
				else
				{
					warlock_spell_t::execute();

					if (p()->buffs.demonic_calling->up())
					{
						trigger_wild_imp(p());
						p()->buffs.demonic_calling->expire();
					}

					if (p()->buffs.molten_core->check())
						p()->buffs.molten_core->decrement();

					if (result_is_hit(execute_state->result) && target->health_percentage() < p()->spec.decimation->effectN(1).base_value())
						p()->buffs.molten_core->trigger();
				}
			}

			virtual void impact(action_state_t* s)
			{
				warlock_spell_t::impact(s);

				if (result_is_hit(s->result))
					trigger_soul_leech(p(), s->result_amount * p()->talents.soul_leech->effectN(1).percent());
				if (p()->sets.has_set_bonus(SET_T16_2PC_CASTER))
				{
					p()->buffs.tier16_2pc_fiery_wrath->trigger();
				}

			}

			virtual timespan_t execute_time() const
			{
				timespan_t t = warlock_spell_t::execute_time();

				if (p()->buffs.molten_core->up())
					t *= 1.0 + p()->buffs.molten_core->data().effectN(1).percent();

				return t;
			}

			virtual double crit_chance(double /* crit */, int /* delta_level */) const
			{
				// Soul fire always crits
				return 1.0;
			}

			virtual double action_multiplier() const
			{
				double m = warlock_spell_t::action_multiplier();

				m *= 1.0 + p()->cache.spell_crit();

				m *= 1.0 + p()->talents.grimoire_of_sacrifice->effectN(4).percent() * p()->buffs.grimoire_of_sacrifice->stack();

				return m;
			}

			virtual double cost() const
			{
				double c = warlock_spell_t::cost();

				if (p()->buffs.molten_core->check())
					c *= 1.0 + p()->buffs.molten_core->data().effectN(1).percent();

				return c;
			}

			virtual bool ready()
			{
				return (meta_spell && p()->buffs.metamorphosis->check()) ? meta_spell->ready() : warlock_spell_t::ready();
			}
		};


		struct chaos_bolt_t : public warlock_spell_t
		{
			chaos_bolt_t(warlock_t* p) :
				warlock_spell_t(p, "Chaos Bolt")
			{
				havoc_consume = 3;
				if (p->spec.pyroclasm->ok())
					backdraft_consume = 3;

				hasted_ticks = false;

				if (p->talents.grimoire_of_sacrifice->ok())
					num_ticks = p->talents.grimoire_of_sacrifice->effectN(12).time_value().total_seconds() / data().effectN(2).period().total_seconds();
			}

			virtual double crit_chance(double /* crit */, int /* delta_level */) const
			{
				// Chaos Bolt always crits
				return 1.0;
			}

			virtual double cost() const
			{
				double c = warlock_spell_t::cost();

				if (p()->buffs.dark_soul->check())
					c *= 1.0 + p()->sets.set(SET_T15_2PC_CASTER)->effectN(2).percent();

				return c;
			}

			virtual double action_multiplier() const
			{
				double m = warlock_spell_t::action_multiplier();

				if (p()->mastery_spells.emberstorm->ok())
					m *= 1.0 + p()->cache.mastery_value();

				m *= 1.0 + p()->cache.spell_crit();

				return m;
			}

			virtual void execute()
			{
				warlock_spell_t::execute();

				if (!result_is_hit(execute_state->result)) refund_embers(p());
			}
		};

		struct life_tap_t : public warlock_spell_t
		{
			life_tap_t(warlock_t* p) :
				warlock_spell_t(p, "Life Tap")
			{
				harmful = false;
			}

			virtual void execute()
			{
				warlock_spell_t::execute();

				double health = player->resources.max[RESOURCE_HEALTH];
				// FIXME: This should be implemented as a real health gain, but we don't have an easy way to do temporary percentage-wise resource gains
				if (p()->talents.soul_link->ok() && p()->buffs.grimoire_of_sacrifice->up())
					health *= 1.0 + p()->talents.grimoire_of_sacrifice->effectN(7).percent();

				// FIXME: Implement reduced healing debuff
				if (!p()->glyphs.life_tap->ok()) player->resource_loss(RESOURCE_HEALTH, health * data().effectN(3).percent());
				player->resource_gain(RESOURCE_MANA, health * data().effectN(1).percent(), p()->gains.life_tap);
			}
		};


		struct melee_t : public warlock_spell_t
		{
			melee_t(warlock_t* p) :
				warlock_spell_t("melee", p, p -> find_spell(103988))
			{
				background = true;
				repeating = true;
				base_execute_time = timespan_t::from_seconds(1);
			}

			virtual double action_multiplier() const
			{
				double m = warlock_spell_t::action_multiplier();

				m *= 1.0 + p()->talents.grimoire_of_sacrifice->effectN(4).percent() * p()->buffs.grimoire_of_sacrifice->stack();

				return m;
			}
		};

		struct activate_melee_t : public warlock_spell_t
		{
			activate_melee_t(warlock_t* p) :
				warlock_spell_t("activate_melee", p, spell_data_t::nil())
			{
				trigger_gcd = timespan_t::zero();
				harmful = false;

				if (!p->spells.melee) p->spells.melee = new melee_t(p);
			}

			virtual void execute()
			{
				p()->spells.melee->execute();
			}

			virtual bool ready()
			{
				// FIXME: Properly remove this whole ability later - no time now
				return false;
			}
		};


		struct metamorphosis_t : public warlock_spell_t
		{
			metamorphosis_t(warlock_t* p) :
				warlock_spell_t(p, "Metamorphosis")
			{
				trigger_gcd = timespan_t::zero();
				harmful = false;
				background = true;
			}

			virtual void execute()
			{
				warlock_spell_t::execute();

				assert(cost_event == 0);
				p()->buffs.metamorphosis->trigger();
				cost_event = new (*sim) cost_event_t(p(), this);
			}

			virtual void cancel()
			{
				warlock_spell_t::cancel();

				if (p()->spells.melee) p()->spells.melee->cancel();
				p()->buffs.metamorphosis->expire();
				core_event_t::cancel(cost_event);
			}
		};

		struct activate_metamorphosis_t : public warlock_spell_t
		{
			activate_metamorphosis_t(warlock_t* p) :
				warlock_spell_t("activate_metamorphosis", p)
			{
				trigger_gcd = timespan_t::zero();
				harmful = false;

				if (!p->spells.metamorphosis) p->spells.metamorphosis = new metamorphosis_t(p);
			}

			virtual void execute()
			{
				if (!p()->buffs.metamorphosis->check()) p()->spells.metamorphosis->execute();
			}

			virtual bool ready()
			{
				bool r = warlock_spell_t::ready();

				if (p()->buffs.metamorphosis->check()) r = false;
				else if (p()->resources.current[RESOURCE_DEMONIC_FURY] <= META_FURY_MINIMUM) r = false;
				else if (!p()->spells.metamorphosis->ready()) r = false;

				return r;
			}
		};

		struct cancel_metamorphosis_t : public warlock_spell_t
		{
			cancel_metamorphosis_t(warlock_t* p) :
				warlock_spell_t("cancel_metamorphosis", p)
			{
				trigger_gcd = timespan_t::zero();
				harmful = false;
			}

			virtual void execute()
			{
				warlock_spell_t::execute();

				p()->spells.metamorphosis->cancel();
			}

			virtual bool ready()
			{
				bool r = warlock_spell_t::ready();

				if (!p()->buffs.metamorphosis->check()) r = false;

				return r;
			}
		};


		struct chaos_wave_dmg_t : public warlock_spell_t
		{
			chaos_wave_dmg_t(warlock_t* p) :
				warlock_spell_t("chaos_wave_dmg", p, p -> find_spell(124915))
			{
				aoe = -1;
				background = true;
				dual = true;
			}
		};


		struct chaos_wave_t : public warlock_spell_t
		{
			chaos_wave_dmg_t* cw_damage;

			chaos_wave_t(warlock_t* p) :
				warlock_spell_t("chaos_wave", p, p -> spec.chaos_wave),
				cw_damage(0)
			{
				cooldown = p->cooldowns.hand_of_guldan;

				impact_action = new chaos_wave_dmg_t(p);
				impact_action->stats = stats;
			}

			virtual double action_multiplier() const
			{
				double m = warlock_spell_t::action_multiplier();

				m *= 1.0 + p()->talents.grimoire_of_sacrifice->effectN(4).percent() * p()->buffs.grimoire_of_sacrifice->stack();

				return m;
			}

			virtual timespan_t travel_time() const
			{
				return timespan_t::from_seconds(1.5);
			}

			virtual bool ready()
			{
				bool r = warlock_spell_t::ready();

				if (!p()->buffs.metamorphosis->check()) r = false;

				return r;
			}
		};


		struct touch_of_chaos_t : public warlock_spell_t
		{
			chaos_wave_t* chaos_wave;

			touch_of_chaos_t(warlock_t* p) :
				warlock_spell_t("touch_of_chaos", p, p -> spec.touch_of_chaos), chaos_wave(new chaos_wave_t(p))
			{
				base_multiplier *= 1.0 + p->sets.set(SET_T14_2PC_CASTER)->effectN(3).percent();
				base_multiplier *= 1.0 + p->sets.set(SET_T13_4PC_CASTER)->effectN(1).percent(); // Assumption - need to test whether ToC is affected

				chaos_wave->background = true;
				chaos_wave->base_costs[RESOURCE_DEMONIC_FURY] = 0;

			}

			virtual double action_multiplier() const
			{
				double m = warlock_spell_t::action_multiplier();

				m *= 1.0 + p()->talents.grimoire_of_sacrifice->effectN(4).percent() * p()->buffs.grimoire_of_sacrifice->stack();

				return m;
			}

			virtual void impact(action_state_t* s)
			{
				warlock_spell_t::impact(s);

				if (result_is_hit(s->result))
				{
					trigger_soul_leech(p(), s->result_amount * p()->talents.soul_leech->effectN(1).percent());
					extend_dot(td(s->target)->dots_corruption, 4);
				}
			}

			virtual void execute()
			{
				warlock_spell_t::execute();

				if (p()->buffs.demonic_calling->up())
				{
					trigger_wild_imp(p());
					p()->buffs.demonic_calling->expire();
				}

				//cast CW
				if (p()->sets.has_set_bonus(SET_T16_4PC_CASTER) && rng().roll(0.08))//FIX after dbc update
				{
					chaos_wave->target = target;
					int current_charge = chaos_wave->cooldown->current_charge;
					bool pre_execute_add = true;
					if (current_charge == chaos_wave->cooldown->charges - 1)
					{
						pre_execute_add = false;
					}
					if (pre_execute_add) chaos_wave->cooldown->current_charge++;
					chaos_wave->execute();
					if (!pre_execute_add) chaos_wave->cooldown->current_charge++;
				}

			}

			virtual bool ready()
			{
				bool r = warlock_spell_t::ready();

				if (!p()->buffs.metamorphosis->check()) r = false;

				return r;
			}
		};


		struct fel_flame_t : public warlock_spell_t
		{
			fel_flame_t(warlock_t* p) :
				warlock_spell_t(p, "Fel Flame")
			{
				havoc_consume = 1;
				if (p->specialization() == WARLOCK_DESTRUCTION)
					base_costs[RESOURCE_MANA] *= 1.0 + p->spec.chaotic_energy->effectN(2).percent();
				base_costs[RESOURCE_MANA] *= 2.0; //12.09.13 hotfix. check after dbc update
			}

			virtual void impact(action_state_t* s)
			{
				warlock_spell_t::impact(s);

				if (p()->specialization() == WARLOCK_DESTRUCTION)
				{
					if (s->result == RESULT_HIT)
					{
						trigger_ember_gain(p(), 0.1, p()->gains.fel_flame);
					}
					else if (s->result == RESULT_CRIT)
					{
						trigger_ember_gain(p(), 0.2, p()->gains.fel_flame);
					}
				}

				if (p()->specialization() == WARLOCK_DEMONOLOGY) p()->resource_gain(RESOURCE_DEMONIC_FURY, data().effectN(2).base_value(), p()->gains.fel_flame);
			}

			virtual double action_multiplier() const
			{
				double m = warlock_spell_t::action_multiplier();

				if (p()->specialization() == WARLOCK_AFFLICTION)
					m *= 1.0 + p()->talents.grimoire_of_sacrifice->effectN(3).percent() * p()->buffs.grimoire_of_sacrifice->stack();

				if (p()->specialization() == WARLOCK_DEMONOLOGY)
					m *= 1.0 + p()->talents.grimoire_of_sacrifice->effectN(4).percent() * p()->buffs.grimoire_of_sacrifice->stack();

				if (p()->specialization() == WARLOCK_DESTRUCTION)
					m *= 1.0 + p()->talents.grimoire_of_sacrifice->effectN(5).percent() * p()->buffs.grimoire_of_sacrifice->stack();

				if (p()->mastery_spells.emberstorm->ok())
					m *= 1.0 + p()->mastery_spells.emberstorm->effectN(3).percent() + p()->cache.mastery_value() * p()->emberstorm_e3_from_e1();

				return m;
			}

			virtual bool ready()
			{
				bool r = warlock_spell_t::ready();

				if (p()->buffs.metamorphosis->check()) r = false;

				return r;
			}
		};


		struct void_ray_t : public warlock_spell_t
		{
			void_ray_t(warlock_t* p) :
				warlock_spell_t(p, "Void Ray")
			{
				aoe = -1;
				travel_speed = 0;
				direct_power_mod = data().effectN(1).coeff();
			}

			virtual double action_multiplier() const
			{
				double m = warlock_spell_t::action_multiplier();

				m *= 1.0 + p()->talents.grimoire_of_sacrifice->effectN(4).percent() * p()->buffs.grimoire_of_sacrifice->stack();

				return m;
			}

			virtual void impact(action_state_t* s)
			{
				warlock_spell_t::impact(s);

				if (result_is_hit(s->result))
				{
					extend_dot(td(s->target)->dots_corruption, 3);
				}
			}

			virtual bool ready()
			{
				bool r = warlock_spell_t::ready();

				if (!p()->buffs.metamorphosis->check()) r = false;

				return r;
			}
		};


		struct malefic_grasp_t : public warlock_spell_t
		{
			malefic_grasp_t(warlock_t* p) :
				warlock_spell_t(p, "Malefic Grasp")
			{
				channeled = true;
				hasted_ticks = false;
				may_crit = false;
				stormlash_da_multiplier = 0.0;
				stormlash_ta_multiplier = 2.0;

				stats->add_child(p->get_stats("agony_mg"));
				stats->add_child(p->get_stats("corruption_mg"));
				stats->add_child(p->get_stats("unstable_affliction_mg"));
			}

			virtual double action_multiplier() const
			{
				double m = warlock_spell_t::action_multiplier();

				m *= 1.0 + p()->talents.grimoire_of_sacrifice->effectN(3).percent() * p()->buffs.grimoire_of_sacrifice->stack();

				m *= 1.0 + p()->sets.set(SET_T15_4PC_CASTER)->effectN(1).percent();


				if (p()->buffs.tier16_2pc_empowered_grasp->up())
				{
					m *= 1.0 + p()->buffs.tier16_2pc_empowered_grasp->value();
				}
				return m;
			}

			virtual void tick(dot_t* d)
			{
				warlock_spell_t::tick(d);

				trigger_soul_leech(p(), d->state->result_amount * p()->talents.soul_leech->effectN(1).percent() * 2);

				double multiplier = data().effectN(3).percent();

				// DO NOT CHANGE THE ORDER IF YOU ARE NOT 100% SURE
				if (p()->buffs.tier16_2pc_empowered_grasp->up())
				{
					multiplier += p()->buffs.tier16_2pc_empowered_grasp->value();
				}

				multiplier *= (1.0 + p()->talents.grimoire_of_sacrifice->effectN(3).percent() * p()->buffs.grimoire_of_sacrifice->stack());

				multiplier *= 1.0 + p()->sets.set(SET_T15_4PC_CASTER)->effectN(1).percent();



				trigger_extra_tick(td(d->state->target)->dots_agony, multiplier, true);
				trigger_extra_tick(td(d->state->target)->dots_corruption, multiplier, true);
				trigger_extra_tick(td(d->state->target)->dots_unstable_affliction, multiplier, true);

				consume_tick_resource(d);
			}

			virtual bool usable_moving() const
			{
				bool um = warlock_spell_t::usable_moving();

				if (p()->talents.kiljaedens_cunning->ok())
					um = true;

				return um;
			}
		};


		struct dark_intent_t : public warlock_spell_t
		{
			dark_intent_t(warlock_t* p) :
				warlock_spell_t(p, "Dark Intent")
			{
				harmful = false;
				background = (sim->overrides.spell_power_multiplier != 0);
			}

			virtual void execute()
			{
				warlock_spell_t::execute();

				if (!sim->overrides.spell_power_multiplier)
					sim->auras.spell_power_multiplier->trigger();
			}
		};


		struct soulburn_t : public warlock_spell_t
		{
			soulburn_t(warlock_t* p) :
				warlock_spell_t(p, "Soulburn")
			{
				harmful = false;
			}

			virtual void execute()
			{
				p()->buffs.soulburn->trigger();

				warlock_spell_t::execute();
			}
		};


		struct dark_soul_t : public warlock_spell_t
		{
			dark_soul_t(warlock_t* p) :
				warlock_spell_t("dark_soul", p, p -> spec.dark_soul)
			{
				harmful = false;

				if (p->talents.archimondes_darkness->ok())
				{
					cooldown->charges = p->talents.archimondes_darkness->effectN(1).base_value();
					p->buffs.dark_soul->cooldown->duration = timespan_t::zero();
				}
			}

			virtual void execute()
			{
				warlock_spell_t::execute();

				p()->buffs.dark_soul->trigger();
			}

			virtual bool ready()
			{
				bool r = warlock_spell_t::ready();

				if (p()->buffs.dark_soul->check()) r = false;

				return r;
			}
		};


		struct imp_swarm_t : public warlock_spell_t
		{
			timespan_t base_cooldown;

			imp_swarm_t(warlock_t* p) :
				warlock_spell_t("imp_swarm", p, (p -> specialization() == WARLOCK_DEMONOLOGY && p -> glyphs.imp_swarm -> ok()) ? p -> find_spell(104316) : spell_data_t::not_found()),
				base_cooldown(cooldown -> duration)
			{
				harmful = false;

				stats->add_child(p->get_stats("firebolt"));
			}

			virtual void execute()
			{
				cooldown->duration = base_cooldown * composite_haste();

				warlock_spell_t::execute();

				core_event_t::cancel(p()->demonic_calling_event);
				p()->demonic_calling_event = new (*sim) warlock_t::demonic_calling_event_t(player, cooldown->duration, true);

				int imp_count = data().effectN(1).base_value();
				int j = 0;

				for (size_t i = 0; i < p()->pets.wild_imps.size(); i++)
				{
					if (p()->pets.wild_imps[i]->is_sleeping())
					{
						p()->pets.wild_imps[i]->trigger(true);
						if (++j == imp_count) break;
					}
				}
				if (j != imp_count) sim->errorf("Player %s ran out of wild imps during imp_swarm.\n", p()->name());
				assert(j == imp_count);  // Assert fails if we didn't have enough available wild imps
			}
		};


		struct fire_and_brimstone_t : public warlock_spell_t
		{
			fire_and_brimstone_t(warlock_t* p) :
				warlock_spell_t(p, "Fire and Brimstone")
			{
				harmful = false;
			}

			virtual void execute()
			{
				assert(player->resources.current[RESOURCE_BURNING_EMBER] >= 1.0);

				warlock_spell_t::execute();

				if (p()->buffs.fire_and_brimstone->check())
					p()->buffs.fire_and_brimstone->expire();
				else
					p()->buffs.fire_and_brimstone->trigger();
			}

			virtual bool ready()
			{
				if (player->resources.current[RESOURCE_BURNING_EMBER] < 1)
					return false;

				return warlock_spell_t::ready();
			}
		};


		// AOE SPELLS

		struct seed_of_corruption_aoe_t : public warlock_spell_t
		{
			seed_of_corruption_aoe_t(warlock_t* p) :
				warlock_spell_t("seed_of_corruption_aoe", p, p -> find_spell(27285))
			{
				aoe = -1;
				dual = true;
				background = true;
				callbacks = false;
			}

			virtual double action_multiplier() const
			{
				double m = warlock_spell_t::action_multiplier();

				if (p()->buffs.mannoroths_fury->up())
				{
					m *= 1.0 + p()->talents.mannoroths_fury->effectN(3).percent();
				}
				return m;
			}

			virtual void init()
			{
				warlock_spell_t::init();

				p()->spells.soulburn_seed_of_corruption_aoe->stats = stats;
			}
		};

		struct soulburn_seed_of_corruption_aoe_t : public warlock_spell_t
		{
			corruption_t* corruption;

			soulburn_seed_of_corruption_aoe_t(warlock_t* p) :
				warlock_spell_t("soulburn_seed_of_corruption_aoe", p, p -> find_spell(87385)), corruption(new corruption_t(p, true))
			{
				aoe = -1;
				dual = true;
				background = true;
				callbacks = false;
				corruption->background = true;
				corruption->dual = true;
				corruption->may_miss = false;
				corruption->base_costs[RESOURCE_MANA] = 0;
			}

			virtual void execute()
			{
				warlock_spell_t::execute();

				p()->resource_gain(RESOURCE_SOUL_SHARD, 1, p()->gains.seed_of_corruption);
				p()->shard_react = p()->sim->current_time;
			}

			virtual void impact(action_state_t* s)
			{
				warlock_spell_t::impact(s);

				corruption->target = s->target;
				corruption->execute();
			}
		};

		struct soulburn_seed_of_corruption_t : public warlock_spell_t
		{
			double coefficient;

			soulburn_seed_of_corruption_t(warlock_t* p, double coeff) :
				warlock_spell_t("soulburn_seed_of_corruption", p, p -> find_spell(114790)), coefficient(coeff)
			{
				may_crit = false;
				background = true;
			}

			virtual void impact(action_state_t* s)
			{
				warlock_spell_t::impact(s);

				if (result_is_hit(s->result))
					td(s->target)->soulburn_soc_trigger = data().effectN(3).average(p()) + s->composite_power() * coefficient;
			}
		};

		struct seed_of_corruption_t : public warlock_spell_t
		{
			soulburn_seed_of_corruption_t* soulburn_spell;

			seed_of_corruption_t(warlock_t* p) :
				warlock_spell_t("seed_of_corruption", p, p -> find_spell(27243)), soulburn_spell(new soulburn_seed_of_corruption_t(p, data().effectN(3).coeff()))
			{
				may_crit = false;

				if (!p->spells.seed_of_corruption_aoe) p->spells.seed_of_corruption_aoe = new seed_of_corruption_aoe_t(p);
				if (!p->spells.soulburn_seed_of_corruption_aoe) p->spells.soulburn_seed_of_corruption_aoe = new soulburn_seed_of_corruption_aoe_t(p);

				add_child(p->spells.seed_of_corruption_aoe);
				soulburn_spell->stats = stats;
			}

			virtual void impact(action_state_t* s)
			{
				warlock_spell_t::impact(s);

				if (result_is_hit(s->result))
					td(s->target)->soc_trigger = data().effectN(3).average(p()) + s->composite_power() * data().effectN(3).coeff();
			}

			virtual void execute()
			{
				if (p()->buffs.soulburn->up())
				{
					p()->buffs.soulburn->expire();
					soulburn_spell->target = target;
					soulburn_spell->time_to_execute = time_to_execute;
					soulburn_spell->execute();
				}
				else
				{
					warlock_spell_t::execute();
				}
			}
		};


		struct rain_of_fire_tick_t : public warlock_spell_t
		{
			const spell_data_t& parent_data;

			rain_of_fire_tick_t(warlock_t* p, const spell_data_t& pd) :
				warlock_spell_t("rain_of_fire_tick", p, pd.effectN(2).trigger()), parent_data(pd)
			{
				aoe = -1;
				background = true;
				direct_tick_callbacks = true;
			}

			virtual void impact(action_state_t* s)
			{
				warlock_spell_t::impact(s);

				if (result_is_hit(s->result))
					trigger_ember_gain(p(), 0.2, p()->gains.rain_of_fire, 0.125);
			}

			virtual double action_multiplier() const
			{
				double m = warlock_spell_t::action_multiplier();

				if (p()->buffs.mannoroths_fury->up())
				{
					m *= 1.0 + p()->talents.mannoroths_fury->effectN(3).percent();
				}
				return m;
			}

			virtual proc_types proc_type() const override
			{
				return PROC1_PERIODIC;
			}
		};


		struct rain_of_fire_t : public warlock_spell_t
		{
			rain_of_fire_t(warlock_t* p) :
				warlock_spell_t("rain_of_fire", p, (p -> specialization() == WARLOCK_DESTRUCTION) ? p -> find_spell(104232) : (p -> specialization() == WARLOCK_AFFLICTION) ? p -> find_spell(5740) : spell_data_t::not_found())
			{
				dot_behavior = DOT_CLIP;
				may_miss = false;
				may_crit = false;
				channeled = (p->spec.aftermath->ok()) ? false : true;
				tick_zero = (p->spec.aftermath->ok()) ? false : true;

				tick_action = new rain_of_fire_tick_t(p, data());
			}

			virtual void tick(dot_t* d)
			{
				warlock_spell_t::tick(d);

				if (channeled && d->current_tick != 0) consume_tick_resource(d);
			}

			virtual int hasted_num_ticks(double /*haste*/, timespan_t /*d*/) const
			{
				return num_ticks;
			}

			virtual double composite_target_ta_multiplier(player_t* t) const
			{
				double m = warlock_spell_t::composite_target_ta_multiplier(t);

				if (td(t)->dots_immolate->ticking)
					m *= 1.5;

				return m;
			}

		};


		struct hellfire_tick_t : public warlock_spell_t
		{
			hellfire_tick_t(warlock_t* p, const spell_data_t& s) :
				warlock_spell_t("hellfire_tick", p, s.effectN(1).trigger())
			{
				aoe = -1;
				background = true;
			}


			virtual void impact(action_state_t* s)
			{
				warlock_spell_t::impact(s);

				if (result_is_hit(s->result))
					p()->resource_gain(RESOURCE_DEMONIC_FURY, 3, gain);
			}
		};


		struct hellfire_t : public warlock_spell_t
		{
			hellfire_t(warlock_t* p) :
				warlock_spell_t(p, "Hellfire")
			{
				tick_zero = true;
				may_miss = false;
				channeled = true;
				may_crit = false;

				tick_power_mod = base_td = 0;

				dynamic_tick_action = true;
				tick_action = new hellfire_tick_t(p, data());
			}

			virtual double action_multiplier() const
			{
				double m = warlock_spell_t::action_multiplier();

				if (p()->buffs.mannoroths_fury->up())
				{
					m *= 1.0 + p()->talents.mannoroths_fury->effectN(3).percent();
				}
				return m;
			}

			virtual bool usable_moving() const
			{
				return true;
			}

			virtual void tick(dot_t* d)
			{
				warlock_spell_t::tick(d);

				if (d->current_tick != 0) consume_tick_resource(d);
			}

			virtual bool ready()
			{
				bool r = warlock_spell_t::ready();

				if (p()->buffs.metamorphosis->check()) r = false;

				return r;
			}

			virtual int hasted_num_ticks(double /*haste*/, timespan_t /*d*/) const
			{
				return num_ticks;
			}
		};


		struct immolation_aura_tick_t : public warlock_spell_t
		{
			immolation_aura_tick_t(warlock_t* p) :
				warlock_spell_t("immolation_aura_tick", p, p -> find_spell(5857))
			{
				aoe = -1;
				background = true;
			}
			virtual double action_multiplier() const
			{
				double m = warlock_spell_t::action_multiplier();

				if (p()->buffs.mannoroths_fury->up())
				{
					m *= 1.0 + p()->talents.mannoroths_fury->effectN(3).percent();
				}
				return m;
			}
		};


		struct immolation_aura_t : public warlock_spell_t
		{
			immolation_aura_t(warlock_t* p) :
				warlock_spell_t(p, "Immolation Aura")
			{
				may_miss = false;
				tick_zero = true;
				may_crit = false;

				dynamic_tick_action = true;
				tick_action = new immolation_aura_tick_t(p);
			}

			virtual void tick(dot_t* d)
			{
				if (!p()->buffs.metamorphosis->check() || p()->resources.current[RESOURCE_DEMONIC_FURY] < META_FURY_MINIMUM)
				{
					d->cancel();
					return;
				}

				warlock_spell_t::tick(d);
			}

			virtual void last_tick(dot_t* d)
			{
				warlock_spell_t::last_tick(d);

				core_event_t::cancel(cost_event);
			}

			virtual void impact(action_state_t* s)
			{
				dot_t* d = get_dot();
				bool add_ticks = d->ticking;
				int remaining_ticks = d->num_ticks - d->current_tick;

				warlock_spell_t::impact(s);

				if (result_is_hit(s->result))
				{
					if (add_ticks)
						d->num_ticks = (int)std::min(remaining_ticks + 0.9 * num_ticks, 2.1 * num_ticks);

					if (!cost_event) cost_event = new (*sim) cost_event_t(p(), this);
				}
			}

			virtual int hasted_num_ticks(double /*haste*/, timespan_t /*d*/) const
			{
				return num_ticks;
			}

			virtual bool ready()
			{
				bool r = warlock_spell_t::ready();

				if (!p()->buffs.metamorphosis->check()) r = false;

				dot_t* d = get_dot();
				if (d->ticking && d->num_ticks - d->current_tick > num_ticks * 1.2)  r = false;

				return r;
			}
		};


		struct carrion_swarm_t : public warlock_spell_t
		{
			carrion_swarm_t(warlock_t* p) :
				warlock_spell_t(p, "Carrion Swarm")
			{
				aoe = -1;
			}

			virtual bool ready()
			{
				bool r = warlock_spell_t::ready();

				if (!p()->buffs.metamorphosis->check()) r = false;

				return r;
			}
		};


		// SOUL SWAP

		struct soul_swap_t : public warlock_spell_t
		{
			agony_t* agony;
			corruption_t* corruption;
			unstable_affliction_t* unstable_affliction;
			seed_of_corruption_t* seed_of_corruption;

			cooldown_t* glyph_cooldown;

			soul_swap_t(warlock_t* p) :
				warlock_spell_t("Soul Swap", p, p->find_spell(86121)),
				agony(new agony_t(p)),
				corruption(new corruption_t(p)),
				unstable_affliction(new unstable_affliction_t(p)),
				seed_of_corruption(new seed_of_corruption_t(p)),
				glyph_cooldown(p -> get_cooldown("glyphed_soul_swap"))
			{
				agony->background = true;
				agony->dual = true;
				agony->base_costs[RESOURCE_MANA] = 0;
				corruption->background = true;
				corruption->dual = true;
				corruption->base_costs[RESOURCE_MANA] = 0;
				unstable_affliction->background = true;
				unstable_affliction->dual = true;
				unstable_affliction->base_costs[RESOURCE_MANA] = 0;
				seed_of_corruption->background = true;
				seed_of_corruption->dual = true;
				seed_of_corruption->base_costs[RESOURCE_MANA] = 0;
			}

			virtual void execute()
			{
				warlock_spell_t::execute();

				if (p()->buffs.soul_swap->up())  // EXHALE: Copy(!) the dots from  p() -> soul_swap_buffer.target to target.
				{
					if (target == p()->soul_swap_buffer.source) return;

					p()->buffs.soul_swap->expire();

					if (p()->soul_swap_buffer.agony_was_inhaled)
					{
						p()->soul_swap_buffer.agony->ticking = true; //so that copy works properly
						p()->soul_swap_buffer.agony->copy(target);
						p()->soul_swap_buffer.agony_was_inhaled = false;
					}

					if (p()->soul_swap_buffer.corruption_was_inhaled)
					{
						p()->soul_swap_buffer.corruption->ticking = true; //so that copy works properly
						p()->soul_swap_buffer.corruption->copy(target);
						p()->soul_swap_buffer.corruption_was_inhaled = false;
					}
					if (p()->soul_swap_buffer.unstable_affliction_was_inhaled)
					{
						p()->soul_swap_buffer.unstable_affliction->ticking = true; //so that copy works properly
						p()->soul_swap_buffer.unstable_affliction->copy(target);
						p()->soul_swap_buffer.unstable_affliction_was_inhaled = false;
					}
					if (p()->soul_swap_buffer.seed_of_corruption_was_inhaled)
					{
						p()->soul_swap_buffer.seed_of_corruption->ticking = true; //so that copy works properly
						p()->soul_swap_buffer.seed_of_corruption->copy(target);
						p()->soul_swap_buffer.seed_of_corruption_was_inhaled = false;
					}
				}
				else if (p()->buffs.soulburn->up()) /// SB:SS, just cast Agony/Corruption/UA no matter what the SS buff is.
				{
					p()->buffs.soulburn->expire();

					agony->target = target;
					agony->execute();

					corruption->target = target;
					corruption->execute();

					unstable_affliction->target = target;
					unstable_affliction->execute();

				}
				else //INHALE : Store SS_target and dot ticking state
				{
					p()->buffs.soul_swap->trigger();

					p()->soul_swap_buffer.source = target;

					if (td(target)->dots_corruption->ticking)
					{
						p()->soul_swap_buffer.corruption_was_inhaled = true; //dot is ticking, so copy the state into our buffer dot
						p()->soul_swap_buffer.inhale_dot(td(target)->dots_corruption, p()->soul_swap_buffer.corruption);
					}

					if (td(target)->dots_unstable_affliction->ticking)
					{
						p()->soul_swap_buffer.unstable_affliction_was_inhaled = true; //dot is ticking, so copy the state into our buffer dot
						p()->soul_swap_buffer.inhale_dot(td(target)->dots_unstable_affliction, p()->soul_swap_buffer.unstable_affliction);
					}

					if (td(target)->dots_agony->ticking)
					{
						p()->soul_swap_buffer.agony_was_inhaled = true; //dot is ticking, so copy the state into our buffer dot
						p()->soul_swap_buffer.inhale_dot(td(target)->dots_agony, p()->soul_swap_buffer.agony);
					}

					if (td(target)->dots_seed_of_corruption->ticking)
					{
						p()->soul_swap_buffer.seed_of_corruption_was_inhaled = true; //dot is ticking, so copy the state into our buffer dot
						p()->soul_swap_buffer.inhale_dot(td(target)->dots_seed_of_corruption, p()->soul_swap_buffer.seed_of_corruption);
					}

				}
			}

			virtual bool ready()
			{
				bool r = warlock_spell_t::ready();

				if (p()->buffs.soul_swap->check())
				{
					if (target == p()->soul_swap_buffer.source) r = false;
				}
				else if (!p()->buffs.soulburn->check())
				{
					if (!td(target)->dots_agony->ticking
						&& !td(target)->dots_corruption->ticking
						&& !td(target)->dots_unstable_affliction->ticking
						&& !td(target)->dots_seed_of_corruption->ticking)
						r = false;
				}

				return r;
			}
		};


		// SUMMONING SPELLS

		struct summon_pet_t : public warlock_spell_t
		{
			timespan_t summoning_duration;
			pets::warlock_pet_t* pet;

		private:
			void _init_summon_pet_t(std::string pet_name)
			{
				harmful = false;

				util::tokenize(pet_name);

				pet = dynamic_cast<pets::warlock_pet_t*>(player->find_pet(pet_name));
				if (!pet)
				{
					sim->errorf("Player %s unable to find pet %s for summons.\n", player->name(), pet_name.c_str());
				}
			}

		public:
			summon_pet_t(const std::string& n, warlock_t* p, const std::string& sname = "") :
				warlock_spell_t(p, sname.empty() ? "Summon " + n : sname),
				summoning_duration(timespan_t::zero()),
				pet(0)
			{
				_init_summon_pet_t(n);
			}

			summon_pet_t(const std::string& n, warlock_t* p, int id) :
				warlock_spell_t(n, p, p -> find_spell(id)),
				summoning_duration(timespan_t::zero()),
				pet(0)
			{
				_init_summon_pet_t(n);
			}

			summon_pet_t(const std::string& n, warlock_t* p, const spell_data_t* sd) :
				warlock_spell_t(n, p, sd),
				summoning_duration(timespan_t::zero()),
				pet(0)
			{
				_init_summon_pet_t(n);
			}

			virtual void execute()
			{
				pet->summon(summoning_duration);

				warlock_spell_t::execute();
			}
		};

		struct summon_main_pet_t : public summon_pet_t
		{
			cooldown_t* instant_cooldown;

			summon_main_pet_t(const std::string& n, warlock_t* p) :
				summon_pet_t(n, p), instant_cooldown(p -> get_cooldown("instant_summon_pet"))
			{
				instant_cooldown->duration = timespan_t::from_seconds(60);
			}

			virtual void schedule_execute(action_state_t* state = 0)
			{
				warlock_spell_t::schedule_execute(state);

				if (p()->pets.active)
				{
					p()->pets.active->dismiss();
					p()->pets.active = 0;
				}
			}

			virtual bool ready()
			{
				if (p()->pets.active == pet)
					return false;

				// FIXME: Currently on beta (2012/05/06) summoning a pet during metamorphosis makes you unable
				//        to summon another one for a full minute, regardless of meta. This may not be intended.
				if ((p()->buffs.soulburn->check() || p()->specialization() == WARLOCK_DEMONOLOGY) && instant_cooldown->down())
					return false;

				return summon_pet_t::ready();
			}

			virtual timespan_t execute_time() const
			{
				if (p()->buffs.soulburn->check() || p()->buffs.demonic_rebirth->check() || p()->buffs.metamorphosis->check())
					return timespan_t::zero();

				return warlock_spell_t::execute_time();
			}

			virtual void execute()
			{
				summon_pet_t::execute();

				p()->pets.active = p()->pets.last = pet;

				if (p()->buffs.soulburn->up())
				{
					instant_cooldown->start();
					p()->buffs.soulburn->expire();
				}

				if (p()->buffs.metamorphosis->check())
					instant_cooldown->start();

				if (p()->buffs.demonic_rebirth->up())
					p()->buffs.demonic_rebirth->expire();

				if (p()->buffs.grimoire_of_sacrifice->check())
					p()->buffs.grimoire_of_sacrifice->expire();
			}
		};

		struct flames_of_xoroth_t : public warlock_spell_t
		{
			gain_t* ember_gain;

			flames_of_xoroth_t(warlock_t* p) :
				warlock_spell_t(p, "Flames of Xoroth"), ember_gain(p -> get_gain("flames_of_xoroth"))
			{
				harmful = false;
			}

			virtual double cost() const
			{
				if (p()->pets.active || p()->buffs.grimoire_of_sacrifice->check())
					return 0;
				else
					return warlock_spell_t::cost();
			}

			virtual void execute()
			{
				warlock_spell_t::execute();

				bool gain_ember = false;

				if (p()->buffs.grimoire_of_sacrifice->check())
				{
					p()->buffs.grimoire_of_sacrifice->expire();
					gain_ember = true;
				}
				else if (p()->pets.active)
				{
					p()->pets.active->dismiss();
					p()->pets.active = 0;
					gain_ember = true;
				}
				else if (p()->pets.last)
				{
					p()->pets.last->summon();
					p()->pets.active = p()->pets.last;
				}

				if (gain_ember) p()->resource_gain(RESOURCE_BURNING_EMBER, 1, ember_gain);
			}
		};

		struct infernal_awakening_t : public warlock_spell_t
		{
			infernal_awakening_t(warlock_t* p, spell_data_t* spell) :
				warlock_spell_t("infernal_awakening", p, spell)
			{
				aoe = -1;
				background = true;
				dual = true;
				trigger_gcd = timespan_t::zero();
			}
		};

		struct summon_infernal_t : public summon_pet_t
		{
			infernal_awakening_t* infernal_awakening;

			summon_infernal_t(warlock_t* p) :
				summon_pet_t(p -> talents.grimoire_of_supremacy -> ok() ? "abyssal" : "infernal", p),
				infernal_awakening(0)
			{
				harmful = false;

				cooldown = p->cooldowns.infernal;
				cooldown->duration = data().cooldown(); // Reset the cooldown duration because we're overriding the duration
				cooldown->duration += timespan_t::from_millis(p->sets.set(SET_T13_2PC_CASTER)->effectN(3).base_value());

				summoning_duration = data().effectN(2).trigger()->duration();
				summoning_duration += (p->specialization() == WARLOCK_DEMONOLOGY) ?
					timespan_t::from_seconds(p->sets.set(SET_T13_2PC_CASTER)->effectN(1).base_value()) :
					timespan_t::from_seconds(p->sets.set(SET_T13_2PC_CASTER)->effectN(2).base_value());

				infernal_awakening = new infernal_awakening_t(p, data().effectN(1).trigger());
				infernal_awakening->stats = stats;
				pet->summon_stats = stats;
			}

			virtual void execute()
			{
				summon_pet_t::execute();

				p()->cooldowns.doomguard->start();
				infernal_awakening->execute();
			}
		};

		struct summon_doomguard2_t : public summon_pet_t
		{
			summon_doomguard2_t(warlock_t* p, spell_data_t* spell) :
				summon_pet_t(p -> talents.grimoire_of_supremacy -> ok() ? "terrorguard" : "doomguard", p, spell)
			{
				harmful = false;
				background = true;
				dual = true;
				callbacks = false;
				summoning_duration = data().duration();
				summoning_duration += (p->specialization() == WARLOCK_DEMONOLOGY ?
					timespan_t::from_seconds(p->sets.set(SET_T13_2PC_CASTER)->effectN(1).base_value()) :
					timespan_t::from_seconds(p->sets.set(SET_T13_2PC_CASTER)->effectN(2).base_value()));
			}
		};

		struct summon_doomguard_t : public warlock_spell_t
		{
			summon_doomguard2_t* summon_doomguard2;

			summon_doomguard_t(warlock_t* p) :
				warlock_spell_t(p, p -> talents.grimoire_of_supremacy -> ok() ? "Summon Terrorguard" : "Summon Doomguard"),
				summon_doomguard2(0)
			{
				cooldown = p->cooldowns.doomguard;
				cooldown->duration = data().cooldown(); // Reset the cooldown duration because we're overriding the duration
				cooldown->duration += timespan_t::from_millis(p->sets.set(SET_T13_2PC_CASTER)->effectN(3).base_value());

				harmful = false;
				summon_doomguard2 = new summon_doomguard2_t(p, data().effectN(2).trigger());
				summon_doomguard2->stats = stats;
				summon_doomguard2->pet->summon_stats = stats;
			}

			virtual void execute()
			{
				warlock_spell_t::execute();

				p()->cooldowns.infernal->start();
				summon_doomguard2->execute();
			}
		};

		// TALENT SPELLS

		struct mortal_coil_heal_t : public warlock_heal_t
		{
			mortal_coil_heal_t(warlock_t* p, const spell_data_t& s) :
				warlock_heal_t("mortal_coil_heal", p, s.effectN(3).trigger_spell_id())
			{
				background = true;
				may_miss = false;
			}

			virtual void execute()
			{
				double heal_pct = data().effectN(1).percent();
				base_dd_min = base_dd_max = player->resources.max[RESOURCE_HEALTH] * heal_pct;

				warlock_heal_t::execute();
			}
		};

		struct mortal_coil_t : public warlock_spell_t
		{
			mortal_coil_heal_t* heal;

			mortal_coil_t(warlock_t* p) :
				warlock_spell_t("mortal_coil", p, p -> talents.mortal_coil), heal(0)
			{
				base_dd_min = base_dd_max = 0;
				heal = new mortal_coil_heal_t(p, data());
			}

			virtual void impact(action_state_t* s)
			{
				warlock_spell_t::impact(s);

				if (result_is_hit(s->result))
					heal->execute();
			}
		};

		struct grimoire_of_sacrifice_t : public warlock_spell_t
		{
			grimoire_of_sacrifice_t(warlock_t* p) :
				warlock_spell_t("grimoire_of_sacrifice", p, p -> talents.grimoire_of_sacrifice)
			{
				harmful = false;
			}

			virtual bool ready()
			{
				if (!p()->pets.active) return false;

				return warlock_spell_t::ready();
			}

			virtual void execute()
			{
				if (p()->pets.active)
				{
					warlock_spell_t::execute();

					p()->pets.active->dismiss();
					p()->pets.active = 0;
					p()->buffs.grimoire_of_sacrifice->trigger();

					// FIXME: Demonic rebirth should really trigger on any pet death, but this is the only pet death we care about for now
					if (p()->spec.demonic_rebirth->ok())
						p()->buffs.demonic_rebirth->trigger();

					// FIXME: Add heal event.
				}
			}
		};


		struct grimoire_of_service_t : public summon_pet_t
		{
			grimoire_of_service_t(warlock_t* p, const std::string& pet_name) :
				summon_pet_t("service_" + pet_name, p, p -> talents.grimoire_of_service -> ok() ? p -> find_class_spell("Grimoire: " + pet_name) : spell_data_t::not_found())
			{
				cooldown = p->get_cooldown("grimoire_of_service");
				cooldown->duration = data().cooldown();
				summoning_duration = data().duration();
				if (pet)
					pet->summon_stats = stats;
			}
		};


		struct mannoroths_fury_t : public warlock_spell_t
		{
			mannoroths_fury_t(warlock_t* p) :
				warlock_spell_t("mannoroths_fury", p, p -> talents.mannoroths_fury)
			{
				harmful = false;
			}

			virtual void execute()
			{
				warlock_spell_t::execute();

				p()->buffs.mannoroths_fury->trigger();
			}
		};


	} // end actions namespace


	warlock_td_t::warlock_td_t(player_t* target, warlock_t* p) :
		actor_pair_t(target, p),
		ds_started_below_20(false),
		shadowflame_stack(1),
		agony_stack(1),
		soc_trigger(0),
		soulburn_soc_trigger(0)
	{
		dots_corruption = target->get_dot("corruption", p);
		dots_unstable_affliction = target->get_dot("unstable_affliction", p);
		dots_agony = target->get_dot("agony", p);
		dots_doom = target->get_dot("doom", p);
		dots_immolate = target->get_dot("immolate", p);
		dots_drain_soul = target->get_dot("drain_soul", p);
		dots_shadowflame = target->get_dot("shadowflame", p);
		dots_malefic_grasp = target->get_dot("malefic_grasp", p);
		dots_seed_of_corruption = target->get_dot("seed_of_corruption", p);
		dots_soulburn_seed_of_corruption = target->get_dot("soulburn_seed_of_corruption", p);
		dots_haunt = target->get_dot("haunt", p);

		debuffs_haunt = buff_creator_t(*this, "haunt", source->find_class_spell("Haunt"));
	}

	warlock_t::warlock_t(sim_t* sim, const std::string& name, race_e r) :
		player_t(sim, WARLOCK, name, r),
		havoc_target(0),
		latest_corruption_target(0),
		pets(pets_t()),
		buffs(buffs_t()),
		cooldowns(cooldowns_t()),
		talents(talents_t()),
		spec(specs_t()),
		mastery_spells(mastery_spells_t()),
		gains(gains_t()),
		procs(procs_t()),
		glyphs(glyphs_t()),
		spells(spells_t()),
		soul_swap_buffer(soul_swap_buffer_t()),
		demonic_calling_event(0),
		initial_burning_embers(1),
		initial_demonic_fury(200),
		default_pet(""),
		ember_react((initial_burning_embers >= 1.0) ? timespan_t::zero() : timespan_t::max()),
		shard_react(timespan_t::zero())
	{
		base.distance = 40;

		cooldowns.infernal = get_cooldown("summon_infernal");
		cooldowns.doomguard = get_cooldown("summon_doomguard");
		cooldowns.imp_swarm = get_cooldown("imp_swarm");
		cooldowns.hand_of_guldan = get_cooldown("hand_of_guldan");
	}

	warlock_t::~warlock_t()
	{
		delete soul_swap_buffer.agony;
		delete soul_swap_buffer.corruption;
		delete soul_swap_buffer.unstable_affliction;
		delete soul_swap_buffer.seed_of_corruption;
	}

	double warlock_t::composite_player_multiplier(school_e school) const
	{
		double m = player_t::composite_player_multiplier(school);

		double mastery_value = mastery_spells.master_demonologist->ok() ? cache.mastery_value() : 0.0;

		if (buffs.metamorphosis->up()) // FIXME: Is corruption an exception, or did they change it so it only applies to a few spells specifically?
		{
			m *= 1.0 + spec.demonic_fury->effectN(1).percent() * 3 + mastery_value * 3;
		}
		else
		{
			m *= 1.0 + mastery_value;
		}

		if (buffs.tier16_2pc_fiery_wrath->up())
		{
			m *= 1.0 + buffs.tier16_2pc_fiery_wrath->value();
		}
		return m;
	}


	void warlock_t::invalidate_cache(cache_e c)
	{
		player_t::invalidate_cache(c);

		switch (c)
		{
		case CACHE_MASTERY:
			if (mastery_spells.master_demonologist->ok())
			{
				player_t::invalidate_cache(CACHE_PLAYER_DAMAGE_MULTIPLIER);
			}
			break;
		default: break;
		}
	}

	double warlock_t::composite_spell_crit() const
	{
		double sc = player_t::composite_spell_crit();

		if (specialization() == WARLOCK_DESTRUCTION)
		{
			if (buffs.dark_soul->up())
			{
				sc += spec.dark_soul->effectN(1).percent();
			}
			if (buffs.tier16_4pc_ember_fillup->up())
			{
				sc += find_spell(145164)->effectN(1).percent();
			}
		}

		return sc;
	}


	double warlock_t::composite_spell_haste() const
	{
		double h = player_t::composite_spell_haste();

		if (specialization() == WARLOCK_AFFLICTION)
		{
			if (buffs.dark_soul->up())
			{
				h *= 1.0 / (1.0 + spec.dark_soul->effectN(1).percent());
			}

		}

		return h;
	}


	double warlock_t::composite_mastery() const
	{
		double m = player_t::composite_mastery();

		if (specialization() == WARLOCK_DEMONOLOGY)
		{
			if (buffs.dark_soul->up())
			{
				m += spec.dark_soul->effectN(1).average(this);
			}

		}

		return m;
	}

	double warlock_t::resource_gain(resource_e resource_type, double amount, gain_t* source, action_t* action)
	{
		if (resource_type == RESOURCE_DEMONIC_FURY)
			amount *= 1.0 + sets.set(SET_T15_4PC_CASTER)->effectN(3).percent();

		return player_t::resource_gain(resource_type, amount, source, action);
	}

	double warlock_t::mana_regen_per_second() const
	{
		double mp5 = player_t::mana_regen_per_second();

		mp5 /= cache.spell_speed();

		return mp5;
	}


	double warlock_t::composite_armor() const
	{
		return player_t::composite_armor() + spec.fel_armor->effectN(2).base_value();
	}


	void warlock_t::halt()
	{
		player_t::halt();

		if (spells.melee) spells.melee->cancel();
	}


	double warlock_t::matching_gear_multiplier(attribute_e attr) const
	{
		if (attr == ATTR_INTELLECT)
			return spec.nethermancy->effectN(1).percent();

		return 0.0;
	}

	static const std::string supremacy_pet(const std::string& pet_name, bool translate = true)
	{
		if (!translate) return pet_name;
		if (pet_name == "felhunter")  return "observer";
		if (pet_name == "felguard")   return "wrathguard";
		if (pet_name == "succubus")   return "shivarra";
		if (pet_name == "voidwalker") return "voidlord";
		if (pet_name == "imp")        return "fel imp";
		return "";
	}

	action_t* warlock_t::create_action(const std::string& action_name,
		const std::string& options_str)
	{
		action_t* a;

		if ((action_name == "summon_pet" || action_name == "service_pet") && default_pet.empty())
		{
			sim->errorf("Player %s used a generic pet summoning action without specifying a default_pet.\n", name());
			return nullptr;
		}

		using namespace actions;

		if (action_name == "conflagrate") a = new           conflagrate_t(this);
		else if (action_name == "corruption") a = new            corruption_t(this);
		else if (action_name == "agony") a = new                 agony_t(this);
		else if (action_name == "doom") a = new                  doom_t(this);
		else if (action_name == "chaos_bolt") a = new            chaos_bolt_t(this);
		else if (action_name == "chaos_wave") a = new            chaos_wave_t(this);
		else if (action_name == "curse_of_the_elements") a = new curse_of_the_elements_t(this);
		else if (action_name == "touch_of_chaos") a = new        touch_of_chaos_t(this);
		else if (action_name == "drain_life") a = new            drain_life_t(this);
		else if (action_name == "drain_soul") a = new            drain_soul_t(this);
		else if (action_name == "grimoire_of_sacrifice") a = new grimoire_of_sacrifice_t(this);
		else if (action_name == "haunt") a = new                 haunt_t(this);
		else if (action_name == "immolate") a = new              immolate_t(this);
		else if (action_name == "incinerate") a = new            incinerate_t(this);
		else if (action_name == "life_tap") a = new              life_tap_t(this);
		else if (action_name == "malefic_grasp") a = new         malefic_grasp_t(this);
		else if (action_name == "metamorphosis") a = new activate_metamorphosis_t(this);
		else if (action_name == "cancel_metamorphosis") a = new  cancel_metamorphosis_t(this);
		else if (action_name == "melee") a = new        activate_melee_t(this);
		else if (action_name == "mortal_coil") a = new           mortal_coil_t(this);
		else if (action_name == "shadow_bolt") a = new           shadow_bolt_t(this);
		else if (action_name == "shadowburn") a = new            shadowburn_t(this);
		else if (action_name == "soul_fire") a = new             soul_fire_t(this);
		else if (action_name == "unstable_affliction") a = new   unstable_affliction_t(this);
		else if (action_name == "hand_of_guldan") a = new        hand_of_guldan_t(this);
		else if (action_name == "fel_flame") a = new             fel_flame_t(this);
		else if (action_name == "void_ray") a = new              void_ray_t(this);
		else if (action_name == "dark_intent") a = new           dark_intent_t(this);
		else if (action_name == "dark_soul") a = new             dark_soul_t(this);
		else if (action_name == "soulburn") a = new              soulburn_t(this);
		else if (action_name == "havoc") a = new                 havoc_t(this);
		else if (action_name == "seed_of_corruption") a = new    seed_of_corruption_t(this);
		else if (action_name == "rain_of_fire") a = new          rain_of_fire_t(this);
		else if (action_name == "hellfire") a = new              hellfire_t(this);
		else if (action_name == "immolation_aura") a = new       immolation_aura_t(this);
		else if (action_name == "carrion_swarm") a = new         carrion_swarm_t(this);
		else if (action_name == "imp_swarm") a = new             imp_swarm_t(this);
		else if (action_name == "fire_and_brimstone") a = new    fire_and_brimstone_t(this);
		else if (action_name == "soul_swap") a = new             soul_swap_t(this);
		else if (action_name == "flames_of_xoroth") a = new      flames_of_xoroth_t(this);
		else if (action_name == "mannoroths_fury") a = new mannoroths_fury_t(this);
		else if (action_name == "summon_infernal") a = new       summon_infernal_t(this);
		else if (action_name == "summon_doomguard") a = new      summon_doomguard_t(this);
		else if (action_name == "summon_felhunter") a = new summon_main_pet_t(supremacy_pet("felhunter", talents.grimoire_of_supremacy->ok()), this);
		else if (action_name == "summon_felguard") a = new summon_main_pet_t(supremacy_pet("felguard", talents.grimoire_of_supremacy->ok()), this);
		else if (action_name == "summon_succubus") a = new summon_main_pet_t(supremacy_pet("succubus", talents.grimoire_of_supremacy->ok()), this);
		else if (action_name == "summon_voidwalker") a = new summon_main_pet_t(supremacy_pet("voidwalker", talents.grimoire_of_supremacy->ok()), this);
		else if (action_name == "summon_imp") a = new summon_main_pet_t(supremacy_pet("imp", talents.grimoire_of_supremacy->ok()), this);
		else if (action_name == "summon_pet") a = new summon_main_pet_t(supremacy_pet(default_pet, talents.grimoire_of_supremacy->ok()), this);
		else if (action_name == "service_felguard") a = new grimoire_of_service_t(this, "felguard");
		else if (action_name == "service_felhunter") a = new grimoire_of_service_t(this, "felhunter");
		else if (action_name == "service_imp") a = new grimoire_of_service_t(this, "imp");
		else if (action_name == "service_succubus") a = new grimoire_of_service_t(this, "succubus");
		else if (action_name == "service_voidwalker") a = new grimoire_of_service_t(this, "voidwalker");
		else if (action_name == "service_pet") a = new grimoire_of_service_t(this, default_pet);
		else return player_t::create_action(action_name, options_str);

		a->parse_options(0, options_str);

		return a;
	}


	pet_t* warlock_t::create_pet(const std::string& pet_name,
		const std::string& /* pet_type */)
	{
		pet_t* p = find_pet(pet_name);

		if (p) return p;

		using namespace pets;

		if (pet_name == "felguard") return new    felguard_pet_t(sim, this);
		if (pet_name == "felhunter") return new   felhunter_pet_t(sim, this);
		if (pet_name == "imp") return new         imp_pet_t(sim, this);
		if (pet_name == "succubus") return new    succubus_pet_t(sim, this);
		if (pet_name == "voidwalker") return new  voidwalker_pet_t(sim, this);
		if (pet_name == "infernal") return new    infernal_pet_t(sim, this);
		if (pet_name == "doomguard") return new   doomguard_pet_t(sim, this);

		if (pet_name == "wrathguard") return new  wrathguard_pet_t(sim, this);
		if (pet_name == "observer") return new    observer_pet_t(sim, this);
		if (pet_name == "fel_imp") return new     fel_imp_pet_t(sim, this);
		if (pet_name == "shivarra") return new    shivarra_pet_t(sim, this);
		if (pet_name == "voidlord") return new    voidlord_pet_t(sim, this);
		if (pet_name == "abyssal") return new     abyssal_pet_t(sim, this);
		if (pet_name == "terrorguard") return new terrorguard_pet_t(sim, this);

		if (pet_name == "service_felguard") return new    felguard_pet_t(sim, this, pet_name);
		if (pet_name == "service_felhunter") return new   felhunter_pet_t(sim, this, pet_name);
		if (pet_name == "service_imp") return new         imp_pet_t(sim, this, pet_name);
		if (pet_name == "service_succubus") return new    succubus_pet_t(sim, this, pet_name);
		if (pet_name == "service_voidwalker") return new  voidwalker_pet_t(sim, this, pet_name);

		return 0;
	}


	void warlock_t::create_pets()
	{
		create_pet("felhunter");
		create_pet("imp");
		create_pet("succubus");
		create_pet("voidwalker");
		create_pet("infernal");
		create_pet("doomguard");

		create_pet("observer");
		create_pet("fel_imp");
		create_pet("shivarra");
		create_pet("voidlord");
		create_pet("abyssal");
		create_pet("terrorguard");

		if (specialization() == WARLOCK_DEMONOLOGY)
		{
			create_pet("felguard");
			create_pet("wrathguard");

			create_pet("service_felguard");

			for (size_t i = 0; i < pets.wild_imps.size(); i++)
			{
				pets.wild_imps[i] = new pets::wild_imp_pet_t(sim, this);
				if (i > 0)
					pets.wild_imps[i]->quiet = 1;
			}
		}

		create_pet("service_felhunter");
		create_pet("service_imp");
		create_pet("service_succubus");
		create_pet("service_voidwalker");
	}



	void warlock_t::init_spells()
	{
		player_t::init_spells();

		// New set bonus system
		static const set_bonus_description_t set_bonuses =
		{
			//  C2P    C4P    M2P    M4P    T2P    T4P    H2P    H4P
			{ 105888, 105787,     0,     0,     0,     0,     0,     0 }, // Tier13
			{ 123136, 123141,     0,     0,     0,     0,     0,     0 }, // Tier14
			{ 138129, 138134,     0,     0,     0,     0,     0,     0 }, // Tier15
			{ 145072, 145091,     0,     0,     0,     0,     0,     0 }, // Tier16
		};
		sets.register_spelldata(set_bonuses);

		// General
		spec.nethermancy = find_spell(86091);
		spec.fel_armor = find_spell(104938);
		spec.pandemic = find_spell(131973);

		spec.dark_soul = find_specialization_spell("Dark Soul: Instability", "dark_soul");
		if (!spec.dark_soul->ok()) spec.dark_soul = find_specialization_spell("Dark Soul: Knowledge", "dark_soul");
		if (!spec.dark_soul->ok()) spec.dark_soul = find_specialization_spell("Dark Soul: Misery", "dark_soul");

		// Affliction
		spec.nightfall = find_specialization_spell("Nightfall");

		// Demonology
		spec.decimation = find_specialization_spell("Decimation");
		spec.demonic_fury = find_specialization_spell("Demonic Fury");
		spec.metamorphosis = find_specialization_spell("Metamorphosis");
		spec.molten_core = find_specialization_spell("Molten Core");
		spec.demonic_rebirth = find_specialization_spell("Demonic Rebirth");
		spec.wild_imps = find_specialization_spell("Wild Imps");

		spec.doom = (find_specialization_spell("Metamorphosis: Doom")->ok()) ? find_spell(603) : spell_data_t::not_found();
		spec.touch_of_chaos = (find_specialization_spell("Metamorphosis: Touch of Chaos")->ok()) ? find_spell(103964) : spell_data_t::not_found();
		spec.chaos_wave = (find_specialization_spell("Metamorphosis: Chaos Wave")->ok()) ? find_spell(124916) : spell_data_t::not_found();

		// Destruction
		spec.aftermath = find_specialization_spell("Aftermath");
		spec.backdraft = find_specialization_spell("Backdraft");
		spec.burning_embers = find_specialization_spell("Burning Embers");
		spec.chaotic_energy = find_specialization_spell("Chaotic Energy");
		spec.fire_and_brimstone = find_specialization_spell("Fire and Brimstone");
		spec.pyroclasm = find_specialization_spell("Pyroclasm");

		mastery_spells.emberstorm = find_mastery_spell(WARLOCK_DESTRUCTION);
		mastery_spells.potent_afflictions = find_mastery_spell(WARLOCK_AFFLICTION);
		mastery_spells.master_demonologist = find_mastery_spell(WARLOCK_DEMONOLOGY);

		talents.soul_leech = find_talent_spell("Soul Leech");
		talents.harvest_life = find_talent_spell("Harvest Life");
		talents.mortal_coil = find_talent_spell("Mortal Coil");
		talents.shadowfury = find_talent_spell("Shadowfury");
		talents.soul_link = find_talent_spell("Soul Link");
		talents.grimoire_of_supremacy = find_talent_spell("Grimoire of Supremacy");
		talents.grimoire_of_service = find_talent_spell("Grimoire of Service");
		talents.grimoire_of_sacrifice = find_talent_spell("Grimoire of Sacrifice");
		talents.archimondes_darkness = find_talent_spell("Archimonde's Darkness");
		talents.kiljaedens_cunning = find_talent_spell("Kil'jaeden's Cunning");
		talents.mannoroths_fury = find_talent_spell("Mannoroth's Fury");

		glyphs.conflagrate = find_glyph_spell("Glyph of Conflagrate");
		glyphs.curse_of_elements = find_glyph_spell("Glyph of Curse of the Elements");
		glyphs.demon_training = find_glyph_spell("Glyph of Demon Training");
		glyphs.havoc = find_glyph_spell("Glyph of Havoc");
		glyphs.life_tap = find_glyph_spell("Glyph of Life Tap");
		glyphs.imp_swarm = find_glyph_spell("Glyph of Imp Swarm");
		glyphs.soul_shards = find_glyph_spell("Glyph of Soul Shards");
		glyphs.burning_embers = find_glyph_spell("Glyph of Burning Embers");
		glyphs.shadow_bolt = find_glyph_spell("Glyph of Shadow Bolt");
		glyphs.siphon_life = find_glyph_spell("Glyph of Siphon Life");
		glyphs.unstable_affliction = find_glyph_spell("Glyph of Unstable Affliction");

		spec.imp_swarm = (glyphs.imp_swarm->ok()) ? find_spell(104316) : spell_data_t::not_found();

		spells.tier15_2pc = find_spell(138483);

		soul_swap_buffer.agony = new dot_t("soul_swap_buffer_agony", this, this);
		soul_swap_buffer.corruption = new dot_t("soul_swap_buffer_corruption", this, this);
		soul_swap_buffer.unstable_affliction = new dot_t("soul_swap_buffer_unstable_affliction", this, this);
		soul_swap_buffer.seed_of_corruption = new dot_t("soul_swap_buffer_seed_of_corruption", this, this);
	}


	void warlock_t::init_base_stats()
	{
		player_t::init_base_stats();

		base.stats.attack_power += -10;
		base.attack_power_per_strength = 2.0;
		base.spell_power_per_intellect = 1.0;

		base.attribute_multiplier[ATTR_STAMINA] *= 1.0 + spec.fel_armor->effectN(1).percent();

		// If we don't have base mp5 defined for this level in extra_data.inc, approximate:
		if (base.mana_regen_per_second == 0) base.mana_regen_per_second = resources.base[RESOURCE_MANA] * 0.01;

		base.mana_regen_per_second *= 1.0 + spec.chaotic_energy->effectN(1).percent();

		if (specialization() == WARLOCK_AFFLICTION)  resources.base[RESOURCE_SOUL_SHARD] = 4;
		if (specialization() == WARLOCK_DEMONOLOGY)  resources.base[RESOURCE_DEMONIC_FURY] = 1000;
		if (specialization() == WARLOCK_DESTRUCTION) resources.base[RESOURCE_BURNING_EMBER] = 4;

		if (default_pet.empty())
		{
			if (specialization() == WARLOCK_DEMONOLOGY)
				default_pet = "felguard";
			else
				default_pet = "felhunter";
		}

		diminished_kfactor = 0.009830;
		diminished_dodge_cap = 0.006650;
		diminished_parry_cap = 0.006650;
	}


	void warlock_t::init_scaling()
	{
		player_t::init_scaling();

		scales_with[STAT_SPIRIT] = 0;
		scales_with[STAT_STAMINA] = 0;
	}


	void warlock_t::create_buffs()
	{
		player_t::create_buffs();


		buffs.backdraft = buff_creator_t(this, "backdraft", spec.backdraft->effectN(1).trigger()).max_stack(6);
		buffs.dark_soul = buff_creator_t(this, "dark_soul", spec.dark_soul).add_invalidate(CACHE_CRIT).add_invalidate(CACHE_HASTE).add_invalidate(CACHE_MASTERY);
		buffs.metamorphosis = buff_creator_t(this, "metamorphosis", spec.metamorphosis).add_invalidate(CACHE_PLAYER_DAMAGE_MULTIPLIER);
		buffs.molten_core = buff_creator_t(this, "molten_core", find_spell(122355)).activated(false).max_stack(10);
		buffs.soulburn = buff_creator_t(this, "soulburn", find_class_spell("Soulburn"));
		buffs.grimoire_of_sacrifice = buff_creator_t(this, "grimoire_of_sacrifice", talents.grimoire_of_sacrifice);
		buffs.demonic_calling = buff_creator_t(this, "demonic_calling", spec.wild_imps->effectN(1).trigger()).duration(timespan_t::zero());
		buffs.fire_and_brimstone = buff_creator_t(this, "fire_and_brimstone", find_class_spell("Fire and Brimstone"))
			.cd(timespan_t::zero());
		buffs.soul_swap = buff_creator_t(this, "soul_swap", find_spell(86211));
		buffs.havoc = buff_creator_t(this, "havoc", find_class_spell("Havoc"))
			.cd(timespan_t::zero());
		buffs.demonic_rebirth = buff_creator_t(this, "demonic_rebirth", find_spell(88448)).cd(find_spell(89140)->duration());
		buffs.mannoroths_fury = buff_creator_t(this, "mannoroths_fury", talents.mannoroths_fury);
		buffs.tier16_4pc_ember_fillup = buff_creator_t(this, "ember_master", find_spell(145164))
			.cd(find_spell(145165)->duration())
			.add_invalidate(CACHE_CRIT);
		buffs.tier16_2pc_destructive_influence = buff_creator_t(this, "destructive_influence", find_spell(145075))
			.chance(sets.set(SET_T16_2PC_CASTER)->effectN(4).percent())
			.duration(timespan_t::from_seconds(10))
			.default_value(find_spell(145075)->effectN(1).percent());

		buffs.tier16_2pc_empowered_grasp = buff_creator_t(this, "empowered_grasp", find_spell(145082))
			.chance(sets.set(SET_T16_2PC_CASTER)->effectN(2).percent())
			.default_value(find_spell(145082)->effectN(2).percent());
		buffs.tier16_2pc_fiery_wrath = buff_creator_t(this, "fiery_wrath", find_spell(145085))
			.chance(sets.set(SET_T16_2PC_CASTER)->effectN(4).percent())
			.duration(timespan_t::from_seconds(10))
			.add_invalidate(CACHE_PLAYER_DAMAGE_MULTIPLIER)
			.default_value(find_spell(145085)->effectN(1).percent());
	}


	void warlock_t::init_gains()
	{
		player_t::init_gains();

		gains.life_tap = get_gain("life_tap");
		gains.soul_leech = get_gain("soul_leech");
		gains.tier13_4pc = get_gain("tier13_4pc");
		gains.nightfall = get_gain("nightfall");
		gains.drain_soul = get_gain("drain_soul");
		gains.incinerate = get_gain("incinerate");
		gains.incinerate_fnb = get_gain("incinerate_fnb");
		gains.incinerate_t15_4pc = get_gain("incinerate_t15_4pc");
		gains.conflagrate = get_gain("conflagrate");
		gains.conflagrate_fnb = get_gain("conflagrate_fnb");
		gains.rain_of_fire = get_gain("rain_of_fire");
		gains.immolate = get_gain("immolate");
		gains.immolate_fnb = get_gain("immolate_fnb");
		gains.fel_flame = get_gain("fel_flame");
		gains.shadowburn = get_gain("shadowburn");
		gains.miss_refund = get_gain("miss_refund");
		gains.siphon_life = get_gain("siphon_life");
		gains.seed_of_corruption = get_gain("seed_of_corruption");
		gains.haunt_tier16_4pc = get_gain("haunt_tier16_4pc");
	}


	void warlock_t::init_benefits()
	{
		player_t::init_benefits();
	}


	void warlock_t::init_procs()
	{
		player_t::init_procs();

		procs.wild_imp = get_proc("wild_imp");
	}

	void warlock_t::apl_precombat()
	{
		std::string& precombat_list =
			get_action_priority_list("precombat")->action_list_str;

		if (sim->allow_flasks)
		{
			// Flask
			if (level > 85)
				precombat_list = "flask,type=warm_sun";
			else if (level >= 80)
				precombat_list = "flask,type=draconic_mind";
		}

		if (sim->allow_food)
		{
			// Food
			if (level >= 80)
			{
				precombat_list += "/food,type=";
				precombat_list +=
					(level > 85) ? "mogu_fish_stew" : "seafood_magnifique_feast";
			}
		}

		add_action("Dark Intent", "if=!aura.spell_power_multiplier.up",
			"precombat");

		precombat_list +=
			"/summon_pet,if=!talent.grimoire_of_sacrifice.enabled|buff.grimoire_of_sacrifice.down";

		precombat_list += "/snapshot_stats";

		precombat_list +=
			"/grimoire_of_sacrifice,if=talent.grimoire_of_sacrifice.enabled";
		precombat_list += "/service_pet,if=talent.grimoire_of_service.enabled";

		if (sim->allow_potions)
		{
			// Pre-potion
			if (level > 85)
				precombat_list += "/jade_serpent_potion";
			else if (level >= 80)
				precombat_list += "/volcanic_potion";
		}

		add_action("Curse of the Elements", "if=debuff.magic_vulnerability.down");

		// Usable Item
		for (int i = as<int>(items.size()) - 1; i >= 0; i--)
		{
			if (items[i].parsed.use.active())
			{
				action_list_str += "/use_item,name=";
				action_list_str += items[i].name();
			}
		}

		if (sim->allow_potions)
		{
			// Potion
			if (level > 85)
				action_list_str +=
				"/jade_serpent_potion,if=buff.bloodlust.react|target.health.pct<=20";
			else if (level > 80)
				action_list_str +=
				"/volcanic_potion,if=buff.bloodlust.react|target.health.pct<=20";
		}

		action_list_str += init_use_profession_actions();
		action_list_str += init_use_racial_actions();
		if (specialization() == WARLOCK_DEMONOLOGY)
		{
			if (spec.imp_swarm->ok())
				action_list_str +=
				"/imp_swarm,if=(buff.dark_soul.up|(cooldown.dark_soul.remains>(120%(1%spell_haste)))|time_to_die<32)&time>3";
		}

		add_action(
			spec.dark_soul,
			"if=!talent.archimondes_darkness.enabled|(talent.archimondes_darkness.enabled&(charges=2|trinket.proc.intellect.react|trinket.stacking_proc.intellect.react|target.health.pct<=10))");

		action_list_str += "/service_pet,if=talent.grimoire_of_service.enabled";

		if (specialization() == WARLOCK_DEMONOLOGY)
		{
			action_list_str += "/felguard:felstorm";
			action_list_str += "/wrathguard:wrathstorm";
		}

		int multidot_max = 3;

		switch (specialization())
		{
		case WARLOCK_AFFLICTION:
			multidot_max = 6;
			break;
		case WARLOCK_DESTRUCTION:
			multidot_max = 3;
			break;
		case WARLOCK_DEMONOLOGY:
			multidot_max = 4;
			break;
		default:
			break;
		}

		action_list_str += "/run_action_list,name=aoe,if=active_enemies>"
			+ util::to_string(multidot_max);

		add_action("Summon Doomguard");
		add_action("Summon Infernal", "", "aoe");

	}

	void warlock_t::apl_global_filler()
	{
		add_action("Life Tap");
	}

	void warlock_t::apl_default()
	{
		add_action(
			"Corruption",
			"if=(!ticking|remains<tick_time)&target.time_to_die>=6&miss_react");
		add_action("Shadow Bolt");

		// AoE action list
		add_action("Corruption", "cycle_targets=1,if=!ticking", "aoe");
		add_action("Shadow Bolt", "", "aoe");
	}

	void warlock_t::apl_affliction()
	{
		bool has_unerring_vision_of_leishen = find_item(
			"unerring_vision_of_lei_shen") != nullptr;

		add_action("Soul Swap", "if=buff.soulburn.up");
		if (has_unerring_vision_of_leishen)
		{
			add_action("Soulburn", "line_cd=5,if=buff.perfect_aim.react");
			add_action(
				"Soulburn",
				"if=(buff.dark_soul.up|trinket.proc.intellect.react|trinket.stacking_proc.intellect.react>6)&(dot.agony.ticks_remain<=action.agony.add_ticks%2|dot.corruption.ticks_remain<=action.corruption.add_ticks%2|dot.unstable_affliction.ticks_remain<=action.unstable_affliction.add_ticks%2)&shard_react&(dot.unstable_affliction.crit_pct<100&dot.corruption.crit_pct<100&dot.agony.crit_pct<100)");
			add_action(
				"Soulburn",
				"if=(dot.unstable_affliction.ticks_remain<=1|dot.corruption.ticks_remain<=1|dot.agony.ticks_remain<=1)&shard_react&target.health.pct<=20&(dot.unstable_affliction.crit_pct<100&dot.corruption.crit_pct<100&dot.agony.crit_pct<100)");
		}
		else
		{
			add_action(
				"Soulburn",
				"if=(buff.dark_soul.up|trinket.proc.intellect.react|trinket.stacking_proc.intellect.react>6)&(dot.agony.ticks_remain<=action.agony.add_ticks%2|dot.corruption.ticks_remain<=action.corruption.add_ticks%2|dot.unstable_affliction.ticks_remain<=action.unstable_affliction.add_ticks%2)&shard_react");
			add_action(
				"Soulburn",
				"if=(dot.unstable_affliction.ticks_remain<=1|dot.corruption.ticks_remain<=1|dot.agony.ticks_remain<=1)&shard_react&target.health.pct<=20");
		}

		//SS Inhale

		add_action(
			"Soul Swap",
			"if=active_enemies>1&buff.soul_swap.down&(buff.dark_soul.up|trinket.proc.intellect.react|trinket.stacking_proc.intellect.react>6)");

		//SS Exhale

		add_action(
			"Soul Swap",
			"cycle_targets=1,if=active_enemies>1&buff.soul_swap.up&(dot.agony.ticks_remain<=action.agony.add_ticks%2|dot.corruption.ticks_remain<=action.corruption.add_ticks%2|dot.unstable_affliction.ticks_remain<=action.unstable_affliction.add_ticks%2)&shard_react");

		add_action(
			"Haunt",
			"if=!in_flight_to_target&remains<cast_time+travel_time+tick_time&shard_react&target.health.pct<=20");
		add_action("Drain Soul", "interrupt=1,chain=1,if=target.health.pct<=20");
		if (spec.pandemic->ok())
		{
			add_action(
				"Haunt",
				"if=!in_flight_to_target&remains<cast_time+travel_time+tick_time&shard_react");

			add_action(
				"Agony",
				"if=(tick_damage*n_ticks*(100+crit_pct_current)>4*dot.agony.tick_dmg*dot.agony.ticks_remain*(100+dot.agony.crit_pct))&miss_react");

			if (has_unerring_vision_of_leishen)
			{

				add_action(
					"Corruption",
					"if=((stat.spell_power>spell_power&ticks_remain<add_ticks%2)|(stat.spell_power>spell_power*1.5)|remains<gcd)&miss_react&dot.corruption.crit_pct<100");
				add_action(
					"Unstable Affliction",
					"if=((stat.spell_power>spell_power&ticks_remain<add_ticks%2)|(stat.spell_power>spell_power*1.5)|remains<cast_time+gcd)&miss_react&dot.unstable_affliction.crit_pct<100");
			}
			else
			{
				add_action(
					"Corruption",
					"if=((stat.spell_power>spell_power&ticks_remain<add_ticks%2)|(stat.spell_power>spell_power*1.5)|remains<gcd)&miss_react");
				add_action(
					"Unstable Affliction",
					"if=((stat.spell_power>spell_power&ticks_remain<add_ticks%2)|(stat.spell_power>spell_power*1.5)|remains<cast_time+gcd)&miss_react");

			}
		}
		else
		{
			add_action("Soulburn", "line_cd=15,if=buff.dark_soul.up&shard_react");
			add_action(
				"Agony",
				"cycle_targets=1,if=(!ticking|remains<=action.drain_soul.new_tick_time*2)&target.time_to_die>=8&miss_react");
			add_action(
				"Corruption",
				"cycle_targets=1,if=(!ticking|remains<tick_time)&target.time_to_die>=6&miss_react");
			add_action(
				"Unstable Affliction",
				"cycle_targets=1,if=(!ticking|remains<(cast_time+tick_time))&target.time_to_die>=5&miss_react");
		}

		add_action("Life Tap",
			"if=buff.dark_soul.down&buff.bloodlust.down&mana.pct<50");
		add_action("Malefic Grasp", "chain=1,interrupt_if=target.health.pct<=20");
		add_action("Life Tap",
			"moving=1,if=mana.pct<80&mana.pct<target.health.pct");
		add_action("Fel Flame", "moving=1");

		// AoE action list
		add_action(
			"Soulburn",
			"cycle_targets=1,if=buff.soulburn.down&!dot.soulburn_seed_of_corruption.ticking&!action.soulburn_seed_of_corruption.in_flight_to_target&shard_react",
			"aoe");
		add_action("Soul Swap",
			"if=buff.soulburn.up&!dot.agony.ticking&!dot.corruption.ticking",
			"aoe");
		add_action(
			"Soul Swap",
			"cycle_targets=1,if=buff.soulburn.up&dot.corruption.ticking&!dot.agony.ticking",
			"aoe");
		add_action(
			"Seed of Corruption",
			"cycle_targets=1,if=(buff.soulburn.down&!in_flight_to_target&!ticking)|(buff.soulburn.up&!dot.soulburn_seed_of_corruption.ticking&!action.soulburn_seed_of_corruption.in_flight_to_target)",
			"aoe");
		add_action(
			"Haunt",
			"cycle_targets=1,if=!in_flight_to_target&debuff.haunt.remains<cast_time+travel_time&shard_react",
			"aoe");
		add_action("Life Tap", "if=mana.pct<70", "aoe");
		add_action("Fel Flame", "cycle_targets=1,if=!in_flight_to_target", "aoe");
	}

	void warlock_t::apl_demonology()
	{
		bool has_unerring_vision_of_leishen = find_item(
			"unerring_vision_of_lei_shen") != nullptr;

		if (has_unerring_vision_of_leishen)
		{
			add_action("Metamorphosis", "if=buff.perfect_aim.react&active_enemies>1");
			add_action(
				spec.doom,
				"cycle_targets=1,if=buff.metamorphosis.up&buff.perfect_aim.react&(crit_pct<100|ticks_remain<=add_ticks)");
			add_action(
				"Soul Fire",
				"if=buff.metamorphosis.up&buff.molten_core.react&(buff.perfect_aim.react&buff.perfect_aim.remains>cast_time)");
			add_action(
				spec.doom,
				"cycle_targets=1,if=buff.metamorphosis.up&(ticks_remain<=1|(ticks_remain+1<n_ticks&buff.dark_soul.up))&target.time_to_die>=30&miss_react&dot.doom.crit_pct<100");
			action_list_str +=
				"/cancel_metamorphosis,if=buff.metamorphosis.up&buff.dark_soul.down&demonic_fury<=650&target.time_to_die>30&(cooldown.metamorphosis.remains<4|demonic_fury<=300)&!(action.hand_of_guldan.in_flight&dot.shadowflame.remains)";
			add_action(
				"Soul Fire",
				"if=buff.metamorphosis.up&buff.molten_core.react&(buff.dark_soul.remains<action.shadow_bolt.cast_time|buff.dark_soul.remains>cast_time)");
			add_action(spec.touch_of_chaos, "if=buff.metamorphosis.up");
			add_action(
				"Hand of Gul'dan",
				"if=buff.perfect_aim.react&buff.perfect_aim.remains>travel_time");
			add_action(
				"Metamorphosis",
				"if=(buff.dark_soul.up&buff.dark_soul.remains<demonic_fury%32)|demonic_fury>=950|demonic_fury%32>target.time_to_die|buff.perfect_aim.react|(action.hand_of_guldan.in_flight&dot.shadowflame.remains)");
			add_action(
				"Corruption",
				"cycle_targets=1,if=buff.perfect_aim.react&(crit_pct<100|ticks_remain<=add_ticks)");
			add_action(
				"Corruption",
				"cycle_targets=1,if=!ticking&target.time_to_die>=6&miss_react");
			add_action(
				"Corruption",
				"cycle_targets=1,if=spell_power<stat.spell_power&ticks_remain<=add_ticks%2&target.time_to_die>=6&miss_react&crit_pct<100");
		}
		else
		{
			add_action(
				spec.doom,
				"cycle_targets=1,if=buff.metamorphosis.up&(ticks_remain<=1|(ticks_remain+1<n_ticks&buff.dark_soul.up)|(ticks_remain<=add_ticks%2&stat.spell_power>spell_power))&target.time_to_die>=30&miss_react");
			action_list_str +=
				"/cancel_metamorphosis,if=buff.metamorphosis.up&buff.dark_soul.down&demonic_fury<=650&target.time_to_die>30&(cooldown.metamorphosis.remains<4|demonic_fury<=300)&!(action.hand_of_guldan.in_flight&dot.shadowflame.remains)";
			add_action(
				"Soul Fire",
				"if=buff.metamorphosis.up&buff.molten_core.react&(buff.dark_soul.remains<action.shadow_bolt.cast_time|buff.dark_soul.remains>cast_time)");
			add_action(spec.touch_of_chaos, "if=buff.metamorphosis.up");
			add_action(
				"Metamorphosis",
				"if=(buff.dark_soul.up&buff.dark_soul.remains<demonic_fury%32)|demonic_fury>=950|demonic_fury%32>target.time_to_die|(action.hand_of_guldan.in_flight&dot.shadowflame.remains)");
			add_action(
				"Corruption",
				"cycle_targets=1,if=!ticking&target.time_to_die>=6&miss_react");
			add_action(
				"Corruption",
				"cycle_targets=1,if=spell_power<stat.spell_power&ticks_remain<=add_ticks%2&target.time_to_die>=6&miss_react");
		}
		add_action(
			"Hand of Gul'dan",
			"if=!in_flight&dot.shadowflame.remains<travel_time+action.shadow_bolt.cast_time&(charges=2|dot.shadowflame.remains>travel_time|(charges=1&recharge_time<4))");
		add_action(
			"Soul Fire",
			"if=buff.molten_core.react&(buff.dark_soul.remains<action.shadow_bolt.cast_time|buff.dark_soul.remains>cast_time)&(buff.molten_core.react>9|target.health.pct<=28)");
		add_action("Life Tap", "if=mana.pct<60");
		add_action("Shadow Bolt");
		add_action("Fel Flame", "moving=1");

		// AoE action list
		if (find_class_spell("Metamorphosis")->ok())
			get_action_priority_list("aoe")->action_list_str +=
			"/cancel_metamorphosis,if=buff.metamorphosis.up&dot.corruption.remains>10&demonic_fury<=650&buff.dark_soul.down&!dot.immolation_aura.ticking";

		add_action("Immolation Aura", "if=buff.metamorphosis.up", "aoe");
		add_action("Void Ray", "if=buff.metamorphosis.up&dot.corruption.remains<10",
			"aoe");
		add_action(
			spec.doom,
			"cycle_targets=1,if=buff.metamorphosis.up&(!ticking|remains<tick_time|(ticks_remain+1<n_ticks&buff.dark_soul.up))&target.time_to_die>=30&miss_react",
			"aoe");
		add_action("Void Ray", "if=buff.metamorphosis.up", "aoe");
		add_action("Corruption",
			"cycle_targets=1,if=!ticking&target.time_to_die>30&miss_react",
			"aoe");
		add_action("Hand of Gul'dan", "", "aoe");
		add_action(
			"Metamorphosis",
			"if=dot.corruption.remains<10|buff.dark_soul.up|demonic_fury>=950|demonic_fury%32>target.time_to_die",
			"aoe");
		add_action("Hellfire", "chain=1,interrupt=1", "aoe");
		add_action("Life Tap", "", "aoe");

	}

	void warlock_t::apl_destruction()
	{
		bool has_unerring_vision_of_leishen = find_item(
			"unerring_vision_of_lei_shen") != nullptr;

		add_action("Rain of Fire", "if=!ticking&!in_flight&active_enemies>1");
		add_action("Havoc", "target=2,if=active_enemies>1");
		if (has_unerring_vision_of_leishen)
			add_action(
				"Shadowburn",
				"if=ember_react&(burning_ember>3.5|mana.pct<=20|target.time_to_die<20|buff.havoc.stack>=1|trinket.proc.intellect.react|(trinket.stacking_proc.intellect.remains<cast_time*4&trinket.stacking_proc.intellect.remains>cast_time)|buff.perfect_aim.react)");
		else
			add_action(
				"Shadowburn",
				"if=ember_react&(burning_ember>3.5|mana.pct<=20|target.time_to_die<20|buff.havoc.stack>=1|trinket.proc.intellect.react|(trinket.stacking_proc.intellect.remains<cast_time*4&trinket.stacking_proc.intellect.remains>cast_time))");
		if (has_unerring_vision_of_leishen)
			add_action(
				"Chaos Bolt",
				"if=ember_react&target.health.pct>20&buff.perfect_aim.react&buff.perfect_aim.remains>cast_time");
		if (spec.pandemic->ok())
		{
			add_action(
				"Immolate",
				"cycle_targets=1,if=n_ticks*crit_pct_current>3*dot.immolate.ticks_remain*dot.immolate.crit_pct&miss_react");
		}
		else
			add_action(
				"Immolate",
				"cycle_targets=1,if=(!ticking|remains<(action.incinerate.cast_time+cast_time))&target.time_to_die>=5&miss_react");
		add_action("Conflagrate", "if=charges=2&buff.havoc.stack=0");
		add_action("Rain of Fire", "if=!ticking&!in_flight,moving=1");
		add_action(
			"Chaos Bolt",
			"if=ember_react&target.health.pct>20&(buff.backdraft.stack<3|level<86|(active_enemies>1&action.incinerate.cast_time<1))&(burning_ember>(4.5-active_enemies)|buff.skull_banner.remains>cast_time|(trinket.proc.intellect.react&trinket.proc.intellect.remains>cast_time)|(trinket.stacking_proc.intellect.remains<cast_time*2.5&trinket.stacking_proc.intellect.remains>cast_time))");
		add_action(
			"Chaos Bolt",
			"if=ember_react&target.health.pct>20&(buff.havoc.stack=3&buff.havoc.remains>cast_time)");
		add_action("Conflagrate");
		add_action("Incinerate");
		add_action("Fel Flame", "moving=1");

		// AoE action list
		add_action("Rain of Fire", "if=!ticking&!in_flight", "aoe");
		add_action("Fire and Brimstone",
			"if=ember_react&buff.fire_and_brimstone.down", "aoe");
		add_action("Immolate", "if=buff.fire_and_brimstone.up&!ticking", "aoe");
		add_action("Conflagrate", "if=buff.fire_and_brimstone.up", "aoe");
		add_action("Incinerate", "if=buff.fire_and_brimstone.up", "aoe");
		add_action("Incinerate", "", "aoe");

	}

	void warlock_t::init_action_list()
	{
		if (action_list_str.empty())
		{
			clear_action_priority_lists();

			apl_precombat();

			switch (specialization())
			{
			case WARLOCK_AFFLICTION:
				apl_affliction();
				break;
			case WARLOCK_DESTRUCTION:
				apl_destruction();
				break;
			case WARLOCK_DEMONOLOGY:
				apl_demonology();
				break;
			default:
				apl_default();
				break;
			}

			apl_global_filler();

			use_default_action_list = true;
		}

		player_t::init_action_list();
	}

	void warlock_t::init_resources(bool force)
	{
		player_t::init_resources(force);

		if (pets.active)
			pets.active->init_resources(force);
	}

	void warlock_t::combat_begin()
	{
		player_t::combat_begin();

		resources.current[RESOURCE_BURNING_EMBER] = initial_burning_embers;
		resources.current[RESOURCE_DEMONIC_FURY] = initial_demonic_fury;

		if (specialization() == WARLOCK_DEMONOLOGY)
		{
			buffs.demonic_calling->trigger();
			demonic_calling_event = new (*sim) demonic_calling_event_t(this, rng().range(timespan_t::zero(),
				timespan_t::from_seconds((spec.wild_imps->effectN(1).period().total_seconds() + spec.imp_swarm->effectN(3).base_value()) * composite_spell_speed())));
		}
	}


	void warlock_t::reset()
	{
		player_t::reset();

		for (size_t i = 0; i < sim->actor_list.size(); i++)
		{
			warlock_td_t* td = target_data[sim->actor_list[i]];
			if (td) td->reset();
		}

		pets.active = 0;
		ember_react = (initial_burning_embers >= 1.0) ? timespan_t::zero() : timespan_t::max();
		shard_react = timespan_t::zero();
		core_event_t::cancel(demonic_calling_event);
		havoc_target = 0;
	}


	void warlock_t::create_options()
	{
		player_t::create_options();

		option_t warlock_options[] =
		{
		  opt_int("burning_embers", initial_burning_embers),
		  opt_int("demonic_fury", initial_demonic_fury),
		  opt_string("default_pet", default_pet),
		  opt_null()
		};

		option_t::copy(options, warlock_options);
	}


	bool warlock_t::create_profile(std::string& profile_str, save_e stype, bool save_html)
	{
		player_t::create_profile(profile_str, stype, save_html);

		if (stype == SAVE_ALL)
		{
			if (initial_burning_embers != 1) profile_str += "burning_embers=" + util::to_string(initial_burning_embers) + "\n";
			if (initial_demonic_fury != 200) profile_str += "demonic_fury=" + util::to_string(initial_demonic_fury) + "\n";
			if (!default_pet.empty())       profile_str += "default_pet=" + default_pet + "\n";
		}

		return true;
	}


	void warlock_t::copy_from(player_t* source)
	{
		player_t::copy_from(source);

		warlock_t* p = debug_cast<warlock_t*>(source);

		initial_burning_embers = p->initial_burning_embers;
		initial_demonic_fury = p->initial_demonic_fury;
		default_pet = p->default_pet;
	}


	set_e warlock_t::decode_set(const item_t& item) const
	{
		if (item.slot != SLOT_HEAD &&
			item.slot != SLOT_SHOULDERS &&
			item.slot != SLOT_CHEST &&
			item.slot != SLOT_HANDS &&
			item.slot != SLOT_LEGS)
		{
			return SET_NONE;
		}

		const char* s = item.name();

		if (strstr(s, "_of_the_faceless_shroud")) return SET_T13_CASTER;

		if (strstr(s, "shaskin_")) return SET_T14_CASTER;

		if (strstr(s, "_of_the_thousandfold_hells")) return SET_T15_CASTER;

		if (strstr(s, "_of_the_horned_nightmare")) return SET_T16_CASTER;

		if (strstr(s, "_gladiators_felweave_")) return SET_PVP_CASTER;

		return SET_NONE;
	}

	expr_t* warlock_t::create_expression(action_t* a, const std::string& name_str)
	{
		if (name_str == "ember_react")
		{
			struct ember_react_expr_t : public expr_t
			{
				warlock_t& player;
				ember_react_expr_t(warlock_t& p) :
					expr_t("ember_react"), player(p) { }
				virtual double evaluate() { return player.resources.current[RESOURCE_BURNING_EMBER] >= 1 && player.sim->current_time >= player.ember_react; }
			};
			return new ember_react_expr_t(*this);
		}
		else if (name_str == "shard_react")
		{
			struct shard_react_expr_t : public expr_t
			{
				warlock_t& player;
				shard_react_expr_t(warlock_t& p) :
					expr_t("shard_react"), player(p) { }
				virtual double evaluate() { return player.resources.current[RESOURCE_SOUL_SHARD] >= 1 && player.sim->current_time >= player.shard_react; }
			};
			return new shard_react_expr_t(*this);
		}
		else if (name_str == "felstorm_is_ticking")
		{
			struct felstorm_is_ticking_expr_t : public expr_t
			{
				pets::warlock_pet_t* felguard;
				felstorm_is_ticking_expr_t(pets::warlock_pet_t* f) :
					expr_t("felstorm_is_ticking"), felguard(f) { }
				virtual double evaluate() { return (felguard) ? felguard->special_action->get_dot()->ticking : false; }
			};
			return new felstorm_is_ticking_expr_t(debug_cast<pets::warlock_pet_t*>(find_pet("felguard")));
		}
		else
		{
			return player_t::create_expression(a, name_str);
		}
	}


	// WARLOCK MODULE INTERFACE =================================================

	struct warlock_module_t : public module_t
	{
		warlock_module_t() : module_t(WARLOCK) {}

		virtual player_t* create_player(sim_t* sim, const std::string& name, race_e r = RACE_NONE) const
		{
			return new warlock_t(sim, name, r);
		}
		virtual bool valid() const { return true; }
		virtual void init(sim_t*) const {}
		virtual void combat_begin(sim_t*) const {}
		virtual void combat_end(sim_t*) const {}
	};

} // end unnamed namespace

const module_t* module_t::warlock()
{
	static warlock_module_t m;
	return &m;
}
