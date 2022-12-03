#include <iostream>
#include <Test.h>
#include <Shell.h>

int main(int argc, char** argv) {
    // **********************************************************

    Shell::GetInstance().init();

    Shell::GetInstance().run();

    // **********************************************************

    return 0;
}
