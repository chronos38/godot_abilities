#pragma once

#ifdef GAMEPLAY_ABILITIES_SHAREDLIB
#if defined _WIN32 || defined __CYGWIN__
#ifdef GAMEPLAY_ABILITIES_EXPORT
#ifdef __GNUC__
#define GAMEPLAY_ABILITIES_API __attribute__((dllexport))
#else
#define GAMEPLAY_ABILITIES_API __declspec(dllexport)
#endif
#else
#ifdef __GNUC__
#define GAMEPLAY_ABILITIES_API __attribute__((dllimport))
#else
#define GAMEPLAY_ABILITIES_API __declspec(dllimport)
#endif
#endif
#else
#if __GNUC__ >= 4
#define GAMEPLAY_ABILITIES_API __attribute__((visibility("default")))
#else
#define GAMEPLAY_ABILITIES_API
#endif
#endif
#else
#define GAMEPLAY_ABILITIES_API
#endif

#include <core/array.h>
#include <core/func_ref.h>
#include <core/pool_vector.h>
#include <core/reference.h>
#include <core/rid.h>
#include <core/string_name.h>
#include <core/typedefs.h>
#include <core/ustring.h>
#include <core/variant.h>
#include <core/vector.h>
#include <memory.h>

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <type_traits>

template <class T>
struct GameplayDeleter {
	constexpr GameplayDeleter() noexcept = default;
	template <class U>
	GameplayDeleter(const GameplayDeleter<U> &d) noexcept {}

	void operator()(T *ptr) const {
		memdelete(ptr);
	}

	template <class U>
	void operator()(U *ptr) const {
		memdelete(ptr);
	}
};

template <class T>
struct GameplayDeleter<T[]> {
	constexpr GameplayDeleter() noexcept = default;
	template <class U>
	GameplayDeleter(const GameplayDeleter<U[]> &d) noexcept {}

	void operator()(T *ptr) const {
		memdelete_arr(ptr);
	}

	template <class U>
	void operator()(U *ptr) const {
		memdelete_arr(ptr);
	}
};

template <class T, class Deleter = GameplayDeleter<T> >
class GameplayPtr : public std::unique_ptr<T, Deleter> {
public:
	using pointer = typename std::unique_ptr<T, Deleter>::pointer;
	using element_type = typename std::unique_ptr<T, Deleter>::element_type;
	using deleter_type = typename std::unique_ptr<T, Deleter>::deleter_type;

	constexpr GameplayPtr() noexcept : std::unique_ptr<T, Deleter>() {}
	constexpr GameplayPtr(std::nullptr_t) noexcept : std::unique_ptr<T, Deleter>(nullptr) {}
	explicit GameplayPtr(pointer p) noexcept : std::unique_ptr<T, Deleter>(p) {}
	GameplayPtr(pointer p, const deleter_type &d) noexcept : std::unique_ptr<T, Deleter>(p, d) {}
	GameplayPtr(pointer p, deleter_type &&d) noexcept : std::unique_ptr<T, Deleter>(p, d) {}
	GameplayPtr(GameplayPtr &&p) noexcept : std::unique_ptr<T, Deleter>(std::move(p)) {}
	template <class U, class E>
	GameplayPtr(GameplayPtr<U, E> &&p) noexcept : std::unique_ptr<T, Deleter>(std::move(p)) {}

	GameplayPtr &operator=(GameplayPtr &&r) noexcept {
		if (this != std::addressof(r)) {
			this->reset(r.release());
		}

		return *this;
	}

	template <class U, class E>
	GameplayPtr &operator=(GameplayPtr<U, E> &&r) noexcept {
		this->reset(r.release());
		return *this;
	}

	GameplayPtr &operator=(std::nullptr_t) noexcept {
		this->reset();
		return *this;
	}

	bool is_valid() const {
		return this->get() != nullptr;
	}

	bool is_null() const {
		return this->get() == nullptr;
	}
};

template <class T, class Deleter>
class GameplayPtr<T[], Deleter> : public std::unique_ptr<T[], Deleter> {
public:
	using pointer = typename std::unique_ptr<T, Deleter>::pointer;
	using element_type = typename std::unique_ptr<T, Deleter>::element_type;
	using deleter_type = typename std::unique_ptr<T, Deleter>::deleter_type;

	constexpr GameplayPtr() noexcept : std::unique_ptr<T[], Deleter>() {}
	constexpr GameplayPtr(std::nullptr_t) noexcept : std::unique_ptr<T[], Deleter>(nullptr) {}
	explicit GameplayPtr(pointer p) noexcept : std::unique_ptr<T[], Deleter>(p) {}
	template <class U>
	explicit GameplayPtr(U p) noexcept : std::unique_ptr<T[], Deleter>(p) {}
	GameplayPtr(pointer p, const deleter_type &d) noexcept : std::unique_ptr<T[], Deleter>(p, d) {}
	GameplayPtr(pointer p, deleter_type &&d) noexcept : std::unique_ptr<T[], Deleter>(p, d) {}
	GameplayPtr(GameplayPtr &&p) noexcept : std::unique_ptr<T[], Deleter>(std::move(p)) {}
	template <class U, class E>
	GameplayPtr(GameplayPtr<U, E> &&p) noexcept : std::unique_ptr<T[], Deleter>(std::move(p)) {}

	GameplayPtr &operator=(GameplayPtr &&r) noexcept {
		if (this != std::addressof(r)) {
			this->reset(r.release());
		}

		return *this;
	}

