#include "gameplay_ability_system.h"
#include "gameplay_ability.h"
#include "gameplay_attribute.h"
#include "gameplay_effect.h"
#include "gameplay_effect_magnitude.h"
#include "gameplay_tags.h"

#include <core/os/input.h>
#include <core/os/input_event.h>
#include <scene/resources/packed_scene.h>

#include <algorithm>
#include <limits>
#include <numeric>

namespace {
constexpr auto gameplay_cue_activated = "gameplay_cue_activated";
constexpr auto gameplay_cue_removed = "gameplay_cue_removed";

constexpr auto gameplay_effect_activated = "gameplay_effect_activated";
constexpr auto gameplay_effect_infliction_failed = "gameplay_effect_infliction_failed";
constexpr auto gameplay_effect_removal_failed = "gameplay_effect_removal_failed";
constexpr auto gameplay_effect_ended = "gameplay_effect_ended";

constexpr auto gameplay_ability_activated = "gameplay_ability_activated";
constexpr auto gameplay_ability_cancelled = "gameplay_ability_cancelled";
constexpr auto gameplay_ability_blocked = "gameplay_ability_blocked";
constexpr auto gameplay_ability_ready = "gameplay_ability_ready";

constexpr auto gameplay_attribute_changed = "gameplay_attribute_changed";
constexpr auto gameplay_base_attribute_changed = "gameplay_base_attribute_changed";
} // namespace

void GameplayEvent::set_event_tag(const String &value) {
	event_tag = value;
}

const String &GameplayEvent::get_event_tag() const {
	return event_tag;
}

void GameplayEvent::add_event_target(Node *target) {
	if (auto system = dynamic_cast<GameplayAbilitySystem *>(target)) {
		event_targets.push_back(target);
	}
}

const Array &GameplayEvent::get_event_targets() const {
	return event_targets;
}

void GameplayEvent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_event_tag", "value"), &GameplayEvent::set_event_tag);
	ClassDB::bind_method(D_METHOD("get_event_tag"), &GameplayEvent::get_event_tag);
	ClassDB::bind_method(D_METHOD("add_event_target", "value"), &GameplayEvent::add_event_target);
	ClassDB::bind_method(D_METHOD("get_event_targets"), &GameplayEvent::get_event_targets);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "event_tag"), "set_event_tag", "get_event_tag");
}

void GameplayEffectNode::initialise(GameplayAbilitySystem *source, GameplayAbilitySystem *target, const Ref<GameplayEffect> &effect, int64_t level, double normalised_level) {
	this->source = source;
	this->target = target;
	this->effect = effect;
	this->level = level;
	this->normalised_level = normalised_level;
}

Node *GameplayEffectNode::get_source() const {
	return source;
}

Node *GameplayEffectNode::get_target() const {
	return target;
}

Ref<GameplayEffect> GameplayEffectNode::get_effect() const {
	return effect;
}

double GameplayEffectNode::get_duration() const {
	return duration;
}

int64_t GameplayEffectNode::get_stacks() const {
	if (auto system = get_stacking_system()) {
		auto effect_name = effect->get_effect_name();

		if (system->effect_stacking.has(effect_name)) {
			return system->effect_stacking.get(effect_name).stacks;
		} else {
			return 1;
		}
	} else {
		return internal_stacks;
	}
}

int64_t GameplayEffectNode::get_level() const {
	return level;
}

double GameplayEffectNode::get_normalised_level() const {
	return normalised_level;
}

void GameplayEffectNode::add_stack(int64_t value) {
	using ActiveEffectEntry = GameplayAbilitySystem::ActiveEffectEntry;

	if (value <= 0) {
		if (value < 0) {
			remove_stack(-value);
		}
	} else if (auto system = get_stacking_system()) {
		auto effect_name = effect->get_effect_name();

		if (system->effect_stacking.has(effect_name)) {
			auto &&effect_entry = system->effect_stacking.get(effect_name);
			auto current_stacks = effect_entry.stacks;
			auto stacks = current_stacks + value;
			stack_overflow = stacks > effect->get_maximum_stacks();

			if (!stack_applied) {
				previous_stack = current_stacks;
			}
			if (stack_overflow) {
				stacks = effect->get_maximum_stacks();
			}

			effect_entry.stacks = stacks;
		} else {
			system->effect_stacking.set(effect_name, ActiveEffectEntry{ this, level, value });
		}

		stack_applied = true;
	}
}

void GameplayEffectNode::remove_stack(int64_t value) {
	if (value <= 0) {
		if (value < 0) {
			add_stack(-value);
		}
	} else if (auto system = get_stacking_system()) {
		auto effect_name = effect->get_effect_name();
		auto has_effect = system->effect_stacking.has(effect_name);

		if (has_effect) {
			auto &&effect_entry = system->effect_stacking.get(effect_name);
			auto current_stacks = effect_entry.stacks;
			auto stacks = current_stacks - value;

			if (!stack_applied) {
				previous_stack = current_stacks;
			}

			effect_entry.stacks = stacks;
		} else {
			WARN_PRINT("No effect present to remove.");
		}

		stack_applied = true;
	} else {
		internal_stacks = 0;
		stack_applied = true;
	}
}

