#include "log.hpp"
#include "slab/slab.h"

namespace logd {
class Parser {
private:
	const slab_t* prop_slab;

public:
	Parser() {
		// this->prop_slab = slab_create();
	}
};
}; // namespace logd
