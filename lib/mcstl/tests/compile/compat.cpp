#include <bits/mcstl_compatibility.h>

template <typename T>
void c()
{
	T a = 1, b = 2, c = 3;
	mcstl::compare_and_swap(&a, b, c);
}

void c32()
{
	c<mcstl::int32>();
}
void c64()
{
	c<mcstl::int64>();
}
void clcast()
{
	c<mcstl::lcas_t>();
}

template <typename T>
void f()
{
	T a = 1, b = 2;
	mcstl::fetch_and_add(&a, b);
}

void f32()
{
	f<mcstl::int32>();
}
void f64()
{
	f<mcstl::int64>();
}
void flcast()
{
	f<mcstl::lcas_t>();
}


void y()
{
	mcstl::yield();
}

int main()
{
}