void GameplayEffectNode::effect_process(double delta) {
	bool duration_refreshed = false;

	if (effect->get_duration_type() == DurationType::Instant) {
		queue_delete();
		return;
	}
	if (effect->get_duration_type() == DurationType::HasDuration) {
		duration -= delta;

		if (duration <= 0) {
			switch (effect->get_stack_expiration()) {
				case StackExpiration::ClearStack: {
					apply_effects(effect->get_normal_expiration_effects());
					end_effect(false);
				} break;
				case StackExpiration::RefreshDuration: {
					duration = calculate_duration();
					duration_refreshed = true;
				} break;
				case StackExpiration::RemoveSingleStackAndRefreshDuration: {
					duration = calculate_duration();
					duration_refreshed = true;
					remove_stack(1);
				} break;
				default: {
				} break;
			}
		}
	}
	if (stack_applied) {
		auto stacks = get_stacks();

		if (stacks <= 0) {
			if (effect->get_duration_type() == DurationType::HasDuration) {
				end_effect(!duration_refreshed && duration > 0);
			} else {
				end_effect(true);
			}
		} else if (previous_stack != stacks) {
			if (previous_stack < stacks) {
				execute_effect();
			}
			if (effect->get_duration_refresh() == StackDurationRefresh::OnApplication) {
				duration = calculate_duration();
			}
			if (effect->get_period_reset() == StackPeriodReset::OnApplication) {
				period = 0;
			}
		}
	}
	if (effect->get_period().is_valid()) {
		auto threshold = calculate_period_threshold();
		period += delta;

		if (period >= threshold) {
			period -= threshold;
			execute_effect();
		}
	}
	if (stack_overflow) {
		apply_effects(effect->get_overflow_effects());

		if (effect->get_clear_overflow_stack()) {
			remove_stack(get_stacks());
		}
	}

	stack_overflow = false;
	stack_applied = false;
}

void GameplayEffectNode::set_effect_process(bool value) {
	should_effect_process = value;
}

void GameplayEffectNode::_notification(int notification) {
	GameplayNode::_notification(notification);

	switch (notification) {
		case NOTIFICATION_PARENTED: {
			start_effect();
		} break;
		case NOTIFICATION_INTERNAL_PROCESS: {
			if (should_effect_process) {
				auto delta = get_process_delta_time();
				effect_process(delta);
			}
		} break;
		default: {
		} break;
	}
}

double GameplayEffectNode::calculate_duration() const {
	auto duration_magnitude = effect->get_duration_magnitude();

	if (duration_magnitude.is_valid()) {
		return duration_magnitude->calculate_magnitude(source, target, effect, level, normalised_level);
	} else {
		return 0;
	}
}

double GameplayEffectNode::calculate_period_threshold() const {
	auto period_magnitude = effect->get_period();

	if (period_magnitude.is_valid()) {
		return period_magnitude->calculate_magnitude(source, target, effect, level, normalised_level);
	} else {
		return 0;
	}
}

void GameplayEffectNode::apply_effect(const Ref<GameplayEffect> &effect) {
	if (!is_queued_for_deletion()) {
		target->apply_effect(source, effect, 1, level, normalised_level);
	}
}

void GameplayEffectNode::apply_effects(const Array &effects) {
	if (!is_queued_for_deletion()) {
		target->apply_effects(source, effects, 1, level, normalised_level);
	}
}

void GameplayEffectNode::execute_effect() {
	if (!is_queued_for_deletion()) {
		if (target->get_active_tags()->has_all(effect->get_ongoing_tags())) {
			target->execute_effect(this);
		}
	}
}

GameplayAbilitySystem *GameplayEffectNode::get_stacking_system() const {
	switch (effect->get_stacking_type()) {
		case StackingType::AggregateOnSource: {
			return source;
		} break;
		case StackingType::AggregateOnTarget: {
			return target;
		} break;
		default: {
			return nullptr;
		} break;
	}
}

void GameplayEffectNode::start_effect() {
	switch (effect->get_duration_type()) {
		case DurationType::Instant: {
			execute_effect();
			queue_delete();
			return; // Remove instant effects immediately.
		} break;
		case DurationType::HasDuration: {
			ERR_FAIL_COND(effect->get_duration_magnitude().is_null());
			duration = calculate_duration();
		} break;
		default: {
		} break;
	}

	// Tags
	target->add_tags(effect->get_target_tags());

	// Abilities
	for (Ref<PackedScene> packed_scene : effect->get_granted_abilities()) {
		if (auto node = packed_scene->instance()) {
			if (auto ability = dynamic_cast<GameplayAbility *>(node)) {
				granted_abilities.push_back(ability);
				target->add_ability(ability);
			}
		}
	}

	// Period execution
	if (effect->get_period().is_valid() && effect->get_execute_period_on_application()) {
		execute_effect();
	} else if (effect->get_duration_type() == DurationType::Infinite) {
		execute_effect();
	}

	target->active_effects.push_back(this);
	target->emit_signal(gameplay_effect_activated, target, effect);
}

void GameplayEffectNode::end_effect(bool cancelled) {
	// Abilities
	for (auto ability : granted_abilities) {
		target->remove_ability(ability);
	}

	// Expiration Effects
	if (cancelled) {
		apply_effects(effect->get_premature_expiration_effects());
	} else {
		apply_effects(effect->get_normal_expiration_effects());
	}

	// Tags
	target->remove_tags(effect->get_target_tags());

	// Reset Duration
	duration = 0;

	// Signal
	target->emit_signal(gameplay_effect_ended, target, effect, cancelled);

	// Purge
	target->active_effects.erase(this);
	queue_delete();
}

void GameplayEffectNode::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_source"), &GameplayEffectNode::get_source);
	ClassDB::bind_method(D_METHOD("get_target"), &GameplayEffectNode::get_target);
	ClassDB::bind_method(D_METHOD("get_effect"), &GameplayEffectNode::get_effect);
	ClassDB::bind_method(D_METHOD("get_duration"), &GameplayEffectNode::get_duration);
	ClassDB::bind_method(D_METHOD("get_stacks"), &GameplayEffectNode::get_stacks);
	ClassDB::bind_method(D_METHOD("get_level"), &GameplayEffectNode::get_level);
	ClassDB::bind_method(D_METHOD("get_normalised_level"), &GameplayEffectNode::get_normalised_level);
	ClassDB::bind_method(D_METHOD("effect_process", "delta"), &GameplayEffectNode::effect_process);
	ClassDB::bind_method(D_METHOD("set_effect_process", "value"), &GameplayEffectNode::set_effect_process);
}

