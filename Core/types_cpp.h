#ifndef _H_TYPES_CPP
#define _H_TYPES_CPP

#include <string>

using namespace std;

namespace SDDC {
	typedef struct DeviceItem {
		uint8_t index;
		string product;
		string serial_number;
	} DeviceItem;
}

#endif