#pragma once

#include "gameplay_node.h"

#include <core/hash_map.h>
#include <core/resource.h>
#include <scene/main/node.h>

class GameplayAbilitySystem;
class GameplayTagContainer;
class GameplayEffect;
class GameplayEvent;
class InputEvent;

/** What will trigger this ability. */
namespace AbilityTrigger {
enum Type {
	/** Ability gets triggered via gameplay events. */
	GampeplayEvent,
	/** Ability gets triggered if owner gets a tag added. */
	OwnedTagAdded,
	/** Ability gets triggered if the specified tag gets removed from the owner. */
	OwnedTagRemoved
};
}

VARIANT_ENUM_CAST(AbilityTrigger::Type);

/** Defines for what the ability is waiting. */
namespace WaitType {
enum Type {
	None,
	Delay,
	Event,
	ActionPressed,
	ActionReleased,
	AttributeChanged,
	BaseAttributeChanged,
	EffectAdded,
	EffectRemoved,
	EffectStackAdded,
	EffectStackRemoved,
	TagAdded,
	TagRemoved
};
}

VARIANT_ENUM_CAST(WaitType::Type);

/** Trigger data */
class GAMEPLAY_ABILITIES_API GameplayAbilityTriggerData : public GameplayResource {
	GDCLASS(GameplayAbilityTriggerData, GameplayResource);
	OBJ_CATEGORY("GameplayAbilities");

public:
	virtual ~GameplayAbilityTriggerData() = default;

	void set_trigger_tag(const String &value);
	String get_trigger_tag() const;
	void set_trigger_type(AbilityTrigger::Type value);
	AbilityTrigger::Type get_trigger_type() const;

private:
	/** Tag that will trigger the ability. */
	String trigger_tag;
	/** Flags on how ability will get triggered. */
	AbilityTrigger::Type trigger_type = AbilityTrigger::GampeplayEvent;

	static void _bind_methods();
};

/**
 * This class defines a gameplay ability serves as runtime container for effects.
 * Two virtual methods are defined for the user to override in script:
 *     _on_activate_ability()     - Gets called if activation was requested.
 *     _on_end_ability(cancelled) - Gets called if ability should end, either normally or cancelled.
 *     _on_gameplay_event(event)  - Gets called if a gameplay event is received.
 *
 *     _can_event_activate_ability(event) - Return true if event can activate this ability.
 *     _can_activate_ability(target)      - Return true if ability can be activated and applied on target.
 *
 *     _on_wait_completed(type, data) - Gets executed if the wait handle got triggered.
 *     _on_wait_interrupted(payload)  - Gets executed if a wait handle got interrupted by another one or if explicitly called.
 *     _on_wait_cancelled(payload)    - Gets executed if the ability got cancelled while waiting.
 */
class GAMEPLAY_ABILITIES_API GameplayAbility : public GameplayNode {
	GDCLASS(GameplayAbility, GameplayNode);
	OBJ_CATEGORY("GameplayAbilities");

	friend class GameplayAbilitySystem;

public:
	virtual ~GameplayAbility() = default;

	/** Wait handle stuff */
	struct WaitData {
		WaitType::Type type = WaitType::None;
		Variant data;
	};

	GameplayAbility();

	const WaitData &get_wait_handle() const;
	void initialise(GameplayAbilitySystem *system);

	/** Returns true if this ability is active in any way, shape or form. */
	bool is_active() const;
	/** Checks if ability is currently on cooldown. */
	bool is_cooldown() const;
	/** Returns true if this ability got triggered. */
	bool is_triggerable() const;
	/** Returns normalised level necessary for calculations. */
	double get_normalised_level() const;

