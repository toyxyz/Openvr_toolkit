#include "app/Application.h"

#include <iostream>

int main()
{
    const ovtr::Application application;
    return application.runCliDiagnostics(std::cout);
}

