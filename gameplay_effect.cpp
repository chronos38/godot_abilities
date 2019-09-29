#include "gameplay_effect.h"
#include "gameplay_ability.h"
#include "gameplay_ability_system.h"
#include "gameplay_attribute.h"
#include "gameplay_effect_magnitude.h"
#include "gameplay_tags.h"

#include <scene/resources/packed_scene.h>

void GameplayEffectModifier::set_attribute(const StringName &value) {
	attribute = value;
}

StringName GameplayEffectModifier::get_attribute() const {
	return attribute;
}

void GameplayEffectModifier::set_modifier_operation(ModifierOperation::Type value) {
	modifier_operation = value;
}

ModifierOperation::Type GameplayEffectModifier::get_modifier_operation() const {
	return modifier_operation;
}

void GameplayEffectModifier::set_modifier_magnitude(const Ref<GameplayEffectMagnitude> &value) {
	modifier_magnitude = value;
}

Ref<GameplayEffectMagnitude> GameplayEffectModifier::get_modifier_magnitude() const {
	return modifier_magnitude;
}

void GameplayEffectModifier::_bind_methods() {
	/** Methods */
	ClassDB::bind_method(D_METHOD("set_attribute", "value"), &GameplayEffectModifier::set_attribute);
	ClassDB::bind_method(D_METHOD("get_attribute"), &GameplayEffectModifier::get_attribute);
	ClassDB::bind_method(D_METHOD("set_modifier_operation", "value"), &GameplayEffectModifier::set_modifier_operation);
	ClassDB::bind_method(D_METHOD("get_modifier_operation"), &GameplayEffectModifier::get_modifier_operation);
	ClassDB::bind_method(D_METHOD("set_modifier_magnitude", "value"), &GameplayEffectModifier::set_modifier_magnitude);
	ClassDB::bind_method(D_METHOD("get_modifier_magnitude"), &GameplayEffectModifier::get_modifier_magnitude);

	/** Properties */
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "attribute"), "set_attribute", "get_attribute");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "modifier_operation", PROPERTY_HINT_ENUM, "Add,Subtract,Multiply,Divide,Override"), "set_modifier_operation", "get_modifier_operation");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "modifier_magnitude", PROPERTY_HINT_RESOURCE_TYPE, "GameplayEffectMagnitude"), "set_modifier_magnitude", "get_modifier_magnitude");

	/** Constants */
	BIND_ENUM_CONSTANT(MODIFIER_OPERATION_ADD);
	BIND_ENUM_CONSTANT(MODIFIER_OPERATION_SUBTRACT);
	BIND_ENUM_CONSTANT(MODIFIER_OPERATION_MULTIPLY);
	BIND_ENUM_CONSTANT(MODIFIER_OPERATION_DIVIDE);
	BIND_ENUM_CONSTANT(MODIFIER_OPERATION_OVERRIDE);
}

const Array &GameplayEffectCustomExecutionResult::get_modifiers() const {
	return modifiers;
}

bool GameplayEffectCustomExecutionResult::should_trigger_additional_effects() const {
	return trigger_additional_effects;
}

void GameplayEffectCustomExecutionResult::add_modifier(const Ref<GameplayEffectModifier> &modifier) {
	modifiers.push_back(modifier);
}

void GameplayEffectCustomExecutionResult::set_trigger_additional_effects(bool value) {
	trigger_additional_effects = value;
}

void GameplayEffectCustomExecutionScript::_bind_methods() {
	BIND_VMETHOD(MethodInfo(Variant::OBJECT, "_execute", PropertyInfo(Variant::OBJECT, "source"), PropertyInfo(Variant::OBJECT, "target"), PropertyInfo(Variant::OBJECT, "effect_node"), PropertyInfo(Variant::INT, "level"), PropertyInfo(Variant::REAL, "normalised_level")));
}

Ref<GameplayEffectCustomExecutionResult> GameplayEffectCustomExecution::execute(Node *source, Node *target, GameplayEffectNode *effect, int64_t level, double normalised_level) {
	if (script.is_null()) {
		script = GameplayPtr<ScriptInstance>(execution_script->instance_create(this));
	}
	if (script.is_valid()) {
		return script->call("_execute", source, target, effect, level, normalised_level);
	} else {
		WARN_PRINTS("Could not instantiate custom effect execution script: " + execution_script->get_path());
	}

	return {};
}

