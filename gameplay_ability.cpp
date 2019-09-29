#include "gameplay_ability.h"
#include "gameplay_ability_system.h"
#include "gameplay_effect.h"
#include "gameplay_effect_magnitude.h"
#include "gameplay_tags.h"

#include <core/os/input.h>
#include <core/os/input_event.h>
#include <scene/animation/animation_player.h>

#include <algorithm>

namespace {
constexpr auto _on_activate_ability = "_on_activate_ability";
constexpr auto _on_end_ability = "_on_end_ability";
constexpr auto _on_gameplay_event = "_on_gameplay_event";

constexpr auto _can_event_activate_ability = "_can_event_activate_ability";
constexpr auto _can_activate_ability = "_can_activate_ability";

constexpr auto _on_wait_completed = "_on_wait_completed";
constexpr auto _on_wait_interrupted = "_on_wait_interrupted";
constexpr auto _on_wait_cancelled = "_on_wait_cancelled";

constexpr auto gameplay_ability_ready = "gameplay_ability_ready";
} // namespace

void GameplayAbilityTriggerData::set_trigger_tag(const String &value) {
	trigger_tag = value;
}

String GameplayAbilityTriggerData::get_trigger_tag() const {
	return trigger_tag;
}

void GameplayAbilityTriggerData::set_trigger_type(AbilityTrigger::Type value) {
	trigger_type = value;
}

AbilityTrigger::Type GameplayAbilityTriggerData::get_trigger_type() const {
	return trigger_type;
}

void GameplayAbilityTriggerData::_bind_methods() {
	/** Methods */
	ClassDB::bind_method(D_METHOD("set_trigger_tag", "value"), &GameplayAbilityTriggerData::set_trigger_tag);
	ClassDB::bind_method(D_METHOD("get_trigger_tag"), &GameplayAbilityTriggerData::get_trigger_tag);
	ClassDB::bind_method(D_METHOD("set_trigger_type", "value"), &GameplayAbilityTriggerData::set_trigger_type);
	ClassDB::bind_method(D_METHOD("get_trigger_type"), &GameplayAbilityTriggerData::get_trigger_type);

	/** Properties */
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "trigger_tag"), "set_trigger_tag", "get_trigger_tag");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "trigger_type", PROPERTY_HINT_ENUM, "Event,Tag Added,Tag Removed"), "set_trigger_type", "get_trigger_type");
}

GameplayAbility::GameplayAbility() {
}

const GameplayAbility::WaitData &GameplayAbility::get_wait_handle() const {
	return wait_handle;
}

void GameplayAbility::initialise(GameplayAbilitySystem *system) {
	this->source = system;
}

bool GameplayAbility::is_active() const {
	return active;
}

bool GameplayAbility::is_cooldown() const {
	return get_remaining_cooldown() > 0;
}

bool GameplayAbility::is_triggerable() const {
	return triggers.size() > 0;
}

double GameplayAbility::get_normalised_level() const {
	auto mlevel = static_cast<double>(maximum_level);
	auto clevel = static_cast<double>(current_level);
	return CLAMP(clevel / mlevel, 0.0, 1.0);
}

bool GameplayAbility::can_trigger(const String &trigger_tag, AbilityTrigger::Type trigger_type) const {
	for (auto &&variant : triggers) {
		auto trigger = static_cast<Ref<GameplayAbilityTriggerData> >(variant);

		if (trigger.is_null()) {
			continue;
		}
		if (trigger->get_trigger_tag().matchn(trigger_tag) && trigger->get_trigger_type() == trigger_type) {
			return true;
		}
	}

	return false;
}

bool GameplayAbility::can_event_activate_ability(const Ref<GameplayEvent> &event) {
	if (has_method(_can_event_activate_ability)) {
		return static_cast<bool>(call(_can_event_activate_ability, event));
	} else {
		return true;
	}
}

bool GameplayAbility::try_event_activate_ability(const Ref<GameplayEvent> &event) {
	if (has_method(_on_gameplay_event)) {
		call_deferred(_on_gameplay_event, event);
		return true;
	} else {
		return false;
	}
}

bool GameplayAbility::can_activate_ability() {
	// Check if active.
	if (is_active()) {
		return false;
	}

	// Check if method for activation exists.
	if (!has_method(_on_activate_ability)) {
		return false;
	}

	// Check cooldown and cost.
	if (!check_ability_cooldown()) {
		return false;
	}
	if (!check_ability_cost()) {
		return false;
	}

	// Check source tags.
	auto source_tags = source->get_active_tags();
	if (!check_tag_requirement(source_tags, source_required_tags, source_blocked_tags)) {
		return false;
	}

	// Check if active abilities are blocking.
	auto &&active_abilities = source->get_active_abilities_vector();
	for (auto ability : active_abilities) {
		if (ability->get_block_ability_tags()->has_any(ability_tags)) {
			return false;
		}
	}

	// Ability can get activated.
	return true;
}

