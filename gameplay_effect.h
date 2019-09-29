#pragma once

//#include "gameplay_effect_magnitude.h"
#include "gameplay_node.h"

class GameplayTagContainer;
class GameplayAttribute;
class GameplayEffect;
class ScalableFloat;
class GameplayEffectNode;
class GameplayEffectMagnitude;
class GameplayAbility;
class GameplayAbilitySystem;
class PackedScene;

/** Defines how effect duration should be handled. */
namespace DurationType {
enum Type {
	/** Effect gets applied instantaneously. */
	Instant,
	/** Effect is applied infinitely. */
	Infinite,
	/** Effect is applied over specified duration. */
	HasDuration
};
}

VARIANT_ENUM_CAST(DurationType::Type);

/** Defines how calculated magnitude gets applied. */
namespace ModifierOperation {
enum Type {
	/** Adds magnitude to attribute. */
	Add,
	/** Subtracts magnitude from attribute. */
	Subtract,
	/** Multiplies attribute with magnitude. */
	Multiply,
	/** Divides attribute by magnitude. */
	Divide,
	/** Overrides attribute with magnitude. */
	Override
};
}

VARIANT_ENUM_CAST(ModifierOperation::Type);

/** Defines stacking for effects. */
namespace StackingType {
enum Type {
	/** Each application is treated is separate instance. */
	None,
	/** Stacks are aggregate on the source. */
	AggregateOnSource,
	/** Stacks are aggregate on the target. */
	AggregateOnTarget
};
}

VARIANT_ENUM_CAST(StackingType::Type);

/** Defines how duration gets refreshed on stacking. */
namespace StackDurationRefresh {
enum Type {
	/** Duration gets refreshed on each applied stack. */
	OnApplication,
	/** Duration gets never refreshed. */
	NeverRefresh
};
}

VARIANT_ENUM_CAST(StackDurationRefresh::Type);

/** Defines how period gets reset on stacking. */
namespace StackPeriodReset {
enum Type {
	/** Reset period on each applied stack. */
	OnApplication,
	/** Period gets never reseted. */
	NeverReset
};
}

VARIANT_ENUM_CAST(StackPeriodReset::Type);

/** Defines what happens if duration expires. */
namespace StackExpiration {
enum Type {
	/** Clears stack and effect expires. */
	ClearStack,
	/** Refresh duration and remove one stack. Effect expires after last stack is removed. */
	RemoveSingleStackAndRefreshDuration,
	/** Just refresh the duration. */
	RefreshDuration
};
}

VARIANT_ENUM_CAST(StackExpiration::Type);

/** Defines what attribute gets modified. */
class GAMEPLAY_ABILITIES_API GameplayEffectModifier : public GameplayResource {
	GDCLASS(GameplayEffectModifier, GameplayResource);
	OBJ_CATEGORY("GameplayAbilities");

public:
	virtual ~GameplayEffectModifier() = default;

	void set_attribute(const StringName &value);
	StringName get_attribute() const;
	void set_modifier_operation(ModifierOperation::Type value);
	ModifierOperation::Type get_modifier_operation() const;
	void set_modifier_magnitude(const Ref<GameplayEffectMagnitude> &value);
	Ref<GameplayEffectMagnitude> get_modifier_magnitude() const;

private:
	static constexpr auto MODIFIER_OPERATION_ADD = ModifierOperation::Add;
	static constexpr auto MODIFIER_OPERATION_SUBTRACT = ModifierOperation::Subtract;
	static constexpr auto MODIFIER_OPERATION_MULTIPLY = ModifierOperation::Multiply;
	static constexpr auto MODIFIER_OPERATION_DIVIDE = ModifierOperation::Divide;
	static constexpr auto MODIFIER_OPERATION_OVERRIDE = ModifierOperation::Override;

	/** Attribute that gets modified. */
	StringName attribute;
	/** Operation for modification. */
	ModifierOperation::Type modifier_operation = ModifierOperation::Add;
	/** Magnitude to apply. */
	Ref<GameplayEffectMagnitude> modifier_magnitude;
	/** ??? */
	Ref<GameplayTagContainer> source_tags = make_reference<GameplayTagContainer>();
	/** ??? */
	Ref<GameplayTagContainer> target_tags = make_reference<GameplayTagContainer>();

	static void _bind_methods();
};

/** Custom execution result class. */
class GAMEPLAY_ABILITIES_API GameplayEffectCustomExecutionResult : public GameplayResource {
	GDCLASS(GameplayEffectCustomExecutionResult, GameplayResource);
	OBJ_CATEGORY("GameplayAbilities");

public:
	virtual ~GameplayEffectCustomExecutionResult() = default;