GameplayAbilitySystem::GameplayAbilitySystem() {
	/** Server Methods */
	rpc_config("server_activate_ability", MultiplayerAPI::RPC_MODE_MASTER);
	rpc_config("server_apply_effect", MultiplayerAPI::RPC_MODE_MASTER);
	rpc_config("server_remove_effect", MultiplayerAPI::RPC_MODE_MASTER);
	/** Client Methods */
	rpc_config("client_activate_ability", MultiplayerAPI::RPC_MODE_PUPPET);
	rpc_config("client_apply_effect", MultiplayerAPI::RPC_MODE_PUPPET);
	rpc_config("client_remove_effect", MultiplayerAPI::RPC_MODE_PUPPET);
	rpc_config("client_update_attribute", MultiplayerAPI::RPC_MODE_PUPPET);
	rpc_config("client_ability_activated", MultiplayerAPI::RPC_MODE_PUPPET);
	rpc_config("client_ability_blocked", MultiplayerAPI::RPC_MODE_PUPPET);
	rpc_config("client_effect_activated", MultiplayerAPI::RPC_MODE_PUPPET);
	rpc_config("client_infliction_failed", MultiplayerAPI::RPC_MODE_PUPPET);
	rpc_config("client_update_attributes", MultiplayerAPI::RPC_MODE_PUPPET);
	/** Synchronised Methods */
	rpc_config("sync_apply_cue", MultiplayerAPI::RPC_MODE_REMOTESYNC);
	rpc_config("sync_remove_cue", MultiplayerAPI::RPC_MODE_REMOTESYNC);
}

const Ref<GameplayAttributeSet> &GameplayAbilitySystem::get_attributes() const {
	return attributes;
}

Ref<GameplayTagContainer> GameplayAbilitySystem::get_active_tags() const {
	return active_tags;
}

GameplayAbility *GameplayAbilitySystem::get_ability_by_name(const StringName &name) const {
	auto ability = std::find_if(begin(abilities), end(abilities), [&](GameplayAbility *ability) {
		return ability->get_ability_name() == name;
	});
	return ability != end(abilities) ? *ability : nullptr;
}

GameplayAbility *GameplayAbilitySystem::get_ability_by_index(int64_t index) const {
	return index < 0 && index >= abilities.size() ? nullptr : abilities[index];
}

int64_t GameplayAbilitySystem::get_ability_count() const {
	return abilities.size();
}

Array GameplayAbilitySystem::get_active_abilities() const {
	Array result;
	for (auto &&active_ability : active_abilities) {
		result.push_back(active_ability);
	}
	return result;
}

const Vector<GameplayAbility *> &GameplayAbilitySystem::get_abilities_vector() const {
	return abilities;
}

const Vector<GameplayAbility *> &GameplayAbilitySystem::get_active_abilities_vector() const {
	return active_abilities;
}

const Ref<GameplayTagContainer> &GameplayAbilitySystem::get_persistent_cues() const {
	return persistent_cues;
}

Array GameplayAbilitySystem::query_active_effects_by_tag(const String &tag) const {
	Array result;

	for (auto effect_node : active_effects) {
		auto &&effect = effect_node->get_effect();

		if (!effect_node->is_queued_for_deletion() && effect->get_effect_tags()->has_tag(tag)) {
			result.append(effect_node);
		}
	}

	return result;
}

Array GameplayAbilitySystem::query_active_effects(const Ref<GameplayTagContainer> &tags) const {
	Array result;

	for (auto effect_node : active_effects) {
		auto &&effect = effect_node->get_effect();

		if (!effect_node->is_queued_for_deletion() && effect->get_effect_tags()->has_any(tags)) {
			result.append(effect_node);
		}
	}

	return result;
}

double GameplayAbilitySystem::get_remaining_effect_duration(const Ref<GameplayEffect> &effect) const {
	if (effect.is_null()) {
		return 0;
	}

	auto effect_node = std::find_if(begin(active_effects), end(active_effects), [&effect](GameplayEffectNode *node) {
		return node->get_effect()->get_effect_name() == effect->get_effect_name();
	});

	return effect_node != end(active_effects) ? (*effect_node)->get_duration() : 0.0;
}

bool GameplayAbilitySystem::handle_event(const Ref<GameplayEvent> &event) {
	if (event.is_null()) {
		return false;
	}

	auto result = false;

	for (auto ability : abilities) {
		if (ability->is_active()) {
			ability->process_wait(WaitType::Event, event->get_event_tag());
		} else if (ability->can_trigger(event->get_event_tag(), AbilityTrigger::GampeplayEvent)) {
			ability->targets = event->get_event_targets();
			result = ability->try_activate_ability() || result;
		}
	}

	return result;
}

bool GameplayAbilitySystem::has_attribute(const StringName &name) const {
	return attributes->has_attribute(name);
}

Ref<GameplayAttribute> GameplayAbilitySystem::get_attribute(const StringName &name) const {
	return attributes->has_attribute(name) ? attributes->get_attribute(name) : Ref<GameplayAttribute>();
}

Ref<GameplayAttributeData> GameplayAbilitySystem::get_attribute_data(const StringName &name) const {
	return attributes->has_attribute(name) ? get_attribute(name)->get_attribute_data() : Ref<GameplayAttributeData>();
}

double GameplayAbilitySystem::get_base_attribute_value(const StringName &name) const {
	return get_attribute_data(name)->get_base_value();
}

double GameplayAbilitySystem::get_current_attribute_value(const StringName &name) const {
	return get_attribute_data(name)->get_current_value();
}

bool GameplayAbilitySystem::update_base_attribute(const StringName &name, double value, UpdateAttributeOperation::Type operation /*= UpdateAttributeOperation::None*/) {
	if (has_attribute(name)) {
		auto attribute = get_attribute_data(name);
		auto old_base = attribute->get_base_value();
		auto old_value = attribute->get_current_value();

		switch (operation) {
			case UpdateAttributeOperation::None: {
				attribute->set_base_value(value);
			} break;
			case UpdateAttributeOperation::Relative: {
				auto factor = old_value / old_base;
				attribute->set_base_value(value);
				attribute->set_current_value(old_value * factor);
			} break;
			case UpdateAttributeOperation::Absolute: {
				auto delta = old_value - old_base;
				attribute->set_base_value(value);
				attribute->set_current_value(old_value + delta);
			} break;
			case UpdateAttributeOperation::Override: {
				attribute->set_base_value(value);
				attribute->set_current_value(value);
			} break;
			default: {
			} break;
		}

		for (auto &&ability : abilities) {
			if (ability->is_active()) {
				ability->process_wait(WaitType::BaseAttributeChanged, name);
			}
		}

		emit_signal(gameplay_base_attribute_changed, this, attribute, old_base, old_value);
		return true;
	} else {
		return false;
	}
}

