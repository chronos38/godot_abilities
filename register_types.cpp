#include "register_types.h"

#include "gameplay_ability.h"
#include "gameplay_ability_system.h"
#include "gameplay_attribute.h"
#include "gameplay_effect.h"
#include "gameplay_effect_magnitude.h"
#include "gameplay_node.h"
#include "gameplay_tags.h"

#include <core/class_db.h>

void register_gameplay_abilities_types() {
	/** Nodes */
	ClassDB::register_class<GameplayAttributeInitialiser>();
	ClassDB::register_class<GameplayAbilitySystem>();
	ClassDB::register_class<GameplayEffectNode>();
	ClassDB::register_class<GameplayAbility>();

	/** Resources */
	ClassDB::register_class<ScalableFloat>();
	ClassDB::register_class<AttributeBasedFloat>();
	ClassDB::register_class<CustomCalculatedFloat>();
	ClassDB::register_class<CustomMagnitudeCalculator>();
	ClassDB::register_class<GameplayAttributeData>();
	ClassDB::register_class<GameplayTagContainer>();
	ClassDB::register_class<GameplayEffectModifier>();
	ClassDB::register_class<GameplayEffectCustomExecutionResult>();
	ClassDB::register_class<GameplayEffectCustomExecutionScript>();
	ClassDB::register_class<GameplayEffectCustomExecution>();
	ClassDB::register_class<GameplayEffectCustomApplicationRequirementScript>();
	ClassDB::register_class<GameplayEffectCustomApplicationRequirement>();
	ClassDB::register_class<ConditionalGameplayEffect>();
	ClassDB::register_class<GameplayEffectCue>();
	ClassDB::register_class<GameplayEffect>();
	ClassDB::register_class<GameplayAttribute>();
	ClassDB::register_class<GameplayAttributeSet>();
	ClassDB::register_class<GameplayAbilityTriggerData>();
	ClassDB::register_class<GameplayEvent>();
}

void unregister_gameplay_abilities_types() {
}
