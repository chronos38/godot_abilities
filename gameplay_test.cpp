#include "gameplay_test.h"
#include "gameplay_ability.h"
#include "gameplay_ability_system.h"
#include "gameplay_attribute.h"
#include "gameplay_effect.h"
#include "gameplay_effect_magnitude.h"
#include "gameplay_tags.h"

#include <core/class_db.h>
#include <core/os/os.h>
#include <scene/main/scene_tree.h>
#include <scene/main/viewport.h>

#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

namespace {
constexpr auto delta = 6.0f;
constexpr auto max_health = "max_health";
constexpr auto health = "health";
constexpr auto max_mana = "max_mana";
constexpr auto mana = "mana";
constexpr auto max_stamina = "max_stamina";
constexpr auto stamina = "stamina";
constexpr auto attack = "attack";
constexpr auto defence = "defence";
constexpr auto magic_attack = "magic_attack";
constexpr auto magic_defence = "magic_defence";
constexpr auto agility = "agility";
constexpr auto luck = "luck";

// final_action allows you to ensure something gets run at the end of a scope
template <class F>
class final_action {
public:
	explicit final_action(F f) noexcept : f_(std::move(f)) {}

	final_action(final_action &&other) noexcept : f_(std::move(other.f_)), invoke_(other.invoke_) {
		other.invoke_ = false;
	}

	final_action(const final_action &) = delete;
	final_action &operator=(const final_action &) = delete;
	final_action &operator=(final_action &&) = delete;

	~final_action() noexcept {
		if (invoke_) f_();
	}

private:
	F f_;
	bool invoke_{ true };
};

// finally() - convenience function to generate a final_action
template <class F>
final_action<F> finally(const F &f) noexcept {
	return final_action<F>(f);
}

template <class F>
final_action<F> finally(F &&f) noexcept {
	return final_action<F>(std::forward<F>(f));
}

class TestSceneTree : public SceneTree {
public:
	virtual ~TestSceneTree() = default;

	virtual void init() override {
		SceneTree::init();
	}

	virtual bool idle(float p_time) override {
		bool result = SceneTree::idle(p_time);
		get_root()->propagate_notification(Node::NOTIFICATION_INTERNAL_PROCESS);
		return result;
	}

	virtual bool iteration(float p_time) override {
		bool result = SceneTree::iteration(p_time);
		get_root()->propagate_notification(Node::NOTIFICATION_INTERNAL_PHYSICS_PROCESS);
		return result;
	}

	virtual void finish() override {
		SceneTree::finish();
	}
};

class TestAttributeSet : public GameplayAttributeSet {
public:
	virtual ~TestAttributeSet() = default;

	const StringName max_health = "max_health";
	const StringName health = "health";
	const StringName max_mana = "max_mana";
	const StringName mana = "mana";
	const StringName max_stamina = "max_stamina";
	const StringName stamina = "stamina";
	const StringName attack = "attack";
	const StringName defence = "defence";
	const StringName magic_attack = "magic_attack";
	const StringName magic_defence = "magic_defence";
	const StringName agility = "agility";
	const StringName luck = "luck";

	TestAttributeSet() {
		add_attribute(max_health, 100);
		add_attribute(health, 100);
		add_attribute(max_mana, 100);
		add_attribute(mana, 100);
		add_attribute(max_stamina, 100);
		add_attribute(stamina, 100);
		add_attribute(attack, 100);
		add_attribute(defence, 100);
		add_attribute(magic_attack, 100);
		add_attribute(magic_defence, 100);
		add_attribute(agility, 100);
		add_attribute(luck, 100);
	}

private:
	static void _bind_methods() {}
};

class BaseTestAbility : public GameplayAbility {
	GDCLASS(BaseTestAbility, ::GameplayAbility);

public:
	Ref<ScalableFloat> const_1 = make_reference<ScalableFloat>();
	Ref<ScalableFloat> const_5 = make_reference<ScalableFloat>();
	Ref<ScalableFloat> const_10 = make_reference<ScalableFloat>();
	Ref<ScalableFloat> const_25 = make_reference<ScalableFloat>();
	Ref<ScalableFloat> const_50 = make_reference<ScalableFloat>();
	Ref<ScalableFloat> const_75 = make_reference<ScalableFloat>();
	Ref<ScalableFloat> const_100 = make_reference<ScalableFloat>();
	Ref<ScalableFloat> const_200 = make_reference<ScalableFloat>();

	BaseTestAbility() {
		const_1->set_value(1);
		const_5->set_value(5);
		const_10->set_value(10);
		const_25->set_value(25);
		const_50->set_value(50);
		const_75->set_value(75);
		const_100->set_value(100);
		const_200->set_value(200);
	}

	virtual ~BaseTestAbility() = default;

protected:
	virtual void _on_activate_ability() {}
	virtual void _on_end_ability(bool cancelled) {}
	virtual void _on_gameplay_event(const Ref<GameplayEvent> &event) {}
	virtual bool _can_event_activate_ability(const Ref<GameplayEvent> &event) { return true; }
	virtual bool _can_activate_ability(Node *target) { return true; }
	virtual void _on_wait_completed(WaitType::Type type, const Variant &data) {}
	virtual void _on_wait_interrupted(const Variant &payload = {}) {}
	virtual void _on_wait_cancelled(const Variant &payload = {}) {}

private:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("_on_activate_ability"), &BaseTestAbility::_on_activate_ability);
		ClassDB::bind_method(D_METHOD("_on_end_ability", "cancelled"), &BaseTestAbility::_on_end_ability);
		ClassDB::bind_method(D_METHOD("_on_gameplay_event", "event"), &BaseTestAbility::_on_gameplay_event);
		ClassDB::bind_method(D_METHOD("_can_event_activate_ability", "event"), &BaseTestAbility::_can_event_activate_ability);
		ClassDB::bind_method(D_METHOD("_can_activate_ability", "target"), &BaseTestAbility::_can_activate_ability);
		ClassDB::bind_method(D_METHOD("_on_wait_completed", "type", "data"), &BaseTestAbility::_on_wait_completed);
		ClassDB::bind_method(D_METHOD("_on_wait_interrupted", "payload"), &BaseTestAbility::_on_wait_interrupted);
		ClassDB::bind_method(D_METHOD("_on_wait_cancelled", "type"), &BaseTestAbility::_on_wait_cancelled);
	}
};

class AttackAbility : public BaseTestAbility {
	Ref<GameplayEffect> damage_effect = make_reference<GameplayEffect>();
	Ref<GameplayEffect> cooldown_effect = make_reference<GameplayEffect>();
	Ref<GameplayEffect> cost_effect = make_reference<GameplayEffect>();

public:
	bool wait_cancel = false;
	bool wait_interrupt = false;
	bool wait_for_event = false;
	bool wait_for_delay = false;
	bool got_cancelled = false;
	const StringName event_tag = "event.collision";