void GameplayAbilitySystem::add_tag(const String &tag) {
	active_tags->append(tag);

	for (auto &&ability : abilities) {
		if (ability->is_active()) {
			ability->process_wait(WaitType::TagAdded, tag);
		} else if (ability->can_trigger(tag, AbilityTrigger::OwnedTagAdded)) {
			ability->targets = targets;
			ability->activate_ability();
		}
	}
}

void GameplayAbilitySystem::add_tags(const Ref<GameplayTagContainer> &tags) {
	active_tags->append_tags(tags);

	for (auto &&ability : abilities) {
		if (ability->get_triggers().empty()) {
			continue;
		}

		for (auto &&tag : tags) {
			if (ability->is_active()) {
				ability->process_wait(WaitType::TagAdded, tag);
			} else if (ability->can_trigger(tag, AbilityTrigger::OwnedTagAdded)) {
				ability->targets = targets;
				ability->activate_ability();
			}
		}
	}
}

void GameplayAbilitySystem::remove_tag(const String &tag) {
	active_tags->remove(tag);

	for (auto &&ability : abilities) {
		if (ability->is_active()) {
			ability->process_wait(WaitType::TagRemoved, tag);
		} else if (ability->can_trigger(tag, AbilityTrigger::OwnedTagRemoved)) {
			ability->targets = targets;
			ability->activate_ability();
		}
	}
}

void GameplayAbilitySystem::remove_tags(const Ref<GameplayTagContainer> &tags) {
	active_tags->remove_tags(tags);

	for (auto ability : abilities) {
		for (auto &&tag : tags) {
			if (ability->is_active()) {
				ability->process_wait(WaitType::TagRemoved, tag);
			} else if (ability->can_trigger(tag, AbilityTrigger::OwnedTagRemoved)) {
				ability->targets = targets;
				ability->activate_ability();
			}
		}
	}
}

void GameplayAbilitySystem::add_ability(Node *node) {
	if (auto ability = dynamic_cast<GameplayAbility *>(node)) {
		abilities.push_back(ability);
		ability->initialise(this);

		if (ability->get_parent() != this) {
			call_deferred("add_child", node);
		}
	}
}

void GameplayAbilitySystem::add_abilities(const Array &abilities) {
	for (Node *node : abilities) {
		add_ability(node);
	}
}

void GameplayAbilitySystem::remove_ability(Node *node) {
	if (auto ability = dynamic_cast<GameplayAbility *>(node)) {
		auto index = abilities.find(ability);

		if (index >= 0) {
			abilities.remove(index);

			if (ability->is_active()) {
				active_abilities.erase(ability);
			}

			ability->queue_delete();
		}
	}
}

void GameplayAbilitySystem::remove_abilities(const Array &abilities) {
	for (Node *ability : abilities) {
		remove_ability(ability);
	}
}

void GameplayAbilitySystem::activate_ability(Node *node) {
	if (get_multiplayer().is_null() || !get_multiplayer()->has_network_peer()) {
		// If there is no multiplayer then we assume a single player game.
		internal_activate_ability(node);
	} else if (auto ability = dynamic_cast<GameplayAbility *>(node)) {
		switch (ability->get_network_execution()) {
			case NetworkExecution::LocalOnly: {
				// Execute locally only, this my only be useful for single player games.
				internal_activate_ability(ability);
			} break;
			case NetworkExecution::ServerInitiated: {
				// Server checks infliction and then triggers execution on clients.
				rpc("server_activate_ability", ability->get_path());
			} break;
			case NetworkExecution::ServerOnly: {
				// Easiest one, server handles execution and distributes state.
				rpc("server_activate_ability", ability->get_path());
			} break;
			default: {
				ERR_EXPLAIN("Received invalid network execution.");
				ERR_FAIL();
			} break;
		}
	}
}

void GameplayAbilitySystem::cancel_ability(Node *node) {
	if (get_multiplayer().is_null() || !get_multiplayer()->has_network_peer()) {
		// If there is no multiplayer then we assume a single player game.
		internal_cancel_ability(node);
	} else if (auto ability = dynamic_cast<GameplayAbility *>(node)) {
		switch (ability->get_network_execution()) {
			case NetworkExecution::LocalOnly: {
				// Execute locally only, this my only be useful for single player games.
				internal_cancel_ability(ability);
			} break;
			case NetworkExecution::ServerInitiated: {
				// Server checks infliction and then triggers execution on clients.
				rpc("server_cancel_ability", ability->get_path());
			} break;
			case NetworkExecution::ServerOnly: {
				// Easiest one, server handles execution and distributes state.
				rpc("server_cancel_ability", ability->get_path());
			} break;
			default: {
				ERR_EXPLAIN("Received invalid network execution.");
				ERR_FAIL();
			} break;
		}
	}
}

bool GameplayAbilitySystem::can_apply_effect(Node *source, const Ref<GameplayEffect> &effect, int64_t stacks /*= 1*/, int64_t level /*= 1*/, double normalised_level /*= 1*/) const {
	if (effect.is_null()) {
		return false;
	}

	for (auto &&node : active_effects) {
		auto &&active_effect = node->get_effect();

		if (*active_effect == *effect) {
			if (node->get_stacks() + stacks > effect->get_maximum_stacks() && effect->get_deny_overflow_application()) {
				return false;
			}
		}
		if (effect->get_effect_tags()->has_any(active_effect->get_application_immunity_tags())) {
			return false;
		}
	}

	for (Ref<GameplayEffectCustomApplicationRequirement> custom_requirement : effect->get_application_requirements()) {
		if (!custom_requirement->execute(source, this, effect, level, normalised_level)) {
			return false;
		}
	}

	auto &&modifiers = effect->get_modifiers();
	return std::all_of(begin(modifiers), end(modifiers), [&](Ref<GameplayEffectModifier> modifier) {
		auto attribute_name = modifier->get_attribute();
		ERR_FAIL_COND_V(!attributes->has_attribute(attribute_name), false);

		auto magnitude = modifier->get_modifier_magnitude()->calculate_magnitude(source, this, effect, level, normalised_level);
		auto attribute = attributes->get_attribute_data(attribute_name);
		auto value = attribute->get_current_value();

		return execute_magnitude(magnitude, value, modifier->get_modifier_operation()) >= 0;
	});
}