	template <class U, class E>
	GameplayPtr &operator=(GameplayPtr<U, E> &&r) noexcept {
		this->reset(r.release());
		return *this;
	}

	GameplayPtr &operator=(std::nullptr_t) noexcept {
		this->reset();
		return *this;
	}

	bool is_valid() const {
		return this->get() != nullptr;
	}

	bool is_null() const {
		return this->get() == nullptr;
	}
};

template <class T, std::enable_if_t<!std::is_array<T>::value, int> = 0>
static GameplayPtr<T> make_gameplay_ptr() {
	return GameplayPtr<T>(memnew(T));
}

template <class T, class Initialiser, std::enable_if_t<!std::is_array<T>::value, int> = 0>
static GameplayPtr<T> make_gameplay_ptr(Initialiser &&init) {
	auto obj = GameplayPtr<T>(memnew(T));
	init(obj.get());
	return obj;
}

template <class T, std::enable_if_t<std::is_array<T>::value && std::extent<T>::value == 0, int> = 0>
static GameplayPtr<T> make_gameplay_ptr(std::size_t size) {
	using Type = std::remove_extent_t<T>;
	return GameplayPtr<T>(memnew_arr(Type, size));
}

template <class T, class Initialiser, std::enable_if_t<std::is_array<T>::value && std::extent<T>::value == 0, int> = 0>
static GameplayPtr<T> make_gameplay_ptr(std::size_t size, Initialiser &&init) {
	using Type = std::remove_extent_t<T>;
	auto obj = GameplayPtr<T>(memnew_arr(Type, size));
	init(obj.get());
	return obj;
}

template <class T>
static Ref<T> make_reference() {
	return Ref<T>(memnew(T));
}

template <class T, class Initialiser>
static Ref<T> make_reference(Initialiser &&init) {
	auto ref = Ref<T>(memnew(T));
	init(ref);
	return ref;
}

template <class T>
class ArrayContainer : public Array {
public:
	ArrayContainer() = default;
	ArrayContainer(ArrayContainer &&array) = default;
	ArrayContainer(const ArrayContainer &array) = default;

	ArrayContainer(const Array &array) :
			Array(array) {}

	ArrayContainer &operator=(const ArrayContainer &value) {
		Array::operator=(value);
		apply_array_type(*this);
		return *this;
	}

	ArrayContainer &operator=(const Array &value) {
		Array::operator=(value);
		apply_array_type(*this);
		return *this;
	}

	Variant *begin() {
		return empty() ? nullptr : &operator[](0);
	}

	const Variant *begin() const {
		return empty() ? nullptr : &operator[](0);
	}

	const Variant *cbegin() const {
		return begin();
	}

	Variant *end() {
		return begin() + size();
	}

	const Variant *end() const {
		return begin() + size();
	}

	const Variant *cend() const {
		return end();
	}

private:
	static void apply_array_type(Array &array) {
		for (int i = 0; i < array.size(); i++) {
			if (array[i].get_type() == Variant::NIL) {
				array[i] = Ref<T>(memnew(T));
			} else if (array[i].get_type() != Variant::OBJECT) {
				array[i] = Ref<T>(memnew(T));
			} else {
				auto ref = static_cast<Ref<T> >(array[i]);

				if (ref.is_null()) {
					array[i] = Ref<T>(memnew(T));
				} else if (!dynamic_cast<T *>(*ref)) {
					array[i] = Ref<T>(memnew(T));
				}
			}
		}
	}
};

inline Variant *begin(Array &array) {
	return array.empty() ? nullptr : &array[0];
}

inline const Variant *begin(const Array &array) {
	return array.empty() ? nullptr : &array[0];
}

inline const Variant *cbegin(const Array &array) {
	return begin(array);
}

inline Variant *end(Array &array) {
	return array.empty() ? nullptr : &array[0] + array.size();
}

inline const Variant *end(const Array &array) {
	return array.empty() ? nullptr : &array[0] + array.size();
}

inline const Variant *cend(const Array &array) {
	return end(array);
}

template <class T>
T *begin(Vector<T> &array) {
	return array.ptrw();
}

template <class T>
const T *begin(const Vector<T> &array) {
	return array.ptr();
}

template <class T>
const T *cbegin(const Vector<T> &array) {
	return begin(array);
}

template <class T>
T *end(Vector<T> &array) {
	return begin(array) + array.size();
}

template <class T>
const T *end(const Vector<T> &array) {
	return begin(array) + array.size();
}

template <class T>
const T *cend(const Vector<T> &array) {
	return end(array);
}

template <class T>
T *begin(PoolVector<T> &array) {
	return array.write().ptr();
}

template <class T>
const T *begin(const PoolVector<T> &array) {
	return array.read().ptr();
}

template <class T>
const T *cbegin(const PoolVector<T> &array) {
	return begin(array);
}

template <class T>
T *end(PoolVector<T> &array) {
	return begin(array) + array.size();
}

template <class T>
const T *end(const PoolVector<T> &array) {
	return begin(array) + array.size();
}

template <class T>
const T *cend(const PoolVector<T> &array) {
	return end(array);
}

inline String operator"" _s(const char *str, std::size_t size) {
	return String(str);
}

inline StringName operator"" _n(const char *str, std::size_t size) {
	return StringName(str);
}

#define GA_RPC_MASTER
#define GA_RPC_PUPPET
#define GA_RPC_REMOTE
#define GA_RPC_REMOTESYNC