	AttackAbility() {
		// Set damage effect.
		Array modifiers;
		modifiers.append(make_reference<GameplayEffectModifier>([this](Ref<GameplayEffectModifier> modifier) {
			modifier->set_attribute(health);
			modifier->set_modifier_operation(ModifierOperation::Subtract);
			modifier->set_modifier_magnitude(const_10);
		}));
		damage_effect->set_modifiers(modifiers);

		// Set cost effect.
		modifiers = Array();
		modifiers.push_back(make_reference<GameplayEffectModifier>([this](Ref<GameplayEffectModifier> modifier) {
			modifier->set_attribute(stamina);
			modifier->set_modifier_operation(ModifierOperation::Subtract);
			modifier->set_modifier_magnitude(const_10);
		}));
		cost_effect->set_modifiers(modifiers);
		set_cost_effect(cost_effect);

		// Set cooldown effect.
		auto tags = cooldown_effect->get_effect_tags();
		tags->append("attack.cooldown");
		cooldown_effect->set_duration_type(DurationType::HasDuration);
		cooldown_effect->set_duration_magnitude(const_10);
		set_cooldown_effect(cooldown_effect);

		// Set blocked source tags.
		set_source_blocked_tags(make_reference<GameplayTagContainer>([this](Ref<GameplayTagContainer> tags) {
			tags->append("attack.blocked");
		}));

		// Set required source tags.
		set_source_required_tags(make_reference<GameplayTagContainer>([this](Ref<GameplayTagContainer> tags) {
			tags->append("equipment.weapon");
		}));

		// Set blocked target tags.
		set_target_blocked_tags(make_reference<GameplayTagContainer>([this](Ref<GameplayTagContainer> tags) {
			tags->append("attack.immune");
		}));
	}

	virtual ~AttackAbility() = default;

	virtual void _on_activate_ability() override {
		if (wait_for_event) {
			wait_event(event_tag);
		} else if (wait_for_delay) {
			wait_delay(delta + 0.1);
		} else {
			apply_effect_on_targets(filter_targets(), damage_effect);
			commit_ability();
		}
	}

	virtual void _on_end_ability(bool cancelled) override {
		got_cancelled = cancelled;
	}

	virtual void _on_wait_completed(WaitType::Type type, const Variant &data) override {
		if (type == WaitType::Event && data == event_tag) {
			apply_effect_on_targets(filter_targets(), damage_effect);
			commit_ability();
		} else {
			end_ability();
		}
	}

	virtual void _on_wait_interrupted(const Variant &payload = {}) override {
		wait_interrupt = is_active();
		reset_wait_handle();
		cancel_ability();
	}

	virtual void _on_wait_cancelled(const Variant &payload = {}) override {
		wait_cancel = is_active();
		reset_wait_handle();
		cancel_ability();
	}
};

class CancellationAbility : public BaseTestAbility {
public:
	virtual ~CancellationAbility() = default;

	virtual void _on_activate_ability() override {
		auto cancel_effect = make_reference<GameplayEffect>();
		cancel_effect->get_cancel_ability_tags()->append("ability.*");
		apply_effect_on_source(cancel_effect);
		commit_ability();
	}
};

class ApplyEffectAbility : public BaseTestAbility {
public:
	virtual ~ApplyEffectAbility() = default;

	Vector<Ref<GameplayEffect> > source_effects;
	Vector<Ref<GameplayEffect> > target_effects;

	virtual void _on_activate_ability() override {
		auto &&targets = filter_targets();

		for (auto &&effect : source_effects) {
			apply_effect_on_source(effect);
		}
		for (auto &&effect : target_effects) {
			apply_effect_on_targets(targets, effect);
		}

		commit_ability();
	}
};

class TestScriptInstance : public ScriptInstance {
public:
	virtual ~TestScriptInstance() = default;

	virtual bool set(const StringName &p_name, const Variant &p_value) override {
		throw std::logic_error("The method or operation is not implemented.");
	}

	virtual bool get(const StringName &p_name, Variant &r_ret) const override {
		throw std::logic_error("The method or operation is not implemented.");
	}

	virtual void get_property_list(List<PropertyInfo> *p_properties) const override {
		throw std::logic_error("The method or operation is not implemented.");
	}

	virtual Variant::Type get_property_type(const StringName &p_name, bool *r_is_valid = NULL) const override {
		throw std::logic_error("The method or operation is not implemented.");
	}

	virtual void get_method_list(List<MethodInfo> *p_list) const override {
		throw std::logic_error("The method or operation is not implemented.");
	}

	virtual bool has_method(const StringName &p_method) const override {
		throw std::logic_error("The method or operation is not implemented.");
	}

	virtual Variant call(const StringName &p_method, VARIANT_ARG_LIST) override {
		if (p_method == "_execute") {
			if (p_arg4.get_type() == Variant::INT) {
				return 10 * static_cast<int64_t>(p_arg4);
			}
		}

		return {};
	}

	virtual Variant call(const StringName &p_method, const Variant **p_args, int p_argcount, Variant::CallError &r_error) override {
		throw std::logic_error("The method or operation is not implemented.");
	}

	virtual void notification(int p_notification) override {
		throw std::logic_error("The method or operation is not implemented.");
	}

	virtual Ref<Script> get_script() const override {
		throw std::logic_error("The method or operation is not implemented.");
	}

	virtual MultiplayerAPI::RPCMode get_rpc_mode(const StringName &p_method) const override {
		throw std::logic_error("The method or operation is not implemented.");
	}

	virtual MultiplayerAPI::RPCMode get_rset_mode(const StringName &p_variable) const override {
		throw std::logic_error("The method or operation is not implemented.");
	}

	virtual ScriptLanguage *get_language() override {
		throw std::logic_error("The method or operation is not implemented.");
	}
};

class TestScript : public Script {
public:
	virtual ~TestScript() = default;

	virtual bool can_instance() const override {
		throw std::logic_error("The method or operation is not implemented.");
	}

	virtual Ref<Script> get_base_script() const override {
		throw std::logic_error("The method or operation is not implemented.");
	}

	virtual StringName get_instance_base_type() const override {
		throw std::logic_error("The method or operation is not implemented.");
	}

	virtual ScriptInstance *instance_create(Object *p_this) override {
		return memnew(TestScriptInstance);
	}

	virtual bool instance_has(const Object *p_this) const override {
		throw std::logic_error("The method or operation is not implemented.");
	}

	virtual bool has_source_code() const override {
		throw std::logic_error("The method or operation is not implemented.");
	}

	virtual String get_source_code() const override {
		throw std::logic_error("The method or operation is not implemented.");
	}

	virtual void set_source_code(const String &p_code) override {
		throw std::logic_error("The method or operation is not implemented.");
	}

	virtual Error reload(bool p_keep_state = false) override {
		throw std::logic_error("The method or operation is not implemented.");
	}

	virtual MethodInfo get_method_info(const StringName &p_method) const override {
		throw std::logic_error("The method or operation is not implemented.");
	}

	virtual bool is_tool() const override {
		throw std::logic_error("The method or operation is not implemented.");
	}

	virtual bool is_valid() const override {
		throw std::logic_error("The method or operation is not implemented.");
	}

	virtual ScriptLanguage *get_language() const override {
		throw std::logic_error("The method or operation is not implemented.");
	}

	virtual bool has_script_signal(const StringName &p_signal) const override {
		throw std::logic_error("The method or operation is not implemented.");
	}

	virtual void get_script_signal_list(List<MethodInfo> *r_signals) const override {
		throw std::logic_error("The method or operation is not implemented.");
	}

	virtual bool get_property_default_value(const StringName &p_property, Variant &r_value) const override {
		throw std::logic_error("The method or operation is not implemented.");
	}

	virtual void get_script_method_list(List<MethodInfo> *p_list) const override {
		throw std::logic_error("The method or operation is not implemented.");
	}

	virtual void get_script_property_list(List<PropertyInfo> *p_list) const override {
		throw std::logic_error("The method or operation is not implemented.");
	}

	virtual bool has_method(const StringName &p_method) const override {
		throw std::logic_error("The method or operation is not implemented.");
	}
};
} // namespace

#pragma region ability activation