Array GameplayAbilitySystem::filter_effects(Node *source, const Array &effects) const {
	ArrayContainer<GameplayEffect> result;

	for (Ref<GameplayEffect> effect : effects) {
		if (effect.is_valid() && can_apply_effect(source, effect)) {
			result.append(effect);
		}
	}

	return result;
}

bool GameplayAbilitySystem::try_apply_effect(Node *source, const Ref<GameplayEffect> &effect, int64_t stacks /*= 1*/, int64_t level /*= 1*/, double normalised_level /*= 1*/) {
	if (can_apply_effect(source, effect, stacks, level, normalised_level)) {
		apply_effect(source, effect, stacks, level);
		return true;
	}

	return false;
}

void GameplayAbilitySystem::apply_effect(Node *source, const Ref<GameplayEffect> &effect, int64_t stacks /*= 1*/, int64_t level /*= 1*/, double normalised_level /*= 1*/) {
	ERR_FAIL_COND(effect.is_null());
	if (get_multiplayer().is_valid() && get_multiplayer()->has_network_peer()) {
		rpc("server_apply_effect", source->get_path(), effect, stacks, level, normalised_level);
	} else {
		internal_apply_effect(source, effect, stacks, level, normalised_level);
	}
}

void GameplayAbilitySystem::apply_effects(Node *source, const Array &effects, int64_t stacks /*= 1*/, int64_t level /*= 1*/, double normalised_level /*= 1*/) {
	for (Ref<GameplayEffect> effect : effects) {
		apply_effect(source, effect, stacks, level, normalised_level);
	}
}

void GameplayAbilitySystem::remove_effect(Node *source, const Ref<GameplayEffect> &effect, int64_t stacks /*= 1*/, int64_t level /*= 1*/) {
	ERR_FAIL_COND(effect.is_null());
	if (get_multiplayer().is_valid() && get_multiplayer()->has_network_peer()) {
		rpc("server_remove_effect", source->get_path(), effect, stacks, level);
	} else {
		internal_remove_effect(source, effect, stacks, level);
	}
}

void GameplayAbilitySystem::remove_effect_node(Node *source, Node *effect_node, int64_t stacks /*= 1*/, int64_t level /*= 1*/) {
	ERR_FAIL_COND(effect_node == nullptr);
	if (get_multiplayer().is_valid() && get_multiplayer()->has_network_peer()) {
		rpc("server_remove_effect", source->get_path(), effect_node, stacks, level);
	} else {
		internal_remove_effect_node(source, effect_node, stacks, level);
	}
}

void GameplayAbilitySystem::apply_cue(const String &cue, double level /*= 1*/, double magnitude /*= 0*/, bool persistent /*= false*/) {
	if (get_multiplayer().is_valid() && get_multiplayer()->has_network_peer()) {
		rpc("sync_apply_cue", cue, level, magnitude, persistent);
	} else {
		sync_apply_cue(cue, level, magnitude, persistent);
	}
}

void GameplayAbilitySystem::remove_cue(const String &cue) {
	if (get_multiplayer().is_valid() && get_multiplayer()->has_network_peer()) {
		rpc("sync_remove_cue", cue);
	} else {
		sync_remove_cue(cue);
	}
}

int64_t GameplayAbilitySystem::get_stack_count(const Ref<GameplayEffect> &effect) const {
	if (effect.is_valid() && effect_stacking.has(effect->get_effect_name())) {
		return effect_stacking[effect->get_effect_name()].stacks;
	}

	return 0;
}

int64_t GameplayAbilitySystem::get_stack_level(const Ref<GameplayEffect> &effect) const {
	if (effect.is_valid() && effect_stacking.has(effect->get_effect_name())) {
		return effect_stacking[effect->get_effect_name()].level;
	}

	return 0;
}

void GameplayAbilitySystem::set_attribute_set(const Ref<GameplayAttributeSet> &value) {
	attributes = value;
}

const Ref<GameplayAttributeSet> &GameplayAbilitySystem::get_attribute_set() const {
	return attributes;
}

void GameplayAbilitySystem::add_target(Node *value) {
	targets.append(value);
}

void GameplayAbilitySystem::remove_target(Node *value) {
	targets.erase(value);
}

void GameplayAbilitySystem::set_targets(const Array &value) {
	targets = value;
}

const Array &GameplayAbilitySystem::get_targets() const {
	return targets;
}

void GameplayAbilitySystem::_notification(int notification) {
	GameplayNode::_notification(notification);
}

void GameplayAbilitySystem::execute_effect(GameplayEffectNode *node) {
	auto source = static_cast<GameplayAbilitySystem *>(node->get_source());
	auto target = static_cast<GameplayAbilitySystem *>(node->get_target());
	auto effect = node->get_effect();
	auto level = node->get_level();
	auto normalised_level = node->get_normalised_level();
	auto trigger_effects = false;

	// Apply modifiers via period or instantly.
	auto &&modifiers = effect->get_modifiers();
	apply_modifiers(node, modifiers);

	// Apply custom executions.
	for (Ref<GameplayEffectCustomExecution> execution : effect->get_executions()) {
		auto result = execution->execute(source, target, node, level, normalised_level);
		auto &&modifiers = result->get_modifiers();

		if (modifiers.size()) {
			apply_modifiers(node, modifiers);
		}

		trigger_effects = result->should_trigger_additional_effects() || trigger_effects;
	}

	// Check if conditional effects should be triggered.
	if (trigger_effects) {
		auto &&effects = effect->get_conditional_erffects();

		for (Ref<ConditionalGameplayEffect> conditional : effects) {
			if (conditional->can_apply(source->get_active_tags())) {
				target->apply_effect(source, conditional->get_effect(), 1, level, normalised_level);
			}
		}
	}

	// Remove effects which have removal tags.
	auto &&removal_effects = target->query_active_effects(effect->get_remove_effect_tags());
	for (Node *child : removal_effects) {
		auto effect_node = static_cast<GameplayEffectNode *>(child);
		target->remove_effect_node(source, child, std::numeric_limits<int32_t>::max(), effect_node->get_level());
	}

	// Iterate all active abilities and check if they should be cancelled.
	for (auto &&ability : active_abilities) {
		if (ability->get_ability_tags()->has_any(effect->get_cancel_ability_tags())) {
			ability->cancel_ability();
		}
	}
}

