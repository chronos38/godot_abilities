#pragma once

#include "gameplay_node.h"

#include <core/resource.h>
#include <core/script_language.h>
#include <scene/resources/curve.h>

class GameplayAttribute;
class GameplayAbilitySystem;
class GameplayTagContainer;
class GameplayEffect;

/** Base resource for magnitude calculations */
class GAMEPLAY_ABILITIES_API GameplayEffectMagnitude : public GameplayResource {
	GDCLASS(GameplayEffectMagnitude, GameplayResource);
	OBJ_CATEGORY("GameplayAbilities");

public:
	virtual ~GameplayEffectMagnitude() = default;

	/** Has to be overridden, will otherwise return always 0. */
	virtual double calculate_magnitude(const Node *source, const Node *target, const Ref<GameplayEffect> &effect, int64_t level, double normalised_level);

private:
	static void _bind_methods();
};

/** Describes either a flat float value or scaled one via a curve. */
class GAMEPLAY_ABILITIES_API ScalableFloat : public GameplayEffectMagnitude {
	GDCLASS(ScalableFloat, GameplayEffectMagnitude);
	OBJ_CATEGORY("GameplayAbilities");

public:
	virtual ~ScalableFloat() = default;

	/** value * curve->interpolate(level) */
	double calculate_magnitude(const Node *source, const Node *target, const Ref<GameplayEffect> &effect, int64_t level, double normalised_level) override;

	void set_value(double value);
	double get_value() const;
	void set_curve(const Ref<Curve> &value);
	Ref<Curve> get_curve() const;

private:
	/** Flat value to use or multiply with curve level. */
	double value = 0;
	/** Curve graph for multiplication. */
	Ref<Curve> curve;

	static void _bind_methods();
};

/** From where to get attribute values. */
namespace AttributeOrigin {
enum Type {
	/** Attribute is read from source. */
	Source,
	/** Attribute is read from target. */
	Target
};
}

VARIANT_ENUM_CAST(AttributeOrigin::Type);

/** Which value is used for attribute calculation. */
namespace AttributeCalculation {
enum Type {
	/** Use the current attribute value. */
	CurrentValue,
	/** Use the base attribute value. */
	BaseValue,
	/** Use the delta between (Current - Base). */
	DeltaValue
};
}

VARIANT_ENUM_CAST(AttributeCalculation::Type);

/** Class for attribute based magnitudes. */
class GAMEPLAY_ABILITIES_API AttributeBasedFloat : public GameplayEffectMagnitude {
	GDCLASS(AttributeBasedFloat, GameplayEffectMagnitude);
	OBJ_CATEGORY("GameplayAbilities");

public:
	virtual ~AttributeBasedFloat() = default;

	/** coefficient * (pre_multiply_addition + attribute * curve->interpolate(level)) + post_multiply_addition */
	double calculate_magnitude(const Node *source, const Node *target, const Ref<GameplayEffect> &effect, int64_t level, double normalised_level) override;

	void set_coefficient(const Ref<ScalableFloat> &value);
	Ref<ScalableFloat> get_coefficient() const;
	void set_pre_multiply_addition(const Ref<ScalableFloat> &value);
	Ref<ScalableFloat> get_pre_multiply_addition() const;
	void set_post_multiply_addition(const Ref<ScalableFloat> &value);
	Ref<ScalableFloat> get_post_multiply_addition() const;
	void set_backing_attribute(const StringName &value);
	StringName get_backing_attribute() const;
	void set_attribute_curve(const Ref<Curve> &value);
	Ref<Curve> get_attribute_curve() const;
	void set_attribute_origin(AttributeOrigin::Type value);
	AttributeOrigin::Type get_attribute_origin() const;
	void set_attribute_calculation(AttributeCalculation::Type value);
	AttributeCalculation::Type get_attribute_calculation() const;
	void set_source_tag_filter(const Ref<GameplayTagContainer> &value);
	Ref<GameplayTagContainer> get_source_tag_filter() const;
	void set_target_tag_filter(const Ref<GameplayTagContainer> &value);
	Ref<GameplayTagContainer> get_target_tag_filter() const;

private:
	static constexpr auto ATTRIBUTE_ORIGIN_SOURCE = AttributeOrigin::Source;
	static constexpr auto ATTRIBUTE_ORIGIN_TARGET = AttributeOrigin::Target;