SCENARIO("check ability activation on single target with wait") {
	auto scene_tree = make_gameplay_ptr<TestSceneTree>();

	GIVEN("attack ability with cost and cooldown which waits") {
		// Scene Tree
		auto _ = finally([&scene_tree] { scene_tree->finish(); });
		auto root = make_gameplay_ptr<Node>();
		scene_tree->call("_change_scene", root.get());
		scene_tree->init();

		// Source
		auto source_attributes = make_reference<TestAttributeSet>();
		auto source = make_gameplay_ptr<GameplayAbilitySystem>();
		source->set_attribute_set(source_attributes);
		source->add_tag("equipment.weapon");
		root->add_child(source.get());

		// Target
		auto target_attributes = make_reference<TestAttributeSet>();
		auto target = make_gameplay_ptr<GameplayAbilitySystem>();
		target->set_attribute_set(target_attributes);
		source->add_target(target.get());
		root->add_child(target.get());

		// Ability
		auto ability = make_gameplay_ptr<AttackAbility>([](AttackAbility *ability) {
			ability->wait_for_event = true;
		});
		source->add_ability(ability.get());

		WHEN("ability activates on valid target but no event is fired") {
			source->activate_ability(ability.get());
			scene_tree->idle(delta);

			THEN("ability is active and waits for collision event") {
				CHECK(ability->is_active());
				CHECK(ability->get_wait_handle().type == WaitType::Event);
				REQUIRE(ability->get_wait_handle().data.get_type() == Variant::STRING);
			}
		}

		WHEN("ability activates on valid target and event is fired") {
			auto event = make_reference<GameplayEvent>([&](Ref<GameplayEvent> event) {
				event->set_event_tag(ability->event_tag);
			});
			/** Activate ability and wait for event. */
			source->activate_ability(ability.get());
			scene_tree->idle(delta);
			/** Process event and commit ability. */
			ability->process_wait(WaitType::Event, ability->event_tag);
			scene_tree->idle(delta);

			THEN("ability was committed successfully and on cooldown") {
				CHECK(!ability->is_active());
				CHECK(ability->get_wait_handle().type != WaitType::Event);
				CHECK(ability->get_remaining_cooldown() == 4);
				CHECK(source->get_current_attribute_value(stamina) == 90);
				REQUIRE(target->get_current_attribute_value(health) == 90);
			}
		}

		WHEN("ability activates on valid target, event is fired and enough time passed") {
			auto event = make_reference<GameplayEvent>([&](Ref<GameplayEvent> event) {
				event->set_event_tag(ability->event_tag);
			});
			/** Activate ability and wait for event. */
			source->activate_ability(ability.get());
			scene_tree->idle(delta);
			/** Process event and commit ability. */
			ability->process_wait(WaitType::Event, ability->event_tag);
			scene_tree->idle(delta);
			/** Process one more time so that ability is no longer on cooldown. */
			scene_tree->idle(delta);

			THEN("ability was committed successfully and is no more on cooldown") {
				CHECK(!ability->is_active());
				CHECK(ability->get_remaining_cooldown() == 0);
				CHECK(source->get_current_attribute_value(stamina) == 90);
				REQUIRE(target->get_current_attribute_value(health) == 90);
			}
		}
	}
}

SCENARIO("check ability tag requirements without wait") {
	auto scene_tree = make_gameplay_ptr<TestSceneTree>();

	GIVEN("attack ability without required source tags") {
		// Scene Tree
		auto _ = finally([&scene_tree] { scene_tree->finish(); });
		auto root = make_gameplay_ptr<Node>();
		scene_tree->call("_change_scene", root.get());
		scene_tree->init();

		// Source
		auto source_attributes = make_reference<TestAttributeSet>();
		auto source = make_gameplay_ptr<GameplayAbilitySystem>();
		source->set_attribute_set(source_attributes);
		root->add_child(source.get());

		// Target
		auto target_attributes = make_reference<TestAttributeSet>();
		auto target = make_gameplay_ptr<GameplayAbilitySystem>();
		target->set_attribute_set(target_attributes);
		source->add_target(target.get());
		root->add_child(target.get());

		// Ability
		auto ability = make_gameplay_ptr<AttackAbility>();
		source->add_ability(ability.get());

		WHEN("tries to activate ability") {
			source->activate_ability(ability.get());
			scene_tree->idle(2 * delta);

			THEN("activation was blocked") {
				CHECK(!ability->is_active());
				CHECK(!ability->can_activate_ability());
				CHECK(ability->can_activate_ability_on_target(target.get()));
				CHECK(target->get_current_attribute_value(stamina) == 100);
				REQUIRE(target->get_current_attribute_value(health) == 100);
			}
		}
	}

	GIVEN("target which blocks attack ability") {
		// Scene Tree
		auto _ = finally([&scene_tree] { scene_tree->finish(); });
		auto root = make_gameplay_ptr<Node>();
		scene_tree->call("_change_scene", root.get());
		scene_tree->init();

		// Source
		auto source_attributes = make_reference<TestAttributeSet>();
		auto source = make_gameplay_ptr<GameplayAbilitySystem>();
		source->set_attribute_set(source_attributes);
		source->add_tag("equipment.weapon");
		root->add_child(source.get());

		// Target
		auto target_attributes = make_reference<TestAttributeSet>();
		auto target = make_gameplay_ptr<GameplayAbilitySystem>();
		target->set_attribute_set(target_attributes);
		target->add_tag("attack.immune");
		source->add_target(target.get());
		root->add_child(target.get());

		// Ability
		auto ability = make_gameplay_ptr<AttackAbility>();
		source->add_ability(ability.get());

		WHEN("ability is activated on target") {
			source->activate_ability(ability.get());
			scene_tree->idle(delta);

			THEN("ability was activated and is on cooldown but target is unaffected") {
				CHECK(!ability->is_active());
				CHECK(!ability->can_activate_ability());
				CHECK(ability->get_remaining_cooldown() > 0);
				CHECK(!ability->can_activate_ability_on_target(target.get()));
				CHECK(source->get_current_attribute_value(stamina) == 90);
				REQUIRE(target->get_current_attribute_value(health) == 100);
			}
		}

		WHEN("ability is activated on target and cooldown period is over") {
			source->activate_ability(ability.get());
			scene_tree->idle(2 * delta);

			THEN("ability is no longer on cooldown but target is unaffected") {
				CHECK(!ability->is_active());
				CHECK(ability->can_activate_ability());
				CHECK(ability->get_remaining_cooldown() == 0);
				CHECK(!ability->can_activate_ability_on_target(target.get()));
				CHECK(source->get_current_attribute_value(stamina) == 90);
				REQUIRE(target->get_current_attribute_value(health) == 100);
			}
		}
	}

	GIVEN("source with blocking tag") {
		// Scene Tree
		auto _ = finally([&scene_tree] { scene_tree->finish(); });
		auto root = make_gameplay_ptr<Node>();
		scene_tree->call("_change_scene", root.get());
		scene_tree->init();

		// Source
		auto source_attributes = make_reference<TestAttributeSet>();
		auto source = make_gameplay_ptr<GameplayAbilitySystem>();
		source->set_attribute_set(source_attributes);
		source->add_tag("equipment.weapon");
		source->add_tag("attack.blocked");
		root->add_child(source.get());

		// Target
		auto target_attributes = make_reference<TestAttributeSet>();
		auto target = make_gameplay_ptr<GameplayAbilitySystem>();
		target->set_attribute_set(target_attributes);
		source->add_target(target.get());
		root->add_child(target.get());

		// Ability
		auto ability = make_gameplay_ptr<AttackAbility>();
		source->add_ability(ability.get());

		WHEN("tries to activate ability") {
			source->activate_ability(ability.get());
			scene_tree->idle(2 * delta);

			THEN("ability activation was blocked") {
				CHECK(!ability->is_active());
				CHECK(!ability->can_activate_ability());
				CHECK(ability->can_activate_ability_on_target(target.get()));
				CHECK(source->get_current_attribute_value(stamina) == 100);
				REQUIRE(target->get_current_attribute_value(health) == 100);
			}
		}
	}

	GIVEN("blocking source and target") {
		// Scene Tree
		auto _ = finally([&scene_tree] { scene_tree->finish(); });
		auto root = make_gameplay_ptr<Node>();
		scene_tree->call("_change_scene", root.get());
		scene_tree->init();

		// Source
		auto source_attributes = make_reference<TestAttributeSet>();
		auto source = make_gameplay_ptr<GameplayAbilitySystem>();
		source->set_attribute_set(source_attributes);
		source->add_tag("equipment.weapon");
		source->add_tag("attack.blocked");
		root->add_child(source.get());

		// Target
		auto target_attributes = make_reference<TestAttributeSet>();
		auto target = make_gameplay_ptr<GameplayAbilitySystem>();
		target->set_attribute_set(target_attributes);
		target->add_tag("attack.immune");
		source->add_target(target.get());
		root->add_child(target.get());

		// Ability
		auto ability = make_gameplay_ptr<AttackAbility>();
		source->add_ability(ability.get());

		WHEN("tries to activate ability") {
			source->activate_ability(ability.get());
			scene_tree->idle(2 * delta);

			THEN("ability activation was blocked") {
				CHECK(!ability->is_active());
				CHECK(!ability->can_activate_ability());
				CHECK(!ability->can_activate_ability_on_target(target.get()));
				CHECK(source->get_current_attribute_value(stamina) == 100);
				REQUIRE(target->get_current_attribute_value(health) == 100);
			}
		}
	}

	GIVEN("ability which blocks another one") {
		// Scene Tree
		auto _ = finally([&scene_tree] { scene_tree->finish(); });
		auto root = make_gameplay_ptr<Node>();
		scene_tree->call("_change_scene", root.get());
		scene_tree->init();

		// Source
		auto source_attributes = make_reference<TestAttributeSet>();
		auto source = make_gameplay_ptr<GameplayAbilitySystem>();
		source->set_attribute_set(source_attributes);
		source->add_tag("equipment.weapon");
		root->add_child(source.get());

		// Target
		auto target_attributes = make_reference<TestAttributeSet>();
		auto target = make_gameplay_ptr<GameplayAbilitySystem>();
		target->set_attribute_set(target_attributes);
		source->add_target(target.get());
		root->add_child(target.get());

		// Ability
		auto executing_ability = make_gameplay_ptr<AttackAbility>([](AttackAbility *ability) {
			ability->wait_for_event = true;
			ability->get_block_ability_tags()->append("ability.attack");
		});
		auto blocked_ability = make_gameplay_ptr<AttackAbility>([](AttackAbility *ability) {
			ability->get_ability_tags()->append("ability.attack");
		});
		source->add_ability(executing_ability.get());
		source->add_ability(blocked_ability.get());

		WHEN("blocking ability executes") {
			// Start blocking ability.
			source->activate_ability(executing_ability.get());
			scene_tree->idle(delta);

			THEN("block activation of blocked one") {
				CHECK(executing_ability->is_active());
				REQUIRE(!blocked_ability->can_activate_ability());
			}
		}
	}
}