void GameplayAbilitySystem::apply_modifiers(GameplayEffectNode *node, const Array &modifiers) {
	auto source = static_cast<GameplayAbilitySystem *>(node->get_source());
	auto target = static_cast<GameplayAbilitySystem *>(node->get_target());
	auto effect = node->get_effect();

	struct AttributeChanges {
		Ref<GameplayAttribute> attribute;
		double old_value = 0;
	};

	HashMap<StringName, AttributeChanges> changes;

	for (Ref<GameplayEffectModifier> modifier : modifiers) {
		auto magnitude = modifier->get_modifier_magnitude()->calculate_magnitude(source, target, effect, node->get_level(), node->get_normalised_level());
		auto attribute_name = modifier->get_attribute();
		ERR_FAIL_COND(!attributes->has_attribute(attribute_name));

		auto attribute = attributes->get_attribute(attribute_name);
		auto attribute_data = attribute->get_attribute_data();
		auto value = attribute_data->get_current_value();

		if (!changes.has(attribute_name)) {
			changes.set(attribute_name, AttributeChanges{ attribute, value });
		}

		value = execute_magnitude(magnitude, value, modifier->get_modifier_operation());
		attribute_data->set_current_value(value);
	}

	auto pairs = make_gameplay_ptr<const decltype(changes)::Pair *[]>(changes.size());
	changes.get_key_value_ptr_array(pairs.get());

	for (unsigned i = 0, n = changes.size(); i < n; i++) {
		auto &&change = pairs[i]->data;

		for (auto &&ability : active_abilities) {
			ability->process_wait(WaitType::AttributeChanged, change.attribute);
		}

		target->emit_signal(gameplay_attribute_changed, target, change.attribute, change.old_value);
	}
}

void GameplayAbilitySystem::add_active_ability(GameplayAbility *ability) {
	active_abilities.push_back(ability);
}

void GameplayAbilitySystem::remove_active_ability(GameplayAbility *ability) {
	for (int i = 0, n = active_abilities.size(); i < n; i++) {
		auto active_ability = active_abilities[i];

		if (active_ability == ability) {
			active_abilities.remove(i);
			break;
		}
	}
}

void GameplayAbilitySystem::server_activate_ability(const NodePath &ability_path) {
	if (get_multiplayer().is_valid() && get_multiplayer()->has_network_peer() && !is_network_master()) {
		ERR_EXPLAIN("Invalid network execution call for multiplayer setup.");
		ERR_FAIL();
	}

	if (auto ability = dynamic_cast<GameplayAbility *>(get_node(ability_path))) {
		switch (ability->get_network_execution()) {
			case NetworkExecution::LocalOnly: {
				// Do nothing.
			} break;
			case NetworkExecution::ServerInitiated: {
				rpc("client_activate_ability", ability_path);
			} break;
			case NetworkExecution::ServerOnly: {
				internal_activate_ability(ability);
				replicate_attributes();
			} break;
			default: {
			} break;
		}
	}
}

void GameplayAbilitySystem::client_activate_ability(const NodePath &ability_path) {
	if (get_multiplayer().is_valid() && get_multiplayer()->has_network_peer() && is_network_master()) {
		ERR_EXPLAIN("Invalid network execution call for multiplayer setup.");
		ERR_FAIL();
	}
	internal_activate_ability(get_node(ability_path));
}

void GameplayAbilitySystem::internal_activate_ability(Node *node) {
	if (auto ability = dynamic_cast<GameplayAbility *>(node)) {
		if (ability->can_activate_ability()) {
			if (get_multiplayer().is_valid() && get_multiplayer()->has_network_peer() && is_network_master()) {
				rpc("client_ability_activated", ability->get_path());
			}

			auto &&cancel_tags = ability->get_cancel_ability_tags();

			for (auto active_ability : active_abilities) {
				auto &&active_tags = active_ability->get_ability_tags();
				if (active_tags->has_any(cancel_tags)) {
					cancel_ability(active_ability);
				}
			}

			ability->targets = targets;
			ability->activate_ability();
			emit_signal(gameplay_ability_activated, this, ability);
		} else {
			if (get_multiplayer().is_valid() && get_multiplayer()->has_network_peer() && is_network_master()) {
				rpc("client_ability_blocked", ability);
			}
			emit_signal(gameplay_ability_blocked, this, ability);
		}
	}
}

void GameplayAbilitySystem::server_cancel_ability(const NodePath &ability_path) {
	if (get_multiplayer().is_valid() && get_multiplayer()->has_network_peer() && !is_network_master()) {
		ERR_EXPLAIN("Invalid network execution call for multiplayer setup.");
		ERR_FAIL();
	}
	internal_cancel_ability(get_node(ability_path));

	if (auto ability = dynamic_cast<GameplayAbility *>(get_node(ability_path))) {
		switch (ability->get_network_execution()) {
			case NetworkExecution::LocalOnly: {
				// Do nothing.
			} break;
			case NetworkExecution::ServerInitiated: {
				rpc("client_cancel_ability");
			} break;
			case NetworkExecution::ServerOnly: {
				internal_cancel_ability(ability);
				replicate_attributes();
			} break;
			default: {
			} break;
		}
	}
}