void GameplayEffectCustomExecution::set_execution_script(const Ref<Script> &value) {
	execution_script = value;
}

Ref<Script> GameplayEffectCustomExecution::get_execution_script() {
	return execution_script;
}

void GameplayEffectCustomExecution::_bind_methods() {
	/** Methods */
	ClassDB::bind_method(D_METHOD("set_execution_script"), &GameplayEffectCustomExecution::set_execution_script);
	ClassDB::bind_method(D_METHOD("get_execution_script"), &GameplayEffectCustomExecution::get_execution_script);

	/** Properties */
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "execution_script", PROPERTY_HINT_RESOURCE_TYPE, "Script"), "set_execution_script", "get_execution_script");
}

void GameplayEffectCustomExecutionResult::_bind_methods() {
	/** Methods */
	ClassDB::bind_method(D_METHOD("get_modifiers"), &GameplayEffectCustomExecutionResult::get_modifiers);
	ClassDB::bind_method(D_METHOD("should_trigger_additional_effects"), &GameplayEffectCustomExecutionResult::should_trigger_additional_effects);
	ClassDB::bind_method(D_METHOD("add_modifier", "modifier"), &GameplayEffectCustomExecutionResult::add_modifier);
	ClassDB::bind_method(D_METHOD("set_trigger_additional_effects", "value"), &GameplayEffectCustomExecutionResult::set_trigger_additional_effects);
}

void GameplayEffectCustomApplicationRequirementScript::_bind_methods() {
	BIND_VMETHOD(MethodInfo(Variant::BOOL, "_execute", PropertyInfo(Variant::OBJECT, "source"), PropertyInfo(Variant::OBJECT, "target"), PropertyInfo(Variant::OBJECT, "effect"), PropertyInfo(Variant::INT, "level"), PropertyInfo(Variant::REAL, "normalised_level")));
}

bool GameplayEffectCustomApplicationRequirement::execute(const Node *source, const Node *target, const Ref<GameplayEffect> &effect, int64_t level, double normalised_level) {
	if (script.is_null()) {
		script = GameplayPtr<ScriptInstance>(requirement_script->instance_create(this));
	}
	if (script.is_valid()) {
		return script->call("_execute", source, target, effect, level, normalised_level);
	} else {
		WARN_PRINTS("Could not instantiate custom effect application requirement script: " + requirement_script->get_path());
	}

	return true;
}

void GameplayEffectCustomApplicationRequirement::set_requirement_script(const Ref<Script> &value) {
	requirement_script = value;
}

Ref<Script> GameplayEffectCustomApplicationRequirement::get_requirement_script() {
	return requirement_script;
}

void GameplayEffectCustomApplicationRequirement::_bind_methods() {
	/** Methods */
	ClassDB::bind_method(D_METHOD("set_requirement_script"), &GameplayEffectCustomApplicationRequirement::set_requirement_script);
	ClassDB::bind_method(D_METHOD("get_requirement_script"), &GameplayEffectCustomApplicationRequirement::get_requirement_script);

	/** Properties */
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "requirement_script", PROPERTY_HINT_RESOURCE_TYPE, "Script"), "set_requirement_script", "get_requirement_script");
}

bool ConditionalGameplayEffect::can_apply(const Ref<GameplayTagContainer> &source_tags) const {
	return source_tags->has_all(required_source_tags);
}

void ConditionalGameplayEffect::set_effect(const Ref<GameplayEffect> &value) {
	effect = value;
}

Ref<GameplayEffect> ConditionalGameplayEffect::get_effect() const {
	return effect;
}

void ConditionalGameplayEffect::set_required_source_tags(const Ref<GameplayTagContainer> &) {
	ERR_EXPLAIN("GameplayTagContainer are readonly properties.");
	ERR_FAIL();
}

Ref<GameplayTagContainer> ConditionalGameplayEffect::get_required_source_tags() const {
	return required_source_tags;
}