	/** For ability system to read custom modifiers. */
	const Array &get_modifiers() const;
	/** Check if additional effects should be triggered. */
	bool should_trigger_additional_effects() const;

protected:
	/** Adds an additional modifier to the result. */
	void add_modifier(const Ref<GameplayEffectModifier> &modifier);
	/** Set if additional effects should be triggered. */
	void set_trigger_additional_effects(bool value);

private:
	/** Additional modifiers to apply. */
	Array modifiers;
	/** Trigger additional effects after immediately. */
	bool trigger_additional_effects = false;

	static void _bind_methods();
};

/** Defines virtual method with arguments that should return a GameplayEffectCustomExecutionResult instance. */
class GAMEPLAY_ABILITIES_API GameplayEffectCustomExecutionScript : public GameplayResource {
	GDCLASS(GameplayEffectCustomExecutionScript, GameplayResource);
	OBJ_CATEGORY("GameplayAbilities");

public:
	virtual ~GameplayEffectCustomExecutionScript() = default;

private:
	static void _bind_methods();
};

/** Container class for custom execution script which provides native interface for custom execution. */
class GAMEPLAY_ABILITIES_API GameplayEffectCustomExecution : public GameplayResource {
	GDCLASS(GameplayEffectCustomExecution, GameplayResource);
	OBJ_CATEGORY("GameplayAbilities");

public:
	virtual ~GameplayEffectCustomExecution() = default;

	Ref<GameplayEffectCustomExecutionResult> execute(Node *source, Node *target, GameplayEffectNode *effect, int64_t level, double normalised_level);

	void set_execution_script(const Ref<Script> &value);
	Ref<Script> get_execution_script();

private:
	/** Calculation script which inherits from GameplayEffectCustomExecutionScript or implements at least the required method. */
	Ref<Script> execution_script;

	/** Laze loaded script instance. Will be created at first usage and used henceforth. */
	GameplayPtr<ScriptInstance> script = nullptr;

	static void _bind_methods();
};

/** Defines virtual method with arguments that should return a bool. */
class GAMEPLAY_ABILITIES_API GameplayEffectCustomApplicationRequirementScript : public GameplayResource {
	GDCLASS(GameplayEffectCustomApplicationRequirementScript, GameplayResource);
	OBJ_CATEGORY("GameplayAbilities");

public:
	virtual ~GameplayEffectCustomApplicationRequirementScript() = default;

private:
	static void _bind_methods();
};

/** Defines custom application requirements. */
class GAMEPLAY_ABILITIES_API GameplayEffectCustomApplicationRequirement : public GameplayResource {
	GDCLASS(GameplayEffectCustomApplicationRequirement, GameplayResource);
	OBJ_CATEGORY("GameplayAbilities");

public:
	virtual ~GameplayEffectCustomApplicationRequirement() = default;

	bool execute(const Node *source, const Node *target, const Ref<GameplayEffect> &effect, int64_t level, double normalised_level);

	void set_requirement_script(const Ref<Script> &value);
	Ref<Script> get_requirement_script();

private:
	/** Requirement script which returns true if effect can be applied. */
	Ref<Script> requirement_script;

	/** Laze loaded script instance. Will be created at first usage and used henceforth. */
	GameplayPtr<ScriptInstance> script = nullptr;

	static void _bind_methods();
};

/** Defines a conditional gameplay effect. */
class GAMEPLAY_ABILITIES_API ConditionalGameplayEffect : public GameplayResource {
	GDCLASS(ConditionalGameplayEffect, GameplayResource);
	OBJ_CATEGORY("GameplayAbilities");

public:
	virtual ~ConditionalGameplayEffect() = default;

	/** Check if the source tags satisfy condition. */
	bool can_apply(const Ref<GameplayTagContainer> &source_tags) const;

	void set_effect(const Ref<GameplayEffect> &value);
	Ref<GameplayEffect> get_effect() const;
	void set_required_source_tags(const Ref<GameplayTagContainer> &value);
	Ref<GameplayTagContainer> get_required_source_tags() const;

private:
	/** Gameplay effect that will be applied to target. */
	Ref<GameplayEffect> effect;
	/** Tags the source has to have for the effect to apply. */
	Ref<GameplayTagContainer> required_source_tags = make_reference<GameplayTagContainer>();

	static void _bind_methods();
};

/** Cues serve as trigger for other engine systems such as animation and sound. */
class GAMEPLAY_ABILITIES_API GameplayEffectCue : public GameplayResource {
	GDCLASS(GameplayEffectCue, GameplayResource);
	OBJ_CATEGORY("GameplayAbilities");

public:
	virtual ~GameplayEffectCue() = default;

