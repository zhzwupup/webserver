#include "Thread.h"
#include <iostream>
#include <thread>

void foo() { std::cout << std::this_thread::get_id() << std::endl; }

int main() {
  Thread thrd1(foo);
  Thread thrd2(foo);
  thrd1.start();
  thrd2.start();
}