SCENARIO("check ability interruption while executing") {
	auto scene_tree = make_gameplay_ptr<TestSceneTree>();

	GIVEN("executing ability waiting for an event") {
		// Scene Tree
		auto _ = finally([&scene_tree] { scene_tree->finish(); });
		auto root = make_gameplay_ptr<Node>();
		scene_tree->call("_change_scene", root.get());
		scene_tree->init();

		// Source
		auto source_attributes = make_reference<TestAttributeSet>();
		auto source = make_gameplay_ptr<GameplayAbilitySystem>();
		source->set_attribute_set(source_attributes);
		source->add_tag("equipment.weapon");
		root->add_child(source.get());

		// Target
		auto target_attributes = make_reference<TestAttributeSet>();
		auto target = make_gameplay_ptr<GameplayAbilitySystem>();
		target->set_attribute_set(target_attributes);
		source->add_target(target.get());
		root->add_child(target.get());

		// Ability
		auto ability = make_gameplay_ptr<AttackAbility>([](AttackAbility *ability) {
			ability->wait_for_event = true;
		});
		source->add_ability(ability.get());

		WHEN("a different wait handle is called") {
			// First activate wait handle.
			source->activate_ability(ability.get());
			scene_tree->idle(delta);
			// Then interrupt it.
			ability->wait_delay(delta + 0.1);
			scene_tree->idle(delta);

			THEN("ability got interrupted and cancelled") {
				CHECK(!ability->is_active());
				CHECK(ability->get_wait_handle().type == WaitType::None);
				REQUIRE(ability->wait_interrupt);
			}
		}
	}

	GIVEN("executing ability waiting for a delay") {
		// Scene Tree
		auto _ = finally([&scene_tree] { scene_tree->finish(); });
		auto root = make_gameplay_ptr<Node>();
		scene_tree->call("_change_scene", root.get());
		scene_tree->init();

		// Source
		auto source_attributes = make_reference<TestAttributeSet>();
		auto source = make_gameplay_ptr<GameplayAbilitySystem>();
		source->set_attribute_set(source_attributes);
		source->add_tag("equipment.weapon");
		root->add_child(source.get());

		// Target
		auto target_attributes = make_reference<TestAttributeSet>();
		auto target = make_gameplay_ptr<GameplayAbilitySystem>();
		target->set_attribute_set(target_attributes);
		source->add_target(target.get());
		root->add_child(target.get());

		// Ability
		auto ability = make_gameplay_ptr<AttackAbility>([](AttackAbility *ability) {
			ability->wait_for_delay = true;
		});
		source->add_ability(ability.get());

		WHEN("explicitly call interrupt callback") {
			// First activate wait handle.
			source->activate_ability(ability.get());
			scene_tree->idle(delta);
			// Then interrupt it.
			ability->_on_wait_interrupted();

			THEN("ability got interrupted and cancelled") {
				CHECK(!ability->is_active());
				CHECK(ability->get_wait_handle().type == WaitType::None);
				REQUIRE(ability->wait_interrupt);
			}
		}
	}
}