void ConditionalGameplayEffect::_bind_methods() {
	/** Methods */
	ClassDB::bind_method(D_METHOD("can_apply"), &ConditionalGameplayEffect::can_apply);
	ClassDB::bind_method(D_METHOD("set_effect", "value"), &ConditionalGameplayEffect::set_effect);
	ClassDB::bind_method(D_METHOD("get_effect"), &ConditionalGameplayEffect::get_effect);
	ClassDB::bind_method(D_METHOD("set_required_source_tags", "value"), &ConditionalGameplayEffect::set_required_source_tags);
	ClassDB::bind_method(D_METHOD("get_required_source_tags"), &ConditionalGameplayEffect::get_required_source_tags);

	/** Properties */
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "effect", PROPERTY_HINT_RESOURCE_TYPE, "GameplayEffect"), "set_effect", "get_effect");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "required_source_tags", PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagContainer"), "set_required_source_tags", "get_required_source_tags");
}

void GameplayEffectCue::set_minimum_level(double value) {
	minimum_level = value;
}

double GameplayEffectCue::get_minimum_level() const {
	return minimum_level;
}

void GameplayEffectCue::set_maximum_level(double value) {
	maximum_level = value;
}

double GameplayEffectCue::get_maximum_level() const {
	return maximum_level;
}

void GameplayEffectCue::set_cue_tags(const Ref<GameplayTagContainer> &) {
	ERR_EXPLAIN("GameplayTagContainer are readonly properties.");
	ERR_FAIL();
}

Ref<GameplayTagContainer> GameplayEffectCue::get_cue_tags() const {
	return cue_tags;
}

void GameplayEffectCue::_bind_methods() {
	/** Methods */
	ClassDB::bind_method(D_METHOD("set_minimum_level", "value"), &GameplayEffectCue::set_minimum_level);
	ClassDB::bind_method(D_METHOD("get_minimum_level"), &GameplayEffectCue::get_minimum_level);
	ClassDB::bind_method(D_METHOD("set_maximum_level", "value"), &GameplayEffectCue::set_maximum_level);
	ClassDB::bind_method(D_METHOD("get_maximum_level"), &GameplayEffectCue::get_maximum_level);
	ClassDB::bind_method(D_METHOD("set_cue_tags", "value"), &GameplayEffectCue::set_cue_tags);
	ClassDB::bind_method(D_METHOD("get_cue_tags"), &GameplayEffectCue::get_cue_tags);

	BIND_VMETHOD(MethodInfo("_on_expired", PropertyInfo(Variant::BOOL, "cancelled")));

	/** Properties */
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "minimum_level"), "set_minimum_level", "get_minimum_level");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "maximum_level"), "set_maximum_level", "get_maximum_level");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "cue_tags", PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagContainer"), "set_cue_tags", "get_cue_tags");
}

void GameplayEffect::set_effect_name(const StringName &value) {
	effect_name = value;
}

StringName GameplayEffect::get_effect_name() const {
	return effect_name;
}

void GameplayEffect::set_duration_type(DurationType::Type value) {
	duration_type = value;
}

DurationType::Type GameplayEffect::get_duration_type() const {
	return duration_type;
}

void GameplayEffect::set_duration_magnitude(const Ref<GameplayEffectMagnitude> &value) {
	duration_magnitude = value;
}

Ref<GameplayEffectMagnitude> GameplayEffect::get_duration_magnitude() const {
	return duration_magnitude;
}

void GameplayEffect::set_period(const Ref<ScalableFloat> &value) {
	period = value;
}

Ref<ScalableFloat> GameplayEffect::get_period() const {
	return period;
}

void GameplayEffect::set_execute_period_on_application(bool value) {
	execute_period_on_application = value;
}

bool GameplayEffect::get_execute_period_on_application() const {
	return execute_period_on_application;
}

void GameplayEffect::set_modifiers(const Array &value) {
	modifiers = value;
}

const Array &GameplayEffect::get_modifiers() const {
	return modifiers;
}

void GameplayEffect::set_executions(const Array &value) {
	executions = value;
}

const Array &GameplayEffect::get_executions() const {
	return executions;
}

void GameplayEffect::set_infliction_chance(const Ref<ScalableFloat> &value) {
	infliction_chance = value;
}

Ref<ScalableFloat> GameplayEffect::get_infliction_chance() const {
	return infliction_chance;
}

void GameplayEffect::set_application_requirements(const Array &value) {
	application_requirements = value;
}

const Array &GameplayEffect::get_application_requirements() const {
	return application_requirements;
}

