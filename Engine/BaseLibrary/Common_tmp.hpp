#pragma once
#pragma once
#include <string>
#include <algorithm>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include <mathfu/mathfu_exc.hpp>
using namespace mathfu;

class Color
{
public:
	inline Color() {}
	inline Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a): r(r), g(g), b(b), a(a) {}
	inline Color(uint8_t r, uint8_t g, uint8_t b): Color(r, g, b, 255) {}
	inline Color(uint8_t greyscale) : Color(greyscale, greyscale, greyscale, 255) {}

	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;

	inline Color operator + (int val)
	{
		Color color;
		color.r = r + val;
		color.g = g + val;
		color.b = b + val;

		return color;
	}

	inline bool operator == (const Color& color)
	{
		return memcmp(this, &color, sizeof(Color)) == 0;
	}

public:
	static Color BLACK;
	static Color WHITE;
	static Color RED;
	static Color GREEN;
	static Color BLUE;
};

struct RectNormed
{
	RectNormed() :bottomLeftPercentNormed(0, 0), topRightPercentNormed(1, 1) {}

	Vector2f bottomLeftPercentNormed;
	Vector2f topRightPercentNormed;
};

template<class T>
class Rect
{
public:
	inline Rect():left(0), right(0), top(0), bottom(0)
	{}

	inline Rect(const Vector2f& pos, const Vector2f& size)
	: left(pos.x()), right(pos.x() + size.x()), top(pos.y()), bottom(pos.y() + size.y())
	{}

	inline Rect(const T& left, const T& top, const T& right, const T& bottom)
	:left(left), right(right), top(top), bottom(bottom)
	{}
	
	static Rect FromSize(float left, float top, float width, float height)
	{
		return Rect(left, top, left + width, top + height);
	}

	void MoveSides(const Rect& offset)
	{
		left   += offset.left;
		right  += offset.right;
		top	   += offset.top;
		bottom += offset.bottom;
	}

	void MoveSidesLocal(const Rect& offset)
	{
		left   -= offset.left;
		right  += offset.right;
		top	   -= offset.top;
		bottom += offset.bottom;
	}

	void Union(const Rect& other)
	{
		*this = Rect::Union(*this, other);
	};

	void UnionWidth(float width)
	{
		Rect selfCopy = *this;
		selfCopy.SetWidth(width);

		Union(*this, selfCopy);
	}

	void UnionHeight(float height)
	{
		Rect selfCopy = *this;
		selfCopy.SetHeight(height);

		Union(*this, selfCopy);
	}

	static Rect Union(const Rect& a, const Rect& b)
	{
		Rect result;

		float maxLeft = std::min(a.left, b.left);
		float maxTop = std::min(a.top, b.top);
		float minBottom = std::max(a.bottom, b.bottom);
		float minRight = std::max(a.right, b.right);

		result.left = maxLeft;
		result.top = maxTop;
		result.right = minRight;
		result.bottom = minBottom;

		return result;
	};

	void Intersect(const Rect& other)
	{
		*this = Rect::Intersect(*this, other);
	}

	static Rect Intersect(const Rect& a, const Rect& b)
	{
		Rect result;

		float maxLeft = std::max(a.left, b.left);
		float maxTop = std::max(a.top, b.top);
		float minBottom = std::min(a.bottom, b.bottom);
		float minRight = std::min(a.right, b.right);

		result.left = maxLeft;
		result.top = maxTop;
		result.right = minRight;
		result.bottom = minBottom;

		return result;
	}

	Rect operator -() const
	{
		Rect result;
		result.left = -left;
		result.right = -right;
		result.top = -top;
		result.bottom = -bottom;
		return result;
	};

	Rect operator - (const Rect& other) const
	{
		Rect result;
		result.left = left - other.top;
		result.right = right - other.right;
		result.top = top - other.top;
		result.bottom = bottom - other.bottom;
		return result;
	}

	bool IsPointInside(Vector2i point) const
	{
		return	point.x() >= left && point.x() <= right	&&
				point.y() >= top  && point.y() <= bottom;
	}

	Vector2f GetSize() const { return Vector2f(GetWidth(), GetHeight()); }
	Vector2f GetPos() const { return Vector2f(left, top); }
	float GetPosX() const { return GetPos().x(); }
	float GetPosY() const { return GetPos().y(); }

	float GetSizeX() const { return GetSize().x(); }
	float GetSizeY() const { return GetSize().y(); }

	Vector2f GetCenter() const
	{
		return Vector2f(left + 0.5f * GetWidth(), top + 0.5f * GetHeight());
	}

	bool operator == (const Rect<T>& other) const
	{
		return memcmp(this, &other, sizeof(Rect<T>)) == 0;
	}

	bool operator != (const Rect<T>& other) const
	{
		return memcmp(this, &other, sizeof(Rect<T>)) != 0;
	}

	void SetWidth(float width)
	{
		right = left + width;
	}

	void SetHeight(float height)
	{
		bottom = top + height;
	}

	float GetWidth() const
	{
		return right - left;
	}

	float GetHeight() const
	{
		return bottom - top;
	}

	void SetSize(const Vector2f& size)
	{
		SetWidth(size.x());
		SetHeight(size.y());
	}

	T left;
	T right;
	T top;
	T bottom;
};

typedef Rect<float> RectF;