SCENARIO("check ability cost") {
	auto scene_tree = make_gameplay_ptr<TestSceneTree>();

	GIVEN("ability with high cost") {
		// Scene Tree
		auto _ = finally([&scene_tree] { scene_tree->finish(); });
		auto root = make_gameplay_ptr<Node>();
		scene_tree->call("_change_scene", root.get());
		scene_tree->init();

		// Source
		auto source_attributes = make_reference<TestAttributeSet>();
		auto source = make_gameplay_ptr<GameplayAbilitySystem>();
		source->set_attribute_set(source_attributes);
		source->add_tag("equipment.weapon");
		root->add_child(source.get());

		// Target
		auto target_attributes = make_reference<TestAttributeSet>();
		auto target = make_gameplay_ptr<GameplayAbilitySystem>();
		target->set_attribute_set(target_attributes);
		source->add_target(target.get());
		root->add_child(target.get());

		// Ability
		auto ability = make_gameplay_ptr<AttackAbility>([](AttackAbility *ability) {
			Ref<GameplayEffectModifier> modifier = ability->get_cost_effect()->get_modifiers().front();
			modifier->set_modifier_magnitude(ability->const_200);
			ability->wait_for_delay = true;
		});
		source->add_ability(ability.get());

		WHEN("ability cost exceeds available resources") {
			THEN("ability will not activate") {
				REQUIRE(!ability->can_activate_ability());
			}
		}

		WHEN("waiting ability will not activate if cost exceed resources") {
			// First activate wait handle.
			source->activate_ability(ability.get());
			scene_tree->idle(delta);

			THEN("ability will not activate") {
				CHECK(!ability->is_active());
				CHECK(!ability->can_activate_ability());
				CHECK(source->get_current_attribute_value(stamina) == 100);
				REQUIRE(target->get_current_attribute_value(health) == 100);
			}
		}

		WHEN("instant ability will not execute if cost exceed resources") {
			// First activate wait handle.
			ability->wait_for_delay = false;
			source->activate_ability(ability.get());
			scene_tree->idle(delta);

			THEN("do not execute ability") {
				CHECK(!ability->is_active());
				CHECK(!ability->can_activate_ability());
				CHECK(source->get_current_attribute_value(stamina) == 100);
				REQUIRE(target->get_current_attribute_value(health) == 100);
			}
		}
	}
}

#pragma endregion

#pragma region ability cancellation

SCENARIO("cancel ability mid execution") {
	auto scene_tree = make_gameplay_ptr<TestSceneTree>();

	GIVEN("ability which executes and another one with cancellation tags") {
		// Scene Tree
		auto _ = finally([&scene_tree] { scene_tree->finish(); });
		auto root = make_gameplay_ptr<Node>();
		scene_tree->call("_change_scene", root.get());
		scene_tree->init();

		// Source
		auto source_attributes = make_reference<TestAttributeSet>();
		auto source = make_gameplay_ptr<GameplayAbilitySystem>();
		source->set_attribute_set(source_attributes);
		source->add_tag("equipment.weapon");
		root->add_child(source.get());

		// Target
		auto target_attributes = make_reference<TestAttributeSet>();
		auto target = make_gameplay_ptr<GameplayAbilitySystem>();
		target->set_attribute_set(target_attributes);
		source->add_target(target.get());
		root->add_child(target.get());

		// Ability
		auto executing_ability = make_gameplay_ptr<AttackAbility>([](AttackAbility *ability) {
			ability->wait_for_event = true;
			ability->get_ability_tags()->append("ability.attack");
		});
		auto cancelling_ability = make_gameplay_ptr<AttackAbility>([](AttackAbility *ability) {
			ability->get_cancel_ability_tags()->append("ability.*");
		});
		source->add_ability(executing_ability.get());
		source->add_ability(cancelling_ability.get());

		// Start execution
		source->activate_ability(executing_ability.get());
		scene_tree->idle(delta);

		WHEN("cancelling ability activates") {
			// Cancelling ability executes
			source->activate_ability(cancelling_ability.get());
			scene_tree->idle(delta);

			THEN("executing ability got cancelled") {
				CHECK(executing_ability->got_cancelled);
				REQUIRE(!executing_ability->is_active());
			}
		}
	}

	GIVEN("ability which executes and another one with cancellation effects") {
		// Scene Tree
		auto _ = finally([&scene_tree] { scene_tree->finish(); });
		auto root = make_gameplay_ptr<Node>();
		scene_tree->call("_change_scene", root.get());
		scene_tree->init();

		// Source
		auto source_attributes = make_reference<TestAttributeSet>();
		auto source = make_gameplay_ptr<GameplayAbilitySystem>();
		source->set_attribute_set(source_attributes);
		source->add_tag("equipment.weapon");
		root->add_child(source.get());

		// Target
		auto target_attributes = make_reference<TestAttributeSet>();
		auto target = make_gameplay_ptr<GameplayAbilitySystem>();
		target->set_attribute_set(target_attributes);
		source->add_target(target.get());
		root->add_child(target.get());

		// Ability
		auto executing_ability = make_gameplay_ptr<AttackAbility>([](AttackAbility *ability) {
			ability->wait_for_event = true;
			ability->get_ability_tags()->append("ability.attack");
		});
		auto cancelling_ability = make_gameplay_ptr<CancellationAbility>();
		source->add_ability(executing_ability.get());
		source->add_ability(cancelling_ability.get());

		// Start execution
		source->activate_ability(executing_ability.get());
		scene_tree->idle(delta);

		WHEN("cancelling ability activates") {
			// Cancelling ability executes
			source->activate_ability(cancelling_ability.get());
			scene_tree->idle(delta);

			THEN("executing ability got cancelled") {
				CHECK(executing_ability->got_cancelled);
				REQUIRE(!executing_ability->is_active());
			}
		}
	}
}

#pragma endregion

#pragma region effect tagging

SCENARIO("check if effect target tags get applied") {
	auto scene_tree = make_gameplay_ptr<TestSceneTree>();

	GIVEN("ability with effect that adds target test tag") {
		// Scene Tree
		auto _ = finally([&scene_tree] { scene_tree->finish(); });
		auto root = make_gameplay_ptr<Node>();
		scene_tree->call("_change_scene", root.get());
		scene_tree->init();

		// Source
		auto source_attributes = make_reference<TestAttributeSet>();
		auto source = make_gameplay_ptr<GameplayAbilitySystem>();
		source->set_attribute_set(source_attributes);
		root->add_child(source.get());

		// Target
		auto target_attributes = make_reference<TestAttributeSet>();
		auto target = make_gameplay_ptr<GameplayAbilitySystem>();
		target->set_attribute_set(target_attributes);
		source->add_target(target.get());
		root->add_child(target.get());

		// Ability
		auto ability = make_gameplay_ptr<ApplyEffectAbility>([](ApplyEffectAbility *ability) {
			ability->target_effects.push_back(make_reference<GameplayEffect>([](Ref<GameplayEffect> effect) {
				effect->set_duration_type(DurationType::Infinite);
				effect->get_target_tags()->append("test");
			}));
		});
		source->add_ability(ability.get());

		WHEN("ability activates and executes") {
			// Start execution
			source->activate_ability(ability.get());
			scene_tree->idle(delta);

			THEN("target has test tag") {
				REQUIRE(target->get_active_tags()->has_tag("test"));
			}
		}
	}
}

SCENARIO("check if effects with removal tags get removed") {
	auto scene_tree = make_gameplay_ptr<TestSceneTree>();

	GIVEN("ability with effect that removes test tag") {
		// Scene Tree
		auto _ = finally([&scene_tree] { scene_tree->finish(); });
		auto root = make_gameplay_ptr<Node>();
		scene_tree->call("_change_scene", root.get());
		scene_tree->init();

		// Source
		auto source_attributes = make_reference<TestAttributeSet>();
		auto source = make_gameplay_ptr<GameplayAbilitySystem>();
		source->set_attribute_set(source_attributes);
		root->add_child(source.get());

		// Target
		auto target_attributes = make_reference<TestAttributeSet>();
		auto target = make_gameplay_ptr<GameplayAbilitySystem>();
		target->set_attribute_set(target_attributes);
		source->add_target(target.get());
		root->add_child(target.get());

		// Ability
		auto ability = make_gameplay_ptr<ApplyEffectAbility>([](ApplyEffectAbility *ability) {
			ability->target_effects.push_back(make_reference<GameplayEffect>([](Ref<GameplayEffect> effect) {
				effect->set_duration_type(DurationType::Infinite);
				effect->get_remove_effect_tags()->append("test.effect");
				effect->get_target_tags()->append("test.target2");
				effect->get_effect_tags()->append("test.removal");
			}));
		});
		source->add_ability(ability.get());

		// Apply existing effect
		target->apply_effect(source.get(), make_reference<GameplayEffect>([](Ref<GameplayEffect> effect) {
			effect->set_duration_type(DurationType::Infinite);
			effect->get_target_tags()->append("test.target1");
			effect->get_effect_tags()->append("test.effect");
		}));

		// Query tags
		auto query_tags = make_reference<GameplayTagContainer>([](Ref<GameplayTagContainer> tags) {
			tags->append("test.*");
		});

		WHEN("ability activates and executes") {
			// Start execution
			source->activate_ability(ability.get());
			scene_tree->idle(delta);

			THEN("effect with test tag has been removed") {
				CHECK(target->query_active_effects(query_tags).size() == 1);
				CHECK(!target->get_active_tags()->has_tag("test.target1"));
				REQUIRE(target->get_active_tags()->has_tag("test.target2"));
			}
		}
	}
}