bool GameplayAbility::can_activate_ability_on_target(const Node *node) {
	// Check target requirements if target is set
	if (auto target = dynamic_cast<const GameplayAbilitySystem *>(node)) {
		// Check target tags.
		auto target_tags = target->get_active_tags();

		if (!check_tag_requirement(target_tags, target_required_tags, target_blocked_tags)) {
			return false;
		}

		// Check implemented method or return true.
		if (has_method(_can_activate_ability)) {
			return static_cast<bool>(call(_can_activate_ability, target));
		} else {
			return true;
		}
	}

	return false;
}

bool GameplayAbility::try_activate_ability() {
	if (!can_activate_ability()) {
		return false;
	}

	active = true;
	call_deferred(_on_activate_ability);
	return true;
}

void GameplayAbility::activate_ability() {
	active = true;
	call_deferred(_on_activate_ability);
	source->add_active_ability(this);
}

void GameplayAbility::commit_ability() {
	if (cooldown_effect.is_valid()) {
		source->apply_effect(source, cooldown_effect);
	}
	if (cost_effect.is_valid()) {
		source->apply_effect(source, cost_effect);
	}

	active = false;
	source->remove_active_ability(this);
}

void GameplayAbility::end_ability() {
	if (active) {
		active = false;
		reset_wait_handle();
		call_deferred(_on_end_ability, false);

		source->remove_active_ability(this);
	}
}

void GameplayAbility::cancel_ability() {
	if (active) {
		active = false;
		reset_wait_handle();
		call_deferred(_on_end_ability, true);

		source->remove_active_ability(this);
	}
}

double GameplayAbility::get_remaining_cooldown() const {
	if (cooldown_effect.is_null()) {
		return 0;
	}

	auto tags = cooldown_effect->get_effect_tags();
	auto effects = source->query_active_effects(tags);

	if (effects.empty()) {
		return 0;
	}

	Vector<double> effect_durations;

	for (Node *node : effects) {
		auto effect_node = static_cast<GameplayEffectNode *>(node);
		effect_durations.push_back(effect_node->get_duration());
	}
	std::sort(begin(effect_durations), end(effect_durations));
	return effect_durations[effect_durations.size() - 1];
}

bool GameplayAbility::check_ability_cost() const {
	if (cost_effect.is_null()) {
		return true;
	}

	auto level = get_normalised_level();
	return source->can_apply_effect(source, cost_effect, 1, current_level, level);
}

bool GameplayAbility::check_ability_cooldown() const {
	return get_remaining_cooldown() <= 0;
}

void GameplayAbility::apply_effect_on_source(const Ref<GameplayEffect> &effect, int64_t stacks /*= 1*/, int64_t level /*= -1*/) {
	apply_effect_on_target(source, effect, stacks, level);
}

void GameplayAbility::apply_effect_on_target(Node *node, const Ref<GameplayEffect> &effect, int64_t stacks /*= 1*/, int64_t level /*= -1*/) {
	if (auto target = dynamic_cast<GameplayAbilitySystem *>(node)) {
		double normalised_level = 1;

		if (level < 0) {
			level = get_current_level();
			normalised_level = get_normalised_level();
		} else {
			level = MIN(level, get_max_level());
			normalised_level = level / static_cast<double>(maximum_level);
		}

		target->apply_effect(source, effect, stacks, level, normalised_level);
	}
}

void GameplayAbility::apply_effect_on_targets(const Array &targets, const Ref<GameplayEffect> &effect, int64_t stacks /*= 1*/, int64_t level /*= -1*/) {
	for (Node *target : targets) {
		apply_effect_on_target(target, effect, stacks, level);
	}
}

void GameplayAbility::remove_effect_from_source(const Ref<GameplayEffect> &effect, int64_t stacks /*= 1*/, int64_t level /*= -1*/) {
	remove_effect_from_target(source, effect, stacks, level);
}

void GameplayAbility::remove_effect_from_target(Node *node, const Ref<GameplayEffect> &effect, int64_t stacks /*= 1*/, int64_t level /*= -1*/) {
	if (auto target = dynamic_cast<GameplayAbilitySystem *>(node)) {
		target->remove_effect(source, effect, stacks, level < 0 ? get_current_level() : MIN(level, get_max_level()));
	}
}

