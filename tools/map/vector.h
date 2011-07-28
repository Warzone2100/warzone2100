#ifndef TOOLS_MAP_WZCONFIG_H
#define TOOLS_MAP_WZCONFIG_H

struct Rotation;
struct Vector3i;
struct Vector2i
{
	Vector2i() {}
	Vector2i(int x, int y) : x(x), y(y) {}
	Vector2i(Vector3i const &r); // discards the z value

	int x, y;
};
struct Vector2f
{
	Vector2f() {}
	Vector2f(float x, float y) : x(x), y(y) {}
	Vector2f(Vector2i const &v) : x(v.x), y(v.y) {}

	float x, y;
};
struct Vector3i
{
	Vector3i() {}
	Vector3i(int x, int y, int z) : x(x), y(y), z(z) {}
	Vector3i(Vector2i const &xy, int z) : x(xy.x), y(xy.y), z(z) {}
	Vector3i(Rotation const &r);
	int x, y, z;
};
struct Vector3f
{
	Vector3f() {}
	Vector3f(float x, float y, float z) : x(x), y(y), z(z) {}
	Vector3f(Vector3i const &v) : x(v.x), y(v.y), z(v.z) {}
	Vector3f(Vector3f const &v) : x(v.x), y(v.y), z(v.z) {}
	Vector3f(Vector2f const &xy, float z) : x(xy.x), y(xy.y), z(z) {}

	float x, y, z;
};

#endif // #ifndef TOOLS_MAP_WZCONFIG_H