SCENARIO("check if effects with immunity tags are denied application") {
	auto scene_tree = make_gameplay_ptr<TestSceneTree>();

	GIVEN("target has effect which grants immunity") {
		// Scene Tree
		auto _ = finally([&scene_tree] { scene_tree->finish(); });
		auto root = make_gameplay_ptr<Node>();
		scene_tree->call("_change_scene", root.get());
		scene_tree->init();

		// Source
		auto source_attributes = make_reference<TestAttributeSet>();
		auto source = make_gameplay_ptr<GameplayAbilitySystem>();
		source->set_attribute_set(source_attributes);
		root->add_child(source.get());

		// Target
		auto target_attributes = make_reference<TestAttributeSet>();
		auto target = make_gameplay_ptr<GameplayAbilitySystem>();
		target->set_attribute_set(target_attributes);
		root->add_child(target.get());

		// Ability
		auto blocked_effect = make_reference<GameplayEffect>([](Ref<GameplayEffect> effect) {
			effect->set_duration_type(DurationType::Infinite);
			effect->get_effect_tags()->append("test.effect");
		});

		// Apply existing effect
		target->apply_effect(source.get(), make_reference<GameplayEffect>([](Ref<GameplayEffect> effect) {
			effect->set_duration_type(DurationType::Infinite);
			effect->get_application_immunity_tags()->append("test.effect");
		}));

		WHEN("trying to apply effect where target has immunity") {
			// Start execution
			scene_tree->idle(delta);
			bool application_failed = !target->try_apply_effect(source.get(), blocked_effect);

			THEN("effect application failed") {
				REQUIRE(application_failed);
			}
		}
	}
}

SCENARIO("check ongoing tags are used for continuous effect execution") {
	auto scene_tree = make_gameplay_ptr<TestSceneTree>();

	GIVEN("effect with ongoing tag requirements") {
		// Scene Tree
		auto _ = finally([&scene_tree] { scene_tree->finish(); });
		auto root = make_gameplay_ptr<Node>();
		scene_tree->call("_change_scene", root.get());
		scene_tree->init();

		// Source
		auto source_attributes = make_reference<TestAttributeSet>();
		auto source = make_gameplay_ptr<GameplayAbilitySystem>();
		source->set_attribute_set(source_attributes);
		root->add_child(source.get());

		// Target
		auto target_attributes = make_reference<TestAttributeSet>();
		auto target = make_gameplay_ptr<GameplayAbilitySystem>();
		target->set_attribute_set(target_attributes);
		source->add_target(target.get());
		root->add_child(target.get());

		// Ability
		auto ability = make_gameplay_ptr<ApplyEffectAbility>([](ApplyEffectAbility *ability) {
			ability->target_effects.push_back(make_reference<GameplayEffect>([](Ref<GameplayEffect> effect) {
				Array modifiers;
				modifiers.append(make_reference<GameplayEffectModifier>([](Ref<GameplayEffectModifier> modifier) {
					modifier->set_attribute(health);
					modifier->set_modifier_operation(ModifierOperation::Subtract);
					modifier->set_modifier_magnitude(make_reference<ScalableFloat>([](Ref<ScalableFloat> magnitude) {
						magnitude->set_value(10);
					}));
				}));
				effect->set_modifiers(modifiers);
				effect->set_duration_type(DurationType::Infinite);
				effect->set_period(make_reference<ScalableFloat>([](Ref<ScalableFloat> magnitude) {
					magnitude->set_value(5);
				}));
				effect->get_ongoing_tags()->append("test");
			}));
		});
		source->add_ability(ability.get());

		WHEN("effect applies on application and three periods") {
			// First execution which applies effect but won't execute due to ongoing requirement not met.
			source->activate_ability(ability.get());
			scene_tree->idle(delta);
			auto first_health = target->get_current_attribute_value(health);
			// Second execution satisfy requirements and applies effect.
			target->add_tag("test");
			scene_tree->idle(delta);
			auto second_health = target->get_current_attribute_value(health);
			// Third execution shouldn't meet requirements.
			target->remove_tag("test");
			scene_tree->idle(delta);
			auto third_health = target->get_current_attribute_value(health);
			// Fourth execution satisfy requirements and applies effect.
			target->add_tag("test");
			scene_tree->idle(delta);
			auto fourth_health = target->get_current_attribute_value(health);

			THEN("only two execution apply when conditions are met") {
				REQUIRE(first_health == 100.0);
				REQUIRE(second_health == 90.0);
				REQUIRE(third_health == 90.0);
				REQUIRE(fourth_health == 80.0);
			}
		}
	}
}

#pragma endregion

#pragma region effect modifiers

SCENARIO("scalable float magnitude should apply according to curve and level") {
	auto scene_tree = make_gameplay_ptr<TestSceneTree>();

	GIVEN("modifier which scales for five levels") {
		// Scene Tree
		auto _ = finally([&scene_tree] { scene_tree->finish(); });
		auto root = make_gameplay_ptr<Node>();
		scene_tree->call("_change_scene", root.get());
		scene_tree->init();

		// Source
		auto source_attributes = make_reference<TestAttributeSet>();
		auto source = make_gameplay_ptr<GameplayAbilitySystem>();
		source->set_attribute_set(source_attributes);
		root->add_child(source.get());

		// Target
		auto target_attributes = make_reference<TestAttributeSet>();
		auto target = make_gameplay_ptr<GameplayAbilitySystem>();
		target->set_attribute_set(target_attributes);
		source->add_target(target.get());
		root->add_child(target.get());

		// Ability
		auto ability = make_gameplay_ptr<ApplyEffectAbility>([](ApplyEffectAbility *ability) {
			ability->set_max_level(5);
			ability->target_effects.push_back(make_reference<GameplayEffect>([](Ref<GameplayEffect> effect) {
				Array modifiers;
				modifiers.append(make_reference<GameplayEffectModifier>([](Ref<GameplayEffectModifier> modifier) {
					modifier->set_attribute(health);
					modifier->set_modifier_operation(ModifierOperation::Subtract);
					modifier->set_modifier_magnitude(make_reference<ScalableFloat>([](Ref<ScalableFloat> magnitude) {
						auto curve = make_reference<Curve>([](Ref<Curve> curve) {
							curve->add_point(Vector2(0, 0), 0, 1);
							curve->add_point(Vector2(1, 1), 1, 0);
							curve->bake();
						});
						magnitude->set_value(50);
						magnitude->set_curve(curve);
					}));
				}));
				effect->set_modifiers(modifiers);
			}));
		});
		source->add_ability(ability.get());

		WHEN("ability is at level 1") {
			ability->set_current_level(1);
			source->activate_ability(ability.get());
			scene_tree->idle(delta);

			THEN("current health equals 90") {
				int current_health = round(target->get_current_attribute_value(health));
				REQUIRE(current_health == 90);
			}
		}

		WHEN("ability is at level 2") {
			ability->set_current_level(2);
			source->activate_ability(ability.get());
			scene_tree->idle(delta);

			THEN("current health equals 80") {
				int current_health = round(target->get_current_attribute_value(health));
				REQUIRE(current_health == 80);
			}
		}

		WHEN("ability is at level 3") {
			ability->set_current_level(3);
			source->activate_ability(ability.get());
			scene_tree->idle(delta);

			THEN("current health equals 70") {
				int current_health = round(target->get_current_attribute_value(health));
				REQUIRE(current_health == 70);
			}
		}

		WHEN("ability is at level 4") {
			ability->set_current_level(4);
			source->activate_ability(ability.get());
			scene_tree->idle(delta);

			THEN("current health equals 60") {
				int current_health = round(target->get_current_attribute_value(health));
				REQUIRE(current_health == 60);
			}
		}

		WHEN("ability is at level 5") {
			ability->set_current_level(5);
			source->activate_ability(ability.get());
			scene_tree->idle(delta);

			THEN("current health equals 50") {
				int current_health = round(target->get_current_attribute_value(health));
				REQUIRE(current_health == 50);
			}
		}
	}
}