void GameplayEffect::set_conditional_erffects(const Array &value) {
	conditional_erffects = value;
}

const Array &GameplayEffect::get_conditional_erffects() const {
	return conditional_erffects;
}

void GameplayEffect::set_overflow_effects(const Array &value) {
	overflow_effects = value;
}

const Array &GameplayEffect::get_overflow_effects() const {
	return overflow_effects;
}

void GameplayEffect::set_deny_overflow_application(bool value) {
	deny_overflow_application = value;
}

bool GameplayEffect::get_deny_overflow_application() const {
	return deny_overflow_application;
}

void GameplayEffect::set_clear_overflow_stack(bool value) {
	clear_overflow_stack = value;
}

bool GameplayEffect::get_clear_overflow_stack() const {
	return clear_overflow_stack;
}

void GameplayEffect::set_premature_expiration_effects(const Array &value) {
	premature_expiration_effects = value;
}

const Array &GameplayEffect::get_premature_expiration_effects() const {
	return premature_expiration_effects;
}

void GameplayEffect::set_normal_expiration_effects(const Array &value) {
	normal_expiration_effects = value;
}

const Array &GameplayEffect::get_normal_expiration_effects() const {
	return normal_expiration_effects;
}

void GameplayEffect::set_cues_require_successful_application(bool value) {
	cues_require_successful_application = value;
}

bool GameplayEffect::get_cues_require_successful_application() const {
	return cues_require_successful_application;
}

void GameplayEffect::set_cues_ignore_stacking(bool value) {
	cues_ignore_stacking = value;
}

bool GameplayEffect::get_cues_ignore_stacking() const {
	return cues_ignore_stacking;
}

void GameplayEffect::set_cues(const Array &value) {
	cues = value;
}

const Array &GameplayEffect::get_cues() const {
	return cues;
}

void GameplayEffect::set_effect_tags(const Ref<GameplayTagContainer> &value) {
	ERR_EXPLAIN("GameplayTagContainer are readonly properties.");
	ERR_FAIL();
}

Ref<GameplayTagContainer> GameplayEffect::get_effect_tags() const {
	return effect_tags;
}

void GameplayEffect::set_target_tags(const Ref<GameplayTagContainer> &value) {
	ERR_EXPLAIN("GameplayTagContainer are readonly properties.");
	ERR_FAIL();
}

Ref<GameplayTagContainer> GameplayEffect::get_target_tags() const {
	return target_tags;
}

void GameplayEffect::set_ongoing_tags(const Ref<GameplayTagContainer> &value) {
	ERR_EXPLAIN("GameplayTagContainer are readonly properties.");
	ERR_FAIL();
}

Ref<GameplayTagContainer> GameplayEffect::get_ongoing_tags() const {
	return ongoing_tags;
}

void GameplayEffect::set_remove_effect_tags(const Ref<GameplayTagContainer> &value) {
	ERR_EXPLAIN("GameplayTagContainer are readonly properties.");
	ERR_FAIL();
}

Ref<GameplayTagContainer> GameplayEffect::get_remove_effect_tags() const {
	return remove_effect_tags;
}

void GameplayEffect::set_application_immunity_tags(const Ref<GameplayTagContainer> &value) {
	ERR_EXPLAIN("GameplayTagContainer are readonly properties.");
	ERR_FAIL();
}

Ref<GameplayTagContainer> GameplayEffect::get_application_immunity_tags() const {
	return application_immunity_tags;
}

void GameplayEffect::set_cancel_ability_tags(const Ref<GameplayTagContainer> &value) {
	ERR_EXPLAIN("GameplayTagContainer are readonly properties.");
	ERR_FAIL();
}

Ref<GameplayTagContainer> GameplayEffect::get_cancel_ability_tags() const {
	return cancel_ability_tags;
}

void GameplayEffect::set_stacking_type(StackingType::Type value) {
	stacking_type = value;
}

StackingType::Type GameplayEffect::get_stacking_type() const {
	return stacking_type;
}

void GameplayEffect::set_maximum_stacks(int64_t value) {
	maximum_stacks = value;
}

int64_t GameplayEffect::get_maximum_stacks() const {
	return maximum_stacks;
}

void GameplayEffect::set_duration_refresh(StackDurationRefresh::Type value) {
	duration_refresh = value;
}

