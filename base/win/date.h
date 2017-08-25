#pragma once

#include "base/strings/stringprintf.h"

#include <windows.h>
#include <oleauto.h>

// Date

namespace {

class Date
{
public:
	static const double day;
	static const double hour;
	static const double min;
	static const double sec;
	static const double msec;

private:
  Date();
};

const double Date::sec	= 1.0;
const double Date::min	= 60 * Date::sec;
const double Date::hour	= 60 * Date::min;
const double Date::day	= 24 * Date::hour;
const double Date::msec	= sec / 1000;

}