SCENARIO("attribute based magnitude should change according to attribute") {
	auto scene_tree = make_gameplay_ptr<TestSceneTree>();

	GIVEN("modifier which scales for five levels") {
		// Scene Tree
		auto _ = finally([&scene_tree] { scene_tree->finish(); });
		auto root = make_gameplay_ptr<Node>();
		scene_tree->call("_change_scene", root.get());
		scene_tree->init();

		// Source
		auto source_attributes = make_reference<TestAttributeSet>();
		auto source = make_gameplay_ptr<GameplayAbilitySystem>();
		source->set_attribute_set(source_attributes);
		root->add_child(source.get());

		// Target
		auto target_attributes = make_reference<TestAttributeSet>();
		auto target = make_gameplay_ptr<GameplayAbilitySystem>();
		target->set_attribute_set(target_attributes);
		source->add_target(target.get());
		root->add_child(target.get());

		// Ability
		auto ability = make_gameplay_ptr<ApplyEffectAbility>([](ApplyEffectAbility *ability) {
			ability->set_max_level(5);
			ability->target_effects.push_back(make_reference<GameplayEffect>([](Ref<GameplayEffect> effect) {
				Array modifiers;
				modifiers.append(make_reference<GameplayEffectModifier>([](Ref<GameplayEffectModifier> modifier) {
					modifier->set_attribute(health);
					modifier->set_modifier_operation(ModifierOperation::Subtract);
					modifier->set_modifier_magnitude(make_reference<AttributeBasedFloat>([](Ref<AttributeBasedFloat> magnitude) {
						magnitude->set_attribute_origin(AttributeOrigin::Source);
						magnitude->set_attribute_calculation(AttributeCalculation::CurrentValue);
						magnitude->set_backing_attribute(attack);
						magnitude->set_coefficient(make_reference<ScalableFloat>([](Ref<ScalableFloat> magnitude) {
							auto curve = make_reference<Curve>([](Ref<Curve> curve) {
								curve->add_point(Vector2(0, 0), 0, 1);
								curve->add_point(Vector2(1, 1), 1, 0);
								curve->bake();
							});
							magnitude->set_value(0.5);
							magnitude->set_curve(curve);
						}));
					}));
				}));
				effect->set_modifiers(modifiers);
			}));
		});
		source->add_ability(ability.get());

		WHEN("ability is level 1") {
			ability->set_current_level(1);
			source->activate_ability(ability.get());
			scene_tree->idle(delta);

			THEN("current health is 90") {
				int current_health = round(target->get_current_attribute_value(health));
				REQUIRE(current_health == 90);
			}
		}

		WHEN("ability is level 2") {
			ability->set_current_level(2);
			source->activate_ability(ability.get());
			scene_tree->idle(delta);

			THEN("current health is 80") {
				int current_health = round(target->get_current_attribute_value(health));
				REQUIRE(current_health == 80);
			}
		}

		WHEN("ability is level 3") {
			ability->set_current_level(3);
			source->activate_ability(ability.get());
			scene_tree->idle(delta);

			THEN("current health is 70") {
				int current_health = round(target->get_current_attribute_value(health));
				REQUIRE(current_health == 70);
			}
		}

		WHEN("ability is level 4") {
			ability->set_current_level(4);
			source->activate_ability(ability.get());
			scene_tree->idle(delta);

			THEN("current health is 60") {
				int current_health = round(target->get_current_attribute_value(health));
				REQUIRE(current_health == 60);
			}
		}

		WHEN("ability is level 5") {
			ability->set_current_level(5);
			source->activate_ability(ability.get());
			scene_tree->idle(delta);

			THEN("current health is 50") {
				int current_health = round(target->get_current_attribute_value(health));
				REQUIRE(current_health == 50);
			}
		}
	}
}

SCENARIO("custom execution magnitude should do fancy stuff") {
	auto scene_tree = make_gameplay_ptr<TestSceneTree>();

	GIVEN("") {
		// Scene Tree
		auto _ = finally([&scene_tree] { scene_tree->finish(); });
		auto root = make_gameplay_ptr<Node>();
		scene_tree->call("_change_scene", root.get());
		scene_tree->init();

		// Source
		auto source_attributes = make_reference<TestAttributeSet>();
		auto source = make_gameplay_ptr<GameplayAbilitySystem>();
		source->set_attribute_set(source_attributes);
		root->add_child(source.get());

		// Target
		auto target_attributes = make_reference<TestAttributeSet>();
		auto target = make_gameplay_ptr<GameplayAbilitySystem>();
		target->set_attribute_set(target_attributes);
		source->add_target(target.get());
		root->add_child(target.get());

		// Ability
		auto ability = make_gameplay_ptr<ApplyEffectAbility>([&](ApplyEffectAbility *ability) {
			ability->set_max_level(5);
			ability->target_effects.push_back(make_reference<GameplayEffect>([&](Ref<GameplayEffect> effect) {
				Array modifiers;
				modifiers.append(make_reference<GameplayEffectModifier>([&](Ref<GameplayEffectModifier> modifier) {
					modifier->set_attribute(health);
					modifier->set_modifier_operation(ModifierOperation::Subtract);
					modifier->set_modifier_magnitude(make_reference<CustomCalculatedFloat>([&](Ref<CustomCalculatedFloat> magnitude) {
						magnitude->set_calculation_script(make_reference<TestScript>());
					}));
				}));
				effect->set_modifiers(modifiers);
			}));
		});
		source->add_ability(ability.get());

		WHEN("ability is level 1") {
			ability->set_current_level(1);
			source->activate_ability(ability.get());
			scene_tree->idle(delta);

			THEN("current health is 90") {
				int current_health = round(target->get_current_attribute_value(health));
				REQUIRE(current_health == 90);
			}
		}

		WHEN("ability is level 2") {
			ability->set_current_level(2);
			source->activate_ability(ability.get());
			scene_tree->idle(delta);

			THEN("current health is 80") {
				int current_health = round(target->get_current_attribute_value(health));
				REQUIRE(current_health == 80);
			}
		}

		WHEN("ability is level 3") {
			ability->set_current_level(3);
			source->activate_ability(ability.get());
			scene_tree->idle(delta);

			THEN("current health is 70") {
				int current_health = round(target->get_current_attribute_value(health));
				REQUIRE(current_health == 70);
			}
		}

		WHEN("ability is level 4") {
			ability->set_current_level(4);
			source->activate_ability(ability.get());
			scene_tree->idle(delta);

			THEN("current health is 60") {
				int current_health = round(target->get_current_attribute_value(health));
				REQUIRE(current_health == 60);
			}
		}

		WHEN("ability is level 5") {
			ability->set_current_level(5);
			source->activate_ability(ability.get());
			scene_tree->idle(delta);

			THEN("current health is 50") {
				int current_health = round(target->get_current_attribute_value(health));
				REQUIRE(current_health == 50);
			}
		}
	}
}

