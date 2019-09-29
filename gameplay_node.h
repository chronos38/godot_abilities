#pragma once

#include "gameplay_api.h"

#include <core/os/thread.h>
#include <core/resource.h>
#include <core/vector.h>
#include <scene/main/node.h>

class GAMEPLAY_ABILITIES_API GameplayNode : public Node {
	GDCLASS(GameplayNode, Node);
	OBJ_CATEGORY("GameplayAbilities");

public:
	virtual ~GameplayNode() = default;

	template <class T>
	T *find_child() const {
		for (int i = 0, n = get_child_count(); i < n; i++) {
			if (auto child = dynamic_cast<T *>(get_child(i))) {
				return child;
			}
		}

		return nullptr;
	}

	template <class T, class Predicate>
	T *find_child(Predicate &&p) const {
		for (int i = 0, n = get_child_count(); i < n; i++) {
			if (auto child = dynamic_cast<T *>(get_child(i))) {
				if (p(child)) {
					return child;
				}
			}
		}

		return nullptr;
	}

	template <class T, class Predicate>
	void for_each_child(Predicate &&p) const {
		for (int i = 0, n = get_child_count(); i < n; i++) {
			if (auto child = dynamic_cast<T *>(get_child(i))) {
				p(child);
			}
		}
	}

	template <class T>
	Vector<T *> find_all_children() const {
		Vector<T *> result;

		for (int i = 0, n = get_child_count(); i < n; i++) {
			if (auto child = dynamic_cast<T *>(get_child(i))) {
				result.push_back(child);
			}
		}

		return result;
	}

	template <class T>
	Vector<T *> find_all_children_multilevel() const {
		Vector<T *> result;
		Vector<Node *> nodes;
		nodes.push_back(const_cast<GameplayNode *>(this));

		for (int i = 0; i < nodes.size(); i++) {
			auto node = nodes[i];

			for (int j = 0, n = node->get_child_count(); j < n; i++) {
				if (auto child = node->get_child(j)) {
					nodes.push_back(child);

					if (auto typed_node = dynamic_cast<T *>(child)) {
						result.push_back(typed_node);
					}
				}
			}
		}

		return result;
	}

	template <class... Args>
	Variant execute(const StringName &method, Args &&... args) {
		if (auto rpc_mode = get_node_rpc_mode(method)) {
			if (rpc_mode->value() != MultiplayerAPI::RPC_MODE_DISABLED) {
				return rpc(method, std::forward<Args>(args)...);
			}
		}

		return call(method, std::forward<Args>(args)...);
	}

	Node *_bind_find_child(const String &class_name) const;
	Array _bind_find_all_children(const String &class_name) const;
	Array _bind_find_all_children_multilevel(const String &class_name) const;

	Dictionary serialise() const;
	void deserialise(const Dictionary &data);

private:
	/** Bindings */
	static void _bind_methods();
};

class GAMEPLAY_ABILITIES_API GameplayResource : public Resource {
	GDCLASS(GameplayResource, Resource);
	OBJ_CATEGORY("GameplayAbilities");

public:
	virtual ~GameplayResource() = default;

	Dictionary serialise() const;
	void deserialise(const Dictionary &data);

private:
	/** Bindings */
	static void _bind_methods();
};