void GameplayAbilitySystem::client_cancel_ability(const NodePath &ability_path) {
	if (get_multiplayer().is_valid() && get_multiplayer()->has_network_peer() && is_network_master()) {
		ERR_EXPLAIN("Invalid network execution call for multiplayer setup.");
		ERR_FAIL();
	}
	internal_cancel_ability(get_node(ability_path));
}

void GameplayAbilitySystem::internal_cancel_ability(Node *node) {
	if (get_multiplayer().is_valid() && get_multiplayer()->has_network_peer() && is_network_master()) {
		rpc("client_cancel_ability", node->get_path());
	}

	if (auto ability = dynamic_cast<GameplayAbility *>(node)) {
		ability->cancel_ability();
		emit_signal(gameplay_ability_cancelled, this, ability);
	}
}

void GameplayAbilitySystem::server_apply_effect(const NodePath &source_path, const Ref<GameplayEffect> &effect, int64_t stacks, int64_t level, double normalised_level) {
	if (get_multiplayer().is_valid() && get_multiplayer()->has_network_peer() && !is_network_master()) {
		ERR_EXPLAIN("Invalid network execution call for multiplayer setup.");
		ERR_FAIL();
	}
	internal_apply_effect(get_node(source_path), effect, stacks, level, normalised_level);
	replicate_attributes();
}

void GameplayAbilitySystem::client_apply_effect(const NodePath &source_path, const Ref<GameplayEffect> &effect, int64_t stacks, int64_t level, double normalised_level) {
	if (get_multiplayer().is_valid() && get_multiplayer()->has_network_peer() && is_network_master()) {
		ERR_EXPLAIN("Invalid network execution call for multiplayer setup.");
		ERR_FAIL();
	}
	internal_apply_effect(get_node(source_path), effect, stacks, level, normalised_level);
}

void GameplayAbilitySystem::internal_apply_effect(Node *node, const Ref<GameplayEffect> &effect, int64_t stacks, int64_t level, double normalised_level) {
	if (auto source = dynamic_cast<GameplayAbilitySystem *>(node)) {
		if (can_apply_effect(source, effect, stacks, level, normalised_level)) {
			if (effect->get_infliction_chance().is_valid() && rgenerator(rengine) > effect->get_infliction_chance()->calculate_magnitude(source, this, effect, level, normalised_level)) {
				if (get_multiplayer().is_valid() && get_multiplayer()->has_network_peer() && is_network_master()) {
					rpc("client_infliction_failed", effect);
				}
				emit_signal(gameplay_effect_infliction_failed, this, effect);
			} else {
				GameplayAbilitySystem *aggregate_source = nullptr;

				switch (effect->get_stacking_type()) {
					case StackingType::AggregateOnSource: {
						aggregate_source = source;
					} break;
					case StackingType::AggregateOnTarget: {
						aggregate_source = this;
					} break;
					default: {
					} break;
				}

				if (aggregate_source) {
					auto effect_name = effect->get_effect_name();
					auto &&stacking = aggregate_source->effect_stacking;

					if (stacking.has(effect_name)) {
						auto &&effect_data = stacking[effect_name];

						if (effect_data.level == level) {
							auto effect_node = stacking[effect_name].effect_node;
							effect_node->add_stack(stacks);

							for (auto ability : active_abilities) {
								ability->process_wait(WaitType::EffectStackAdded, effect_node);
							}
						} else if (effect_data.level < level) {
							auto effect_node = stacking[effect_name].effect_node;
							active_effects.erase(effect_node);
							stacking.erase(effect_name);
							effect_node->queue_delete();
							add_effect(source, effect, stacks, level, normalised_level);
						} else {
							WARN_PRINT("Level of given effect is lower than already existing one.");
						}
					} else {
						add_effect(source, effect, stacks, level, normalised_level);
					}
				} else {
					add_effect(source, effect, stacks, level, normalised_level);
				}

				// GA-TODO: Distribute effect and/or effect changes to peers.
			}
		}
	}
}

void GameplayAbilitySystem::server_remove_effect(const NodePath &source_path, const Ref<GameplayEffect> &effect, int64_t stacks, int64_t level) {
	if (get_multiplayer().is_valid() && get_multiplayer()->has_network_peer() && !is_network_master()) {
		ERR_EXPLAIN("Invalid network execution call for multiplayer setup.");
		ERR_FAIL();
	}
	internal_remove_effect(get_node(source_path), effect, stacks, level);
	replicate_attributes();
}

void GameplayAbilitySystem::client_remove_effect(const NodePath &source_path, const Ref<GameplayEffect> &effect, int64_t stacks, int64_t level) {
	if (get_multiplayer().is_valid() && get_multiplayer()->has_network_peer() && is_network_master()) {
		ERR_EXPLAIN("Invalid network execution call for multiplayer setup.");
		ERR_FAIL();
	}
	internal_remove_effect(get_node(source_path), effect, stacks, level);
}

void GameplayAbilitySystem::internal_remove_effect(Node *node, const Ref<GameplayEffect> &effect, int64_t stacks, int64_t level) {
	if (auto source = dynamic_cast<GameplayAbilitySystem *>(node)) {
		GameplayAbilitySystem *aggregate_source = nullptr;

		switch (effect->get_stacking_type()) {
			case StackingType::AggregateOnSource: {
				aggregate_source = source;
			} break;
			case StackingType::AggregateOnTarget: {
				aggregate_source = this;
			} break;
			default: {
				for (auto effect_node : active_effects) {
					if (effect_node->get_effect()->get_effect_name() == effect->get_effect_name()) {
						active_effects.erase(effect_node);
						effect_node->queue_delete();

						for (auto ability : active_abilities) {
							ability->process_wait(WaitType::EffectStackRemoved, effect_node);
							ability->process_wait(WaitType::EffectRemoved, effect_node);
						}
					}
				}

				return;
			} break;
		}

		if (aggregate_source) {
			auto effect_name = effect->get_effect_name();
			auto &&stacking = aggregate_source->effect_stacking;

			if (stacking.has(effect_name)) {
				auto &&effect_data = stacking[effect_name];
				auto effect_node = effect_data.effect_node;

				if (effect_data.level > level) {
					if (get_multiplayer().is_valid() && get_multiplayer()->has_network_peer() && is_network_master()) {
						rpc("client_effect_removal_failed", effect);
					}
					emit_signal(gameplay_effect_removal_failed, this, effect);
				} else {
					effect_node->remove_stack(stacks);

					if (effect_node->get_stacks() <= 0) {
						for (auto ability : active_abilities) {
							ability->process_wait(WaitType::EffectStackRemoved, effect_node);
							ability->process_wait(WaitType::EffectRemoved, effect_node);
						}
					} else {
						for (auto ability : active_abilities) {
							ability->process_wait(WaitType::EffectStackRemoved, effect_node);
						}
					}
				}
			}
		}
	}
}