#pragma endregion

#pragma region effect executions
// Nothing as of yet.
#pragma endregion

#pragma region effect stacking

SCENARIO("stack aggregation", "[stacking]") {
	auto scene_tree = make_gameplay_ptr<TestSceneTree>();

	GIVEN("effect which aggregates on source") {
		// Scene Tree
		auto _ = finally([&scene_tree] { scene_tree->finish(); });
		auto root = make_gameplay_ptr<Node>();
		scene_tree->call("_change_scene", root.get());
		scene_tree->init();

		// Source
		auto source_attributes = make_reference<TestAttributeSet>();
		auto source = make_gameplay_ptr<GameplayAbilitySystem>();
		source->set_attribute_set(source_attributes);
		source->get_active_tags()->append("source");
		root->add_child(source.get());

		// Target
		auto target_attributes = make_reference<TestAttributeSet>();
		auto target = make_gameplay_ptr<GameplayAbilitySystem>();
		target->set_attribute_set(target_attributes);
		target->get_active_tags()->append("target");
		source->add_target(target.get());
		root->add_child(target.get());

		// Ability
		auto effect = make_reference<GameplayEffect>([](Ref<GameplayEffect> effect) {
			effect->set_effect_name("test.effect");
			effect->set_duration_type(DurationType::Infinite);
			effect->set_stacking_type(StackingType::AggregateOnSource);
			effect->set_maximum_stacks(2);
		});
		auto ability = make_gameplay_ptr<ApplyEffectAbility>([&](ApplyEffectAbility *ability) {
			ability->set_ability_name("test.ability");
			ability->target_effects.push_back(effect);
		});
		source->add_ability(ability.get());

		WHEN("effect applies two times in different frame") {
			// Execute two times for two stacks to apply
			source->activate_ability(ability.get());
			scene_tree->idle(delta);
			source->activate_ability(ability.get());
			scene_tree->idle(delta);

			THEN("source has a stack count of 2") {
				CHECK(target->get_stack_count(effect) == 0);
				REQUIRE(source->get_stack_count(effect) == 2);
			}
		}
	}

	GIVEN("effect which aggregates on target") {
		// Scene Tree
		auto _ = finally([&scene_tree] { scene_tree->finish(); });
		auto root = make_gameplay_ptr<Node>();
		scene_tree->call("_change_scene", root.get());
		scene_tree->init();

		// Source
		auto source_attributes = make_reference<TestAttributeSet>();
		auto source = make_gameplay_ptr<GameplayAbilitySystem>();
		source->set_attribute_set(source_attributes);
		source->get_active_tags()->append("source");
		root->add_child(source.get());

		// Target
		auto target_attributes = make_reference<TestAttributeSet>();
		auto target = make_gameplay_ptr<GameplayAbilitySystem>();
		target->set_attribute_set(target_attributes);
		target->get_active_tags()->append("target");
		source->add_target(target.get());
		root->add_child(target.get());

		// Ability
		auto effect = make_reference<GameplayEffect>([](Ref<GameplayEffect> effect) {
			effect->set_effect_name("test.effect");
			effect->set_duration_type(DurationType::Infinite);
			effect->set_stacking_type(StackingType::AggregateOnTarget);
			effect->set_maximum_stacks(2);
		});
		auto ability = make_gameplay_ptr<ApplyEffectAbility>([&](ApplyEffectAbility *ability) {
			ability->set_ability_name("test.ability");
			ability->target_effects.push_back(effect);
		});
		source->add_ability(ability.get());

		WHEN("effect applies two times in different frame") {
			// Execute two times for two stacks to apply
			source->activate_ability(ability.get());
			scene_tree->idle(delta);
			source->activate_ability(ability.get());
			scene_tree->idle(delta);

			THEN("source has a stack count of 2") {
				CHECK(source->get_stack_count(effect) == 0);
				REQUIRE(target->get_stack_count(effect) == 2);
			}
		}
	}

	GIVEN("effect which applies overflow effect") {
		// Scene Tree
		auto _ = finally([&scene_tree] { scene_tree->finish(); });
		auto root = make_gameplay_ptr<Node>();
		scene_tree->call("_change_scene", root.get());
		scene_tree->init();

		// Source
		auto source_attributes = make_reference<TestAttributeSet>();
		auto source = make_gameplay_ptr<GameplayAbilitySystem>();
		source->set_attribute_set(source_attributes);
		source->get_active_tags()->append("source");
		root->add_child(source.get());

		// Target
		auto target_attributes = make_reference<TestAttributeSet>();
		auto target = make_gameplay_ptr<GameplayAbilitySystem>();
		target->set_attribute_set(target_attributes);
		target->get_active_tags()->append("target");
		source->add_target(target.get());
		root->add_child(target.get());

		// Ability
		auto effect = make_reference<GameplayEffect>([](Ref<GameplayEffect> effect) {
			effect->set_effect_name("test_effect");
			effect->get_effect_tags()->append("test.effect");
			effect->set_duration_type(DurationType::Infinite);
			effect->set_stacking_type(StackingType::AggregateOnSource);

			Array overflow;
			overflow.append(make_reference<GameplayEffect>([](Ref<GameplayEffect> overflow_effect) {
				overflow_effect->set_effect_name("test_overflow_effect");
				overflow_effect->get_effect_tags()->append("test.overflow_effect");
				overflow_effect->get_target_tags()->append("overflow");
				overflow_effect->set_duration_type(DurationType::Infinite);
			}));
			effect->set_overflow_effects(overflow);
		});
		auto ability = make_gameplay_ptr<ApplyEffectAbility>([&](ApplyEffectAbility *ability) {
			ability->set_ability_name("test.ability");
			ability->target_effects.push_back(effect);
		});
		source->add_ability(ability.get());

		WHEN("apply one more stack than allowed") {
			// Let ability overflow its effect.
			source->activate_ability(ability.get());
			scene_tree->idle(delta);
			source->activate_ability(ability.get());
			scene_tree->idle(delta);
			// Execute deferred call and add overflow effect.
			scene_tree->idle(delta);

			THEN("overflow effect is applied") {
				CHECK(target->query_active_effects_by_tag("test.*").size() == 2);
				REQUIRE(target->get_active_tags()->has_tag("overflow"));
			}
		}
	}
}

#pragma endregion

namespace TestGameplayAbilities {
MainLoop *test() {
	try {
		Catch::Session().run();
	} catch (std::exception &e) {
		OS::get_singleton()->printerr("%s\n", e.what());
	}

	OS::get_singleton()->print("Press enter to continue ...");
#ifdef DEBUG_ENABLED
	(void)getchar();
#endif
	return nullptr;
}
} // namespace TestGameplayAbilities