StackDurationRefresh::Type GameplayEffect::get_duration_refresh() const {
	return duration_refresh;
}

void GameplayEffect::set_period_reset(StackPeriodReset::Type value) {
	period_reset = value;
}

StackPeriodReset::Type GameplayEffect::get_period_reset() const {
	return period_reset;
}

void GameplayEffect::set_stack_expiration(StackExpiration::Type value) {
	stack_expiration = value;
}

StackExpiration::Type GameplayEffect::get_stack_expiration() const {
	return stack_expiration;
}

void GameplayEffect::set_granted_abilities(const Array &value) {
	granted_abilities = value;
}

const Array &GameplayEffect::get_granted_abilities() const {
	return granted_abilities;
}

void GameplayEffect::_bind_methods() {
	/** Methods */
	ClassDB::bind_method(D_METHOD("set_effect_name", "value"), &GameplayEffect::set_effect_name);
	ClassDB::bind_method(D_METHOD("get_effect_name"), &GameplayEffect::get_effect_name);
	ClassDB::bind_method(D_METHOD("set_duration_type", "value"), &GameplayEffect::set_duration_type);
	ClassDB::bind_method(D_METHOD("get_duration_type"), &GameplayEffect::get_duration_type);
	ClassDB::bind_method(D_METHOD("set_duration_magnitude", "value"), &GameplayEffect::set_duration_magnitude);
	ClassDB::bind_method(D_METHOD("get_duration_magnitude"), &GameplayEffect::get_duration_magnitude);
	ClassDB::bind_method(D_METHOD("set_period", "value"), &GameplayEffect::set_period);
	ClassDB::bind_method(D_METHOD("get_period"), &GameplayEffect::get_period);
	ClassDB::bind_method(D_METHOD("set_execute_period_on_application", "value"), &GameplayEffect::set_execute_period_on_application);
	ClassDB::bind_method(D_METHOD("get_execute_period_on_application"), &GameplayEffect::get_execute_period_on_application);
	ClassDB::bind_method(D_METHOD("set_modifiers", "value"), &GameplayEffect::set_modifiers);
	ClassDB::bind_method(D_METHOD("get_modifiers"), &GameplayEffect::get_modifiers);
	ClassDB::bind_method(D_METHOD("set_executions", "value"), &GameplayEffect::set_executions);
	ClassDB::bind_method(D_METHOD("get_executions"), &GameplayEffect::get_executions);
	ClassDB::bind_method(D_METHOD("set_infliction_chance", "value"), &GameplayEffect::set_infliction_chance);
	ClassDB::bind_method(D_METHOD("get_infliction_chance"), &GameplayEffect::get_infliction_chance);
	ClassDB::bind_method(D_METHOD("set_application_requirements", "value"), &GameplayEffect::set_application_requirements);
	ClassDB::bind_method(D_METHOD("get_application_requirements"), &GameplayEffect::get_application_requirements);
	ClassDB::bind_method(D_METHOD("set_conditional_erffects", "value"), &GameplayEffect::set_conditional_erffects);
	ClassDB::bind_method(D_METHOD("get_conditional_erffects"), &GameplayEffect::get_conditional_erffects);
	ClassDB::bind_method(D_METHOD("set_overflow_effects", "value"), &GameplayEffect::set_overflow_effects);
	ClassDB::bind_method(D_METHOD("get_overflow_effects"), &GameplayEffect::get_overflow_effects);
	ClassDB::bind_method(D_METHOD("set_deny_overflow_application", "value"), &GameplayEffect::set_deny_overflow_application);
	ClassDB::bind_method(D_METHOD("get_deny_overflow_application"), &GameplayEffect::get_deny_overflow_application);
	ClassDB::bind_method(D_METHOD("set_clear_overflow_stack", "value"), &GameplayEffect::set_clear_overflow_stack);
	ClassDB::bind_method(D_METHOD("get_clear_overflow_stack"), &GameplayEffect::get_clear_overflow_stack);
	ClassDB::bind_method(D_METHOD("set_premature_expiration_effects", "value"), &GameplayEffect::set_premature_expiration_effects);
	ClassDB::bind_method(D_METHOD("get_premature_expiration_effects"), &GameplayEffect::get_premature_expiration_effects);
	ClassDB::bind_method(D_METHOD("set_normal_expiration_effects", "value"), &GameplayEffect::set_normal_expiration_effects);
	ClassDB::bind_method(D_METHOD("get_normal_expiration_effects"), &GameplayEffect::get_normal_expiration_effects);
	ClassDB::bind_method(D_METHOD("set_cues_require_successful_application", "value"), &GameplayEffect::set_cues_require_successful_application);
	ClassDB::bind_method(D_METHOD("get_cues_require_successful_application"), &GameplayEffect::get_cues_require_successful_application);
	ClassDB::bind_method(D_METHOD("set_cues_ignore_stacking", "value"), &GameplayEffect::set_cues_ignore_stacking);
	ClassDB::bind_method(D_METHOD("get_cues_ignore_stacking"), &GameplayEffect::get_cues_ignore_stacking);
	ClassDB::bind_method(D_METHOD("set_cues", "value"), &GameplayEffect::set_cues);
	ClassDB::bind_method(D_METHOD("get_cues"), &GameplayEffect::get_cues);

	ClassDB::bind_method(D_METHOD("set_effect_tags", "value"), &GameplayEffect::set_effect_tags);
	ClassDB::bind_method(D_METHOD("get_effect_tags"), &GameplayEffect::get_effect_tags);
	ClassDB::bind_method(D_METHOD("set_target_tags", "value"), &GameplayEffect::set_target_tags);
	ClassDB::bind_method(D_METHOD("get_target_tags"), &GameplayEffect::get_target_tags);
	ClassDB::bind_method(D_METHOD("set_ongoing_tags", "value"), &GameplayEffect::set_ongoing_tags);
	ClassDB::bind_method(D_METHOD("get_ongoing_tags"), &GameplayEffect::get_ongoing_tags);
	ClassDB::bind_method(D_METHOD("set_remove_effect_tags", "value"), &GameplayEffect::set_remove_effect_tags);
	ClassDB::bind_method(D_METHOD("get_remove_effect_tags"), &GameplayEffect::get_remove_effect_tags);
	ClassDB::bind_method(D_METHOD("set_application_immunity_tags", "value"), &GameplayEffect::set_application_immunity_tags);
	ClassDB::bind_method(D_METHOD("get_application_immunity_tags"), &GameplayEffect::get_application_immunity_tags);
	ClassDB::bind_method(D_METHOD("set_cancel_ability_tags", "value"), &GameplayEffect::set_cancel_ability_tags);
	ClassDB::bind_method(D_METHOD("get_cancel_ability_tags"), &GameplayEffect::get_cancel_ability_tags);

	ClassDB::bind_method(D_METHOD("set_stacking_type", "value"), &GameplayEffect::set_stacking_type);
	ClassDB::bind_method(D_METHOD("get_stacking_type"), &GameplayEffect::get_stacking_type);
	ClassDB::bind_method(D_METHOD("set_maximum_stacks", "value"), &GameplayEffect::set_maximum_stacks);
	ClassDB::bind_method(D_METHOD("get_maximum_stacks"), &GameplayEffect::get_maximum_stacks);
	ClassDB::bind_method(D_METHOD("set_duration_refresh", "value"), &GameplayEffect::set_duration_refresh);
	ClassDB::bind_method(D_METHOD("get_duration_refresh"), &GameplayEffect::get_duration_refresh);
	ClassDB::bind_method(D_METHOD("set_period_reset", "value"), &GameplayEffect::set_period_reset);
	ClassDB::bind_method(D_METHOD("get_period_reset"), &GameplayEffect::get_period_reset);
	ClassDB::bind_method(D_METHOD("set_stack_expiration", "value"), &GameplayEffect::set_stack_expiration);
	ClassDB::bind_method(D_METHOD("get_stack_expiration"), &GameplayEffect::get_stack_expiration);

	ClassDB::bind_method(D_METHOD("set_granted_abilities", "value"), &GameplayEffect::set_granted_abilities);
	ClassDB::bind_method(D_METHOD("get_granted_abilities"), &GameplayEffect::get_granted_abilities);

	/** Properties */
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "effect_name"), "set_effect_name", "get_effect_name");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "duration_type", PROPERTY_HINT_ENUM, "Instant,Infinte,Has Duration"), "set_duration_type", "get_duration_type");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "duration_magnitude", PROPERTY_HINT_RESOURCE_TYPE, "GameplayEffectMagnitude"), "set_duration_magnitude", "get_duration_magnitude");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "period", PROPERTY_HINT_RESOURCE_TYPE, "ScalableFloat"), "set_period", "get_period");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "execute_period_on_application"), "set_execute_period_on_application", "get_execute_period_on_application");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "modifiers"), "set_modifiers", "get_modifiers");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "executions"), "set_executions", "get_executions");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "infliction_chance", PROPERTY_HINT_RESOURCE_TYPE, "ScalableFloat"), "set_infliction_chance", "get_infliction_chance");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "application_requirements"), "set_application_requirements", "get_application_requirements");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "conditional_erffects"), "set_conditional_erffects", "get_conditional_erffects");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "overflow_effects"), "set_overflow_effects", "get_overflow_effects");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "deny_overflow_application"), "set_deny_overflow_application", "get_deny_overflow_application");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "clear_overflow_stack"), "set_clear_overflow_stack", "get_clear_overflow_stack");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "premature_expiration_effects"), "set_premature_expiration_effects", "get_premature_expiration_effects");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "normal_expiration_effects"), "set_normal_expiration_effects", "get_normal_expiration_effects");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "cues_require_successful_application"), "set_cues_require_successful_application", "get_cues_require_successful_application");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "cues_ignore_stacking"), "set_cues_ignore_stacking", "get_cues_ignore_stacking");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "cues"), "set_cues", "get_cues");

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "effect_tags", PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagContainer"), "set_effect_tags", "get_effect_tags");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "target_tags", PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagContainer"), "set_target_tags", "get_target_tags");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "ongoing_tags", PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagContainer"), "set_ongoing_tags", "get_ongoing_tags");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "remove_effect_tags", PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagContainer"), "set_remove_effect_tags", "get_remove_effect_tags");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "application_immunity_tags", PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagContainer"), "set_application_immunity_tags", "get_application_immunity_tags");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "cancel_ability_tags", PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagContainer"), "set_cancel_ability_tags", "get_cancel_ability_tags");

	ADD_PROPERTY(PropertyInfo(Variant::INT, "stacking_type", PROPERTY_HINT_ENUM, "None,Aggregate On Source,Aggregate On Target"), "set_stacking_type", "get_stacking_type");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "maximum_stacks"), "set_maximum_stacks", "get_maximum_stacks");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "duration_refresh", PROPERTY_HINT_ENUM, "On Application,Never"), "set_duration_refresh", "get_duration_refresh");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "period_reset", PROPERTY_HINT_ENUM, "On Application,Never"), "set_period_reset", "get_period_reset");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "stack_expiration", PROPERTY_HINT_ENUM, "Clear Stack,Remove Single Stack And Refresh Duration,Refresh Duration"), "set_stack_expiration", "get_stack_expiration");

	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "granted_abilities"), "set_granted_abilities", "get_granted_abilities");

	/** Constants */
	BIND_ENUM_CONSTANT(DURATION_TYPE_INSTANT);
	BIND_ENUM_CONSTANT(DURATION_TYPE_INFINITE);
	BIND_ENUM_CONSTANT(DURATION_TYPE_HAS_DURATION);

	BIND_ENUM_CONSTANT(STACKING_TYPE_NONE);
	BIND_ENUM_CONSTANT(STACKING_TYPE_AGGREGATE_SOURCE);
	BIND_ENUM_CONSTANT(STACKING_TYPE_AGGREGATE_TARGET);

	BIND_ENUM_CONSTANT(STACKING_DURATION_REFRESH_ON_APPLICATION);
	BIND_ENUM_CONSTANT(STACKING_DURATION_REFRESH_NEVER);

	BIND_ENUM_CONSTANT(STACKING_PERIOD_RESET_ON_APPLICATION);
	BIND_ENUM_CONSTANT(STACKING_PERIOD_RESET_NEVER);

	BIND_ENUM_CONSTANT(STACKING_EXPIRATION_CLEAR);
	BIND_ENUM_CONSTANT(STACKING_EXPIRATION_REMOVE_REFRESH);
	BIND_ENUM_CONSTANT(STACKING_EXPIRATION_REFRESH);
}