	/** Returns true if tag triggers ability. */
	bool can_trigger(const String &trigger_tag, AbilityTrigger::Type trigger_type) const;
	/** Checks trigger and then either calls _can_event_activate_ability or returns true. */
	bool can_event_activate_ability(const Ref<GameplayEvent> &event);
	/** Tries to activate ability via given gameplay event. */
	bool try_event_activate_ability(const Ref<GameplayEvent> &event);
	/** Checks tag requirements and then either calls _can_activate_ability or returns true. */
	bool can_activate_ability();
	/** Checks if ability can be activated on given target. */
	bool can_activate_ability_on_target(const Node *target);
	/** Calls can_activate_ability and will activate ability depending on return value. */
	bool try_activate_ability();
	/** Activates ability. */
	void activate_ability();
	/** Commit ability cost, has to be called by _on_activate_ability. */
	void commit_ability();
	/** Ends ability activation, has to be called by _on_activate_ability. */
	void end_ability();
	/** Cancels active ability or does nothing if not active. */
	void cancel_ability();
	/** Gets the remaining cooldown on this ability. */
	double get_remaining_cooldown() const;
	/** Checks if ability has enough resources to activate. */
	bool check_ability_cost() const;
	/** Checks if ability is not on cooldown. */
	bool check_ability_cooldown() const;

	/** Adds an effect to the source. */
	void apply_effect_on_source(const Ref<GameplayEffect> &effect, int64_t stacks = 1, int64_t level = -1);
	/** Adds an effect to the target. */
	void apply_effect_on_target(Node *target, const Ref<GameplayEffect> &effect, int64_t stacks = 1, int64_t level = -1);
	/** Adds an effect to several targets. */
	void apply_effect_on_targets(const Array &targets, const Ref<GameplayEffect> &effect, int64_t stacks = 1, int64_t level = -1);
	/** Removes an effect from the source. */
	void remove_effect_from_source(const Ref<GameplayEffect> &effect, int64_t stacks = 1, int64_t level = -1);
	/** Removes an effect from the target. */
	void remove_effect_from_target(Node *target, const Ref<GameplayEffect> &effect, int64_t stacks = 1, int64_t level = -1);
	/** Removes an effect from several targets. */
	void remove_effect_on_targets(const Array &targets, const Ref<GameplayEffect> &effect, int64_t stacks = 1, int64_t level = -1);

	/** Executes a cue or cues of given tag on the target. */
	void execute_gameplay_cue(const String &cue_tag, Node *target);
	/** Same as execute_cue but with additional arguments. */
	void execute_gameplay_cue_parameters(const String &cue_tag, Node *target, double level, double magnitude);
	/** Adds a persistent cue to the owner and will either remove it on ability end or not. */
	void add_gameplay_cue(const String &cue_tag, Node *target, bool remove_on_ability_end = true);
	/** Same as add_gameplay_cue but with additional arguments. */
	void add_gameplay_cue_paramters(const String &cue_tag, Node *target, double level, double magnitude, bool remove_on_ability_end = true);
	/** Removes a persistent cue. */
	void remove_gameplay_cue(const String &cue_tag, Node *target);

	void set_ability_name(const StringName &value);
	StringName get_ability_name() const;
	void set_triggers(const Array &value);
	const Array &get_triggers() const;
	void set_cooldown_effect(const Ref<GameplayEffect> &value);
	const Ref<GameplayEffect> &get_cooldown_effect() const;
	void set_cost_effect(const Ref<GameplayEffect> &value);
	const Ref<GameplayEffect> &get_cost_effect() const;
	void set_max_level(int64_t value);
	int64_t get_max_level() const;
	void set_current_level(int64_t value);
	int64_t get_current_level() const;
	void set_input_action(const StringName &value);
	StringName get_input_action() const;

	void set_ability_tags(const Ref<GameplayTagContainer> &value);
	Ref<GameplayTagContainer> get_ability_tags() const;
	void set_cancel_ability_tags(const Ref<GameplayTagContainer> &value);
	Ref<GameplayTagContainer> get_cancel_ability_tags() const;
	void set_block_ability_tags(const Ref<GameplayTagContainer> &value);
	Ref<GameplayTagContainer> get_block_ability_tags() const;
	void set_source_required_tags(const Ref<GameplayTagContainer> &value);
	Ref<GameplayTagContainer> get_source_required_tags() const;
	void set_source_blocked_tags(const Ref<GameplayTagContainer> &value);
	Ref<GameplayTagContainer> get_source_blocked_tags() const;
	void set_target_required_tags(const Ref<GameplayTagContainer> &value);
	Ref<GameplayTagContainer> get_target_required_tags() const;
	void set_target_blocked_tags(const Ref<GameplayTagContainer> &value);
	Ref<GameplayTagContainer> get_target_blocked_tags() const;