void GameplayAbility::remove_effect_on_targets(const Array &targets, const Ref<GameplayEffect> &effect, int64_t stacks /*= 1*/, int64_t level /*= -1*/) {
	for (Node *target : targets) {
		remove_effect_from_target(target, effect, stacks, level);
	}
}

void GameplayAbility::execute_gameplay_cue(const String &cue_tag, Node *node) {
	if (auto target = dynamic_cast<GameplayAbilitySystem *>(node)) {
		target->apply_cue(cue_tag);
	}
}

void GameplayAbility::execute_gameplay_cue_parameters(const String &cue_tag, Node *node, double level, double magnitude) {
	if (auto target = dynamic_cast<GameplayAbilitySystem *>(node)) {
		target->apply_cue(cue_tag, level, magnitude);
	}
}

void GameplayAbility::add_gameplay_cue(const String &cue_tag, Node *node, bool remove_on_ability_end /*= true*/) {
	if (auto target = dynamic_cast<GameplayAbilitySystem *>(node)) {
		target->apply_cue(cue_tag, 1, 0, true);
	}
}

void GameplayAbility::add_gameplay_cue_paramters(const String &cue_tag, Node *node, double level, double magnitude, bool remove_on_ability_end /*= true*/) {
	if (auto target = dynamic_cast<GameplayAbilitySystem *>(node)) {
		target->apply_cue(cue_tag, level, magnitude, true);
	}
}

void GameplayAbility::remove_gameplay_cue(const String &cue_tag, Node *node) {
	if (auto target = dynamic_cast<GameplayAbilitySystem *>(node)) {
		target->remove_cue(cue_tag);
	}
}

void GameplayAbility::set_ability_name(const StringName &value) {
	ability_name = value;
}

StringName GameplayAbility::get_ability_name() const {
	return ability_name;
}

void GameplayAbility::set_network_execution(NetworkExecution::Type value) {
	network_execution = value;
}

NetworkExecution::Type GameplayAbility::get_network_execution() const {
	return network_execution;
}

void GameplayAbility::set_triggers(const Array &value) {
	triggers = value;
}

const Array &GameplayAbility::get_triggers() const {
	return triggers;
}

void GameplayAbility::set_cooldown_effect(const Ref<GameplayEffect> &value) {
	cooldown_effect = value;
}

const Ref<GameplayEffect> &GameplayAbility::get_cooldown_effect() const {
	return cooldown_effect;
}

void GameplayAbility::set_cost_effect(const Ref<GameplayEffect> &value) {
	cost_effect = value;
}

const Ref<GameplayEffect> &GameplayAbility::get_cost_effect() const {
	return cost_effect;
}

void GameplayAbility::set_max_level(int64_t value) {
	maximum_level = value;
}

int64_t GameplayAbility::get_max_level() const {
	return maximum_level;
}

void GameplayAbility::set_current_level(int64_t value) {
	current_level = value;
}

int64_t GameplayAbility::get_current_level() const {
	return current_level;
}

void GameplayAbility::set_input_action(const StringName &value) {
	input_action = value;
}

StringName GameplayAbility::get_input_action() const {
	return input_action;
}

void GameplayAbility::set_ability_tags(const Ref<GameplayTagContainer> &value) {
	ability_tags = value;
}

Ref<GameplayTagContainer> GameplayAbility::get_ability_tags() const {
	return ability_tags;
}

void GameplayAbility::set_cancel_ability_tags(const Ref<GameplayTagContainer> &value) {
	cancel_abilities_tags = value;
}

Ref<GameplayTagContainer> GameplayAbility::get_cancel_ability_tags() const {
	return cancel_abilities_tags;
}

void GameplayAbility::set_block_ability_tags(const Ref<GameplayTagContainer> &value) {
	block_abilities_tags = value;
}

Ref<GameplayTagContainer> GameplayAbility::get_block_ability_tags() const {
	return block_abilities_tags;
}

void GameplayAbility::set_source_required_tags(const Ref<GameplayTagContainer> &value) {
	source_required_tags = value;
}

Ref<GameplayTagContainer> GameplayAbility::get_source_required_tags() const {
	return source_required_tags;
}

void GameplayAbility::set_source_blocked_tags(const Ref<GameplayTagContainer> &value) {
	source_blocked_tags = value;
}

Ref<GameplayTagContainer> GameplayAbility::get_source_blocked_tags() const {
	return source_blocked_tags;
}

