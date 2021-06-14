I've added 3 different MinHook libraries in this version. With the C port and re-write of SingleFile, I'm focusing on Code Optimization and decluttering the Source code. I'm doing this in two ways, one with using C and the effiencies of not relying on STL and other libraries, as well as not using CRT functions. I also wanted up to clean up the vcxproj and sln, as before Release wouldn't work. Now I have added minhookr and _minhookr. minhookr is the default option for release mode, and saves a few bytes of space using a custom memset implementation rather than CRT's. This shouldn't really impact you however if you want to change this and use minhook as it comes compiled, that's your choice. For license purposes, here's the code.

This function
```cpp
void* _memset(void* _ptr, int val, int n) {
  const unsigned char pv = val;
  unsigned char* ptr = (unsigned char*)(s);
  for (; 0 < n; ++ptr, --n) {
    ptr[0] = pv;
  return (_ptr);
}
```
is at line 14 of hde/hde32.c

The function "hde32_disasm" is modified at line 29 to
```cpp
  _memset(hs, 0, sizeof(hde32s));
 ```
 
 Any license abnormalities I'm unaware of please raise in an issue, I'd been happy to publish the full source code to my port.
