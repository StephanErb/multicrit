#include <vector>
#include <algorithm>

template <typename T>
void s()
{
	std::vector<T> a(42);
	std::sort(a.begin(), a.end());
	std::stable_sort(a.begin(), a.end());
}

template <typename T>
void sc()
{
	std::vector<T> a(42);
	std::sort(a.begin(), a.end(), std::less<T>());
}

void si()
{
	s<int>();
}
void sd()
{
	s<double>();
}

void sci()
{
	sc<int>();
}
void scd()
{
	sc<double>();
}

int main()
{
}