	void set_minimum_level(double value);
	double get_minimum_level() const;
	void set_maximum_level(double value);
	double get_maximum_level() const;
	void set_cue_tags(const Ref<GameplayTagContainer> &value);
	Ref<GameplayTagContainer> get_cue_tags() const;

private:
	/** Minimum level this cue supports. */
	double minimum_level = 0;
	/** Maximum level this cue supports. */
	double maximum_level = 1;
	/** Tags the source has to have for this cue to trigger. */
	Ref<GameplayTagContainer> cue_tags = make_reference<GameplayTagContainer>();

	static void _bind_methods();
};

/**
 * Effect class is a data holder which gets applied to the target and defines how and what gets modified in what capacity.
 */
class GAMEPLAY_ABILITIES_API GameplayEffect : public GameplayResource {
	GDCLASS(GameplayEffect, GameplayResource);
	OBJ_CATEGORY("GameplayAbilities");

public:
	virtual ~GameplayEffect() = default;

	void set_effect_name(const StringName &value);
	StringName get_effect_name() const;
	void set_duration_type(DurationType::Type value);
	DurationType::Type get_duration_type() const;
	void set_duration_magnitude(const Ref<GameplayEffectMagnitude> &value);
	Ref<GameplayEffectMagnitude> get_duration_magnitude() const;
	void set_period(const Ref<ScalableFloat> &value);
	Ref<ScalableFloat> get_period() const;
	void set_execute_period_on_application(bool value);
	bool get_execute_period_on_application() const;
	void set_modifiers(const Array &value);
	const Array &get_modifiers() const;
	void set_executions(const Array &value);
	const Array &get_executions() const;
	void set_infliction_chance(const Ref<ScalableFloat> &value);
	Ref<ScalableFloat> get_infliction_chance() const;
	void set_application_requirements(const Array &value);
	const Array &get_application_requirements() const;
	void set_conditional_erffects(const Array &value);
	const Array &get_conditional_erffects() const;
	void set_overflow_effects(const Array &value);
	const Array &get_overflow_effects() const;
	void set_deny_overflow_application(bool value);
	bool get_deny_overflow_application() const;
	void set_clear_overflow_stack(bool value);
	bool get_clear_overflow_stack() const;
	void set_premature_expiration_effects(const Array &value);
	const Array &get_premature_expiration_effects() const;
	void set_normal_expiration_effects(const Array &value);
	const Array &get_normal_expiration_effects() const;
	void set_cues_require_successful_application(bool value);
	bool get_cues_require_successful_application() const;
	void set_cues_ignore_stacking(bool value);
	bool get_cues_ignore_stacking() const;
	void set_cues(const Array &value);
	const Array &get_cues() const;

	void set_effect_tags(const Ref<GameplayTagContainer> &value);
	Ref<GameplayTagContainer> get_effect_tags() const;
	void set_target_tags(const Ref<GameplayTagContainer> &value);
	Ref<GameplayTagContainer> get_target_tags() const;
	void set_ongoing_tags(const Ref<GameplayTagContainer> &value);
	Ref<GameplayTagContainer> get_ongoing_tags() const;
	void set_remove_effect_tags(const Ref<GameplayTagContainer> &value);
	Ref<GameplayTagContainer> get_remove_effect_tags() const;
	void set_application_immunity_tags(const Ref<GameplayTagContainer> &value);
	Ref<GameplayTagContainer> get_application_immunity_tags() const;
	void set_cancel_ability_tags(const Ref<GameplayTagContainer> &value);
	Ref<GameplayTagContainer> get_cancel_ability_tags() const;

	void set_stacking_type(StackingType::Type value);
	StackingType::Type get_stacking_type() const;
	void set_maximum_stacks(int64_t value);
	int64_t get_maximum_stacks() const;
	void set_duration_refresh(StackDurationRefresh::Type value);
	StackDurationRefresh::Type get_duration_refresh() const;
	void set_period_reset(StackPeriodReset::Type value);
	StackPeriodReset::Type get_period_reset() const;
	void set_stack_expiration(StackExpiration::Type value);
	StackExpiration::Type get_stack_expiration() const;

	void set_granted_abilities(const Array &value);
	const Array &get_granted_abilities() const;

private:
	static constexpr auto DURATION_TYPE_INSTANT = DurationType::Instant;
	static constexpr auto DURATION_TYPE_INFINITE = DurationType::Infinite;
	static constexpr auto DURATION_TYPE_HAS_DURATION = DurationType::HasDuration;

	static constexpr auto STACKING_TYPE_NONE = StackingType::None;
	static constexpr auto STACKING_TYPE_AGGREGATE_SOURCE = StackingType::AggregateOnSource;
	static constexpr auto STACKING_TYPE_AGGREGATE_TARGET = StackingType::AggregateOnTarget;