	static constexpr auto ATTRIBUTE_CALCULATION_CURRENT_VALUE = AttributeCalculation::CurrentValue;
	static constexpr auto ATTRIBUTE_CALCULATION_BASE_VALUE = AttributeCalculation::BaseValue;
	static constexpr auto ATTRIBUTE_CALCULATION_DELTA_VALUE = AttributeCalculation::DeltaValue;

	/** Coefficient for attribute added by pre multiplicative value. */
	Ref<ScalableFloat> coefficient;
	/** Added to calculation before applying coefficient. */
	Ref<ScalableFloat> pre_multiply_addition;
	/** Added to calculation after coefficient has been applied. */
	Ref<ScalableFloat> post_multiply_addition;

	/** Attribute value to capture. */
	StringName backing_attribute;
	/** Attribute value to capture. */
	Ref<Curve> attribute_curve;
	/** From where to get the attribute. */
	AttributeOrigin::Type attribute_origin = AttributeOrigin::Source;
	/** Which attribute value to capture. */
	AttributeCalculation::Type attribute_calculation = AttributeCalculation::CurrentValue;

	/** Only applies calculation if all tags are present on the source. */
	Ref<GameplayTagContainer> source_tag_filter = make_reference<GameplayTagContainer>();
	/** Only applies calculation if all tags are present on the target. */
	Ref<GameplayTagContainer> target_tag_filter = make_reference<GameplayTagContainer>();

	static void _bind_methods();
};

/** Custom calculator for gameplay effect magnitudes. */
class GAMEPLAY_ABILITIES_API CustomMagnitudeCalculator : public GameplayResource {
	GDCLASS(CustomMagnitudeCalculator, GameplayResource);
	OBJ_CATEGORY("GameplayAbilities");

	static void _bind_methods();
};

/** Class to provide a more in depth way of calculating magnitudes. */
class GAMEPLAY_ABILITIES_API CustomCalculatedFloat : public GameplayEffectMagnitude {
	GDCLASS(CustomCalculatedFloat, GameplayEffectMagnitude);
	OBJ_CATEGORY("GameplayAbilities");

public:
	virtual ~CustomCalculatedFloat() = default;

	/** coefficient * (pre_multiply_addition + script->calculate_magnitude(...)) + post_multiply_addition */
	double calculate_magnitude(const Node *source, const Node *target, const Ref<GameplayEffect> &effect, int64_t level, double normalised_level) override;

	void set_coefficient(const Ref<ScalableFloat> &value);
	Ref<ScalableFloat> get_coefficient() const;
	void set_pre_multiply_addition(const Ref<ScalableFloat> &value);
	Ref<ScalableFloat> get_pre_multiply_addition() const;
	void set_post_multiply_addition(const Ref<ScalableFloat> &value);
	Ref<ScalableFloat> get_post_multiply_addition() const;
	void set_calculation_script(const Ref<Script> &value);
	Ref<Script> get_calculation_script() const;

private:
	/** Coefficient for attribute added by pre multiplicative value. */
	Ref<ScalableFloat> coefficient;
	/** Added to calculation before applying coefficient. */
	Ref<ScalableFloat> pre_multiply_addition;
	/** Added to calculation after coefficient has been applied. */
	Ref<ScalableFloat> post_multiply_addition;

	/** Custom calculation script, has to extend CustomMagnitudeCalculator. */
	Ref<Script> custom_calculation_script;

	/** Laze loaded script instance. Will be created at first usage and used henceforth. */
	GameplayPtr<ScriptInstance> script = nullptr;

	static void _bind_methods();
};
