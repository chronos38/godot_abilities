#pragma once

#include "gameplay_node.h"

#include <core/hash_map.h>
#include <core/vector.h>

#include <cmath>
#include <random>

class GameplayEffect;
class GameplayEffectCue;
class GameplayEffectModifier;
class GameplayAbility;
class GameplayAttribute;
class GameplayAttributeData;
class GameplayAttributeSet;
class GameplayTagContainer;
class GameplayAbilitySystem;

namespace UpdateAttributeOperation {
enum Type {
	/** Won't update current value. */
	None,
	/** Will multiply current value with relative change of base value. */
	Relative,
	/** Will add delta of base change to current value. */
	Absolute,
	/** Overrides current value to new base value. */
	Override
};
}

class GAMEPLAY_ABILITIES_API GameplayEvent : public GameplayResource {
	GDCLASS(GameplayEvent, GameplayResource);
	OBJ_CATEGORY("GameplayAbilities");

public:
	virtual ~GameplayEvent() = default;

	void set_event_tag(const String &value);
	const String &get_event_tag() const;

	void add_event_target(Node *target);
	const Array &get_event_targets() const;

private:
	/** Tag identifying this event. */
	String event_tag;
	/** Event target is the one being targeted by the event. */
	Array event_targets;

	static void _bind_methods();
};

class GAMEPLAY_ABILITIES_API GameplayEffectNode : public GameplayNode {
	GDCLASS(GameplayEffectNode, GameplayNode);
	OBJ_CATEGORY("GameplayAbilities");

public:
	virtual ~GameplayEffectNode() = default;

	void initialise(GameplayAbilitySystem *source, GameplayAbilitySystem *target, const Ref<GameplayEffect> &effect, int64_t level, double normalised_level);

	Node *get_source() const;
	Node *get_target() const;

	Ref<GameplayEffect> get_effect() const;
	double get_duration() const;
	int64_t get_stacks() const;
	int64_t get_level() const;
	double get_normalised_level() const;

	void add_stack(int64_t value);
	void remove_stack(int64_t value);

	void effect_process(double delta);
	void set_effect_process(bool value);

protected:
	void _notification(int notification);

private:
	GameplayAbilitySystem *source = nullptr;
	GameplayAbilitySystem *target = nullptr;
	Ref<GameplayEffect> effect;
	int64_t level = 1;
	int64_t previous_stack = 1;
	double normalised_level = 1;
	double duration = 0;
	double period = 0;

	bool stack_overflow = false;
	bool stack_applied = false;
	bool should_effect_process = true;
	int64_t internal_stacks = 1;

	Vector<GameplayAbility *> granted_abilities;

	double calculate_duration() const;
	double calculate_period_threshold() const;
	void apply_effect(const Ref<GameplayEffect> &effect);
	void apply_effects(const Array &effects);
	void execute_effect();
	GameplayAbilitySystem *get_stacking_system() const;

	void start_effect();
	void end_effect(bool cancelled);

	static void _bind_methods();
};

/**
 * Gameplay ability system is the core processing node if the system, it handles all abilities, inactive or active, active effects and state changes.
 * Additionally the system has the following signals which it will emit.
 *     gameplay_cue_activated(source, cue_tag, level, magnitude, persistent)
 *     gameplay_cue_removed(source, cue_tag)
 *
 *     gameplay_effect_activated(source, effect)
 *     gameplay_effect_infliction_failed(source, effect)
 *     gameplay_effect_removal_failed(source, effect)
 *     gameplay_effect_ended(source, effect, cancelled)
 *
 *     gameplay_ability_activated(source, ability)
 *     gameplay_ability_cancelled(source, ability)
 *     gameplay_ability_blocked(source, ability)
 *     gameplay_ability_ready(source, ability)
 *
 *     gameplay_attribute_changed(source, attribute, old_value)
 *     gameplay_base_attribute_changed(source, attribute, old_base, old_value)
 */
class GAMEPLAY_ABILITIES_API GameplayAbilitySystem : public GameplayNode {
	GDCLASS(GameplayAbilitySystem, GameplayNode);
	OBJ_CATEGORY("GameplayAbilities");

	friend class GameplayEffectNode;
	friend class GameplayAbility;

public:
	GameplayAbilitySystem();
	virtual ~GameplayAbilitySystem() = default;

	/** Gets all currently active and owned tags. */
	const Ref<GameplayAttributeSet> &get_attributes() const;
	/** Gets all currently active and owned tags. */
	Ref<GameplayTagContainer> get_active_tags() const;
	/** Gets abilities from current system. */
	GameplayAbility *get_ability_by_name(const StringName &name) const;
	GameplayAbility *get_ability_by_index(int64_t index) const;
	int64_t get_ability_count() const;
	/** Gets all active abilities. */
	Array get_active_abilities() const;
	/** Intended for internal usage. */
	const Vector<GameplayAbility *> &get_abilities_vector() const;
	const Vector<GameplayAbility *> &get_active_abilities_vector() const;
	/** Gets all active effects on this target. */
	const Ref<GameplayTagContainer> &get_persistent_cues() const;
	/** Queries active effects and returns those which match the given tag. */
	Array query_active_effects_by_tag(const String &tag) const;
	/** Queries active effects and returns those with at least one of the given tags. */
	Array query_active_effects(const Ref<GameplayTagContainer> &tags) const;
	/** Gets remaining duration left on active effect. */
	double get_remaining_effect_duration(const Ref<GameplayEffect> &effect) const;