	/** Wait methods for asynchronous operations and ability execution. Each of those method will call a virtual _on_* where star is replace by method name. */

	/** Time based wait handle. */
	void wait_delay(double seconds);

	/** Event based wait handle. */
	void wait_event(const String &event_tag);

	/** Input wait handles. */
	void wait_action_pressed(const StringName &action);
	void wait_action_released(const StringName &action);

	/** Attribute wait handles. */
	void wait_attribute_change(const StringName &attribute);
	void wait_base_attribute_change(const StringName &attribute);

	/** Effect wait handles. */
	void wait_effect_added(const Ref<GameplayEffect> &effect);
	void wait_effect_removed(const Ref<GameplayEffect> &effect);

	/** Tag based wait handle. */
	void wait_tag_added(const String &tag);
	void wait_tag_removed(const String &tag);

	/** Processes wait handle. */
	void process_wait(WaitType::Type process_type, const Variant &data);

	/** Node specific functions. */
	void ability_process(double delta);
	void ability_input();

	void set_ability_process(bool value);
	void set_ability_input(bool value);

	void set_targets(const Array &value);
	const Array &get_targets() const;
	/** Filters all targets on which this ability can be activated. */
	Array filter_targets();

protected:
	void _notification(int notification);

	void handle_wait_cancel();
	void handle_wait_interrupt(WaitType::Type wait_type);
	void reset_wait_handle();

private:
	/** Ability name to distinguish from other abilities. */
	StringName ability_name;

	/** Defines how this ability shall be triggered. */
	ArrayContainer<GameplayAbilityTriggerData> triggers;
	/** Effect for cooldown evaluation. */
	Ref<GameplayEffect> cooldown_effect;
	/** Effect for cost evaluation. */
	Ref<GameplayEffect> cost_effect;
	/** Maximum attainable level. */
	int64_t maximum_level = 1;
	/** Level of the ability. */
	int64_t current_level = 1;
	/** Input action this ability listens to. */
	StringName input_action;

	/** Gameplay cues this ability has. */
	Ref<GameplayTagContainer> gameplay_cues = make_reference<GameplayTagContainer>();

	/** Tags this ability has. */
	Ref<GameplayTagContainer> ability_tags = make_reference<GameplayTagContainer>();
	/** Cancels active abilities with  any of these tags while this one is active. */
	Ref<GameplayTagContainer> cancel_abilities_tags = make_reference<GameplayTagContainer>();
	/** Blocks activation of abilities with any of these tags while this one is active. */
	Ref<GameplayTagContainer> block_abilities_tags = make_reference<GameplayTagContainer>();
	/** The owner of this ability will receive these tags while it is activated. */
	Ref<GameplayTagContainer> activation_granted_tags = make_reference<GameplayTagContainer>();
	/** Ability can only activate if source has all of these tags. */
	Ref<GameplayTagContainer> source_required_tags = make_reference<GameplayTagContainer>();
	/** Ability activation is blocked if source has any of these tags. */
	Ref<GameplayTagContainer> source_blocked_tags = make_reference<GameplayTagContainer>();
	/** Ability can only inflict if target has all of these tags. */
	Ref<GameplayTagContainer> target_required_tags = make_reference<GameplayTagContainer>();
	/** Ability infliction is blocked if target has any of these tags. */
	Ref<GameplayTagContainer> target_blocked_tags = make_reference<GameplayTagContainer>();

	/** Targets at the time this ability got activated. */
	Array targets;

	/** Flag signifying if this ability is active. */
	bool active = false;

	/** Internal stuff for execution handling. */
	GameplayAbilitySystem *source = nullptr;

	bool should_ability_process = true;
	bool should_ability_input = true;

	static bool check_tag_requirement(const Ref<GameplayTagContainer> &tags, const Ref<GameplayTagContainer> &required, const Ref<GameplayTagContainer> &blocked);

	/** Data for wait handle which will be processed. */
	WaitData wait_handle;

	static void _bind_methods();
};