	static constexpr auto STACKING_DURATION_REFRESH_ON_APPLICATION = StackDurationRefresh::OnApplication;
	static constexpr auto STACKING_DURATION_REFRESH_NEVER = StackDurationRefresh::NeverRefresh;

	static constexpr auto STACKING_PERIOD_RESET_ON_APPLICATION = StackPeriodReset::OnApplication;
	static constexpr auto STACKING_PERIOD_RESET_NEVER = StackPeriodReset::NeverReset;

	static constexpr auto STACKING_EXPIRATION_CLEAR = StackExpiration::ClearStack;
	static constexpr auto STACKING_EXPIRATION_REMOVE_REFRESH = StackExpiration::RemoveSingleStackAndRefreshDuration;
	static constexpr auto STACKING_EXPIRATION_REFRESH = StackExpiration::RefreshDuration;

	/** Effect name distinguishing it from others. */
	StringName effect_name;
	/** How the effect gets handled in a timely manner. */
	DurationType::Type duration_type = DurationType::Instant;
	/** If the effect has a duration, then use this to calculate it. 0 is considered as instant and negative values as infinite. */
	Ref<GameplayEffectMagnitude> duration_magnitude;
	/** Period of this effect, negative and 0 values are considered non-periodic. */
	Ref<ScalableFloat> period;
	/** Apply effects on application and each period, or apply only at each period. */
	bool execute_period_on_application = true;
	/** Modifiers that this effect applies on the target. */
	ArrayContainer<GameplayEffectModifier> modifiers;
	/** Custom executions which apply for this effect. */
	ArrayContainer<GameplayEffectCustomExecution> executions;
	/** Value between 0 (for never) and 1 (for always) or if not set, then considered 1. */
	Ref<ScalableFloat> infliction_chance;
	/** Custom requirements for additional application checks. */
	ArrayContainer<GameplayEffectCustomApplicationRequirement> application_requirements;
	/** Conditional effects get applied if a custom execution triggers them. */
	ArrayContainer<ConditionalGameplayEffect> conditional_erffects;
	/** Effects that apply if the stack overflows. */
	ArrayContainer<GameplayEffect> overflow_effects;
	/** Denies application while at maximum stack count. */
	bool deny_overflow_application = false;
	/** Clears stack if it overflows. */
	bool clear_overflow_stack = false;
	/** Effects that get applied if this effects expires prematurely. */
	ArrayContainer<GameplayEffect> premature_expiration_effects;
	/** Effects that get applied if this effects expires normally. */
	ArrayContainer<GameplayEffect> normal_expiration_effects;
	/** Cues require successful application to be triggered. */
	bool cues_require_successful_application = false;
	/** Cues will only trigger on application but not on stacks. */
	bool cues_ignore_stacking = false;
	/** Cues which will get activated if this effect applies or if a custom execution triggers them. */
	ArrayContainer<GameplayEffectCue> cues;

	/** Tags this effect has and is checked against. */
	Ref<GameplayTagContainer> effect_tags = make_reference<GameplayTagContainer>();
	/** Tags that are applied to the target. */
	Ref<GameplayTagContainer> target_tags = make_reference<GameplayTagContainer>();
	/** Tags that are checked if this effect is active or not. */
	Ref<GameplayTagContainer> ongoing_tags = make_reference<GameplayTagContainer>();
	/** Effects with any of these tags will be removed. */
	Ref<GameplayTagContainer> remove_effect_tags = make_reference<GameplayTagContainer>();
	/** Target has immunity against these effect tags. */
	Ref<GameplayTagContainer> application_immunity_tags = make_reference<GameplayTagContainer>();
	/** Cancels abilities with any of these tags who are currently active. */
	Ref<GameplayTagContainer> cancel_ability_tags = make_reference<GameplayTagContainer>();

	/** How stacking is handled. */
	StackingType::Type stacking_type = StackingType::None;
	/** Maximum amount of stacks. */
	int64_t maximum_stacks = 1;
	/** How to refresh duration on stacking. */
	StackDurationRefresh::Type duration_refresh = StackDurationRefresh::OnApplication;
	/** How to reset period on stacking. */
	StackPeriodReset::Type period_reset = StackPeriodReset::OnApplication;
	/** What happens with stack if duration expires. */
	StackExpiration::Type stack_expiration = StackExpiration::RemoveSingleStackAndRefreshDuration;

	/** Abilities added to target while this effect is active. */
	ArrayContainer<PackedScene> granted_abilities;

	static void _bind_methods();
};
