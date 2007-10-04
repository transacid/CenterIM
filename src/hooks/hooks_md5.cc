#include "md5.h"
#include "hooks_md5.h"

std::string hooks_md5::getmd5(const std::string &text) {
    md5_state_t state;
    md5_byte_t digest[16];
    std::string r;
    char buf[3];

    md5_init(&state);
    md5_append(&state, (md5_byte_t *) text.c_str(), text.size());
    md5_finish(&state, digest);

    for(int i = 0; i < 16; i++) {
	snprintf(buf, sizeof(buf), "%02x", digest[i]);
	r.append(buf);
    }

    return r;
}
