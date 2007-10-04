#ifndef HOOKS_MD5_H
#define HOOKS_MD5_H

#include <string>

struct hooks_md5 {
	static std::string getmd5(const std::string& text);
};

#endif