void GameplayAbility::set_target_required_tags(const Ref<GameplayTagContainer> &value) {
	target_required_tags = value;
}

Ref<GameplayTagContainer> GameplayAbility::get_target_required_tags() const {
	return target_required_tags;
}

void GameplayAbility::set_target_blocked_tags(const Ref<GameplayTagContainer> &value) {
	target_blocked_tags = value;
}

Ref<GameplayTagContainer> GameplayAbility::get_target_blocked_tags() const {
	return target_blocked_tags;
}

void GameplayAbility::wait_delay(double seconds) {
	handle_wait_interrupt(WaitType::Delay);
	wait_handle.data = seconds;
}

void GameplayAbility::wait_event(const String &event_tag) {
	handle_wait_interrupt(WaitType::Event);
	wait_handle.data = event_tag;
}

void GameplayAbility::wait_action_pressed(const StringName &action) {
	handle_wait_interrupt(WaitType::ActionPressed);
	wait_handle.data = action;
}

void GameplayAbility::wait_action_released(const StringName &action) {
	handle_wait_interrupt(WaitType::ActionReleased);
	wait_handle.data = action;
}

void GameplayAbility::wait_attribute_change(const StringName &attribute) {
	handle_wait_interrupt(WaitType::AttributeChanged);
	wait_handle.data = attribute;
}

void GameplayAbility::wait_base_attribute_change(const StringName &attribute) {
	handle_wait_interrupt(WaitType::BaseAttributeChanged);
	wait_handle.data = attribute;
}

void GameplayAbility::wait_effect_added(const Ref<GameplayEffect> &effect) {
	handle_wait_interrupt(WaitType::EffectAdded);
	wait_handle.data = effect;
}

void GameplayAbility::wait_effect_removed(const Ref<GameplayEffect> &effect) {
	handle_wait_interrupt(WaitType::EffectRemoved);
	wait_handle.data = effect;
}

void GameplayAbility::wait_tag_added(const String &tag) {
	handle_wait_interrupt(WaitType::TagAdded);
	wait_handle.data = tag;
}

void GameplayAbility::wait_tag_removed(const String &tag) {
	handle_wait_interrupt(WaitType::TagRemoved);
	wait_handle.data = tag;
}

void GameplayAbility::process_wait(WaitType::Type process_type, const Variant &data) {
	if (process_type != wait_handle.type) {
		return;
	}

	switch (wait_handle.type) {
		case WaitType::Delay: {
			auto delta = static_cast<double>(data);
			auto delay = static_cast<double>(wait_handle.data);

			if (delay - delta <= 0) {
				call_deferred(_on_wait_completed, wait_handle.type);
				wait_handle.type = WaitType::None;
			} else {
				wait_handle.data = delay - delta;
			}
		} break;
		case WaitType::Event: {
			auto event_tag = static_cast<String>(data);
			auto wait_event = static_cast<String>(wait_handle.data);

			if (event_tag.matchn(wait_event)) {
				call_deferred(_on_wait_completed, wait_handle.type, event_tag);
				wait_handle.type = WaitType::None;
			}
		} break;
		case WaitType::ActionPressed: {
			auto input = static_cast<Ref<InputEvent> >(data);
			auto input_action = static_cast<StringName>(wait_handle.data);

			if (input->is_action_pressed(input_action)) {
				call_deferred(_on_wait_completed, wait_handle.type, input_action);
				wait_handle.type = WaitType::None;
			}
		} break;
		case WaitType::ActionReleased: {
			auto input = static_cast<Ref<InputEvent> >(data);
			auto input_action = static_cast<StringName>(wait_handle.data);

			if (input->is_action_released(input_action)) {
				call_deferred(_on_wait_completed, wait_handle.type, input_action);
				wait_handle.type = WaitType::None;
			}
		} break;
		case WaitType::AttributeChanged:
		case WaitType::BaseAttributeChanged: {
			auto attribute = static_cast<StringName>(data);
			auto wait_attribute = static_cast<StringName>(wait_handle.data);

			if (wait_attribute == attribute) {
				call_deferred(_on_wait_completed, wait_handle.type, attribute);
				wait_handle.type = WaitType::None;
			}
		} break;
		case WaitType::EffectAdded:
		case WaitType::EffectRemoved: {
			auto effect = static_cast<Ref<GameplayEffect> >(data);
			auto wait_effect = static_cast<Ref<GameplayEffect> >(wait_handle.data);

			if (wait_effect->get_effect_name() == effect->get_effect_name()) {
				call_deferred(_on_wait_completed, wait_handle.type, effect);
				wait_handle.type = WaitType::None;
			}
		} break;
		case WaitType::TagAdded:
		case WaitType::TagRemoved: {
			auto tag = static_cast<String>(data);
			auto wait_tag = static_cast<String>(wait_handle.data);

			if (wait_tag == tag) {
				call_deferred(_on_wait_completed, wait_handle.type, tag);
				wait_handle.type = WaitType::None;
			}
		} break;
		default: {
		} break;
	}
}