	/** Returns true if this ability system triggered any abilities via the given event. */
	bool handle_event(const Ref<GameplayEvent> &event);

	/** Checks if an attribute is present in this ability system. */
	bool has_attribute(const StringName &name) const;
	/** Gets attribute data from this ability system or null if no such attribute exists. */
	Ref<GameplayAttribute> get_attribute(const StringName &name) const;
	Ref<GameplayAttributeData> get_attribute_data(const StringName &name) const;
	double get_base_attribute_value(const StringName &name) const;
	double get_current_attribute_value(const StringName &name) const;
	/** Updates base value of attribute. */
	bool update_base_attribute(const StringName &name, double value, UpdateAttributeOperation::Type operation = UpdateAttributeOperation::None);

	/** Adds tags. */
	void add_tag(const String &tag);
	void add_tags(const Ref<GameplayTagContainer> &tags);
	/** Removes tags. */
	void remove_tag(const String &tag);
	void remove_tags(const Ref<GameplayTagContainer> &tags);
	/** Adds a single ability to this instance. */
	void add_ability(Node *ability);
	void add_abilities(const Array &abilities);
	/** Removes a single ability to this instance. */
	void remove_ability(Node *ability);
	void remove_abilities(const Array &abilities);
	/** Tries to activate ability. */
	void activate_ability(Node *ability);
	/** Tries to activate ability. */
	void cancel_ability(Node *ability);
	/** Checks if a single effect is applicable. */
	bool can_apply_effect(Node *source, const Ref<GameplayEffect> &effect, int64_t stacks = 1, int64_t level = 1, double normalised_level = 1) const;
	/** Filters effects and returns array of those applicable. */
	Array filter_effects(Node *source, const Array &effects) const;
	/** Tries to apply given effect and returns success. */
	bool try_apply_effect(Node *source, const Ref<GameplayEffect> &effect, int64_t stacks = 1, int64_t level = 1, double normalised_level = 1);
	/** Adds a single effect from source to this instance. */
	void apply_effect(Node *source, const Ref<GameplayEffect> &effect, int64_t stacks = 1, int64_t level = 1, double normalised_level = 1);
	void apply_effects(Node *source, const Array &effects, int64_t stacks = 1, int64_t level = 1, double normalised_level = 1);
	/** Adds a single effect from source to this instance. */
	void remove_effect(Node *source, const Ref<GameplayEffect> &effect, int64_t stacks = 1, int64_t level = 1);
	void remove_effect_node(Node *source, Node *effect_node, int64_t stacks = 1, int64_t level = 1);
	/** Adds a single cue to this target. */
	void apply_cue(const String &cue, double level = 1, double magnitude = 0, bool persistent = false);
	/** Removes a cue from the system. */
	void remove_cue(const String &cue);

	/** Gets current stack count of given effect. */
	int64_t get_stack_count(const Ref<GameplayEffect> &effect) const;
	/** Gets current stack level for given effect. */
	int64_t get_stack_level(const Ref<GameplayEffect> &effect) const;

	void set_attribute_set(const Ref<GameplayAttributeSet> &value);
	const Ref<GameplayAttributeSet> &get_attribute_set() const;

	/** Targeting */

	void add_target(Node *target);
	void remove_target(Node *target);
	void set_targets(const Array &value);
	const Array &get_targets() const;

protected:
	void _notification(int notification);

private:
	struct ActiveEffectEntry {
		GameplayEffectNode *effect_node = nullptr;
		int64_t level = 1;
		int64_t stacks = 1;
	};

	HashMap<StringName, ActiveEffectEntry> effect_stacking;

	Array targets;
	Ref<GameplayAttributeSet> attributes;
	Ref<GameplayTagContainer> persistent_cues = make_reference<GameplayTagContainer>();
	Ref<GameplayTagContainer> active_tags = make_reference<GameplayTagContainer>();

	Vector<GameplayAbility *> abilities;
	Vector<GameplayAbility *> active_abilities;
	Vector<GameplayEffectNode *> active_effects;

	static std::random_device rdevice;
	static std::default_random_engine rengine;
	static std::uniform_real_distribution<double> rgenerator;

	void execute_effect(GameplayEffectNode *node);
	void apply_modifiers(GameplayEffectNode *node, const Array &modifiers);

	void add_active_ability(GameplayAbility *ability);
	void remove_active_ability(GameplayAbility *ability);

	void add_effect(GameplayAbilitySystem *source, const Ref<GameplayEffect> &effect, int64_t stacks, int64_t level, double normalised_level);

	static double execute_magnitude(double magnitude, double current_value, int operation);
	static void _bind_methods();
};