#define ENUM_CLASS_BITFLAG( enumClass, enumType )  \
enum class enumClass : enumType; \
class zzzEnum_##enumClass\
{ \
public:\
	zzzEnum_##enumClass(enumClass e) :flag(e) {}\
	operator enumClass() { return flag; }\
	operator bool() { return (enumType)flag != 0; }\
public:\
	enumClass flag;\
};\
inline zzzEnum_##enumClass operator | (enumClass a, enumClass b) \
{\
	return (enumClass)((enumType)a | (enumType)b);\
}\
inline bool operator & (enumClass a, enumClass b)\
{\
	return ((enumType)a & (enumType)b) > 0;\
}\
inline zzzEnum_##enumClass operator ~ (zzzEnum_##enumClass a)\
{\
	return (enumClass)(~(enumType)a.flag);\
}\
inline zzzEnum_##enumClass operator ^ (enumClass a, enumClass b)\
{\
	return (enumClass)((enumType)a ^ (enumType)b);\
}\
inline zzzEnum_##enumClass operator >> (enumClass a, enumClass b)\
{\
	return (enumClass)((enumType)a >> (enumType)b);\
}\
inline zzzEnum_##enumClass operator << (enumClass a, enumClass b)\
{\
	return (enumClass)((enumType)a << (enumType)b);\
}\
inline enumClass& operator |= (enumClass& a, enumClass b)\
{\
	(enumType&)a |= (enumType&)b; \
	return a; \
}\
inline enumClass& operator &= (enumClass& a, enumClass b)\
{\
	(enumType&)a &= (enumType&)b; \
	return a; \
}\
inline enumClass& operator ^= (enumClass& a, enumClass b)\
{\
	(enumType&)a ^= (enumType&)b; \
	return a; \
}\
enum class enumClass : enumType




template<class T>
T saturate(const T& val)
{
	T result = val;

	if (result < 0)
		result = 0;

	if (result > 1)
		result = 1;

	return result;
}

template<class T>
T clamp(const T& val, const T& minVal, const T& maxVal)
{
	T result = val;

	if (result < minVal)
		result = minVal;

	if (result > maxVal)
		result = maxVal;

	return result;
}

///////////////////////////////////////////////////////////////////////////
////////////////////////// DELEGATE, TODO REVIEW //////////////////////////
///////////////////////////////////////////////////////////////////////////
template <typename Signature>
struct Delegate;

template <typename... Args>
struct Delegate<void(Args...)>
{
	struct base {
		virtual ~base() {}
		//virtual bool do_cmp(base* other) = 0;
		virtual void do_call(Args... args) = 0;
	};
	template <typename T>
	struct call : base {
		T d_callback;
		template <typename S>
		call(S&& callback) : d_callback(std::forward<S>(callback)) {}

		//bool do_cmp(base* other) {
		//	call<T>* tmp = dynamic_cast<call<T>*>(other);
		//	return tmp && this->d_callback == tmp->d_callback;
		//}
		void do_call(Args... args) {
			return this->d_callback(std::forward<Args>(args)...);
		}
	};
	std::vector<std::shared_ptr<base>> d_callbacks;

	Delegate(Delegate const& other)
	{
		*this = other;
	}

	Delegate& operator=(Delegate const& other)
	{
		d_callbacks = other.d_callbacks;
		return *this;
	}

	operator bool() const
	{
		return d_callbacks.size() != 0;
	}

public:
	Delegate() {}
	template <typename T>
	Delegate& operator+= (T&& callback) {
		d_callbacks.emplace_back(new call<T>(std::forward<T>(callback)));
		//d_callbacks.emplace(d_callbacks.begin(), new call<T>(std::forward<T>(callback)));
		return *this;
	}
	//template <typename T>
	//Delegate& operator-= (T&& callback) {
	//	call<T> tmp(std::forward<T>(callback));
	//
	//	auto it = std::remove_if(d_callbacks.begin(), d_callbacks.end(), [&](std::unique_ptr<base>& other)
	//	{
	//		return tmp.do_cmp(other.get());
	//	});
	//	 
	//	this->d_callbacks.erase(it, this->d_callbacks.end());
	//	return *this;
	//}

	void operator()(Args... args)
	{
		// I'm sorry but it's necessary to copy the array, because callbacks can remove and add elements to them lol
		auto callbacks = d_callbacks;
		for (auto& callback : callbacks) {
			callback->do_call(args...);
		}
	}
};

template <typename RC, typename Class, typename... Args>
class MemberCall_{
	Class* d_object;
	RC(Class::*d_member)(Args...);
public:
	MemberCall_(Class* object, RC(Class::*member)(Args...))
		: d_object(object)
		, d_member(member) {
	}
	RC operator()(Args... args) {
		return (this->d_object->*this->d_member)(std::forward<Args>(args)...);
	}
	bool operator== (MemberCall_ const& other) const {
		return this->d_object == other.d_object
			&& this->d_member == other.d_member;
	}
	bool operator!= (MemberCall_ const& other) const {
		return !(*this == other);
	}
};

template <typename RC, typename Class, typename... Args>
MemberCall_<RC, Class, Args...> MemberCall(Class& object,
	RC(Class::*member)(Args...)) {
	return MemberCall<RC, Class, Args...>(&object, member);
}