void GameplayAbility::ability_process(double delta) {
	if (cooldown_effect.is_valid()) {
		if (get_remaining_cooldown() <= 0) {
			source->emit_signal(gameplay_ability_ready, source, this);
		}
	}
	if (active) {
		if (source->get_active_tags()->has_any(source_blocked_tags)) {
			source->cancel_ability(this);
		} else if (!source->get_active_tags()->has_all(source_required_tags)) {
			source->cancel_ability(this);
		} else {
			process_wait(WaitType::Delay, delta);
		}
	}
}

void GameplayAbility::ability_input() {
	double cooldown = cooldown_effect.is_valid() ? get_remaining_cooldown() : 0;

	if (cooldown <= 0 && input_action != StringName() && !active) {
		auto input = Input::get_singleton();

		if (input->is_action_pressed(input_action)) {
			source->activate_ability(this);
		}
	}
}

void GameplayAbility::set_ability_process(bool value) {
	should_ability_process = value;
}

void GameplayAbility::set_ability_input(bool value) {
	should_ability_input = value;
}

void GameplayAbility::set_targets(const Array &value) {
	targets = value;
}

const Array &GameplayAbility::get_targets() const {
	return targets;
}

Array GameplayAbility::filter_targets() {
	Array result;

	for (Node *target : targets) {
		if (can_activate_ability_on_target(target)) {
			result.push_back(target);
		}
	}

	return result;
}

void GameplayAbility::_notification(int notification) {
	GameplayNode::_notification(notification);

	switch (notification) {
		case NOTIFICATION_INTERNAL_PROCESS: {
			if (!is_queued_for_deletion() && should_ability_process) {
				auto delta = get_process_delta_time();
				ability_process(delta);
			}
		} break;
		case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
			if (!is_queued_for_deletion() && should_ability_input) {
				ability_input();
			}
		} break;
		default: {
		} break;
	}
}

void GameplayAbility::handle_wait_cancel() {
	if (wait_handle.type != WaitType::None) {
		call_deferred(_on_wait_cancelled, wait_handle.type);
	}
	wait_handle.type = WaitType::None;
}

void GameplayAbility::handle_wait_interrupt(WaitType::Type wait_type) {
	if (wait_handle.type != wait_type) {
		if (wait_handle.type != WaitType::None) {
			call_deferred(_on_wait_interrupted, wait_type);
		}
		wait_handle.type = wait_type;
	}
}

void GameplayAbility::reset_wait_handle() {
	wait_handle.type = WaitType::None;
	wait_handle.data = {};
}

bool GameplayAbility::check_tag_requirement(const Ref<GameplayTagContainer> &tags, const Ref<GameplayTagContainer> &required, const Ref<GameplayTagContainer> &blocked) {
	if (tags->has_any(blocked)) {
		return false;
	}
	return tags->has_all(required);
}

void GameplayAbility::_bind_methods() {
	/** Virtual Methods */
	BIND_VMETHOD(MethodInfo(_on_activate_ability));
	BIND_VMETHOD(MethodInfo(_on_end_ability, PropertyInfo(Variant::BOOL, "cancelled")));
	BIND_VMETHOD(MethodInfo(_on_gameplay_event, PropertyInfo(Variant::OBJECT, "event")));
	BIND_VMETHOD(MethodInfo(Variant::BOOL, _can_event_activate_ability, PropertyInfo(Variant::OBJECT, "event")));
	BIND_VMETHOD(MethodInfo(Variant::BOOL, _can_activate_ability, PropertyInfo(Variant::OBJECT, "target")));

	/** Methods */
	ClassDB::bind_method(D_METHOD("ability_process", "delta"), &GameplayAbility::ability_process);
	ClassDB::bind_method(D_METHOD("ability_input"), &GameplayAbility::ability_input);
	ClassDB::bind_method(D_METHOD("set_ability_process", "value"), &GameplayAbility::set_ability_process);
	ClassDB::bind_method(D_METHOD("set_ability_input", "value"), &GameplayAbility::set_ability_input);

	/** Properties */
}