void GameplayAbilitySystem::server_remove_effect_node(const NodePath &source_path, Node *effect_node, int64_t stacks, int64_t level) {
	if (get_multiplayer().is_valid() && get_multiplayer()->has_network_peer() && !is_network_master()) {
		ERR_EXPLAIN("Invalid network execution call for multiplayer setup.");
		ERR_FAIL();
	}
	internal_remove_effect_node(get_node(source_path), effect_node, stacks, level);
	replicate_attributes();
}

void GameplayAbilitySystem::client_remove_effect_node(const NodePath &source_path, Node *effect_node, int64_t stacks, int64_t level) {
	if (get_multiplayer().is_valid() && get_multiplayer()->has_network_peer() && is_network_master()) {
		ERR_EXPLAIN("Invalid network execution call for multiplayer setup.");
		ERR_FAIL();
	}
	internal_remove_effect_node(get_node(source_path), effect_node, stacks, level);
}

void GameplayAbilitySystem::internal_remove_effect_node(Node *source, Node *node, int64_t stacks, int64_t level) {
	if (auto effect_node = dynamic_cast<GameplayEffectNode *>(node)) {
		auto &&effect = effect_node->get_effect();

		if (effect_node->get_level() > level) {
			if (get_multiplayer().is_valid() && get_multiplayer()->has_network_peer() && is_network_master()) {
				rpc("client_effect_removal_failed", effect);
			}
			emit_signal(gameplay_effect_removal_failed, this, effect);
		} else {
			effect_node->remove_stack(stacks);

			for (auto ability : active_abilities) {
				ability->process_wait(WaitType::EffectRemoved, effect);
			}
		}
	}
}

void GameplayAbilitySystem::sync_apply_cue(const String &cue, double level, double magnitude, bool persistent) {
	if (persistent) {
		persistent_cues->append(cue);
	}
	emit_signal(gameplay_cue_activated, this, cue, level, magnitude, persistent);
}

void GameplayAbilitySystem::sync_remove_cue(const String &cue) {
	persistent_cues->remove(cue);
	emit_signal(gameplay_cue_removed, this, cue);
}

void GameplayAbilitySystem::add_effect(GameplayAbilitySystem *source, const Ref<GameplayEffect> &effect, int64_t stacks, int64_t level, double normalised_level) {
	auto effect_node = memnew(GameplayEffectNode);
	effect_node->initialise(source, this, effect, level, normalised_level);
	effect_node->add_stack(stacks);
	call_deferred("add_child", effect_node);

	for (auto ability : active_abilities) {
		ability->process_wait(WaitType::EffectAdded, effect_node);
	}
	for (auto ability : active_abilities) {
		ability->process_wait(WaitType::EffectStackAdded, effect_node);
	}
}

void GameplayAbilitySystem::replicate_attributes() {
	rpc("client_update_attributes", attributes->get_attributes());
}

void GameplayAbilitySystem::client_update_attribute(const Ref<GameplayAttribute> &update) {
	for (Ref<GameplayAttribute> attribute : attributes->get_attributes()) {
		if (attribute->get_attribute_name() == update->get_attribute_name()) {
			attribute->get_attribute_data()->set_base_value(update->get_attribute_data()->get_base_value());
			attribute->get_attribute_data()->set_current_value(update->get_attribute_data()->get_current_value());
		}
	}
}

void GameplayAbilitySystem::client_ability_activated(Node *ability) {
	emit_signal(gameplay_ability_activated, this, ability);
}

void GameplayAbilitySystem::client_ability_blocked(Node *ability) {
	emit_signal(gameplay_ability_blocked, this, ability);
}

void GameplayAbilitySystem::client_effect_activated(const Ref<GameplayEffect> &effect) {
	emit_signal(gameplay_effect_activated, this, effect);
}

void GameplayAbilitySystem::client_infliction_failed(const Ref<GameplayEffect> &effect) {
	emit_signal(gameplay_effect_infliction_failed, this, effect);
}

void GameplayAbilitySystem::client_effect_removal_failed(const Ref<GameplayEffect> &effect) {
	emit_signal(gameplay_effect_removal_failed, this, effect);
}

void GameplayAbilitySystem::client_update_attributes(const Array &updates) {
	// GA-TODO: Replicate server state to client.
}

double GameplayAbilitySystem::execute_magnitude(double magnitude, double current_value, int operation) {
	ERR_FAIL_COND_V(operation < 0, -1.0);
	ERR_FAIL_COND_V(operation > ModifierOperation::Override, -1.0);

	switch (operation) {
		case ModifierOperation::Add:
			return current_value + magnitude;
		case ModifierOperation::Subtract:
			return current_value - magnitude;
		case ModifierOperation::Multiply:
			return current_value * magnitude;
		case ModifierOperation::Divide:
			return current_value / magnitude;
		case ModifierOperation::Override:
			return magnitude;
	}

	ERR_FAIL_V(-1.0);
}

void GameplayAbilitySystem::_bind_methods() {
}

std::random_device GameplayAbilitySystem::rdevice;
std::default_random_engine GameplayAbilitySystem::rengine = std::default_random_engine(rdevice());
std::uniform_real_distribution<double> GameplayAbilitySystem::rgenerator;
