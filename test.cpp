#include <iostream>
using namespace std;

void foo(int *ptr)
{
    // Modify the local copy of the pointer
    ptr = ptr + 2;
    cout << "In foo:\t" << *ptr << "\n"; 
}

void bar(int *&ptr)
{
    // Modify the original pointer passed by reference
    ptr = ptr + 2;
    cout << "In bar:\t" << *ptr << "\n"; 
}

int main()
{
    int p[7] = {3, 5, 7, 9, 11, 13, 15};
    int *ptr = p;  // Create a pointer variable pointing to the array

    cout << "Before foo:\t" << *ptr << "\n";
    foo(ptr);  // Pass the pointer to foo
    cout << "After foo:\t" << *ptr << "\n";

    cout << "Before bar:\t" << *ptr << "\n";
    bar(ptr);  // Pass the pointer by reference to bar
    cout << "After bar:\t" << *ptr << "\n";

    cout << "Before main mod:\t" << *p << "\n";
    *p = *p + 2;
    cout << "After main mod:\t" << *p << "\n";
    cout << "After main mod:\t" << *p-1 << "\n";

    return 0;
}