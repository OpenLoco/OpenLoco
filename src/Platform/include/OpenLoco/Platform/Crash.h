#pragma once
namespace google_breakpad
{
    class ExceptionHandler;
}

using CExceptionHandler = google_breakpad::ExceptionHandler*;
CExceptionHandler crashInit();
void crashClose(CExceptionHandler);
